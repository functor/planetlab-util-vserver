# Copyright 2005 Princeton University

import errno
import fcntl
import os
import re
import pwd
import signal
import sys
import time
import traceback

import mountimpl
import runcmd
import utmp
import vserverimpl, vduimpl
import cpulimit, bwlimit

from vserverimpl import VS_SCHED_CPU_GUARANTEED as SCHED_CPU_GUARANTEED
from vserverimpl import DLIMIT_INF
from vserverimpl import VC_LIM_KEEP

from vserverimpl import RLIMIT_CPU
from vserverimpl import RLIMIT_RSS
from vserverimpl import RLIMIT_NPROC
from vserverimpl import RLIMIT_NOFILE
from vserverimpl import RLIMIT_MEMLOCK
from vserverimpl import RLIMIT_AS
from vserverimpl import RLIMIT_LOCKS
from vserverimpl import RLIMIT_SIGPENDING
from vserverimpl import RLIMIT_MSGQUEUE
from vserverimpl import VLIMIT_NSOCK
from vserverimpl import VLIMIT_OPENFD
from vserverimpl import VLIMIT_ANON
from vserverimpl import VLIMIT_SHMEM

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


class NoSuchVServer(Exception): pass


class VServer:

    INITSCRIPTS = [('/etc/rc.vinit', 'start'),
                   ('/etc/rc.d/rc', '%(runlevel)d')]

    def __init__(self, name, vm_id = None, vm_running = False):

        self.name = name
        self.limits_changed = False
        self.config_file = "/etc/vservers/%s.conf" % name
        self.dir = "%s/%s" % (vserverimpl.VSERVER_BASEDIR, name)
        if not (os.path.isdir(self.dir) and
                os.access(self.dir, os.R_OK | os.W_OK | os.X_OK)):
            raise NoSuchVServer, "no such vserver: " + name
        self.config = {}
        for config_file in ["/etc/vservers.conf", self.config_file]:
            try:
                self.config.update(self.__read_config_file(config_file))
            except IOError, ex:
                if ex.errno != errno.ENOENT:
                    raise
        self.remove_caps = ~vserverimpl.CAP_SAFE;
        if vm_id == None:
            vm_id = int(self.config['S_CONTEXT'])
        self.ctx = vm_id
        self.vm_running = vm_running

        # For every resource key listed in the limit table, add in a
        # new method with which one can get/set the resource's hard,
        # soft, minimum limits.
        limits = {"CPU": RLIMIT_CPU,
                  "RSS": RLIMIT_RSS,
                  "NPROC": RLIMIT_NPROC,
                  "NOFILE": RLIMIT_NOFILE,
                  "MEMLOCK": RLIMIT_MEMLOCK,
                  "AS": RLIMIT_AS,
                  "LOCKS": RLIMIT_LOCKS,
                  "SIGPENDING": RLIMIT_SIGPENDING,
                  "MSGQUEUE": RLIMIT_MSGQUEUE,
                  "NSOCK": VLIMIT_NSOCK,
                  "OPENFD": VLIMIT_OPENFD,
                  "ANON": VLIMIT_ANON,
                  "SHMEM": VLIMIT_SHMEM}
        for meth in limits.keys():
            resource_type = limits[meth]
            func = lambda \
                       hard=VC_LIM_KEEP,\
                       soft=VC_LIM_KEEP,\
                       minimum=VC_LIM_KEEP:\
                       self.__set_vserver_limit(resource_type,\
                                                hard, \
                                                soft,\
                                                minimum)
            self.__dict__["set_%s_limit"%meth] = func

            func = lambda \
                       hard=VC_LIM_KEEP,\
                       soft=VC_LIM_KEEP,\
                       minimum=VC_LIM_KEEP:\
                       self.__set_vserver_config(meth, resource_type, \
                                                hard, \
                                                soft,\
                                                minimum)
            self.__dict__["set_%s_config"%meth] = func

            func = lambda : self.__get_vserver_limit(resource_type)
            self.__dict__["get_%s_limit"%meth] = func

            func = lambda : self.__get_vserver_config(meth,resource_type)
            self.__dict__["get_%s_config"%meth] = func
    
    def have_limits_changed(self):
        return self.limits_changed

    def __set_vserver_limit(self,resource_type,hard,soft,minimum):
        """Generic set resource limit function for vserver"""
        if self.is_running():
            changed = False
            old_hard, old_soft, old_minimum = self.__get_vserver_limit(resource_type)
            if old_hard != VC_LIM_KEEP and old_hard <> hard: changed = True
            if old_soft != VC_LIM_KEEP and old_soft <> soft: changed = True
            if old_minimu != VC_LIM_KEEP and old_minimum <> minimum: changed = True
            self.limits_changed = self.limits_changed or changed 
            ret = vserverimpl.setrlimit(self.ctx,resource_type,hard,soft,minimum)

    def __set_vserver_config(self,meth,resource_type,hard,soft,minimum):
        """Generic set resource limit function for vserver"""
        resources = {}
        if hard <> VC_LIM_KEEP:
            resources["VS_%s_HARD"%meth] = hard
        if soft <> VC_LIM_KEEP:
            resources["VS_%s_SOFT"%meth] = soft
        if minimum <> VC_LIM_KEEP:
            resources["VS_%s_MINIMUM"%meth] = minimum
        if len(resources)>0:
            self.update_resources(resources)
        self.__set_vserver_limit(resource_type,hard,soft,minimum)

    def __get_vserver_limit(self,resource_type):
        """Generic get resource configuration function for vserver"""
        if self.is_running():
            ret = vserverimpl.getrlimit(self.ctx,resource_type)
        else:
            ret = __get_vserver_config(meth,resource_type)
        return ret

    def __get_vserver_config(self,meth,resource_type):
        """Generic get resource configuration function for vserver"""
        hard = int(self.config.get("VS_%s_HARD"%meth,VC_LIM_KEEP))
        soft = int(self.config.get("VS_%s_SOFT"%meth,VC_LIM_KEEP))
        minimum = int(self.config.get("VS_%s_MINIMUM"%meth,VC_LIM_KEEP))
        return (hard,soft,minimum)

    def set_WHITELISTED_config(self,whitelisted):
        resources = {'VS_WHITELISTED': whitelisted}
        self.update_resources(resources)

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
        offset = 0
        for m in self.config_var_re.finditer(data):
            (key, val) = m.groups()
            newval = todo.pop(key, None)
            if newval != None:
                data = data[:offset+m.start(2)] + str(newval) + data[offset+m.end(2):]
                offset += len(str(newval)) - (m.end(2)-m.start(2))
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

        # replace old file with new
        os.rename(newfile, filename)

    def __do_chroot(self):

        os.chroot(self.dir)
        os.chdir("/")

    def chroot_call(self, fn, *args):

        cwd_fd = os.open(".", os.O_RDONLY)
        try:
            root_fd = os.open("/", os.O_RDONLY)
            try:
                self.__do_chroot()
                result = fn(*args)
            finally:
                os.fchdir(root_fd)
                os.chroot(".")
                os.fchdir(cwd_fd)
                os.close(root_fd)
        finally:
            os.close(cwd_fd)
        return result

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

        resources = {'VS_DISK_MAX': block_limit}
        self.update_resources(resources)

    def is_running(self):
        return vserverimpl.isrunning(self.ctx)
    
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

    def set_sched_config(self, cpu_share, sched_flags):

        """ Write current CPU scheduler parameters to the vserver
        configuration file. This method does not modify the kernel CPU
        scheduling parameters for this context. """

        if cpu_share == int(self.config.get("CPULIMIT", -1)):
            return
        cpu_guaranteed = sched_flags & SCHED_CPU_GUARANTEED
        cpu_config = { "CPULIMIT": cpu_share, "CPUGUARANTEED": cpu_guaranteed }
        self.update_resources(cpu_config)
        if self.vm_running:
            self.set_sched(cpu_share, sched_flags)

    def set_sched(self, cpu_share, sched_flags = 0):
        """ Update kernel CPU scheduling parameters for this context. """
        vserverimpl.setsched(self.ctx, cpu_share, sched_flags)

    def get_sched(self):
        # have no way of querying scheduler right now on a per vserver basis
        return (-1, False)

    def set_bwlimit(self, minrate = bwlimit.bwmin, maxrate = None,
                    exempt_min = None, exempt_max = None,
                    share = None, dev = "eth0"):

        if minrate is None:
            bwlimit.off(self.ctx, dev)
        else:
            bwlimit.on(self.ctx, dev, share,
                       minrate, maxrate, exempt_min, exempt_max)

    def get_bwlimit(self, dev = "eth0"):

        result = bwlimit.get(self.ctx)
        # result of bwlimit.get is (ctx, share, minrate, maxrate)
        if result:
            result = result[1:]
        return result

    def open(self, filename, mode = "r", bufsize = -1):

        return self.chroot_call(open, filename, mode, bufsize)

    def __do_chcontext(self, state_file):

        if state_file:
            print >>state_file, "S_CONTEXT=%u" % self.ctx
            print >>state_file, "S_PROFILE="
            state_file.close()

        if vserverimpl.chcontext(self.ctx):
            self.set_resources()
            vserverimpl.setup_done(self.ctx)

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
        self.limits_changed = False

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

    def set_resources(self):

        """ Called when vserver context is entered for first time,
        should be overridden by subclass. """

        pass

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
        self.limits_changed = False



def create(vm_name, static = False, ctor = VServer):

    options = []
    if static:
        options += ['--static']
    runcmd.run('vuseradd', options + [vm_name])
    vm_id = pwd.getpwnam(vm_name)[2]

    return ctor(vm_name, vm_id)
