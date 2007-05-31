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

#ifndef _LIB_PLANETLAB_H_
#define _LIB_PLANETLAB_H_

#define VC_VXF_SCHED_FLAGS  (VC_VXF_SCHED_HARD | VC_VXF_SCHED_SHARE)

struct sliver_resources {
  unsigned long long vs_cpu;
  unsigned long long vs_cpuguaranteed;
  struct vc_rlimit vs_rss;
  struct vc_rlimit vs_as;
  struct vc_rlimit vs_nproc;
  struct vc_rlimit vs_openfd;
  unsigned long long vs_whitelisted;
};

int adjust_lim(struct vc_rlimit *vcr, struct rlimit *lim);

int
pl_chcontext(xid_t ctx, uint64_t bcaps, struct sliver_resources *slr);

int
pl_setup_done(xid_t ctx);

int
pl_setsched(xid_t ctx, uint32_t cpu_share, uint32_t cpu_sched_flags);

/* scheduler flags */
#define VS_SCHED_CPU_GUARANTEED  1

/* Null byte made explicit */
#define NULLBYTE_SIZE                    1

void pl_get_limits(char *, struct sliver_resources *);
void pl_set_limits(xid_t, struct sliver_resources *);

static int
_PERROR(const char *format, char *file, int line, int _errno, ...)
{
	va_list ap;

	va_start(ap, _errno);
	fprintf(stderr, "%s:%d: ", file, line);
	vfprintf(stderr, format, ap);
	if (_errno)
		fprintf(stderr, ": %s (%d)", strerror(_errno), _errno);
	fputs("\n", stderr);
	fflush(stderr);

	return _errno;
}

#define PERROR(format, args...) _PERROR(format, __FILE__, __LINE__, errno, ## args)
#endif
