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

#include "config.h"
#include "compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include "vserver.h"
#include "vserver-internal.h"
#include "sched_cmd.h"
#include "virtual.h"

#define MEF_DEBUG 1
/*
 * context create
 */
static PyObject *
vserver_chcontext(PyObject *self, PyObject *args)
{
	xid_t xid, ctx;
	struct vc_ctx_caps caps;
	struct vc_ctx_flags flags;
	struct vc_vx_info vc;
	unsigned long long v;

	v = VC_VXF_STATE_SETUP;
	if (!PyArg_ParseTuple(args, "I|K", &ctx, &v))
		return NULL;

	caps.ccaps = ~vc_get_insecureccaps();
	caps.cmask = ~0ull;
	caps.bcaps = ~vc_get_insecurebcaps();
	caps.bmask = ~0ull;

	xid = VC_NOCTX;
	if (vc_get_vx_info(ctx,&vc) != 0) {
		xid = vc_ctx_create(ctx);
		if (xid == VC_NOCTX && errno != EEXIST)
			return PyErr_SetFromErrno(PyExc_OSError);
	}

	flags.mask = flags.flagword = v;
	if (vc_set_cflags(ctx, &flags) == -1)
		return PyErr_SetFromErrno(PyExc_OSError);

	if (xid == VC_NOCTX && vc_ctx_migrate(ctx) == -1)
		return PyErr_SetFromErrno(PyExc_OSError);

#ifdef MEF_DEBUG
	printf("vserver_create xid = %d(%d)\n",xid,ctx);
#endif
	return Py_None;
}

static PyObject *
vserver_set_rlimit(PyObject *self, PyObject *args) {
	struct vc_rlimit limits;
	xid_t xid;
	int resource;
	PyObject *ret;

	limits.min = VC_LIM_KEEP;
	limits.soft = VC_LIM_KEEP;
	limits.hard = VC_LIM_KEEP;

	if (!PyArg_ParseTuple(args, "IiL", &xid, &resource, &limits.hard))
		return NULL;

	ret = Py_None;
	if (vc_set_rlimit(xid, resource, &limits)) 
		ret = PyErr_SetFromErrno(PyExc_OSError);
	else if (vc_get_rlimit(xid, resource, &limits)==-1)
		ret = PyErr_SetFromErrno(PyExc_OSError);
	else
		ret = Py_BuildValue("L",limits.hard);

	return ret;
}

static PyObject *
vserver_get_rlimit(PyObject *self, PyObject *args) {
	struct vc_rlimit limits;
	xid_t xid;
	int resource;
	PyObject *ret;

	limits.min = VC_LIM_KEEP;
	limits.soft = VC_LIM_KEEP;
	limits.hard = VC_LIM_KEEP;

	if (!PyArg_ParseTuple(args, "Ii", &xid, &resource))
		return NULL;

	ret = Py_None;
	if (vc_get_rlimit(xid, resource, &limits)==-1)
		ret = PyErr_SetFromErrno(PyExc_OSError);
	else
		ret = Py_BuildValue("L",limits.hard);

	return ret;
}

/*
 * setsched
 */
static PyObject *
vserver_setsched(PyObject *self, PyObject *args)
{
  xid_t  xid;
  struct vc_set_sched sched;
  struct vc_ctx_flags flags;
  unsigned cpuguaranteed = 0;

  sched.set_mask = (VC_VXSM_FILL_RATE | 
		    VC_VXSM_INTERVAL | 
		    VC_VXSM_TOKENS_MIN | 
		    VC_VXSM_TOKENS_MAX);

  if (!PyArg_ParseTuple(args, "I|I|I|I|I|I|I", &xid, 
			&sched.fill_rate,
			&sched.interval,
			&sched.tokens,
			&sched.tokens_min,
			&sched.tokens_max,
			&cpuguaranteed))
    return NULL;

  flags.flagword = VC_VXF_SCHED_HARD;
  flags.mask |= VC_VXF_SCHED_HARD;
#define VC_VXF_SCHED_SHARE       0x00000800ull
  if (cpuguaranteed==0) {
	  flags.flagword |= VC_VXF_SCHED_SHARE;
	  flags.mask |= VC_VXF_SCHED_SHARE;
  }

  if (vc_set_cflags(xid, &flags) == -1)
	  return PyErr_SetFromErrno(PyExc_OSError);

  if (vc_set_sched(xid, &sched) == -1)
	  return PyErr_SetFromErrno(PyExc_OSError);

  return Py_None;
}

/*
 * setsched
 */

/*  inode vserver commands */
#define VCMD_add_dlimit		VC_CMD(DLIMIT, 1, 0)
#define VCMD_rem_dlimit		VC_CMD(DLIMIT, 2, 0)
#define VCMD_set_dlimit		VC_CMD(DLIMIT, 5, 0)
#define VCMD_get_dlimit		VC_CMD(DLIMIT, 6, 0)

#define CDLIM_UNSET             (0ULL)
#define CDLIM_INFINITY          (~0ULL)
#define CDLIM_KEEP              (~1ULL)

static PyObject *
vserver_get_dlimit(PyObject *self, PyObject *args)
{
	PyObject *res;
	char* path;
	unsigned xid;
	struct vcmd_ctx_dlimit_v0 data;
	int r;

	if (!PyArg_ParseTuple(args, "si", &path,&xid))
		return NULL;

	memset(&data, 0, sizeof(data));
	data.name = path;
	data.flags = 0;
	r = vserver(VCMD_get_dlimit, xid, &data);
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
	struct vcmd_ctx_dlimit_base_v0 init;
	struct vcmd_ctx_dlimit_v0 data;
	int r;

	memset(&data,0,sizeof(data));
	if (!PyArg_ParseTuple(args, "siiiiii", &path,
			      &xid,
			      &data.space_used,
			      &data.space_total,
			      &data.inodes_used,
			      &data.inodes_total,
			      &data.reserved))
		return NULL;

	data.name = path;
	data.flags = 0;

	memset(&init, 0, sizeof(init));
	init.name = path;
	init.flags = 0;

	r = vserver(VCMD_rem_dlimit, xid, &init);
	if (r<0){}
	r = vserver(VCMD_add_dlimit, xid, &init);
	if (r<0){}
	r = vserver(VCMD_set_dlimit, xid, &data);
	if (r<0){}
	return Py_None;	
}

static PyMethodDef  methods[] = {
  { "chcontext", vserver_chcontext, METH_VARARGS,
    "chcontext to vserver with provided flags" },
  { "setsched", vserver_setsched, METH_VARARGS,
    "Change vserver scheduling attributes for given vserver context" },
  { "setdlimit", vserver_set_dlimit, METH_VARARGS,
    "Set disk limits for given vserver context" },
  { "getdlimit", vserver_get_dlimit, METH_VARARGS,
    "Get disk limits for given vserver context" },
  { "setrlimit", vserver_set_rlimit, METH_VARARGS,
    "Set resource limits for given resource of a vserver context" },
  { "getrlimit", vserver_get_rlimit, METH_VARARGS,
    "Get resource limits for given resource of a vserver context" },
  { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initvserverimpl(void)
{
  Py_InitModule("vserverimpl", methods);
}
