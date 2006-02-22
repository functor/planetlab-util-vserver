#!/usr/bin/python
# 
# Bandwidth limit script to run on PlanetLab nodes. The intent is to use
# the Hierarchical Token Bucket (HTB) queueing discipline (qdisc) to:
#
# 1. Cap the total output bandwidth of the node at a specified rate
# (e.g., 5 Mbps).
#
# 2. Allow slices to fairly share this rate. Some slices have more
# shares than other.
#
# For instance, if the node is capped at 5 Mbps, there are N slices,
# and each slice has 1 share, then each slice should get at least 5/N
# Mbps of bandwidth. How HTB is implemented makes this statement a
# little too simplistic. What it really means is that during any
# single time period, only a certain number of bytes can be sent onto
# the wire. Each slice is guaranteed that at least some small number
# of its bytes will be sent. Whatever is left over from the budget, is
# split in proportion to the number of shares each slice has.
#
# The root context is exempt from this sharing and can send as much as
# it needs to.
#
# Some relevant URLs:
#
# 1. http://lartc.org/howto               for how to use tc
# 2. http://luxik.cdi.cz/~devik/qos/htb/  for info on HTB
#
# Andy Bavier <acb@cs.princeton.edu>
# Mark Huang <mlhuang@cs.princeton.edu>
# Copyright (C) 2006 The Trustees of Princeton University
#
# $Id$
#

import sys, os, re, getopt


# Where the tc binary lives.
TC = "/sbin/tc"

# Default interface.
DEV = "eth0"

# Verbosity level.
verbose = 0

# guarantee is the minimum rate in bits per second that each slice is
# guaranteed. The value of this parameter is fairly meaningless, since
# it is unlikely that every slice will try to transmit full blast
# simultaneously. It just needs to be small enough so that the total
# of all outstanding guarantees is less than or equal to the node
# bandwidth cap (see below). A node with a 500kbit cap (the minimum
# recommended) can support up to 500kbit/1000 = 500 slices.
guarantee = 1000

# quantum is the maximum number of bytes that can be borrowed by a
# share (or slice, if each slice gets 1 share) in one time period
# (with HZ=1000, 1 ms). If multiple slices are competing for bandwidth
# above their guarantees, and each is attempting to borrow up to the
# node bandwidth cap, quantums control how the excess bandwidth is
# distributed. Slices with 2 shares will borrow twice the amount in
# one time period as slices with 1 share, so averaged over time, they
# will get twice as much of the excess bandwidth. The value should be
# as small as possible and at least 1 MTU. By default, it would be
# calculated as guarantee/10, but since we use such small guarantees,
# it's better to just set it to a value safely above 1 Ethernet MTU.
quantum = 1600

# cburst is the maximum number of bytes that can be burst onto the
# wire in one time period (with HZ=1000, 1 ms). If multiple slices
# have data queued for transmission, cbursts control how long each
# slice can have the wire for. If not specified, it is set to the
# smallest possible value that would enable the slice's "ceil" rate
# (usually the node bandwidth cap), to be reached if a slice was able
# to borrow enough bandwidth to do so. For now, it's unclear how or if
# to relate this to the notion of shares, so just let tc set the
# default.

# bwcap is the node bandwidth cap in tc format (see below for
# supported suffixes), read in from /etc/planetlab/bwcap. We allow
# each slice to borrow up to this rate, so it is also usually the
# "ceil" rate for each slice. -1 means disabled.
bwcap_file = "/etc/planetlab/bwcap"
bwcap = -1

# There is another parameter that controls how bandwidth is allocated
# between slices on nodes that is outside the scope of HTB. We enforce
# a 16 GByte/day total limit on each slice, which works out to about
# 1.5mbit. If a slice exceeds this byte limit before the day finishes,
# it is capped at (i.e., its "ceil" rate is set to) the smaller of the
# node bandwidth cap or 1.5mbit. pl_mom is in charge of enforcing this
# rule and executes this script to override "ceil".

# root_minor is the special class for the root context. The root
# context is exempt from minrate and fair sharing.
root_minor = 0x2

# default_minor is the special default class for unclassifiable
# packets. Packets should not be classified here very often. They can
# be if a slice's HTB class is deleted before its processes are.
default_minor = 0xFFFF

