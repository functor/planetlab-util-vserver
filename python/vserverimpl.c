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

/*
 * chcontext
 */
static PyObject *
vserver_chcontext(PyObject *self, PyObject *args)
{
  unsigned  xid;
  unsigned  caps_remove = 0;

  if (!PyArg_ParseTuple(args, "I|I", &xid, &caps_remove))
    return NULL;

  if (vc_new_s_context(xid, caps_remove, 0) < 0)
    return PyErr_SetFromErrno(PyExc_OSError);

  return Py_None;
}

/*
 * setsched
 */
static PyObject *
vserver_setsched(PyObject *self, PyObject *args)
{
  unsigned  xid;
  struct vc_set_sched sched;

  sched.set_mask = (VC_VXSM_FILL_RATE | 
		    VC_VXSM_INTERVAL | 
		    VC_VXSM_TOKENS_MIN | 
		    VC_VXSM_TOKENS_MAX);

  if (!PyArg_ParseTuple(args, "I|I|I|I|I", &xid, 
			&sched.fill_rate,
			&sched.interval,
			&sched.tokens_min,
			&sched.tokens_max))
    return NULL;

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

struct  vcmd_ctx_dlimit_base_v0 {
	char *name;
	uint32_t flags;
};

struct  vcmd_ctx_dlimit_v0 {
	char *name;
	uint32_t space_used;			/* used space in kbytes */
	uint32_t space_total;			/* maximum space in kbytes */
	uint32_t inodes_used;			/* used inodes */
	uint32_t inodes_total;			/* maximum inodes */
	uint32_t reserved;			/* reserved for root in % */
	uint32_t flags;
};

#define CDLIM_UNSET             (0ULL)
#define CDLIM_INFINITY          (~0ULL)
#define CDLIM_KEEP              (~1ULL)

static PyObject *
vserver_dlimit(PyObject *self, PyObject *args)
{
	PyObject *res;
	char* path;
	unsigned xid;
	struct vcmd_ctx_dlimit_base_v0 init;
	struct vcmd_ctx_dlimit_v0 data;
	int r;

	memset(&data,0,sizeof(data));
	if (!PyArg_ParseTuple(args, "s(iiiii)", &path, 
			      &data.space_used,
			      &data.space_total,
			      &data.inodes_used,
			      &data.inodes_total,
			      &data.reserved))
		return NULL;

	data.name = path;
	data.flags = 0;

	init.name = path;
	init.flags = 0;

	r = vserver(VCMD_rem_dlimit, xid, &init);
	if (r<0){}
	r = vserver(VCMD_add_dlimit, xid, &init);
	if (r<0){}
	r = vserver(VCMD_set_dlimit, xid, &data);
	if (r<0){}

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



static PyMethodDef  methods[] = {
  { "chcontext", vserver_chcontext, METH_VARARGS,
    "Change to the given vserver context" },
  { "setsched", vserver_setsched, METH_VARARGS,
    "Change vserver scheduling attributes for given vserver context" },
  { "dlimit", vserver_dlimit, METH_VARARGS,
    "Set disk limits for given vserver context" },
  { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initvserverimpl(void)
{
  Py_InitModule("vserverimpl", methods);
}
