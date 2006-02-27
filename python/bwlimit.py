#!/usr/bin/python
# 
# Bandwidth limit module for PlanetLab nodes. The intent is to use the
# Hierarchical Token Bucket (HTB) queueing discipline (qdisc) to allow
# slices to fairly share access to available node bandwidth. We
# currently define three classes of "available node bandwidth":
#
# 1. Available hardware bandwidth (bwmax): The maximum rate of the
# hardware.
#
# 2. Available capped bandwidth (bwcap): The maximum rate allowed to
# non-exempt destinations. By default, equal to bwmax, but may be
# lowered by PIs.
#
# 3. Available uncapped ("exempt") bandwidth: The difference between
# bwmax and what is currently being used of bwcap, or the maximum rate
# allowed to destinations exempt from caps (e.g., Internet2).
#
# All three classes of bandwidth are fairly shared according to the
# notion of "shares". For instance, if the node is capped at 5 Mbps,
# there are N slices, and each slice has 1 share, then each slice
# should get at least 5/N Mbps of bandwidth. How HTB is implemented
# makes this statement a little too simplistic. What it really means
# is that during any single time period, only a certain number of
# bytes can be sent onto the wire. Each slice is guaranteed that at
# least some small number of its bytes will be sent. Whatever is left
# over from the budget, is split in proportion to the number of shares
# each slice has.
#
# Even if the node is not capped at a particular limit (bwcap ==
# bwmax), this module enforces fair share access to bwmax. Also, if
# the node is capped at a particular limit, rules may optionally be
# defined that classify certain packets into the "exempt" class. This
# class receives whatever bandwidth is leftover between bwcap and
# bwmax; slices fairly share this bandwidth as well.
#
# The root context is exempt from sharing and can send as much as it
# needs to.
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
# $Id: bwlimit.py,v 1.4 2006/02/22 23:46:51 mlhuang Exp $
#

import sys, os, re, getopt


# Where the tc binary lives.
TC = "/sbin/tc"

# Default interface.
dev = "eth0"

# For backward compatibility, if bwcap is not specified, attempt to
# get it from here.
bwcap_file = "/etc/planetlab/bwcap"

# Verbosity level.
verbose = 0

# guarantee is the minimum rate in bits per second that each slice is
# guaranteed. The value of this parameter is fairly meaningless, since
# it is unlikely that every slice will try to transmit full blast
# simultaneously. It just needs to be small enough so that the total
# of all outstanding guarantees is less than or equal to the node
# bandwidth cap (see below). A node with a 500kbit cap (the minimum
# recommended) can support up to 500kbit/1kbit = 500 slices.
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

# bwmax should just be large enough that it can be considered
# "unlimited".
bwmax = 1000*1000*1000

# There is another parameter that controls how bandwidth is allocated
# between slices on nodes that is outside the scope of HTB. We enforce
# a 16 GByte/day total limit on each slice, which works out to about
# 1.5mbit. If a slice exceeds this byte limit before the day finishes,
# it is capped at (i.e., its "ceil" rate is set to) the smaller of the
# node bandwidth cap or 1.5mbit. pl_mom is in charge of enforcing this
# rule and executes this script to override "ceil".

# We can support multiple bandwidth limits, by reserving the top
# nibble of the minor classid to be the "subclassid". Theoretically,
# we could support up to 15 subclasses, but for now, we only define
# two: the "default" subclass 1:1 that is capped at the node bandwidth
# cap (in this example, 5mbit) and the "exempt" subclass 1:2 that is
# capped at the hardware speed (in this example, 1gbit). The "exempt"
# subclass is entitled to whatever bandwidth is leftover after the
# node bandwidth cap is reached, and is fairly shared amongst non-root
# slices.
#
#                        1:
# 1:1 (5mbit, 5mbit)            1:2 (1gbit, 1gbit)
#
# 1:1000 (1, 5mbit, 5mbit)      1:2000 (1gbit, 1gbit)
# 1:1001 (1, 1kbit, 5mbit)      1:2001 (1kbit, 1gbit)
# 1:1002 (1, 1kbit, 5mbit)      1:2002 (1kbit, 1gbit)
# ...                           ...
# 1:1FFF (1, 1kbit, 5mbit)      1:2FFF (1kbit, 1gbit)
#
default_minor = 0x1000
exempt_minor = 0x2000

