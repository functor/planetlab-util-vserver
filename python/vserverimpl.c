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
#include <unistd.h>

#include "config.h"
#include "pathconfig.h"
#include "planetlab.h"
#include "virtual.h"
#include "vserver.h"
#include "vserver-internal.h"

/*
 * context create
 */
static PyObject *
vserver_chcontext(PyObject *self, PyObject *args)
{
  xid_t  ctx;
  uint32_t  flags = 0;
  uint32_t  bcaps = ~vc_get_insecurebcaps();
  rspec_t  rspec = { 32, VC_VXF_SCHED_FLAGS, -1, -1 };
  PyObject  *resources;
  PyObject  *cpu_share;

  if (!PyArg_ParseTuple(args, "IO|K", &ctx, &resources, &flags))
    return NULL;
  if (!PyMapping_Check(resources))
    {
      PyErr_SetString(PyExc_TypeError, "invalid resources object");
      return NULL;
    }
  if ((cpu_share = PyMapping_GetItemString(resources, "nm_cpu_share")) &&
      (cpu_share = PyNumber_Int(cpu_share)))
    rspec.cpu_share = PyInt_AsLong(cpu_share);

  if (pl_chcontext(ctx, flags, bcaps, &rspec))
    PyErr_SetFromErrno(PyExc_OSError);

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
  xid_t  ctx;
  rspec_t  rspec = { 32, VC_VXF_SCHED_FLAGS, -1, -1 };
  PyObject  *resources;
  PyObject  *cpu_share;

  if (!PyArg_ParseTuple(args, "IO", &ctx, &resources))
    return NULL;
  if (!PyMapping_Check(resources))
    {
      PyErr_SetString(PyExc_TypeError, "invalid resources object");
      return NULL;
    }
  if ((cpu_share = PyMapping_GetItemString(resources, "nm_cpu_share")) &&
      (cpu_share = PyNumber_Int(cpu_share)))
    rspec.cpu_share = PyInt_AsLong(cpu_share);

  if (pl_setsched(ctx, rspec.cpu_share, rspec.cpu_sched_flags))
    PyErr_SetFromErrno(PyExc_OSError);

  return Py_None;
}

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

	if ((vserver(VCMD_add_dlimit, xid, &init) && errno != EEXIST) ||
            vserver(VCMD_set_dlimit, xid, &data))
          return PyErr_SetFromErrno(PyExc_OSError);

	return Py_None;	
}

static PyObject *
vserver_unset_dlimit(PyObject *self, PyObject *args)
{
  unsigned xid;
  struct vcmd_ctx_dlimit_base_v0 init;

  if (!PyArg_ParseTuple(args, "si", &path, &xid))
    return NULL;

  memset(&init, 0, sizeof(init));
  init.name = path;
  init.flags = 0;

  if (vserver(VCMD_rem_dlimit, xid, &init) && errno != ESRCH)
    return PyErr_SetFromErrno(PyExc_OSError);

  return Py_None;	
}

static PyMethodDef  methods[] = {
  { "chcontext", vserver_chcontext, METH_VARARGS,
    "chcontext to vserver with provided flags" },
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
  PyModule_AddIntConstant(mod, "DLIMIT_KEEP", (int)CDLIM_KEEP);
  PyModule_AddIntConstant(mod, "DLIMIT_INF", (int)CDLIM_INFINITY);
}
