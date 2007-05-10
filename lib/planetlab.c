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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/resource.h>

#include "config.h"
#include "sched_cmd.h"
#include "virtual.h"
#include "vserver.h"
#include "planetlab.h"

static int
create_context(xid_t ctx, uint32_t flags, uint64_t bcaps, struct sliver_resources *slr)
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

  pl_set_limits(ctx, slr);

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
pl_chcontext(xid_t ctx, uint32_t flags, uint64_t bcaps, struct sliver_resources *slr)
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
	  if (create_context(ctx, flags, bcaps,slr))
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
  size_t len = strlen(VSERVERCONF) + strlen(context) + strlen(".conf") + NULLBYTE_SIZE;
  char *conf = (char *)malloc(len);	
  struct pl_resources *r;
  struct pl_resources sliver_list[] = {
    {"CPULIMIT", &slr->vs_cpu},
    {"CPUSHARE", &slr->vs_cpu},
    {"CPUGUARANTEED", &slr->vs_cpuguaranteed},
  
    {"TASKLIMIT", &slr->vs_nproc.hard}, /* backwards compatible */
    {"VS_NPROC_HARD", &slr->vs_nproc.hard},
    {"VS_NPROC_SOFT", &slr->vs_nproc.soft},
    {"VS_NPROC_MINIMUM", &slr->vs_nproc.min},
  
    {"MEMLIMIT", &slr->vs_rss.hard}, /* backwards compatible */
    {"VS_RSS_HARD", &slr->vs_rss.hard},
    {"VS_RSS_SOFT", &slr->vs_rss.soft},
    {"VS_RSS_MINIMUM", &slr->vs_rss.min},
  
    {"VS_AS_HARD", &slr->vs_as.hard},
    {"VS_AS_SOFT", &slr->vs_as.soft},
    {"VS_AS_MINIMUM", &slr->vs_as.min},
  
    {"VS_OPENFD_HARD", &slr->vs_openfd.hard},
    {"VS_OPENFD_SOFT", &slr->vs_openfd.soft},
    {"VS_OPENFD_MINIMUM", &slr->vs_openfd.min},

    {"VS_WHITELISTED", &slr->vs_whitelisted},
    {0,0}
  };

  sprintf(conf, "%s%s.conf", VSERVERCONF, context);

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

  slr->vs_whitelisted = 1;

  /* open the conf file for reading */
  fb = fopen(conf,"r");
  if (fb != NULL) {
    size_t index;
    char *buffer = malloc(1000);
    char *p;
    
    /* the conf file exist */ 
    while((p=fgets(buffer,1000-1,fb))!=NULL) {
      index = 0;
      len = strnlen(buffer,1000);
      WHITESPACE(buffer,index,len);
      if (buffer[index] == '#') 
	continue;
      
      for (r=&sliver_list[0]; r->name; r++)
	if ((p=strstr(&buffer[index],r->name))!=NULL) {
	  /* adjust index into buffer */
	  index+= (p-&buffer[index])+strlen(r->name);
	  
	  /* skip over whitespace */
	  WHITESPACE(buffer,index,len);
	  
	  /* expecting to see = sign */
	  if (buffer[index++]!='=') goto out;
	  
	  /* skip over whitespace */
	  WHITESPACE(buffer,index,len);
	  
	  /* expecting to see a digit for number */
	  if (!isdigit((int)buffer[index])) goto out;
	  
	  *r->limit = atoi(&buffer[index]);
	  if (0) /* for debugging only */
	    fprintf(stderr,"pl_get_limits found %s=%ld\n",
		    r->name,*r->limit);
	  break;
	}
    }
  out:
    fclose(fb);
    free(buffer);
  } else {
    fprintf(stderr,"cannot open %s\n",conf);
  }
  free(conf);
}

static inline int
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


void
pl_set_limits(xid_t ctx, struct sliver_resources *slr)
{
  struct rlimit lim; /* getrlimit values */
  unsigned long long vs_cpu;
  uint32_t cpu_sched_flags;

  if (slr != 0) {
    /* set memory limits */
    getrlimit(RLIMIT_RSS,&lim);
    if (adjust_lim(&slr->vs_rss, &lim)) {
      setrlimit(RLIMIT_RSS, &lim);
      if (vc_set_rlimit(ctx, RLIMIT_RSS, &slr->vs_rss))
	{
	  PERROR("pl_setrlimit(%u, RLIMIT_RSS)", ctx);
	  exit(1);
	}
    }

    /* set address space limits */
    getrlimit(RLIMIT_AS,&lim);
    if (adjust_lim(&slr->vs_as, &lim)) {
      setrlimit(RLIMIT_AS, &lim);
      if (vc_set_rlimit(ctx, RLIMIT_AS, &slr->vs_as))
	{
	  PERROR("pl_setrlimit(%u, RLIMIT_AS)", ctx);
	  exit(1);
	}
    }
    /* set nrpoc limit */
    getrlimit(RLIMIT_NPROC,&lim);
    if (adjust_lim(&slr->vs_nproc, &lim)) {
      setrlimit(RLIMIT_NPROC, &lim);
      if (vc_set_rlimit(ctx, RLIMIT_NPROC, &slr->vs_nproc))
	{
	  PERROR("pl_setrlimit(%u, RLIMIT_NPROC)", ctx);
	  exit(1);
	}
    }

    /* set openfd limit */
    getrlimit(RLIMIT_NOFILE,&lim);
    if (adjust_lim(&slr->vs_openfd, &lim)) {
      setrlimit(RLIMIT_NOFILE, &lim);
      if (vc_set_rlimit(ctx, RLIMIT_NOFILE, &slr->vs_openfd))
	{
	  PERROR("pl_setrlimit(%u, RLIMIT_NOFILE)", ctx);
	  exit(1);
	}
#ifndef VLIMIT_OPENFD
#warning VLIMIT_OPENFD should be defined from standard header
#define VLIMIT_OPENFD	17
#endif
      if (vc_set_rlimit(ctx, VLIMIT_OPENFD, &slr->vs_openfd))
	{
	  PERROR("pl_setrlimit(%u, VLIMIT_OPENFD)", ctx);
	  exit(1);
	}
    }
    vs_cpu = slr->vs_cpu;    
    cpu_sched_flags = slr->vs_cpuguaranteed & VS_SCHED_CPU_GUARANTEED;
  } else {
    vs_cpu = 1;
    cpu_sched_flags = 0;
  }
  
  if (pl_setsched(ctx, vs_cpu, cpu_sched_flags) < 0) {
    PERROR("pl_setsched(&u)", ctx);
    exit(1);
  }
}
