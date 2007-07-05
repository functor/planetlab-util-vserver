// $Id: vserver-internal.h 2446 2007-01-09 13:22:11Z dhozac $    --*- c++ -*--

// Copyright (C) 2003 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifdef H_VSERVER_SYSCALL_INTERNAL_H
#  error vserver-internal.h must not be included more than once
#endif

#ifndef H_VSERVER_SYSCALL_INTERNAL_H
#define H_VSERVER_SYSCALL_INTERNAL_H

#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include "internal.h"
#include "syscall-wrap.h"

#if !defined(__NR_vserver) && defined(ENSC_SYSCALL__NR_vserver)
#  define __NR_vserver	ENSC_SYSCALL__NR_vserver
#endif

inline static ALWAYSINLINE void vc_noop0() {}

#define VC_PREFIX	0)
#define VC_SUFFIX	else (void)(vc_noop0()
#define CALL_VC_NOOP	vc_noop0()
#define CALL_VC_GENERAL(ID, SUFFIX, FUNC, ...)				\
  VC_PREFIX; VC_SELECT(ID) return FUNC ## _ ## SUFFIX(__VA_ARGS__); VC_SUFFIX
#define CALL_VC_GENERAL_CONFIG(BIT, SUFFIX, FUNC, ...)			\
  VC_PREFIX; VC_CBIT(BIT)  return FUNC ## _ ## SUFFIX(__VA_ARGS__); VC_SUFFIX

#ifdef VC_MULTIVERSION_SYSCALL
#  define VC_SELECT(ID)	if (ver>=(ID))
#  define VC_CBIT(BIT)  if ((conf&BIT) == BIT)
#  define CALL_VC(...)					\
  do {							\
    int	ver = utilvserver_checkCompatVersion();		\
    uint_least32_t conf = utilvserver_checkCompatConfig();	\
    if (ver==-1) return -1;				\
    VC_SUFFIX, __VA_ARGS__, VC_PREFIX;			\
    errno = ENOSYS;					\
    return -1;						\
  } while (0)
#else
#  define VC_SELECT(ID) if (1)
#  define VC_CBIT(BIT)  if (1)
#  define CALL_VC(...)					\
  do {							\
    if (1) {} VC_SUFFIX, __VA_ARGS__, VC_PREFIX;	\
    errno = ENOSYS; return -1;				\
  } while (0)
#endif

#ifdef VC_ENABLE_API_COMPAT
#  define CALL_VC_COMPAT(F,...) CALL_VC_GENERAL(0x00010000, compat, F, __VA_ARGS__)
#else
#  define CALL_VC_COMPAT(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_LEGACY
#  define CALL_VC_LEGACY(F,...) CALL_VC_GENERAL(0x00000000, legacy, F, __VA_ARGS__)
#else
#  define CALL_VC_LEGACY(F,...) CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V11
#  define CALL_VC_V11(F,...)	CALL_VC_GENERAL(0x00010000, v11, F, __VA_ARGS__)
#else
#  define CALL_VC_V11(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V13
#  define CALL_VC_V13(F,...)	CALL_VC_GENERAL(0x00010011, v13, F, __VA_ARGS__)
#else
#  define CALL_VC_V13(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V13
#  define CALL_VC_V13A(F,...)	CALL_VC_GENERAL(0x00010012, v13, F, __VA_ARGS__)
#else
#  define CALL_VC_V13A(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V13
#  define CALL_VC_V13B(F,...)	CALL_VC_GENERAL(0x00010021, v13b, F, __VA_ARGS__)
#else
#  define CALL_VC_V13B(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V13OBS
#  define CALL_VC_V13OBS(F,...)	CALL_VC_GENERAL(0x00010011, v13obs, F, __VA_ARGS__)
#else
#  define CALL_VC_V13OBS(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V21
#  define CALL_VC_V21(F,...)	CALL_VC_GENERAL(0x00020100, v21, F, __VA_ARGS__)
#else
#  define CALL_VC_V21(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_V21
#  define CALL_VC_SPACES(F,...)	CALL_VC_GENERAL_CONFIG(VC_VCI_SPACES, spaces, F, __VA_ARGS__)
#else
#  define CALL_VC_SPACES(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_NET
#  define CALL_VC_NET(F,...)	CALL_VC_GENERAL(0x00010016, net, F, __VA_ARGS__)
#else
#  define CALL_VC_NET(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_FSCOMPAT
#  define CALL_VC_FSCOMPAT(F,...)	CALL_VC_GENERAL(0x00010000, fscompat, F, __VA_ARGS__)
#else
#  define CALL_VC_FSCOMPAT(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_OLDPROC
#  define CALL_VC_OLDPROC(F,...)	CALL_VC_GENERAL(0x00000000, oldproc, F, __VA_ARGS__)
#else
#  define CALL_VC_OLDPROC(F,...)	CALL_VC_NOOP
#endif

