#!/usr/bin/python
# 
# PLCAPI access from nodes. Placeholder until we can do this for real
# using certificate- or session-based authentication. Node Manager
# should provide some localhost interface for retrieving the proper
# authentication parameters as well as the API server to use.
#
# Mark Huang <mlhuang@cs.princeton.edu>
# Copyright (C) 2006 The Trustees of Princeton University
#
# $Id: bwlimit.py,v 1.7 2006/03/01 18:54:38 mlhuang Exp $
#

import xmlrpclib

class PLCAPI(xmlrpclib.Server):
    def __init__(self):
        # XXX Only support anonymous calls for now
        self.auth = {'AuthMethod': 'anonymous'}
        xmlrpclib.Server.__init__(self,
                                  uri = "https://www.planet-lab.org/PLCAPI/",
                                  encoding = "utf-8")