# See tc_util.c and http://physics.nist.gov/cuu/Units/binary.html. Be
# warned that older versions of tc interpret "kbps", "mbps", "mbit",
# and "kbit" to mean (in this system) "kibps", "mibps", "mibit", and
# "kibit" and that if an older version is installed, all rates will
# be off by a small fraction.
suffixes = {
    "":         1,
    "bit":	1,
    "kibit":	1024,
    "kbit":	1000,
    "mibit":	1024*1024,
    "mbit":	1000000,
    "gibit":	1024*1024*1024,
    "gbit":	1000000000,
    "tibit":	1024*1024*1024*1024,
    "tbit":	1000000000000,
    "bps":	8,
    "kibps":	8*1024,
    "kbps":	8000,
    "mibps":	8*1024*1024,
    "mbps":	8000000,
    "gibps":	8*1024*1024*1024,
    "gbps":	8000000000,
    "tibps":	8*1024*1024*1024*1024,
    "tbps":	8000000000000
}


# Parses a tc rate string (e.g., 1.5mbit) into bits/second
def get_tc_rate(s):
    m = re.match(r"([0-9.]+)(\D*)", s)
    if m is None:
        return -1
    suffix = m.group(2).lower()
    if suffixes.has_key(suffix):
        return int(float(m.group(1)) * suffixes[suffix])
    else:
        return -1


# Prints a tc rate string
def format_tc_rate(rate):
    if rate >= 1000000:
        return "%.0fmbit" % (rate / 1000000.)
    elif rate >= 1000:
        return "%.0fkbit" % (rate / 1000.)
    else:
        return "%.0fbit" % rate


# Parse /etc/planetlab/bwcap. XXX Should get this from the API
# instead.
def parse_bwcap():
    global bwcap

    try:
        fp = open(bwcap_file, "r")
        line = fp.readline().strip()
        if line:
            bwcap = get_tc_rate(line)
    except:
        pass


# Before doing anything else, parse the node bandwidth cap file
parse_bwcap()


# Get slice xid (500) from slice name ("500" or "princeton_mlh") or
# slice name ("princeton_mlh") from slice xid (500).
def get_slice(xid_or_name):
    labels = ['account', 'password', 'uid', 'gid', 'gecos', 'directory', 'shell']

    for line in file("/etc/passwd"):
        # Comment
        if line.strip() == '' or line[0] in '#':
            continue
        # princeton_mlh:x:...
        fields = line.strip().split(':')
        if len(fields) < len(labels):
            continue
        # {'account': 'princeton_mlh', 'password': 'x', ...}
        pw = dict(zip(labels, fields))
        if xid_or_name == default_minor:
            # Convert 0xffff into "default"
            return "default"
        elif xid_or_name == root_minor:
            # Convert 0x2 into "root"
            return "root"
        elif xid_or_name == int(pw['uid']):
            # Convert xid into name
            return pw['account']
        elif pw['uid'] == xid_or_name or pw['account'] == xid_or_name:
            # Convert name into xid
            return int(pw['uid'])

    return None


# Shortcut for running a tc command
def tc(cmd):
    try:
        if verbose:
            sys.stderr.write("Executing: " + TC + " " + cmd + "\n")
        fileobj = os.popen(TC + " " + cmd, "r")
        output = fileobj.readlines()
        if fileobj.close() is None:
            return output
    except Exception, e:
        pass
    return None


