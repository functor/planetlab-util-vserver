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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/resource.h>
#include <fcntl.h>

#include "vserver.h"
#include "planetlab.h"

static int
create_context(xid_t ctx, uint64_t bcaps, struct sliver_resources *slr)
{
  struct vc_ctx_caps  vc_caps;
  struct vc_net_flags  vc_nf;

  /* Create network context */
  if (vc_net_create(ctx) == VC_NOCTX) {
    if (errno == EEXIST)
      goto process;
    return -1;
  }

  /* Make the network context persistent */
  vc_nf.mask = vc_nf.flagword = VC_NXF_PERSISTENT;
  if (vc_set_nflags(ctx, &vc_nf))
    return -1;

process:
  /*
   * Create context info - this sets the STATE_SETUP and STATE_INIT flags.
   */
  if (vc_ctx_create(ctx) == VC_NOCTX)
    return -1;

  /* Set capabilities - these don't take effect until SETUP flag is unset */
  vc_caps.bcaps = bcaps;
  vc_caps.bmask = ~0ULL;  /* currently unused */
  vc_caps.ccaps = 0;      /* don't want any of these */
  vc_caps.cmask = ~0ULL;
  if (vc_set_ccaps(ctx, &vc_caps))
    return -1;

  pl_set_limits(ctx, slr);

  return 0;
}

int
pl_setup_done(xid_t ctx)
{
  struct vc_ctx_flags  vc_flags;

  /* unset SETUP flag - this allows other processes to migrate */
  /* set the PERSISTENT flag - so the context doesn't vanish */
  /* Don't clear the STATE_INIT flag, as that would make us the init task. */
  vc_flags.mask = VC_VXF_STATE_SETUP|VC_VXF_PERSISTENT;
  vc_flags.flagword = VC_VXF_PERSISTENT;
  if (vc_set_cflags(ctx, &vc_flags))
    return -1;

  return 0;
}

#define RETRY_LIMIT  10

