/* Copyright 2005 Princeton University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met: 

* Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
      
* Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following
disclaimer in the documentation and/or other materials provided
with the distribution.
      
* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived
from this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PRINCETON
UNIVERSITY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. 

*/

#include <Python.h>

#include <errno.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stddef.h>

#include "config.h"
#include "pathconfig.h"
#include "virtual.h"
#include "vserver.h"
#include "planetlab.h"
#include "vserver-internal.h"

#define NONE  ({ Py_INCREF(Py_None); Py_None; })

/*
 * context create
 */
static PyObject *
vserver_chcontext(PyObject *self, PyObject *args)
{
  int  ctx_is_new;
  xid_t  ctx;
  uint_least64_t bcaps = 0;

  if (!PyArg_ParseTuple(args, "I|K", &ctx, &bcaps))
    return NULL;
  bcaps |= ~vc_get_insecurebcaps();

  if ((ctx_is_new = pl_chcontext(ctx, bcaps, 0)) < 0)
    return PyErr_SetFromErrno(PyExc_OSError);

  return PyBool_FromLong(ctx_is_new);
}

static PyObject *
vserver_setup_done(PyObject *self, PyObject *args)
{
  xid_t  ctx;

  if (!PyArg_ParseTuple(args, "I", &ctx))
    return NULL;

  if (pl_setup_done(ctx) < 0)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;
}

static PyObject *
vserver_isrunning(PyObject *self, PyObject *args)
{
  xid_t  ctx;
  PyObject *ret;
  struct stat statbuf;
  char fname[64];

  if (!PyArg_ParseTuple(args, "I", &ctx))
    return NULL;

  sprintf(fname,"/proc/virtual/%d", ctx);

  if(stat(&fname[0],&statbuf)==0)
    ret = PyBool_FromLong(1);
  else
    ret = PyBool_FromLong(0);

  return ret;
}

static PyObject *
__vserver_get_rlimit(xid_t xid, int resource) {
  struct vc_rlimit limits;
  PyObject *ret;

  errno = 0;
  if (vc_get_rlimit(xid, resource, &limits)==-1)
    ret = PyErr_SetFromErrno(PyExc_OSError);
  else
    ret = Py_BuildValue("LLL",limits.hard, limits.soft, limits.min);

  return ret;
}

static PyObject *
vserver_get_rlimit(PyObject *self, PyObject *args) {
  xid_t xid;
  int resource;
  PyObject *ret;

  if (!PyArg_ParseTuple(args, "Ii", &xid, &resource))
    ret = NULL;
  else
    ret = __vserver_get_rlimit(xid, resource);

  return ret;
}

static PyObject *
vserver_set_rlimit(PyObject *self, PyObject *args) {
  struct vc_rlimit limits;
  struct rlimit lim;
  xid_t xid;
  int resource, lresource;
  PyObject *ret;

  limits.min = VC_LIM_KEEP;
  limits.soft = VC_LIM_KEEP;
  limits.hard = VC_LIM_KEEP;

  if (!PyArg_ParseTuple(args, "IiLLL", &xid, &resource, &limits.hard, &limits.soft, &limits.min))
    return NULL;

  lresource = resource;
  switch (resource) {
  case VC_VLIMIT_NSOCK:
  case VC_VLIMIT_ANON:
  case VC_VLIMIT_SHMEM:
    goto do_vc_set_rlimit;
  case VC_VLIMIT_OPENFD:
    lresource = RLIMIT_NOFILE;
    break;
  default:
    break;
  }

  getrlimit(lresource,&lim);
  if (adjust_lim(&limits,&lim)) {
    setrlimit(lresource, &lim);
  }

 do_vc_set_rlimit:
  errno = 0;
  if (vc_set_rlimit(xid, resource, &limits)==-1) 
    ret = PyErr_SetFromErrno(PyExc_OSError);
  else
    ret = __vserver_get_rlimit(xid, resource);

  return ret;
}

/*
 * setsched
 */
