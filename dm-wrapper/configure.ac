#
# Copyright (c) 2012 Citrix Systems, Inc.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

# Prelude.
AC_PREREQ(2.13)
AC_INIT()

LT_INIT

PACKAGE=qemu-wrapper

AM_INIT_AUTOMAKE($PACKAGE,$VERSION)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG([0.22])
AC_PROG_AWK
AC_CHECK_PROG(MD5SUM, md5sum, md5sum)
AC_CHECK_PROG(GREP, grep, grep)
AC_CHECK_PROG(SED, sed, sed)

# Checks for header files.
AC_CHECK_HEADERS([unistd.h fcntl.h errno.h stdlib.h stdint.h stropts.h syslog.h string.h stdio.h stdarg.h])
AC_CHECK_HEADERS([sys/types.h sys/stat.h sys/mman.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM

CPPFLAGS="-D_GNU_SOURCE ${CPPFLAGS}"

# Add option to compile for stubdomain
AC_ARG_ENABLE([syslog],
              AC_HELP_STRING([--disable-syslog], [Write all log on standard output]))

case "x$enable_syslog" in
    x|xtrue)
        CPPFLAGS="$CPPFLAGS -DSYSLOG"
        ;;
    *)
        ;;
esac

# Checks for libraries.

# Check for libevent

AC_ARG_WITH([libevent],
            AC_HELP_STRING([--with-libevent=PATH], [Path to prefix where libevent (or libev) is installed]),
            [LIBEVENT_PREFIX=$with_libevent], [])

case "x$LIBEVENT_PREFIX" in
        x|xno|xyes)
                LIBEVENT_INC=""
                LIBEVENT_LIB="-levent"
                ;;
        *)
                LIBEVENT_INC="-I${LIBEVENT_PREFIX}/include"
                LIBEVENT_LIB="-L${LIBEVENT_PREFIX}/lib -levent"
                ;;
esac

AC_SUBST(LIBEVENT_INC)
AC_SUBST(LIBEVENT_LIB)

have_libevent=true

ORIG_LIBS="${LIBS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
        LIBS="${LIBS} ${LIBEVENT_LIB}"
        CPPFLAGS="${CPPFLAGS} ${LIBEVENT_INC}"
        AC_CHECK_HEADERS([event.h], [], [have_libevent=false])
        AC_CHECK_FUNC([event_init], [], [have_libevent=false])
LIBS="${ORIG_LIBS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

if test "x$have_libevent" = "xfalse"; then
        AC_MSG_ERROR([
*** At least libev or libevent is required.                
])
fi

# libxenstore

AC_ARG_WITH([libxenstore],
            AC_HELP_STRING([--with-libxenstore=PATH], [Path to prefix where libxenstore was installed.]),
            [LIBXENSTORE_PREFIX=$with_libxenstore], [])

case "x$LIBXENSTORE_PREFIX" in
        x|xno|xyes)
                LIBXENSTORE_INC=""
                LIBXENSTORE_LIB="-lxenstore"
                ;;
        *)
                LIBXENSTORE_INC="-I${LIBXENSTORE_PREFIX}/include"
                LIBXENSTORE_LIB="-L${LIBXENSTORE_PREFIX}/lib -lxenstore"
                ;;
esac

AC_SUBST(LIBXENSTORE_INC)
AC_SUBST(LIBXENSTORE_LIB)

have_libxenstore=true

ORIG_LIBS="${LIBS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
	LIBS="${LIBS} ${LIBXENSTORE_LIB}"
	CPPFLAGS="${CPPFLAGS} ${LIBXENSTORE_INC}"
	AC_CHECK_HEADERS([xs.h], [], [have_libxenstore=false])
	AC_CHECK_FUNC(xs_domain_open, [], [have_libxenstore=false])
LIBS="${ORIG_LIBS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

if test "x$have_libxenstore" = "xfalse"; then
	AC_MSG_ERROR([where is libxenstore dude])
fi

# libv4v

