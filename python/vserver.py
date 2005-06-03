# Copyright 2005 Princeton University

import errno
import fcntl
import os
import re
import traceback

import linuxcaps
import passfdimpl
import vserverimpl

from util_vserver_vars import *

CAP_SAFE = (linuxcaps.CAP_CHOWN |
            linuxcaps.CAP_DAC_OVERRIDE |
            linuxcaps.CAP_DAC_READ_SEARCH |
            linuxcaps.CAP_FOWNER |
            linuxcaps.CAP_FSETID |
            linuxcaps.CAP_KILL |
            linuxcaps.CAP_SETGID |
            linuxcaps.CAP_SETUID |
            linuxcaps.CAP_SETPCAP |
            linuxcaps.CAP_SYS_TTY_CONFIG |
            linuxcaps.CAP_LEASE |
            linuxcaps.CAP_SYS_CHROOT |
            linuxcaps.CAP_SYS_PTRACE)

#
# these are the flags taken from the kernel linux/vserver/legacy.h
#
FLAGS_LOCK = 1
FLAGS_SCHED = 2  # XXX - defined in util-vserver/src/chcontext.c
FLAGS_NPROC = 4
FLAGS_PRIVATE = 8
FLAGS_INIT = 16
FLAGS_HIDEINFO = 32
FLAGS_ULIMIT = 64
FLAGS_NAMESPACE = 128


              
class VServer:

    INITSCRIPTS = [('/etc/rc.vinit', 'start'), ('/etc/rc.d/rc', '3')]

    def __init__(self, name):

        self.name = name
        self.config = self.__read_config_file("/etc/vservers.conf")
        self.config.update(self.__read_config_file("/etc/vservers/%s.conf" %
                                                   self.name))
        self.flags = 0
        flags = self.config["S_FLAGS"].split(" ")
        if "lock" in flags:
            self.flags |= FLAGS_LOCK
        if "nproc" in flags:
            self.flags |= FLAGS_NPROC
        self.remove_caps = ~CAP_SAFE
        self.ctx = int(self.config["S_CONTEXT"])

    config_var_re = re.compile(r"^ *([A-Z_]+)=(.*)\n?$", re.MULTILINE)

    def __read_config_file(self, filename):

        f = open(filename, "r")
        data = f.read()
        f.close()
        config = {}
        for m in self.config_var_re.finditer(data):
            (key, val) = m.groups()
            config[key] = val.strip('"')
        return config

    def __do_chroot(self):

        return os.chroot("%s/%s" % (VROOTDIR, self.name))

    def open(self, filename, mode = "r"):

        (sendsock, recvsock) = passfdimpl.socketpair()
        child_pid = os.fork()
        if child_pid == 0:
            try:
                # child process
                self.__do_chroot()
                f = open(filename, mode)
                passfdimpl.sendmsg(f.fileno(), sendsock)
                os._exit(0)
            except EnvironmentError, ex:
                (result, errmsg) = (ex.errno, ex.strerror)
            except Exception, ex:
                (result, errmsg) = (255, str(ex))
            os.write(sendsock, errmsg)
            os._exit(result)

        # parent process

        # XXX - need this since a lambda can't raise an exception
        def __throw(ex):
            raise ex

        os.close(sendsock)
        throw = lambda : __throw(Exception(errmsg))
        while True:
            try:
                (pid, status) = os.waitpid(child_pid, 0)
                if os.WIFEXITED(status):
                    result = os.WEXITSTATUS(status)
                    if result != 255:
                        errmsg = os.strerror(result)
                        throw = lambda : __throw(IOError(result, errmsg))
                    else:
                        errmsg = "unexpected exception in child"
                else:
                    result = -1
                    errmsg = "child killed"
                break
            except OSError, ex:
                if ex.errno != errno.EINTR:
                    os.close(recvsock)
                    raise ex
        fcntl.fcntl(recvsock, fcntl.F_SETFL, os.O_NONBLOCK)
        try:
            (fd, errmsg) = passfdimpl.recvmsg(recvsock)
        except OSError, ex:
            if ex.errno != errno.EAGAIN:
                throw = lambda : __throw(ex)
            fd = 0
        os.close(recvsock)
        if not fd:
            throw()

        return os.fdopen(fd)

    def enter(self):

        state_file = open("/var/run/vservers/%s.ctx" % self.name, "w")
        self.__do_chroot()
        vserverimpl.chcontext(self.ctx, self.remove_caps)
        print >>state_file, "S_CONTEXT=%d" % self.ctx
        print >>state_file, "S_PROFILE=%s" % self.config.get("S_PROFILE", "")
        state_file.close()

    def start(self):

        child_pid = os.fork()
        if child_pid == 0:
            # child process
            try:
                # get a new session
                os.setsid()

                # enter vserver context
                self.enter()

                # use /dev/null for stdin, /var/log/boot.log for stdout/err
                os.close(0)
                os.close(1)
                os.open("/dev/null", os.O_RDONLY)
                os.open("/var/log/boot.log",
                        os.O_WRONLY | os.O_CREAT | os.O_TRUNC)
                os.dup2(1, 2)

                # write same output that vserver script does
                os.write(1, "Starting the virtual server %s\n" % self.name)
                os.write(1, "Server %s is not running\n" % self.name)

                # execute each init script in turn
                # XXX - we don't support all the possible scripts
                for cmd in self.INITSCRIPTS:
                    os.spawnl(os.P_WAIT, *cmd)
            except Exception, ex:
                traceback.print_exc()
            os._exit(0)