static PyObject *
vserver_setsched(PyObject *self, PyObject *args)
{
  xid_t  ctx;
  uint32_t  cpu_share;
  uint32_t  cpu_sched_flags = VC_VXF_SCHED_FLAGS;

  if (!PyArg_ParseTuple(args, "II|I", &ctx, &cpu_share, &cpu_sched_flags))
    return NULL;

  /* ESRCH indicates that there are no processes in the context */
  if (pl_setsched(ctx, cpu_share, cpu_sched_flags) &&
      errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;
}

static PyObject *
vserver_get_dlimit(PyObject *self, PyObject *args)
{
  PyObject *res;
  char* path;
  unsigned xid;
  struct vc_ctx_dlimit data;
  int r;

  if (!PyArg_ParseTuple(args, "si", &path,&xid))
    return NULL;

  memset(&data, 0, sizeof(data));
  r = vc_get_dlimit(path, xid, 0, &data);
  if (r>=0) {
    res = Py_BuildValue("(i,i,i,i,i)",
			data.space_used,
			data.space_total,
			data.inodes_used,
			data.inodes_total,
			data.reserved);
  } else {
    res = PyErr_SetFromErrno(PyExc_OSError);
  }

  return res;
}


static PyObject *
vserver_set_dlimit(PyObject *self, PyObject *args)
{
  char* path;
  unsigned xid;
  struct vc_ctx_dlimit data;

  memset(&data,0,sizeof(data));
  if (!PyArg_ParseTuple(args, "siiiiii", &path,
			&xid,
			&data.space_used,
			&data.space_total,
			&data.inodes_used,
			&data.inodes_total,
			&data.reserved))
    return NULL;

  if ((vc_add_dlimit(path, xid, 0) && errno != EEXIST) ||
      vc_set_dlimit(path, xid, 0, &data))
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;	
}

static PyObject *
vserver_unset_dlimit(PyObject *self, PyObject *args)
{
  char  *path;
  unsigned  xid;

  if (!PyArg_ParseTuple(args, "si", &path, &xid))
    return NULL;

  if (vc_rem_dlimit(path, xid, 0) && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;	
}

static PyObject *
vserver_killall(PyObject *self, PyObject *args)
{
  xid_t	ctx;
  int	sig;
  struct vc_ctx_flags cflags = {
    .flagword = 0,
    .mask = VC_VXF_PERSISTENT
  };
  struct vc_net_flags nflags = {
    .flagword = 0,
    .mask = VC_NXF_PERSISTENT
  };

  if (!PyArg_ParseTuple(args, "Ii", &ctx, &sig))
    return NULL;

  if (vc_ctx_kill(ctx, 0, sig) && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  if (vc_set_cflags(ctx, &cflags) && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  if (vc_set_nflags(ctx, &nflags) && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;
}

static PyObject *
vserver_set_bcaps(PyObject *self, PyObject *args)
{
  xid_t ctx;
  struct vc_ctx_caps caps;

  if (!PyArg_ParseTuple(args, "IK", &ctx, &caps.bcaps))
    return NULL;

  caps.bmask = vc_get_insecurebcaps();
  caps.cmask = caps.ccaps = 0;
  if (vc_set_ccaps(ctx, &caps) == -1 && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;
}

static PyObject *
vserver_text2bcaps(PyObject *self, PyObject *args)
{
  struct vc_ctx_caps caps = { .bcaps = 0 };
  const char *list;
  int len;
  struct vc_err_listparser err;

  if (!PyArg_ParseTuple(args, "s#", &list, &len))
    return NULL;

  vc_list2bcap(list, len, &err, &caps);

  return Py_BuildValue("K", caps.bcaps);
}

static PyObject *
vserver_get_bcaps(PyObject *self, PyObject *args)
{
  xid_t ctx;
  struct vc_ctx_caps caps;

  if (!PyArg_ParseTuple(args, "I", &ctx))
    return NULL;

  if (vc_get_ccaps(ctx, &caps) == -1) {
    if (errno != -ESRCH)
      return PyErr_SetFromErrno(PyExc_OSError);
    else
      caps.bcaps = 0;
  }

  return Py_BuildValue("K", caps.bcaps & vc_get_insecurebcaps());
}

static PyObject *
vserver_bcaps2text(PyObject *self, PyObject *args)
{
  struct vc_ctx_caps caps = { .bcaps = 0 };
  PyObject *list;
  const char *cap;

  if (!PyArg_ParseTuple(args, "K", &caps.bcaps))
    return NULL;

  list = PyString_FromString("");

  while ((cap = vc_lobcap2text(&caps.bcaps)) != NULL) {
    if (list == NULL)
      break;
    PyString_ConcatAndDel(&list, PyString_FromFormat(
			  (PyString_Size(list) > 0 ? ",CAP_%s" : "CAP_%s" ),
			  cap));
  }

  return list;
}

static const struct AF_to_vcNET {
  int af;
  vc_net_nx_type vc_net;
  size_t len;
  size_t offset;
} converter[] = {
  { AF_INET,  vcNET_IPV4, sizeof(struct in_addr),  offsetof(struct sockaddr_in,  sin_addr.s_addr) },
  { AF_INET6, vcNET_IPV6, sizeof(struct in6_addr), offsetof(struct sockaddr_in6, sin6_addr.s6_addr) },
  { 0, 0 }
};

static inline int
convert_address(const char *str, vc_net_nx_type *type, void *dst)
{
  const struct AF_to_vcNET *i;
  for (i = converter; i->af; i++) {
    if (inet_pton(i->af, str, dst)) {
      *type = i->vc_net;
      return 0;
    }
  }
  return -1;
}

static int
get_mask(struct vc_net_nx *addr)
{
  const struct AF_to_vcNET *i;
  struct ifaddrs *head, *ifa;
  int ret = 0;

  for (i = converter; i->af; i++) {
    if (i->vc_net == addr->type)
      break;
  }
  if (!i) {
    errno = EINVAL;
    return -1;
  }

  if (getifaddrs(&head) == -1)
    return -1;
  for (ifa = head; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr->sa_family == i->af &&
        memcmp((char *) ifa->ifa_addr + i->offset, addr->ip, i->len) == 0) {
      switch (addr->type) {
      case vcNET_IVP4:
	memcpy(&addr->mask[0], ifa->ifa_netmask + i->offset, i->len);
	break;
      case vcNET_IPV6: {
	uint32_t *m = ((struct sockaddr_in6 *) ifa->ifa_netmask)->sin6_addr.s6_addr32;
	/* optimization for the common case */
	if ((m[1] & 1) == 1 && (m[2] & 0x80000000) == 0)
	  addr->mask[0] = 64;
	else {
	  addr->mask[0] = 0;
	  while (m[addr->mask[0] / 32] & (addr->mask[0] % 32))
	    addr->mask[0]++;
	}
	break;
      }
      ret = 1;
      break;
    }
  }
  /* no match, use a default */
  if (!ret) {
    switch (addr->type) {
    case vcNET_IPV4:	addr->mask[0] = htonl(0xffffff00); break;
    case vcNET_IPV6:	addr->mask[0] = 64; break;
    default:		addr->mask[0] = 0; break;
    }
  }
  freeifaddrs(head);
  return ret;
}

/* XXX These two functions are really similar */
static PyObject *
vserver_net_add(PyObject *self, PyObject *args)
{
  struct vc_net_nx addr;
  nid_t nid;
  const char *ip;

  if (!PyArg_ParseTuple(args, "Is", &nid, &ip))
    return NULL;

  if (convert_address(ip, &addr.type, &addr.ip) == -1)
    return PyErr_Format(PyExc_ValueError, "%s is not a valid IP address", ip);

  switch (get_mask(&addr)) {
  case -1:
    return PyErr_SetFromErrno(PyExc_OSError);
  case 0:
    /* XXX error here? */
    break;
  }
  addr.count = 1;

  if (vc_net_add(nid, &addr) == -1 && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;
}

static PyObject *
vserver_net_remove(PyObject *self, PyObject *args)
{
  struct vc_net_nx addr;
  nid_t nid;
  const char *ip;

  if (!PyArg_ParseTuple(args, "Is", &nid, &ip))
    return NULL;

  if (strcmp(ip, "all") == 0)
    addr.type = vcNET_ANY;
  else if (strcmp(ip, "all4") == 0)
    addr.type = vcNET_IPV4A;
  else if (strcmp(ip, "all6") == 0)
    addr.type = vcNET_IPV6A;
  else
    if (convert_address(ip, &addr.type, &addr.ip) == -1)
      return PyErr_Format(PyExc_ValueError, "%s is not a valid IP address", ip);

  switch (get_mask(&addr)) {
  case -1:
    return PyErr_SetFromErrno(PyExc_OSError);
  }
  addr.count = 1;

  if (vc_net_remove(nid, &addr) == -1 && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return NONE;
}

static PyMethodDef  methods[] = {
  { "chcontext", vserver_chcontext, METH_VARARGS,
    "chcontext to vserver with provided flags" },
  { "setup_done", vserver_setup_done, METH_VARARGS,
    "Release vserver setup lock" },
  { "setsched", vserver_setsched, METH_VARARGS,
    "Change vserver scheduling attributes for given vserver context" },
  { "setdlimit", vserver_set_dlimit, METH_VARARGS,
    "Set disk limits for given vserver context" },
  { "unsetdlimit", vserver_unset_dlimit, METH_VARARGS,
    "Remove disk limits for given vserver context" },
  { "getdlimit", vserver_get_dlimit, METH_VARARGS,
    "Get disk limits for given vserver context" },
  { "setrlimit", vserver_set_rlimit, METH_VARARGS,
    "Set resource limits for given resource of a vserver context" },
  { "getrlimit", vserver_get_rlimit, METH_VARARGS,
    "Get resource limits for given resource of a vserver context" },
  { "killall", vserver_killall, METH_VARARGS,
    "Send signal to all processes in vserver context" },
  { "isrunning", vserver_isrunning, METH_VARARGS,
    "Check if vserver is running"},
  { "setbcaps", vserver_set_bcaps, METH_VARARGS,
    "Set POSIX capabilities of a vserver context" },
  { "getbcaps", vserver_get_bcaps, METH_VARARGS,
    "Get POSIX capabilities of a vserver context" },
  { "text2bcaps", vserver_text2bcaps, METH_VARARGS,
    "Translate a string of capabilities to a bitmap" },
  { "bcaps2text", vserver_bcaps2text, METH_VARARGS,
    "Translate a capability-bitmap into a string" },
  { "netadd", vserver_net_add, METH_VARARGS,
    "Assign an IP address to a context" },
  { "netremove", vserver_net_remove, METH_VARARGS,
    "Remove IP address(es) from a context" },
  { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initvserverimpl(void)
{
  PyObject  *mod;

  mod = Py_InitModule("vserverimpl", methods);

  /* export the set of 'safe' capabilities */
  PyModule_AddIntConstant(mod, "CAP_SAFE", ~vc_get_insecurebcaps());

  /* export the default vserver directory */
  PyModule_AddStringConstant(mod, "VSERVER_BASEDIR", DEFAULT_VSERVERDIR);

  /* export limit-related constants */
  PyModule_AddIntConstant(mod, "DLIMIT_KEEP", (int)VC_CDLIM_KEEP);
  PyModule_AddIntConstant(mod, "DLIMIT_INF", (int)VC_CDLIM_INFINITY);
  PyModule_AddIntConstant(mod, "VC_LIM_KEEP", (int)VC_LIM_KEEP);

  PyModule_AddIntConstant(mod, "RLIMIT_CPU", (int)RLIMIT_CPU);
  PyModule_AddIntConstant(mod, "RLIMIT_RSS", (int)RLIMIT_RSS);
  PyModule_AddIntConstant(mod, "RLIMIT_NPROC", (int)RLIMIT_NPROC);
  PyModule_AddIntConstant(mod, "RLIMIT_NOFILE", (int)RLIMIT_NOFILE);
  PyModule_AddIntConstant(mod, "RLIMIT_MEMLOCK", (int)RLIMIT_MEMLOCK);
  PyModule_AddIntConstant(mod, "RLIMIT_AS", (int)RLIMIT_AS);
  PyModule_AddIntConstant(mod, "RLIMIT_LOCKS", (int)RLIMIT_LOCKS);

  PyModule_AddIntConstant(mod, "RLIMIT_SIGPENDING", (int)RLIMIT_SIGPENDING);
  PyModule_AddIntConstant(mod, "RLIMIT_MSGQUEUE", (int)RLIMIT_MSGQUEUE);

  PyModule_AddIntConstant(mod, "VLIMIT_NSOCK", (int)VC_VLIMIT_NSOCK);
  PyModule_AddIntConstant(mod, "VLIMIT_OPENFD", (int)VC_VLIMIT_OPENFD);
  PyModule_AddIntConstant(mod, "VLIMIT_ANON", (int)VC_VLIMIT_ANON);
  PyModule_AddIntConstant(mod, "VLIMIT_SHMEM", (int)VC_VLIMIT_SHMEM);

  /* scheduler flags */
  PyModule_AddIntConstant(mod,
			  "VS_SCHED_CPU_GUARANTEED",
			  VS_SCHED_CPU_GUARANTEED);
}
