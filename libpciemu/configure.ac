#
# Copyright (c) 2013 Citrix Systems, Inc.
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
AC_PREREQ(2.13)
AC_INIT()

LT_INIT

LIBPCIEMU_MAJOR_VERSION=`cat $srcdir/version-major`
LIBPCIEMU_MINOR_VERSION=`cat $srcdir/version-minor`
LIBPCIEMU_MICRO_VERSION=`cat $srcdir/version-micro`

LIBPCIEMU_INTERFACE_VERSION_MIN=$LIBPCIEMU_MINOR_VERSION
LIBPCIEMU_INTERFACE_VERSION_MAX=$LIBPCIEMU_MINOR_VERSION
LIBPCIEMU_INTERFACE_REVISION=1

LIBPCIEMU_VERSION=$LIBPCIEMU_MAJOR_VERSION.$LIBPCIEMU_MINOR_VERSION.$LIBPCIEMU_MICRO_VERSION

AC_SUBST(LIBPCIEMU_MAJOR_VERSION)
AC_SUBST(LIBPCIEMU_MINOR_VERSION)
AC_SUBST(LIBPCIEMU_MICRO_VERSION)
AC_SUBST(LIBPCIEMU_VERSION)

AC_SUBST(LIBPCIEMU_INTERFACE_VERSION_MIN)
AC_SUBST(LIBPCIEMU_INTERFACE_VERSION_MAX)
AC_SUBST(LIBPCIEMU_INTERFACE_REVISION)

LT_RELEASE=$LIBPCIEMU_MAJOR_VERSION.$LIBPCIEMU_MINOR_VERSION
LT_CURRENT=$LIBPCIEMU_INTERFACE_VERSION_MAX
LT_REVISION=$LIBPCIEMU_MICRO_VERSION
LT_AGE=`expr $LIBPCIEMU_INTERFACE_VERSION_MAX - $LIBPCIEMU_INTERFACE_VERSION_MIN`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

VERSION=$LIBPCIEMU_VERSION

PACKAGE=libpciemu

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

gl_VISIBILITY

CPPFLAGS="-D_GNU_SOURCE ${CPPFLAGS}"

# Check for pthread

ORIG_LIBS="${LIBS}"
LIBS=""
AC_SEARCH_LIBS([pthread_mutex_lock], [pthread])
PTHREAD_LIB="${LIBS}"
LIBS="${ORIG_LIBS}"
AC_SUBST(PTHREAD_LIB)

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
        AC_MSG_ERROR([*** At least libev or libevent is required.])
fi

# Check for libxc

AC_ARG_WITH([libxc],
            AC_HELP_STRING([--with-libxc=PATH], [Path to prefix where libc was installed.])
            [LIBXC_PREFIX=$with_libxc], [])

case "x$LIBXC_PREFIX" in
        x|xno|xyes)
                LIBXC_INC=""
                LIBXC_LIB="-lxenctrl"
                ;;
        *)
                LIBXC_INC="-I${LIBXC_PREFIX}/include"
                LIBXC_LIB="-L${LIBXC_PREFIX}/lib -lxenctrl"
                ;;
esac

AC_SUBST(LIBXC_INC)
AC_SUBST(LIBXC_LIB)

have_libxc=true

ORIG_LIBS="${LIBS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
	LIBS="${LIBS} ${LIBXC_LIB}"
	CPPFLAGS="${CPPFLAGS} ${LIBXC_INC}"
	AC_CHECK_HEADERS([xenctrl.h], [], [have_libxc=false])
	AC_CHECK_FUNC(xc_interface_open, [], [have_libxc=false])
LIBS="${ORIG_LIBS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

if test "x$have_libxc" = "xfalse"; then
	AC_MSG_ERROR([where is libxc dude])
fi

# Output files.
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(src/config.h)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 app/Makefile
                 libpciemu.pc.src])
AC_CONFIG_FILES([libpciemu-config.src],
                [chmod +x libpciemu-config.src])
AC_OUTPUT
