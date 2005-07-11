# Copyright 2005 Princeton University

import errno
import fcntl
import os
import re
import sys
import time
import traceback

import mountimpl
import linuxcaps
import passfdimpl
import utmp
import vserverimpl, vduimpl

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

    INITSCRIPTS = [('/etc/rc.vinit', 'start'),
                   ('/etc/rc.d/rc', '%(runlevel)d')]

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

    def set_dlimit(self, blocktotal):
        path = "%s/%s" % (VROOTDIR, self.name)
        inodes, blockcount, size = vduimpl.vdu(path)
        blockcount = blockcount >> 1

        if blocktotal > blockcount:
            vserverimpl.setdlimit(path, self.ctx, blockcount>>1, \
                                  blocktotal, inodes, -1, 2)
        else:
            # should raise some error value
            print "block limit (%d) ignored for vserver %s" %(blocktotal,self.name)

    def get_dlimit(self):
        path = "%s/%s" % (VROOTDIR, self.name)
        try:
            blocksused, blocktotal, inodesused, inodestotal, reserved = \
                        vserverimpl.getdlimit(path,self.ctx)
        except OSError, ex:
            if ex.errno == 3:
                # get here if no vserver disk limit has been set for xid
                # set blockused to -1 to indicate no limit
                blocktotal = -1

        return blocktotal

    def open(self, filename, mode = "r", bufsize = -1):

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

        return os.fdopen(fd, mode, bufsize)

    def __do_chcontext(self, state_file = None):

        vserverimpl.chcontext(self.ctx, self.remove_caps)
        if not state_file:
            return
        print >>state_file, "S_CONTEXT=%d" % self.ctx
        print >>state_file, "S_PROFILE=%s" % self.config.get("S_PROFILE", "")
        state_file.close()

    def __prep(self, runlevel, log):

        """ Perform all the crap that the vserver script does before
        actually executing the startup scripts. """

        # remove /var/run and /var/lock/subsys files
        # but don't remove utmp from the top-level /var/run
        RUNDIR = "/var/run"
        LOCKDIR = "/var/lock/subsys"
        filter_fn = lambda fs: filter(lambda f: f != 'utmp', fs)
        garbage = reduce((lambda (out, ff), (dir, subdirs, files):
                          (out + map((dir + "/").__add__, ff(files)),
                           lambda fs: fs)),
                         list(os.walk(RUNDIR)),
                         ([], filter_fn))[0]
        garbage += filter(os.path.isfile, map((LOCKDIR + "/").__add__,
                                              os.listdir(LOCKDIR)))
        for f in garbage:
            os.unlink(f)

        # set the initial runlevel
        f = open(RUNDIR + "/utmp", "w")
        utmp.set_runlevel(f, runlevel)
        f.close()

        # mount /proc and /dev/pts
        self.__do_mount("none", "/proc", "proc")
        # XXX - magic mount options
        self.__do_mount("none", "/dev/pts", "devpts", 0, "gid=5,mode=0620")

    def __do_mount(self, *mount_args):

        try:
            mountimpl.mount(*mount_args)
        except OSError, ex:
            if ex.errno == errno.EBUSY:
                # assume already mounted
                return
            raise ex

    def enter(self):

        state_file = open("/var/run/vservers/%s.ctx" % self.name, "w")
        self.__do_chroot()
        self.__do_chcontext(state_file)

    def start(self, wait, runlevel = 3):

        child_pid = os.fork()
        if child_pid == 0:
            # child process
            try:
                # get a new session
                os.setsid()

                # open state file to record vserver info
                state_file = open("/var/run/vservers/%s.ctx" % self.name, "w")

                # use /dev/null for stdin, /var/log/boot.log for stdout/err
                os.close(0)
                os.close(1)
                os.open("/dev/null", os.O_RDONLY)
                self.__do_chroot()
                log = open("/var/log/boot.log", "w", 0)
                os.dup2(1, 2)

                print >>log, ("%s: starting the virtual server %s" %
                              (time.asctime(time.gmtime()), self.name))

                # perform pre-init cleanup
                self.__prep(runlevel, log)

                # execute each init script in turn
                # XXX - we don't support all scripts that vserver script does
                cmd_pid = 0
                for cmd in self.INITSCRIPTS + [None]:
                    # wait for previous command to terminate, unless it
                    # is the last one and the caller has specified to wait
                    if cmd_pid and (cmd != None or wait):
                        try:
                            os.waitpid(cmd_pid, 0)
                        except:
                            print >>log, "error waiting for %s:" % cmd_pid
                            traceback.print_exc()

                    # end of list
                    if cmd == None:
                        os._exit(0)

                    # fork and exec next command
                    cmd_pid = os.fork()
                    if cmd_pid == 0:
                        try:
                            # enter vserver context
                            self.__do_chcontext(state_file)
                            arg_subst = { 'runlevel': runlevel }
                            cmd_args = [cmd[0]] + map(lambda x: x % arg_subst,
                                                      cmd[1:])
                            print >>log, "executing '%s'" % " ".join(cmd_args)
                            os.execl(cmd[0], *cmd_args)
                        except:
                            traceback.print_exc()
                            os._exit(1)
                    else:
                        # don't want to write state_file multiple times
                        state_file = None

            # we get here due to an exception in the top-level child process
            except Exception, ex:
                traceback.print_exc()
            os._exit(0)

        # parent process
        return child_pid