# root_xid is for the root context. The root context is exempt from
# fair sharing in both the default and exempt subclasses..
root_xid = 0x0000

# default_xid is for unclassifiable packets. Packets should not be
# classified here very often. They can be if a slice's HTB classes are
# deleted before its processes are.
default_xid = 0x0FFF

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


# Parses an integer or a tc rate string (e.g., 1.5mbit) into bits/second
def get_tc_rate(s):
    if type(s) == int:
        return s
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
def get_bwcap():
    bwcap = bwmax
    try:
        fp = open(bwcap_file, "r")
        line = fp.readline().strip()
        if line:
            bwcap = get_tc_rate(line)
    except:
        pass
    if bwcap == -1:
        bwcap = bwmax
    return bwcap


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
        if xid_or_name == root_xid:
            return "root"
        if xid_or_name == default_xid:
            return "default"
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
def init(dev = dev, bwcap = None):
    if bwcap is None:
        # For backward compatibility, if bwcap is not specified,
        # attempt to get it from /etc/planetlab/bwcap.
        bwcap = get_bwcap()
    else:
        # Allow bwcap to be specified as a tc rate string
        bwcap = get_tc_rate(bwcap)

    # Save current state (if any)
    caps = get(dev = dev)

    # Delete root qdisc 1: if it exists. This will also automatically
    # delete any child classes.
    for line in tc("qdisc show dev %s" % dev):
        # Search for the root qdisc 1:
        m = re.match(r"qdisc htb 1:", line)
        if m is not None:
            tc("qdisc del dev %s root handle 1:" % dev)
            break

    # Initialize HTB. The "default" clause specifies that if a packet
    # fails classification, it should go into the class with handle
    # 1FFF.
    tc("qdisc add dev %s root handle 1: htb default %x" % \
       (dev, default_minor | default_xid))

    # Set up a subclass that represents the node bandwidth cap. We
    # allow each slice to borrow up to this rate, so it is also
    # usually the "ceil" rate for each slice.
    tc("class add dev %s parent 1: classid 1:1 htb rate %dbit" % \
       (dev, bwcap))

    # Set up a subclass that represents "exemption" from the node
    # bandwidth cap. It gets whatever bandwidth is leftover after
    # applying the node bandwidth cap to non-exempt packets.
    tc("class add dev %s parent 1: classid 1:2 htb rate %dbit" % \
       (dev, bwmax))

    # Set up the root class (and tell VNET what it is). Packets sent
    # by root end up here and are capped at the node bandwidth
    # cap.
    on(root_xid, dev, minrate = bwmax, maxrate = bwmax)
    file("/proc/sys/vnet/root_class", "w").write("%d" % ((1 << 16) | default_minor | root_xid))

    # Set up the default class. Packets that fail classification end
    # up here.
    on(default_xid, dev, maxrate = bwcap)

    # Reapply bandwidth caps. If the node bandwidth cap is now lower
    # than it was before, "ceil" for each class will be lowered. If
    # the node bandwidth cap is now higher than it was before, "ceil"
    # for each class should be reapplied.
    for (xid, share, minrate, maxrate) in caps:
        if xid != 0 and xid != default_xid:
            on(xid, dev, share = share, minrate = minrate, maxrate = maxrate)