# (Re)initialize the bandwidth limits on this node
def init(dev = DEV):
    # Save current state (if any)
    caps = get(dev = DEV)

    # Delete root qdisc 1: if it exists. This will also automatically
    # delete any child classes.
    for line in tc("qdisc show dev %s" % dev):
        # Search for the root qdisc 1:
        m = re.match(r"qdisc htb 1:", line)
        if m is not None:
            tc("qdisc del dev %s root handle 1:" % dev)
            break

    # Nothing to do
    if bwcap == -1:
        return

    # Initialize HTB. The "default" clause specifies that if a packet
    # fails classification, it should go into the class with handle
    # FFFF.
    tc("qdisc add dev %s root handle 1: htb default FFFF" % dev)

    # Set up the parent class that represents the node bandwidth
    # cap; in other words, the class from which all others borrow.
    tc("class add dev %s parent 1: classid 1:1 htb rate %dbit" % \
       (dev, bwcap))

    # Set up the root class (and tell VNET what it is). Packets sent
    # by root end up here and are capped at the node bandwidth
    # cap.
    on(root_minor, dev, minrate = bwcap, maxrate = bwcap)
    file("/proc/sys/vnet/root_class", "w").write("%d" % ((1 << 16) | root_minor))

    # Set up the default class. Packets that fail classification end
    # up here.
    on(default_minor, dev, maxrate = bwcap)

    # Reapply bandwidth caps. If the node bandwidth cap is now lower
    # than it was before, "ceil" for each class will be lowered. XXX
    # If the node bandwidth cap is now higher than it was before,
    # "ceil" for each class should be raised, but we have no idea
    # whether the lower cap was put on by pl_mom or by an admin, so it
    # is left as it was before, at least until pl_mom gets around to
    # resetting each slice's cap at the beginning of the next
    # day. What *should* happen is that Node Manager should control
    # both the application of the node bandwidth cap and the
    # application of the per-slice bandwidth caps, and there should be
    # only one external caller of this script (pl_mom). Even then,
    # pl_mom should probably be merged into Node Manager at some
    # point.
    for (xid, share, minrate, maxrate) in caps:
        if xid != root_minor and xid != default_minor:
            on(xid, dev, share = share, minrate = minrate, maxrate = maxrate)


# Get the bandwidth limits for a particular slice xid as a tuple (xid,
# share, minrate, maxrate), or all classes as a list of tuples.
def get(xid = None, dev = DEV):
    if xid is None:
        ret = []
    else:
        ret = None

    # class htb 1:2 parent 1:1 leaf 2: prio 0 rate 10Mbit ceil 10Mbit burst 14704b cburst 14704b
    for line in tc("-d class show dev %s" % dev):
        # Search for child classes of 1:1
        m = re.match(r"class htb 1:([0-9a-f]+) parent 1:1", line)
        if m is None:
            continue

        # If we are looking for a particular class
        classid = int(m.group(1), 16)
        if xid is not None and xid != classid:
            continue

        # Parse share
        share = 1
        m = re.search(r"quantum (\d+)", line)
        if m is not None:
            share = int(m.group(1)) / quantum

        # Parse minrate
        minrate = guarantee
        m = re.search(r"rate (\w+)", line)
        if m is not None:
            minrate = get_tc_rate(m.group(1))

        # Parse maxrate 
        maxrate = bwcap
        m = re.search(r"ceil (\w+)", line)
        if m is not None:
            maxrate = get_tc_rate(m.group(1))

        if xid is None:
            # Return a list of parameters
            ret.append((classid, share, minrate, maxrate))
        else:
            # Return the parameters for this class
            ret = (classid, share, minrate, maxrate)
            break

    return ret


# Apply specified bandwidth limit to the specified slice xid
def on(xid, dev = DEV, share = None, minrate = None, maxrate = None):
    # Get defaults from current state if available
    cap = get(xid, dev)
    if cap is not None:
        if share is None:
            share = cap[1]
        if minrate is None:
            minrate = cap[2]
        if maxrate is None:
            maxrate = cap[3]

    # Set defaults
    if share is None:
        share = 1
    if minrate is None:
        minrate = guarantee
    if maxrate is None:
        maxrate = bwcap

    # Allow slices to burst up to the node bandwidth cap by default.
    maxrate = min(maxrate, bwcap)

    # Set up a class for the slice.
    tc("class replace dev %s parent 1:1 classid 1:%x htb rate %dbit ceil %dbit quantum %d" % \
       (dev, xid, minrate, maxrate, share * quantum))

    # Attach a FIFO to the class, which helps to throttle back
    # processes that are sending faster than the token bucket can
    # support.
    tc("qdisc replace dev %s parent 1:%x handle %x pfifo" % \
       (dev, xid, xid))


# Remove class associated with specified slice xid. If further packets
# are seen from this slice, they will be classified into the default
# class 1:FFFF.
def off(xid, dev = DEV):
    tc("class del dev %s classid 1:%x" % (dev, xid))