AC_ARG_WITH(libv4v,
            AC_HELP_STRING([--with-libv4v=PATH], [Path to prefix where libv4v was installed.]),
            [LIBV4V_PREFIX=$with_libv4v], [])

case "x$LIBV4V_PREFIX" in
    x|xno|xyes)
            LIBV4V_INC=""
            LIBV4V_LIB="-lv4v_nointerposer"
            ;;
    *)
            LIBV4V_INC="-I${LIBV4V_PREFIX}/include"
            LIBV4V_LIB="-L${LIBV4V_PREFIX}/lib -lv4v_nointerposer"
            ;;
esac

AC_SUBST(LIBV4V_INC)
AC_SUBST(LIBV4V_LIB)

have_libv4=true
ORIG_LIBS="${LIBS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
    LIBS="${LIBS} ${LIBV4V_LIB}"
    CPPFLAGS="${CPPFLAGS} ${LIBV4V_INC}"
    AC_CHECK_HEADERS([libv4v.h], [], [have_libv4v=false])
    AC_CHECK_FUNC(v4v_socket, [], [have_libv4v=false])
LIBS="${ORIG_LIBS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

if test "x$have_libv4v" = "xfalse"; then
    AC_MSG_ERROR([where is libv4v dude])
fi

# libdmbus

AC_ARG_WITH([libdmbus],
            AC_HELP_STRING([--with-libdmbus=PATH], [Path to prefix where libdmbus was installed.]),
            [LIBDMBUS_PREFIX=$with_libdmbus], [])

case "x$LIBDMBUS_PREFIX" in
        x|xno|xyes)
                LIBDMBUS_INC=""
                LIBDMBUS_LIB="-ldmbus"
                ;;
        *)
                LIBDMBUS_INC="-I${LIBDMBUS_PREFIX}/include"
                LIBDMBUS_LIB="-L${LIBDMBUS_PREFIX}/lib -ldmbus"
                ;;
esac

AC_SUBST(LIBDMBUS_INC)
AC_SUBST(LIBDMBUS_LIB)

have_libdmbus=true

ORIG_LIBS="${LIBS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
	LIBS="${LIBS} ${LIBDMBUS_LIB} ${LIBV4V_LIB}"
	CPPFLAGS="${CPPFLAGS} ${LIBV4V_INC} ${LIBDMBUS_INC}"
	AC_CHECK_HEADERS([libdmbus.h], [], [have_libdmbus=false])
	AC_CHECK_FUNC(dmbus_init, [], [have_libdmbus=false])
LIBS="${ORIG_LIBS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

if test "x$have_libdmbus" = "xfalse"; then
	AC_MSG_ERROR([where is libdmbus dude])
fi

# libpthread

AC_ARG_WITH([libpthread],
            AC_HELP_STRING([--with-libpthread=PATH], [Path to prefix where libpthread was installed.]),
            [LIBPTHREAD_PREFIX=$with_libpthread], [])

case "x$LIBPTHREAD_PREFIX" in
        x|xno|xyes)
                LIBPTHREAD_INC=""
                LIBPTHREAD_LIB="-lpthread"
                ;;
        *)
                LIBPTHREAD_INC="-I${LIBPTHREAD_PREFIX}/include"
                LIBPTHREAD_LIB="-L${LIBPTHREAD_PREFIX}/lib -lpthread"
                ;;
esac

AC_SUBST(LIBPTHREAD_INC)
AC_SUBST(LIBPTHREAD_LIB)

have_libpthread=true

ORIG_LIBS="${LIBS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
	LIBS="${LIBS} ${LIBPTHREAD_LIB}"
	CPPFLAGS="${CPPFLAGS} ${LIBPTHREAD_INC}"
	AC_CHECK_HEADERS([pthread.h], [], [have_libpthread=false])
	AC_CHECK_FUNC(pthread_create, [], [have_libpthread=false])
LIBS="${ORIG_LIBS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

if test "x$have_libpthread" = "xfalse"; then
	AC_MSG_ERROR([where is libpthread dude])
fi

# Output files.
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(src/config.h)

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