# Get the bandwidth limits for a particular slice xid as a tuple (xid,
# share, minrate, maxrate), or all classes as a list of tuples.
def get(xid = None, dev = dev):
    if xid is None:
        ret = []
    else:
        ret = None

    # class htb 1:1002 parent 1:1 leaf 1002: prio 0 rate 10Mbit ceil 10Mbit burst 14704b cburst 14704b
    for line in tc("-d class show dev %s" % dev):
        # Search for child classes of 1:1
        m = re.match(r"class htb 1:([0-9a-f]+) parent 1:1", line)
        if m is None:
            continue

        # If we are looking for a particular class
        classid = int(m.group(1), 16) & default_xid
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
        maxrate = bwmax
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
def on(xid, dev = dev, share = None, minrate = None, maxrate = None):
    # Get defaults from current state if available
    cap = get(xid, dev)
    if cap is not None:
        if share is None:
            share = cap[1]
        if minrate is None:
            minrate = cap[2]
        if maxrate is None:
            maxrate = cap[3]

    # Figure out what the current node bandwidth cap is
    bwcap = bwmax
    for line in tc("-d class show dev %s" % dev):
        # Search for 1:1
        m = re.match(r"class htb 1:1 root .*ceil (\w+)", line)
        if m is not None:
            bwcap = get_tc_rate(m.group(1))
            break

    # Set defaults
    if share is None:
        share = 1
    if minrate is None:
        minrate = guarantee
    else:
        minrate = get_tc_rate(minrate)
    if maxrate is None:
        maxrate = bwcap
    else:
        maxrate = get_tc_rate(maxrate)

    if minrate > maxrate:
        minrate = maxrate

    # Set up subclasses for the slice.
    tc("class replace dev %s parent 1:1 classid 1:%x htb rate %dbit ceil %dbit quantum %d" % \
       (dev, default_minor | xid, min(minrate, bwcap), min(maxrate, bwcap), share * quantum))

    tc("class replace dev %s parent 1:2 classid 1:%x htb rate %dbit ceil %dbit quantum %d" % \
       (dev, exempt_minor | xid, min(minrate, bwmax), bwmax, share * quantum))

    # Attach a FIFO to each subclass, which helps to throttle back
    # processes that are sending faster than the token buckets can
    # support.
    tc("qdisc replace dev %s parent 1:%x handle %x pfifo" % \
       (dev, default_minor | xid, default_minor | xid))

    tc("qdisc replace dev %s parent 1:%x handle %x pfifo" % \
       (dev, exempt_minor | xid, exempt_minor | xid))


# Remove class associated with specified slice xid. If further packets
# are seen from this slice, they will be classified into the default
# class 1:1FFF.
def off(xid, dev = dev):
    tc("class del dev %s classid 1:%x" % (dev, default_minor | xid))
    tc("class del dev %s classid 1:%x" % (dev, exempt_minor | xid))


def usage():
    bwcap_description = format_tc_rate(bwmax)
        
    print """
Usage:

%s [OPTION]... [COMMAND] [ARGUMENT]...

Options:
	-d device	Network interface (default: %s)
        -r rate         Node bandwidth cap (default: %s)
        -g guarantee    Default minimum slice rate (default: %s bits/second)
        -q quantum      Share multiplier (default: %d bytes)
        -h              This message

Commands:
        init
                (Re)initialize bandwidth caps.
        on slice [share] [minrate] [maxrate]
                Set bandwidth cap for the specified slice
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
""" % (sys.argv[0], dev, bwcap_description, guarantee, quantum)
    sys.exit(1)
    

def main():
    global dev, guarantee, quantum, verbose

    # Defaults
    bwcap = get_bwcap()

    (opts, argv) = getopt.getopt(sys.argv[1:], "f:d:r:g:q:vh")
    for (opt, optval) in opts:
        if opt == '-d':
            dev = optval
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
            init(dev, bwcap)

        elif argv[0] == "get" or argv[0] == "show":
            # Show
            if len(argv) >= 2:
                # Show a particular slice
                xid = get_slice(argv[1])
                if xid is None:
                    sys.stderr.write("Error: Invalid slice name or context '%s'\n" % argv[1])
                    usage()
                caps = [get(xid, dev)]
            else:
                # Show all slices
                caps = get(None, dev)

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
                on(xid, dev, *args)

            elif argv[0] == "off" or argv[0] == "del":
                # Disable cap
                off(xid, dev)

            # Backward compatibility with old resman script
            elif argv[0] == "getcap":
                # Get maxrate
                cap = get(xid, dev)
                if cap is not None:
                    (xid, share, minrate, maxrate) = cap
                    print format_tc_rate(maxrate)

            # Backward compatibility with old resman script
            elif argv[0] == "setcap":
                if len(argv) >= 3:
                    # Set maxrate
                    on(xid, dev, maxrate = get_tc_rate(argv[2]))
                else:
                    usage()

            else:
                usage()

        else:
            usage()


if __name__ == '__main__':
    main()