def usage():
    if bwcap == -1:
        bwcap_description = "disabled"
    else:
        bwcap_description = "%d bits/second" % bwcap
        
    print """
Usage:

%s [OPTION]... [COMMAND] [ARGUMENT]...

Options (override configuration file):
	-f file         Configuration file (default: %s)
	-d device	Network interface (default: %s)
        -r rate         Node bandwidth cap (default: %s)
        -g guarantee    Default minimum slice rate (default: %d bits/second)
        -q quantum      Share multiplier (default: %d bytes)
        -h              This message

Commands:
        init
                (Re)load configuration and (re)initialize bandwidth caps.
        on
                Same as init
        on slice [share] [minrate] [maxrate]
                Set bandwidth cap for the specified slice
        off
                Remove all bandwidth caps
        off slice
                Remove bandwidth caps for the specified slice
        get
                Get all bandwidth caps
        get slice
                Get bandwidth caps for the specified slice
        getcap slice
                Get maxrate for the specified slice
        setcap slice maxrate
                Set maxrate for the specified slice
""" % (sys.argv[0], bwcap_file, DEV, bwcap_description, guarantee, quantum)
    sys.exit(1)
    

def main():
    global DEV, bwcap_file, bwcap, guarantee, quantum, verbose

    (opts, argv) = getopt.getopt(sys.argv[1:], "f:d:r:g:q:vh")
    for (opt, optval) in opts:
        if opt == '-f':
            bwcap_file = optval
            parse_bwcap()
        elif opt == '-d':
            DEV = optval
        elif opt == '-r':
            bwcap = get_tc_rate(optval)
        elif opt == '-g':
            guarantee = get_tc_rate(optval)
        elif opt == '-q':
            quantum = int(optval)
        elif opt == '-v':
            verbose += 1
        elif opt == '-h':
            usage()

    if len(argv):
        if argv[0] == "init" or (argv[0] == "on" and len(argv) == 1):
            # (Re)initialize
            init(DEV)

        elif argv[0] == "off" and len(argv) == 1:
            # Disable all caps
            bwcap = -1
            init(DEV)
            sys.stderr.write("Warning: all configured bandwidth limits have been removed\n")

        elif argv[0] == "get" or argv[0] == "show":
            # Show
            if len(argv) >= 2:
                # Show a particular slice
                xid = get_slice(argv[1])
                if xid is None:
                    sys.stderr.write("Error: Invalid slice name or context '%s'\n" % argv[1])
                    usage()
                caps = [get(xid, DEV)]
            else:
                # Show all slices
                caps = get(None, DEV)

            for (xid, share, minrate, maxrate) in caps:
                slice = get_slice(xid)
                if slice is None:
                    # Orphaned (not associated with a slice) class
                    slice = "%d?" % xid
                print "%s: share %d minrate %s maxrate %s" % \
                      (slice, share, format_tc_rate(minrate), format_tc_rate(maxrate))

        elif len(argv) >= 2:
            # slice, ...
            xid = get_slice(argv[1])
            if xid is None:
                sys.stderr.write("Error: Invalid slice name or context '%s'\n" % argv[1])
                usage()

            if argv[0] == "on" or argv[0] == "add" or argv[0] == "replace":
                # Enable cap
                args = []
                if len(argv) >= 3:
                    # ... share, minrate, maxrate
                    casts = [int, get_tc_rate, get_tc_rate]
                    for i, arg in enumerate(argv[2:]):
                        if i >= len(casts):
                            break
                        args.append(casts[i](arg))
                on(xid, DEV, *args)

            elif argv[0] == "off" or argv[0] == "del":
                # Disable cap
                off(xid, DEV)

            # Backward compatibility with old resman script
            elif argv[0] == "getcap":
                # Get maxrate
                cap = get(xid, DEV)
                if cap is not None:
                    (xid, share, minrate, maxrate) = cap
                    print format_tc_rate(maxrate)

            # Backward compatibility with old resman script
            elif argv[0] == "setcap":
                if len(argv) >= 3:
                    # Set maxrate
                    on(xid, DEV, maxrate = get_tc_rate(argv[2]))
                else:
                    usage()

            else:
                usage()

        else:
            usage()


if __name__ == '__main__':
    main()
