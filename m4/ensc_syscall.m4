dnl $Id: ensc_syscall.m4,v 1.2.2.2 2004/03/04 03:12:34 ensc Exp $

dnl Copyright (C) 2004 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
dnl  
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; version 2 of the License.
dnl  
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl  
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

dnl Usage: ENSC_SYSCALL

AC_DEFUN([ENSC_SYSCALL],
[
	AC_REQUIRE([ENSC_KERNEL_HEADERS])
        AC_MSG_CHECKING([for syscall(2) invocation method])
        AC_ARG_WITH([syscall],
        	    [AC_HELP_STRING([--with-syscall=METHOD],
                                    [call syscall(2) with the specified METHOD; valid values are 'fast', 'traditional' and 'auto' (default: auto)])],
                    [],
                    [with_syscall=auto])
        AC_MSG_RESULT([$with_syscall])
        
        case x"$with_syscall" in
            xauto)
		AC_CACHE_CHECK([which syscall(2) invocation works], [ensc_cv_test_syscall],
			       [
				AC_LANG_PUSH(C)
				AC_COMPILE_IFELSE([
#include <asm/unistd.h>
#include <syscall.h>
#include <errno.h>
#define __NR_foo0	300
#define __NR_foo1	301
#define __NR_foo2	302
#define __NR_foo3	303
#define __NR_foo4	304
#define __NR_foo5	305
inline static _syscall0(int, foo0)
inline static _syscall1(int, foo1, int, a)
inline static _syscall2(int, foo2, int, a, int, b)
inline static _syscall3(int, foo3, int, a, int, b, int, c)
inline static _syscall4(int, foo4, int, a, int, b, int, c, int, d)
inline static _syscall5(int, foo5, int, a, int, b, int, c, int, d, int, e)

int main() {
  return foo0() || \
	 foo1(1) || \
	 foo2(1,2) || \
         foo3(1,2,3) || \
         foo4(1,2,3,4) || \
	 foo5(1,2,3,4,5);
}
				],
				[ensc_cv_test_syscall=fast],
				[ensc_cv_test_syscall=traditional])

				AC_LANG_POP
		])
		with_syscall=$ensc_cv_test_syscall
        	;;
            xfast|xtraditional)
        	;;
            *)
        	AC_MSG_ERROR(['$with_syscall' is not a valid value for '--with-syscall'])
        	;;
        esac

        if test x"$with_syscall" = xtraditional; then
            AC_DEFINE(ENSC_SYSCALL_TRADITIONAL,  1, [Define to 1 when the fast syscall(2) invocation does not work])
        fi
        
        AH_BOTTOM([
#if defined(__pic__) && defined(__i386) && !defined(ENSC_SYSCALL_TRADITIONAL)
#  define ENSC_SYSCALL_TRADITIONAL	1
#endif])
])
