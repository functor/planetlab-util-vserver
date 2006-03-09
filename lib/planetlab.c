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

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h>

#include "config.h"
#include "planetlab.h"
#include "sched_cmd.h"
#include "virtual.h"
#include "vserver.h"

static int
create_context(xid_t ctx, uint32_t flags, uint64_t bcaps)
{
  struct vc_ctx_caps  vc_caps;

  /*
   * Create context info - this sets the STATE_SETUP and STATE_INIT flags.
   * Don't ever clear the STATE_INIT flag, that makes us the init task.
   *
   * XXX - the kernel code allows initial flags to be passed as an arg.
   */
  if (vc_ctx_create(ctx) == VC_NOCTX)
    return -1;

  /* set capabilities - these don't take effect until SETUP flag is unset */
  vc_caps.bcaps = bcaps;
  vc_caps.bmask = ~0ULL;  /* currently unused */
  vc_caps.ccaps = 0;      /* don't want any of these */
  vc_caps.cmask = ~0ULL;
  if (vc_set_ccaps(ctx, &vc_caps))
    return -1;

  /* set default scheduling parameters */
  pl_setsched(ctx, 32, 0);

  return 0;
}

int
pl_setup_done(xid_t ctx)
{
  struct vc_ctx_flags  vc_flags;

  /* unset SETUP flag - this allows other processes to migrate */
  vc_flags.mask = VC_VXF_STATE_SETUP;
  vc_flags.flagword = 0;
  if (vc_set_cflags(ctx, &vc_flags))
    return -1;

  return 0;
}

#define RETRY_LIMIT  10

int
pl_chcontext(xid_t ctx, uint32_t flags, uint64_t bcaps)
{
  int  retry_count = 0;

  for (;;)
    {
      struct vc_ctx_flags  vc_flags;

      if (vc_get_cflags(ctx, &vc_flags))
	{
	  if (errno != ESRCH)
	    return -1;

	  /* context doesn't exist - create it */
	  if (create_context(ctx, flags, bcaps))
	    {
	      if (errno == EEXIST)
		/* another process beat us in a race */
		goto migrate;
	      if (errno == EBUSY)
		/* another process is creating - poll the SETUP flag */
		continue;
	      return -1;
	    }

	  /* created context and migrated to it i.e., we're done */
	  return 1;
	}

      /* check the SETUP flag */
      if (vc_flags.flagword & VC_VXF_STATE_SETUP)
	{
	  /* context is still being setup - wait a while then retry */
	  if (retry_count++ >= RETRY_LIMIT)
	    {
	      errno = EBUSY;
	      return -1;
	    }
	  sleep(1);
	  continue;
	}

      /* context has been setup */
    migrate:
      if (!vc_ctx_migrate(ctx))
	break;  /* done */

      /* context disappeared - retry */
    }

  return 0;
}

/* it's okay for a syscall to fail because the context doesn't exist */
#define VC_SYSCALL(x)				\
do						\
{						\
  if (x)					\
    return errno == ESRCH ? 0 : -1;		\
}						\
while (0)

int
pl_setsched(xid_t ctx, uint32_t cpu_share, uint32_t cpu_sched_flags)
{
  struct vc_set_sched  vc_sched;
  struct vc_ctx_flags  vc_flags;
  uint32_t  new_flags;

  vc_sched.set_mask = (VC_VXSM_FILL_RATE | VC_VXSM_INTERVAL | VC_VXSM_TOKENS |
		       VC_VXSM_TOKENS_MIN | VC_VXSM_TOKENS_MAX);
  vc_sched.fill_rate = cpu_share;  /* tokens accumulated per interval */
  vc_sched.interval = 1000;  /* milliseconds */
  vc_sched.tokens = 100;     /* initial allocation of tokens */
  vc_sched.tokens_min = 50;  /* need this many tokens to run */
  vc_sched.tokens_max = 100;  /* max accumulated number of tokens */

  VC_SYSCALL(vc_set_sched(ctx, &vc_sched));

  /* get current flag values */
  VC_SYSCALL(vc_get_cflags(ctx, &vc_flags));

  /* guaranteed CPU corresponds to SCHED_SHARE flag being cleared */
  new_flags = (cpu_sched_flags & VS_SCHED_CPU_GUARANTEED
	       ? 0
	       : VC_VXF_SCHED_SHARE);
  if ((vc_flags.flagword & VC_VXF_SCHED_SHARE) != new_flags)
    {
      vc_flags.mask = VC_VXF_SCHED_FLAGS;
      vc_flags.flagword = new_flags | VC_VXF_SCHED_HARD;
      VC_SYSCALL(vc_set_cflags(ctx, &vc_flags));
    }

  return 0;
}
