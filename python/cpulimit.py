#!/usr/bin/python2 -u

import sys, os, re, string


TCBASE="/rcfs/taskclass/"
RULES="/rcfs/ce/rules/"

SYSCLASS=TCBASE + "system"
SYSRULE=RULES + "system"
SYSCPUSHARE=100
DEFAULTMAXCPUSHARE=8192

def checkckrm():
    checks = [ "/rcfs", TCBASE, RULES ]

    for check in checks:
        try:
            answer = os.stat(check)
        except:
            print "%s does not exist" % check
            return False
    
    return True

def checkclass(tc):
    try:
        answer = os.stat(TCBASE + tc)
        return True

    except:
        print "%s class does not exist" % tc
        return False

def getxid(name):
    xid = -1
    fp = open('/etc/passwd')
    for line in fp.readlines():
        rec = string.splitfields(line,':')
        if rec[0] == name:
            xid = int(rec[2])
            break

    fp.close()
    
    if xid == -1:
        # raise an exception
        pass

    return xid

def write(filename,s):
    fp = os.open(filename,os.O_WRONLY|os.O_CREAT)
    os.write(fp,s)
    os.close(fp)

def vs2ckrm_on(tc):
    xid = getxid(tc)

    try:
        os.mkdir(TCBASE + tc)
    except OSError:
        pass # ignore oserror for file exists
    
    s = "xid=%d,class=%s" % (xid,TCBASE+tc)
    fname = RULES + tc
    write(fname, s)

def vs2ckrm_off(tc):
    fname = TCBASE + tc + "/members"
    for i in range(1,15):
        fp = open(fname)
        lines = fp.readlines()
        try:
            lines.remove("No data to display\n")
        except ValueError:
            pass
        if len(lines) == 0:
            try:
                answer = os.stat(RULES + tc)
                os.unlink(RULES + tc)
                answer = os.stat(TCBASE + tc)                
                os.rmdir(TCBASE + tc)
            except:
                pass
            break

        else:
            print "enter context 1 and kill processes", lines
        

def cpulimit(tc,limit):
    global TCBASE

    fname = TCBASE + tc + "/shares"
    s = "res=cpu,guarantee=%d\n" % limit
    write(fname,s)

def cpuinit():
    global TCBASE

    fname = TCBASE + "shares"
    s = "res=cpu,total_guarantee=%d\n" % DEFAULTMAXCPUSHARE
    write(fname,s)

if __name__ == "__main__":
    try:
        name = sys.argv[1]
        limit = int(sys.argv[2])
    except:
        print "caught exception"

    if checkckrm() is True:
        cpuinit()
        vs2ckrm_on(name)
        cpulimit(name,limit)
        vs2ckrm_off(name)
