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
  uint_least64_t bcaps = ~vc_get_insecurebcaps();

  if (!PyArg_ParseTuple(args, "I", &ctx))
    return NULL;

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
  xid_t  ctx;
  int  sig;

  if (!PyArg_ParseTuple(args, "Ii", &ctx, &sig))
    return NULL;

  if (vc_ctx_kill(ctx, 0, sig) && errno != ESRCH)
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
