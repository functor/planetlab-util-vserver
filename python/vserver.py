# Copyright 2005 Princeton University

import errno
import fcntl
import os
import re
import signal
import sys
import time
import traceback

import mountimpl
import passfdimpl
import utmp
import vserverimpl, vduimpl
import cpulimit, bwlimit



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

    def __init__(self, name, vm_id, vm_running = False, resources = {}):

        self.name = name
        self.config_file = "/etc/vservers/%s.conf" % name
        self.dir = "%s/%s" % (vserverimpl.VSERVER_BASEDIR, name)
        if not (os.path.isdir(self.dir) and
                os.access(self.dir, os.R_OK | os.W_OK | os.X_OK)):
            raise Exception, "no such vserver: " + name
        self.config = {}
        for config_file in ["/etc/vservers.conf", self.config_file]:
            try:
                self.config.update(self.__read_config_file(config_file))
            except IOError, ex:
                if ex.errno != errno.ENOENT:
                    raise
        self.remove_caps = ~vserverimpl.CAP_SAFE;
        self.ctx = vm_id
        self.vm_running = vm_running
        self.resources = resources

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

    def __update_config_file(self, filename, newvars):

        # read old file, apply changes
        f = open(filename, "r")
        data = f.read()
        f.close()
        todo = newvars.copy()
        changed = False
        for m in self.config_var_re.finditer(data):
            (key, val) = m.groups()
            newval = todo.pop(key, None)
            if newval != None:
                data = data[:m.start(2)] + str(newval) + data[m.end(2):]
                changed = True
        for (newkey, newval) in todo.items():
            data += "%s=%s\n" % (newkey, newval)
            changed = True

        if not changed:
            return

        # write new file
        newfile = filename + ".new"
        f = open(newfile, "w")
        f.write(data)
        f.close()

        # 'copy' original file, rename new to original
        backup = filename + ".old"
        try:
            os.unlink(backup)
        except OSError, ex:
            if ex.errno != errno.ENOENT:
                raise
        os.link(filename, backup)
        os.rename(newfile, filename)

    def __do_chroot(self):

        os.chroot(self.dir)
        os.chdir("/")

    def set_disklimit(self, block_limit):

        # block_limit is in kB
        if block_limit == 0:
            vserverimpl.unsetdlimit(self.dir, self.ctx)
            return

        if self.vm_running:
            block_usage = vserverimpl.DLIMIT_KEEP
            inode_usage = vserverimpl.DLIMIT_KEEP
        else:
            # init_disk_info() must have been called to get usage values
            block_usage = self.disk_blocks
            inode_usage = self.disk_inodes

        vserverimpl.setdlimit(self.dir,
                              self.ctx,
                              block_usage,
                              block_limit,
                              inode_usage,
                              vserverimpl.DLIMIT_INF,  # inode limit
                              2)   # %age reserved for root

    def get_disklimit(self):

        try:
            (self.disk_blocks, block_limit, self.disk_inodes, inode_limit,
             reserved) = vserverimpl.getdlimit(self.dir, self.ctx)
        except OSError, ex:
            if ex.errno != errno.ESRCH:
                raise
            # get here if no vserver disk limit has been set for xid
            block_limit = -1

        return block_limit

    def set_sched(self, cpu_share):

        if cpu_share == int(self.config.get("CPULIMIT", -1)):
            return
        # XXX - don't want to have to deal with nm_ flags here
        cpu_guaranteed = int(self.resources.get("nm_sched_flags",
                                                None) == "guaranteed")
        cpu_config = { "CPULIMIT": cpu_share, "CPUGUARANTEED": cpu_guaranteed }
        self.__update_config_file(self.config_file, cpu_config)
        if self.vm_running:
            # caller must ensure cpu_share is consistent with self.resources
            vserverimpl.setsched(self.ctx, self.resources)

    def get_sched(self):
        # have no way of querying scheduler right now on a per vserver basis
        return (-1, False)

    def set_memlimit(self, limit):
        ret = vserverimpl.setrlimit(self.ctx,5,limit)
        return ret

    def get_memlimit(self):
        ret = vserverimpl.getrlimit(self.ctx,5)
        return ret
    
    def set_tasklimit(self, limit):
        ret = vserverimpl.setrlimit(self.ctx,6,limit)
        return ret

    def get_tasklimit(self):
        ret = vserverimpl.getrlimit(self.ctx,6)
        return ret

    def set_bwlimit(self, share, minrate = 1, maxrate = None, dev = "eth0"):

        if share:
            bwlimit.on(self.ctx, dev, share, minrate, maxrate)
        else:
            bwlimit.off(self.ctx, dev)

    def get_bwlimit(self, eth):
        # not implemented yet
        bwlimit = -1
        cap = "unknown"
        minrate = "unknown"
        maxrate = "unknown"
        return (bwlimit, cap, minrate, maxrate)
        
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

    def __do_chcontext(self, state_file):

        if state_file:
            print >>state_file, "S_CONTEXT=%u" % self.ctx
            print >>state_file, "S_PROFILE="
            state_file.close()

        vserverimpl.chcontext(self.ctx, self.resources)

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

        self.vm_running = True

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
                first_child = True
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

    def update_resources(self, resources):

        self.config.update(resources)

        # write new values to configuration file
        self.__update_config_file(self.config_file, resources)

    def init_disk_info(self):

        (self.disk_inodes, self.disk_blocks, size) = vduimpl.vdu(self.dir)

        return size

    def stop(self, signal = signal.SIGKILL):

        vserverimpl.killall(self.ctx, signal)
        self.vm_running = False
