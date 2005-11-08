#!/bin/env python2 -u

# Based on code written by: Andy Bavier, acb@cs.princeton.edu
# 
# Bandwidth limit script to run on PlanetLab nodes.  The intent is to use
# the Hierarchical Token Bucket queueing discipline of 'tc' to (1) cap 
# the output bandwidth of the node at a specified rate (e.g., 5Mbps) and 
# (2) to allow all vservers to fairly share this rate.  For instance,
# if there are N vservers, then each should get at least 5/N Mbps of 
# bandwidth.
#
# Some relevant URLs:
#   http://lartc.org/howto               for how to use tc
#   http://luxik.cdi.cz/~devik/qos/htb/  for info on htb

import sys, os, re, string

# Global variables
TC="/sbin/tc"                 # Where the modified tc program lives
OPS = ["change","add"]  # Sequence of TC ops we'll try

# Support to run system commands
import runcmd
def run(cmd):
    try:
        runcmd.run(cmd)
        ret = True
    except runcmd.Error, ex:
        ret = False

    return ret

def get_defaults(cap_file="/etc/planetlab/bwcap", default_cap="10mbit"):
    # The maximum output bandwidth, read in from cap_file (if it
    # exists). If cap_file does not exist, use default_cap for
    # bandwidth cap.  See also the 'cburst' parameter below.
    cap=default_cap
    try:
        os.stat(cap_file)
        fp = open(cap_file)
        lines = fp.readlines()
        fp.close()
        try:
            cap=string.strip(lines[0])
        except ValueError, ex:
            pass
    except OSError, ex:
        pass

    # How many bytes a single token bucket is allowed to send at once.
    # Small values (i.e., 3080 = two maximum-sized Ethernet packets)
    # provide better fine-grained fairness.  At high rates (e.g.,
    # cap=100mbit) this needs to be raised to allow full throughput.
    cburst=30800

    # The 'share' and 'quantum' parameters both influence the actual throughput
    # seen by a particular vserver:

    # 'share' is the rate at which tokens fill the bucket, and so is
    # the minimum bandwidth given to the task.  I think this just
    # needs to be set to some small value that is the same for all
    # vservers.  With the current value and a 5mbit cap, we can
    # support 5000 vservers (5mbit/1kbit = 5000).  With values lower
    # than 10kbit, the HTB output (from tc -s -d class dev eth0) looks
    # strange... this needs to be looked into further.
    share="1kbit"

    # 'quantum' influences how excess bandwidth (i.e., above the
    # 'share') is distributed to vservers.  Apparently, vservers can
    # send additional packets in proportion to their quantums (and not
    # their shares, as one might expect).  See:
    #   http://luxik.cdi.cz/~devik/qos/htb/manual/userg.htm#sharing
    #   The above link states that 'quantum' is automatically
    #   calculated for shares above 120kbit.  Otherwise it should be
    #   set to a small value but at least one MTU, so I set it to one
    #   MTU.  All vservers are assigned the same quantum and so they
    #   should share equally.
    quantum=1540

    return cap, cburst, share, quantum


def init(eth):
    global TC, OPS

    cap, cburst, share, quantum = get_defaults()
    if cap == "-1": return

    # Install HTB on $ETH.  Specifies that all packets not matching a
    # filter rule go to class with handle 9999.  If we don't supply a
    # default class, it sounds like non-matching packets can be sent
    # at an unlimited rate.
    for op in OPS:
        cmd = "%s qdisc %s dev %s root handle 1: htb default 9999" % (TC,op,eth)
        if run(cmd): break

    # Add a root class with bwcap capped rate
    for op in OPS:
        cmd = "%s class %s dev %s parent 1: classid 1:1 htb rate %s quantum %d" % \
              (TC, op, eth, cap, quantum)
        if run(cmd): break

    # Set up the default class.  Packets will fail to match a filter rule
    # and end up here if they are sent by a process with UID < 500.
    for op in OPS:
        cmd = "%s class %s dev %s parent 1:1 classid 1:9999 htb rate %s ceil %s quantum %d cburst %d" % \
              (TC, op, eth, share, cap, quantum, cburst)
        if run(cmd): break

    # The next command appears to throttle back processes that are
    # sending faster than the token bucket can support, rather than
    # just dropping their packets.
    for op in OPS:
        cmd = "%s qdisc %s dev %s parent 1:9999 handle 9999 pfifo" % \
              (TC, op, eth)
        if run(cmd): break

def on(xid, eth, share, minrate, maxrate = None):
    global TC, OPS

    default_cap, default_cburst, default_share, default_quantum = get_defaults()
    if maxrate == None:
        maxrate = default_cap
    quantum = share * default_quantum

    # Set up the per-vserver token bucket
    for op in OPS:
        cmd = "%s class %s dev %s parent 1:1 classid 1:%d htb rate %s ceil %s quantum %d cburst %d" % \
              (TC, op, eth, xid, minrate, maxrate, quantum, default_cburst)
        if run(cmd): break

    # The next command appears to throttle back processes that are
    # sending faster than the token bucket can support, rather than
    # just dropping their packets.
    for op in OPS:
        cmd = "%s qdisc %s dev %s parent 1:%d handle %d pfifo" % \
              (TC, op, eth, xid, xid)
        if run(cmd): break

    # Matches packets sent by a vserver to the appropriate token bucket.
    # The raw socket module marks each packet with its vserver id.
    # See: http://lartc.org/howto/lartc.qdisc.filters.html for more
    # info on the filter command.
    cmd = "%s filter del dev %s protocol ip prio %d" % (TC, eth, xid)
    run(cmd)
    cmd = "%s filter add dev %s prio %d parent 1:0 protocol ip handle %d fw flowid 1:%d" % \
          (TC, eth, xid, xid, xid)
    run(cmd)

def off(xid, eth):
    cmd = "%s filter del dev %s protocol ip prio %d" % (TC, eth, xid)
    run(cmd)

    cmd = "%s qdisc del dev %s parent 1:%d" % (TC, eth, xid)
    run(cmd)

    cmd = "%s class del dev %s classid 1:%d" % (TC, eth, xid)
    run(cmd)

    