#ifdef VC_ENABLE_API_OLDUTS
#  define CALL_VC_OLDUTS(F,...)		CALL_VC_GENERAL(0x00000000, olduts, F, __VA_ARGS__)
#else
#  define CALL_VC_OLDUTS(F,...)		CALL_VC_NOOP
#endif


  // Some  kernel <-> userspace wrappers; they should be noops in most cases

#if 1
#  define CTX_KERNEL2USER(X)	(((X)==(uint32_t)(-1)) ? VC_NOCTX   : \
				 ((X)==(uint32_t)(-2)) ? VC_SAMECTX : \
				 (xid_t)(X))

#  define CTX_USER2KERNEL(X)	(((X)==VC_DYNAMIC_XID) ? (uint32_t)(-1) : \
				 ((X)==VC_SAMECTX)     ? (uint32_t)(-2) : \
				 (uint32_t)(X))
#else
#  define CTX_USER2KERNEL(X)	(X)
#  define CTX_KERNEL2USER(X)	(X)
#endif

#if 1
#  define EXT2FLAGS_USER2KERNEL(X)	(((X) & ~(VC_IMMUTABLE_FILE_FL|VC_IMMUTABLE_LINK_FL)) | \
					 ((X) & VC_IMMUTABLE_FILE_FL ? EXT2_IMMUTABLE_FILE_FL : 0) | \
					 ((X) & VC_IMMUTABLE_LINK_FL ? EXT2_IMMUTABLE_LINK_FL : 0))
#  define EXT2FLAGS_KERNEL2USER(X)	(((X) & ~(EXT2_IMMUTABLE_FILE_FL|EXT2_IMMUTABLE_LINK_FL)) | \
					 ((X) & EXT2_IMMUTABLE_FILE_FL ? VC_IMMUTABLE_FILE_FL : 0) | \
					 ((X) & EXT2_IMMUTABLE_LINK_FL ? VC_IMMUTABLE_LINK_FL : 0))
#else
#  define EXT2FLAGS_KERNEL2USER(X)	(X)
#  define EXT2FLAGS_USER2KERNEL(X)	(X)
#endif

#if 1
#  define VHI_USER2KERNEL(X)		((((X)==vcVHI_CONTEXT)    ? VHIN_CONTEXT    : \
					  ((X)==vcVHI_SYSNAME)    ? VHIN_SYSNAME    : \
					  ((X)==vcVHI_NODENAME)   ? VHIN_NODENAME   : \
					  ((X)==vcVHI_RELEASE)    ? VHIN_RELEASE    : \
					  ((X)==vcVHI_VERSION)    ? VHIN_VERSION    : \
					  ((X)==vcVHI_MACHINE)    ? VHIN_MACHINE    : \
					  ((X)==vcVHI_DOMAINNAME) ? VHIN_DOMAINNAME : \
					  (X)))
#  define VHI_KERNEL2USER(X)		((((X)==VHIN_CONTEXT)     ? vcVHI_CONTEXT    : \
					  ((X)==VHIN_SYSNAME)     ? vcVHI_SYSNAME    : \
					  ((X)==VHIN_NODENAME)    ? vcVHI_NODENAME   : \
					  ((X)==VHIN_RELEASE)     ? vcVHI_RELEASE    : \
					  ((X)==VHIN_VERSION)     ? vcVHI_VERSION    : \
					  ((X)==VHIN_MACHINE)     ? vcVHI_MACHINE    : \
					  ((X)==VHIN_DOMAINNAME)  ? vcVHI_DOMAINNAME : \
					  (X)))
