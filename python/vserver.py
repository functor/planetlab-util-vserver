# Copyright 2005 Princeton University

$Id$

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
import vserverimpl
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

RLIMITS = {"CPU": RLIMIT_CPU,
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

class NoSuchVServer(Exception): pass


class VServerConfig:
    def __init__(self, name, directory):
        self.name = name
        self.dir = directory

    def get(self, option, default = None):
        try:
            f = open(os.path.join(self.dir, option), "r")
            buf = f.readline().rstrip()
            f.close()
            return buf
        except KeyError, e:
            # No mapping exists for this option
            raise e
        except IOError, e:
            if default is not None:
                return default
            else:
                raise KeyError, "Key %s is not set for %s" % (option, self.name)

    def update(self, option, value):
        try:
            old_umask = os.umask(0022)
            filename = os.path.join(self.dir, option)
            try:
                os.makedirs(os.path.dirname(filename), 0755)
            except:
                pass
            f = open(filename, 'w')
            if isinstance(value, list):
                f.write("%s\n" % "\n".join(value))
            else:
                f.write("%s\n" % value)
            f.close()
            os.umask(old_umask)
        except KeyError, e:
            raise KeyError, "Don't know how to handle %s, sorry" % option


class VServer:

    INITSCRIPTS = [('/etc/rc.vinit', 'start'),
                   ('/etc/rc.d/rc', '%(runlevel)d')]

    def __init__(self, name, vm_id = None, vm_running = None):

        self.name = name
        self.rlimits_changed = False
        self.dir = "%s/%s" % (vserverimpl.VSERVER_BASEDIR, name)
        if not (os.path.isdir(self.dir) and
                os.access(self.dir, os.R_OK | os.W_OK | os.X_OK)):
            raise NoSuchVServer, "no such vserver: " + name
        self.config = VServerConfig(name, "/etc/vservers/%s" % name)
        self.remove_caps = ~vserverimpl.CAP_SAFE;
        if vm_id == None:
            vm_id = int(self.config.get('context'))
        self.ctx = vm_id
        if vm_running == None:
            vm_running = self.is_running()
        self.vm_running = vm_running

    def have_limits_changed(self):
        return self.rlimits_changed

    def set_rlimit_limit(self,type,hard,soft,minimum):
        """Generic set resource limit function for vserver"""
        global RLIMITS
        changed = False
        try:
            old_hard, old_soft, old_minimum = self.get_rlimit_limit(type)
            if old_hard != VC_LIM_KEEP and old_hard <> hard: changed = True
            if old_soft != VC_LIM_KEEP and old_soft <> soft: changed = True
            if old_minimum != VC_LIM_KEEP and old_minimum <> minimum: changed = True
            self.rlimits_changed = self.rlimits_changed or changed 
        except OSError, e:
            if self.is_running(): print "Unexpected error with getrlimit for running context %d" % self.ctx

        resource_type = RLIMITS[type]
        try:
            ret = vserverimpl.setrlimit(self.ctx,resource_type,hard,soft,minimum)
        except OSError, e:
            if self.is_running(): print "Unexpected error with setrlimit for running context %d" % self.ctx

    def set_rlimit_config(self,type,hard,soft,minimum):
        """Generic set resource limit function for vserver"""
        if hard <> VC_LIM_KEEP:
            self.config.update('rlimits/%s.hard' % type.lower(), hard)
        if soft <> VC_LIM_KEEP:
            self.config.update('rlimits/%s.soft' % type.lower(), soft)
        if minimum <> VC_LIM_KEEP:
            self.config.update('rlimits/%s.min' % type.lower(), minimum)
        self.set_rlimit_limit(type,hard,soft,minimum)

    def get_rlimit_limit(self,type):
        """Generic get resource configuration function for vserver"""
        global RLIMITS
        resource_type = RLIMITS[type]
        try:
            ret = vserverimpl.getrlimit(self.ctx,resource_type)
        except OSError, e:
            print "Unexpected error with getrlimit for context %d" % self.ctx
            ret = self.get_rlimit_config(type)
        return ret

    def get_rlimit_config(self,type):
        """Generic get resource configuration function for vserver"""
        hard = int(self.config.get("rlimits/%s.hard"%type.lower(),VC_LIM_KEEP))
        soft = int(self.config.get("rlimits/%s.soft"%type.lower(),VC_LIM_KEEP))
        minimum = int(self.config.get("rlimits/%s.min"%type.lower(),VC_LIM_KEEP))
        return (hard,soft,minimum)

    def set_WHITELISTED_config(self,whitelisted):
        self.config.update('whitelisted', whitelisted)

    def set_capabilities(self, capabilities):
        return vserverimpl.setbcaps(self.ctx, vserverimpl.text2bcaps(capabilities))

    def set_capabilities_config(self, capabilities):
        self.config.update('bcapabilities', capabilities)
	self.set_capabilities(capabilities)

    def get_capabilities(self):
        return vserverimpl.bcaps2text(vserverimpl.getbcaps(self.ctx))
 
    def get_capabilities_config(self):
        return self.config.get('bcapabilities')

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
            try:
                vserverimpl.unsetdlimit(self.dir, self.ctx)
            except OSError, e:
                print "Unexpected error with unsetdlimit for context %d" % self.ctx
            return

        if self.vm_running:
            block_usage = vserverimpl.DLIMIT_KEEP
            inode_usage = vserverimpl.DLIMIT_KEEP
        else:
            # init_disk_info() must have been called to get usage values
            block_usage = self.disk_blocks
            inode_usage = self.disk_inodes


        try:
            vserverimpl.setdlimit(self.dir,
                                  self.ctx,
                                  block_usage,
                                  block_limit,
                                  inode_usage,
                                  vserverimpl.DLIMIT_INF,  # inode limit
                                  2)   # %age reserved for root
        except OSError, e:
            print "Unexpected error with setdlimit for context %d" % self.ctx


        self.config.update('dlimits/0/space_total', block_limit)

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

        if sched_flags & SCHED_CPU_GUARANTEED:
            cpu_guaranteed = cpu_share
        else:
            cpu_guaranteed = 0
        self.config.update('sched/fill-rate2', cpu_share)
        self.config.update('sched/fill-rate', cpu_guaranteed)

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
            print >>state_file, "%u" % self.ctx
            state_file.close()

        if vserverimpl.chcontext(self.ctx, vserverimpl.text2bcaps(self.get_capabilities_config())):
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
        self.__do_chroot()
        self.__do_chcontext(None)

    def start(self, wait, runlevel = 3):
        self.vm_running = True
        self.rlimits_changed = False

        child_pid = os.fork()
        if child_pid == 0:
            # child process
            try:
                # get a new session
                os.setsid()

                # open state file to record vserver info
                state_file = open("/var/run/vservers/%s" % self.name, "w")

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
                self.__do_chcontext(state_file)
		for cmd in self.INITSCRIPTS + [None]:
			try:
			    # enter vserver context
			    arg_subst = { 'runlevel': runlevel }
			    cmd_args = [cmd[0]] + map(lambda x: x % arg_subst,
					    cmd[1:])
			    print >>log, "executing '%s'" % " ".join(cmd_args)
			    os.spawnvp(os.P_WAIT,cmd[0],*cmd_args)
			except:
				traceback.print_exc()
				os._exit(1)

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

    def init_disk_info(self):
        cmd = "/usr/sbin/vdu --script --space --inodes --blocksize 1024 --xid %d %s" % (self.ctx, self.dir)
        (child_stdin, child_stdout, child_stderr) = os.popen3(cmd)
        child_stdin.close()
        line = child_stdout.readline()
        if not line:
            sys.stderr.write(child_stderr.readline())
        (space, inodes) = line.split()
        self.disk_inodes = int(inodes)
        self.disk_blocks = int(space)
        #(self.disk_inodes, self.disk_blocks) = vduimpl.vdu(self.dir)

        return self.disk_blocks * 1024

    def stop(self, signal = signal.SIGKILL):
        vserverimpl.killall(self.ctx, signal)
        self.vm_running = False
        self.rlimits_changed = False



def create(vm_name, static = False, ctor = VServer):

    options = []
    if static:
        options += ['--static']
    runcmd.run('vuseradd', options + [vm_name])
    vm_id = pwd.getpwnam(vm_name)[2]

    return ctor(vm_name, vm_id)