int
pl_chcontext(xid_t ctx, uint64_t bcaps, struct sliver_resources *slr)
{
  int  retry_count = 0;
  int  net_migrated = 0;

  pl_set_ulimits(slr);

  for (;;)
    {
      struct vc_ctx_flags  vc_flags;

      if (vc_get_cflags(ctx, &vc_flags))
	{
	  if (errno != ESRCH)
	    return -1;

	  /* context doesn't exist - create it */
	  if (create_context(ctx, bcaps, slr))
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
      if (net_migrated || !vc_net_migrate(ctx))
	{
	  if (!vc_ctx_migrate(ctx, 0))
	    break;  /* done */
	  net_migrated = 1;
	}

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
		       VC_VXSM_TOKENS_MIN | VC_VXSM_TOKENS_MAX | VC_VXSM_MSEC |
		       VC_VXSM_FILL_RATE2 | VC_VXSM_INTERVAL2 | VC_VXSM_FORCE |
		       VC_VXSM_IDLE_TIME);
  vc_sched.fill_rate = 0;
  vc_sched.fill_rate2 = cpu_share;  /* tokens accumulated per interval */
  vc_sched.interval = vc_sched.interval2 = 1000;  /* milliseconds */
  vc_sched.tokens = 100;     /* initial allocation of tokens */
  vc_sched.tokens_min = 50;  /* need this many tokens to run */
  vc_sched.tokens_max = 100;  /* max accumulated number of tokens */

  if (cpu_share == (uint32_t)VC_LIM_KEEP)
    vc_sched.set_mask &= ~(VC_VXSM_FILL_RATE|VC_VXSM_FILL_RATE2);

  /* guaranteed CPU corresponds to SCHED_SHARE flag being cleared */
  if (cpu_sched_flags & VS_SCHED_CPU_GUARANTEED) {
    new_flags = 0;
    vc_sched.fill_rate = vc_sched.fill_rate2;
  }
  else
    new_flags = VC_VXF_SCHED_SHARE;

  VC_SYSCALL(vc_set_sched(ctx, &vc_sched));

  vc_flags.mask = VC_VXF_SCHED_FLAGS;
  vc_flags.flagword = new_flags | VC_VXF_SCHED_HARD;
  VC_SYSCALL(vc_set_cflags(ctx, &vc_flags));

  return 0;
}

struct pl_resources {
	char *name;
	unsigned long long *limit;
};

#define WHITESPACE(buffer,index,len)     \
  while(isspace((int)buffer[index])) \
	if (index < len) index++; else goto out;

#define VSERVERCONF "/etc/vservers/"
void
pl_get_limits(char *context, struct sliver_resources *slr)
{
  FILE *fb;
  int cwd;
  size_t len = strlen(VSERVERCONF) + strlen(context) + NULLBYTE_SIZE;
  char *conf = (char *)malloc(len + strlen("rlimits/openfd.hard"));
  struct pl_resources *r;
  struct pl_resources sliver_list[] = {
    {"sched/fill-rate2", &slr->vs_cpu},
    {"sched/fill-rate", &slr->vs_cpuguaranteed},
  
    {"rlimits/nproc.hard", &slr->vs_nproc.hard},
    {"rlimits/nproc.soft", &slr->vs_nproc.soft},
    {"rlimits/nproc.min", &slr->vs_nproc.min},
  
    {"rlimits/rss.hard", &slr->vs_rss.hard},
    {"rlimits/rss.soft", &slr->vs_rss.soft},
    {"rlimits/rss.min", &slr->vs_rss.min},
  
    {"rlimits/as.hard", &slr->vs_as.hard},
    {"rlimits/as.soft", &slr->vs_as.soft},
    {"rlimits/as.min", &slr->vs_as.min},
  
    {"rlimits/openfd.hard", &slr->vs_openfd.hard},
    {"rlimits/openfd.soft", &slr->vs_openfd.soft},
    {"rlimits/openfd.min", &slr->vs_openfd.min},

    {"bcapabilities", NULL},
    {0,0}
  };

  sprintf(conf, "%s%s", VSERVERCONF, context);

  slr->vs_cpu = VC_LIM_KEEP;
  slr->vs_cpuguaranteed = 0;

  slr->vs_rss.hard = VC_LIM_KEEP;
  slr->vs_rss.soft = VC_LIM_KEEP;
  slr->vs_rss.min = VC_LIM_KEEP;

  slr->vs_as.hard = VC_LIM_KEEP;
  slr->vs_as.soft = VC_LIM_KEEP;
  slr->vs_as.min = VC_LIM_KEEP;


  slr->vs_nproc.hard = VC_LIM_KEEP;
  slr->vs_nproc.soft = VC_LIM_KEEP;
  slr->vs_nproc.min = VC_LIM_KEEP;

  slr->vs_openfd.hard = VC_LIM_KEEP;
  slr->vs_openfd.soft = VC_LIM_KEEP;
  slr->vs_openfd.min = VC_LIM_KEEP;

  slr->vs_capabilities.bcaps = 0;
  slr->vs_capabilities.bmask = 0;
  slr->vs_capabilities.ccaps = 0;
  slr->vs_capabilities.cmask = 0;

  cwd = open(".", O_RDONLY);
  if (cwd == -1) {
    perror("cannot get a handle on .");
    goto out;
  }
  if (chdir(conf) == -1) {
    fprintf(stderr, "cannot chdir to ");
    perror(conf);
    goto out_fd;
  }

  for (r = &sliver_list[0]; r->name; r++) {
    char buf[1000];
    fb = fopen(r->name, "r");
    if (fb == NULL)
      continue;
    /* XXX: UGLY. */
    if (strcmp(r->name, "bcapabilities") == 0) {
      size_t len, i;
      struct vc_err_listparser err;

      len = fread(buf, 1, sizeof(buf), fb);
      for (i = 0; i < len; i++) {
	if (buf[i] == '\n')
	  buf[i] = ',';
      }
      vc_list2bcap(buf, len, &err, &slr->vs_capabilities);
    }
    else
      if (fgets(buf, sizeof(buf), fb) != NULL && isdigit(*buf))
        *r->limit = atoi(buf);
    fclose(fb);
  }

  fchdir(cwd);
out_fd:
  close(cwd);
out:
  free(conf);
}

int
adjust_lim(struct vc_rlimit *vcr, struct rlimit *lim)
{
  int adjusted = 0;
  if (vcr->min != VC_LIM_KEEP) {
    if (vcr->min > lim->rlim_cur) {
      lim->rlim_cur = vcr->min;
      adjusted = 1;
    }
    if (vcr->min > lim->rlim_max) {
      lim->rlim_max = vcr->min;
      adjusted = 1;
    }
  }

  if (vcr->soft != VC_LIM_KEEP) {
    switch (vcr->min != VC_LIM_KEEP) {
    case 1:
      if (vcr->soft < vcr->min)
	break;
    case 0:
	lim->rlim_cur = vcr->soft;
	adjusted = 1;
    }
  }

  if (vcr->hard != VC_LIM_KEEP) {
    switch (vcr->min != VC_LIM_KEEP) {
    case 1:
      if (vcr->hard < vcr->min)
	break;
    case 0:
	lim->rlim_cur = vcr->hard;
	adjusted = 1;
    }
  }
  return adjusted;
}

static inline void
set_one_ulimit(int resource, struct vc_rlimit *limit)
{
  struct rlimit lim;
  getrlimit(resource, &lim);
  adjust_lim(limit, &lim);
  setrlimit(resource, &lim);
}

void
pl_set_ulimits(struct sliver_resources *slr)
{
  if (!slr)
    return;

  set_one_ulimit(RLIMIT_RSS, &slr->vs_rss);
  set_one_ulimit(RLIMIT_AS, &slr->vs_as);
  set_one_ulimit(RLIMIT_NPROC, &slr->vs_nproc);
  set_one_ulimit(RLIMIT_NOFILE, &slr->vs_openfd);
}

void
pl_set_limits(xid_t ctx, struct sliver_resources *slr)
{
  unsigned long long vs_cpu;
  uint32_t cpu_sched_flags;

  if (slr != 0) {
    /* set memory limits */
    if (vc_set_rlimit(ctx, RLIMIT_RSS, &slr->vs_rss)) {
      PERROR("pl_setrlimit(%u, RLIMIT_RSS)", ctx);
      exit(1);
    }

    /* set address space limits */
    if (vc_set_rlimit(ctx, RLIMIT_AS, &slr->vs_as)) {
      PERROR("pl_setrlimit(%u, RLIMIT_AS)", ctx);
      exit(1);
    }

    /* set nrpoc limit */
    if (vc_set_rlimit(ctx, RLIMIT_NPROC, &slr->vs_nproc)) {
      PERROR("pl_setrlimit(%u, RLIMIT_NPROC)", ctx);
      exit(1);
    }

    /* set openfd limit */
    if (vc_set_rlimit(ctx, RLIMIT_NOFILE, &slr->vs_openfd)) {
      PERROR("pl_setrlimit(%u, RLIMIT_NOFILE)", ctx);
      exit(1);
    }
    if (vc_set_rlimit(ctx, VC_VLIMIT_OPENFD, &slr->vs_openfd)) {
      PERROR("pl_setrlimit(%u, VLIMIT_OPENFD)", ctx);
      exit(1);
    }

    vs_cpu = slr->vs_cpu;    
    cpu_sched_flags = slr->vs_cpuguaranteed & VS_SCHED_CPU_GUARANTEED;

    slr->vs_capabilities.bmask = vc_get_insecurebcaps();
    if (vc_set_ccaps(ctx, &slr->vs_capabilities) < 0) {
      PERROR("pl_setcaps(%u)", ctx);
      exit(1);
    }
  } else {
    vs_cpu = 1;
    cpu_sched_flags = 0;
  }
  
  if (pl_setsched(ctx, vs_cpu, cpu_sched_flags) < 0) {
    PERROR("pl_setsched(%u)", ctx);
    exit(1);
  }
}