#else
#  define VHI_USER2KERNEL(X)		(X)
#  define VHI_KERNEL2USER(X)		(X)
#endif

#if 1
#  define NID_KERNEL2USER(X)	(((X)==(uint32_t)(-1)) ? VC_NONID   : \
				 (xid_t)(X))

#  define NID_USER2KERNEL(X)	(((X)==VC_DYNAMIC_NID) ? (uint32_t)(-1) : \
				 (uint32_t)(X))
#else
#  define NID_USER2KERNEL(X)	(X)
#  define NID_KERNEL2USER(X)	(X)
#endif

#if 1
#  define NETTYPE_USER2KERNEL(X)	((X)==vcNET_IPV4   ? NXA_TYPE_IPV4     : \
					 (X)==vcNET_IPV6   ? NXA_TYPE_IPV6     : \
					 (X)==vcNET_IPV4B  ? (NXA_TYPE_IPV4 | NXA_MOD_BCAST) : \
					 (X)==vcNET_IPV6B  ? (NXA_TYPE_IPV6 | NXA_MOD_BCAST) : \
					 (X)==vcNET_ANY    ? NXA_TYPE_ANY      : \
					 (X))
#  define NETTYPE_KERNEL2USER(X)	((X)==NXA_TYPE_IPV4	? vcNET_IPV4   : \
					 (X)==NXA_TYPE_IPV6	? vcNET_IPV6   : \
					 (X)==(NXA_TYPE_IPV4|NXA_MOD_BCAST) ? vcNET_IPV4B : \
					 (X)==(NXA_TYPE_IPV6|NXA_MOD_BCAST) ? vcNET_IPV6B : \
					 (X)==NXA_TYPE_ANY      ? vcNET_ANY    : \
					 (X))
#else
#  define NETTYPE_USER2KERNEL(X)	(X)
#  define NETTYPE_KERNEL2USER(X)	(X)
#endif

#define CDLIM_USER2KERNEL(X)		((X)==VC_CDLIM_UNSET    ? CDLIM_UNSET    : \
					 (X)==VC_CDLIM_INFINITY ? CDLIM_INFINITY : \
					 (X)==VC_CDLIM_KEEP     ? CDLIM_KEEP     : \
					 (X))

  /// the __typeof__ thing is a hack to deal with the kernel interface
  /// using an unsigned long long value for a uint32_t type
#define CDLIM_KERNEL2USER(X)		((X)==(__typeof__(X))CDLIM_UNSET       ? VC_CDLIM_UNSET    : \
					 (X)==(__typeof__(X))CDLIM_INFINITY    ? VC_CDLIM_INFINITY : \
					 (X)==(__typeof__(X))CDLIM_KEEP        ? VC_CDLIM_KEEP     : \
					 (X))


#define ENSC_STRUCT_IDX(STRUCT,ATTR)			\
  ((char*)(&(STRUCT).ATTR) - (char*)(&(STRUCT)))
#define ENSC_SAME_STRUCT_IDX(LHS,RHS,ATTR)			\
  (ENSC_STRUCT_IDX(LHS,ATTR) == ENSC_STRUCT_IDX(RHS,ATTR))
#define ENSC_SAME_STRUCT_ITEM(LHS,RHS,ATTR)	\
  (ENSC_SAME_STRUCT_IDX(LHS,RHS,ATTR) &&	\
   sizeof((LHS).ATTR)==sizeof((RHS).ATTR) &&	\
   sizeof(LHS)==sizeof(RHS))

#define EXT2_IOC_GETCONTEXT		_IOR('x', 1, long)
#define EXT2_IOC_SETCONTEXT		_IOW('x', 2, long)

#ifndef HAVE_VSERVER
#ifdef ENSC_SYSCALL_TRADITIONAL
inline static UNUSED ALWAYSINLINE
int vserver(uint32_t cmd, uint32_t id, void *data)
{
  return syscall(__NR_vserver, cmd, id, data);
}
#else
inline static UNUSED ALWAYSINLINE
_syscall3(int, vserver,
	  uint32_t, cmd, uint32_t, id, void *, data)
#endif
#endif

#endif	//  H_VSERVER_SYSCALL_INTERNAL_H
