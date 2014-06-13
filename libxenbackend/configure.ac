#
# Copyright (c) 2011 Citrix Systems, Inc.
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

# Prelude.
AC_PREREQ(2.13)
AC_INIT(src/backend.c)

LT_INIT

LIBXENBACKEND_MAJOR_VERSION=`cat $srcdir/version-major`
LIBXENBACKEND_MINOR_VERSION=`cat $srcdir/version-minor`
LIBXENBACKEND_MICRO_VERSION=`cat $srcdir/version-micro`

LIBXENBACKEND_INTERFACE_VERSION_MIN=$LIBXENBACKEND_MINOR_VERSION
LIBXENBACKEND_INTERFACE_VERSION_MAX=$LIBXENBACKEND_MINOR_VERSION
LIBXENBACKEND_INTERFACE_REVISION=1

LIBXENBACKEND_VERSION=$LIBXENBACKEND_MAJOR_VERSION.$LIBXENBACKEND_MINOR_VERSION.$LIBXENBACKEND_MICRO_VERSION

AC_SUBST(LIBXENBACKEND_MAJOR_VERSION)
AC_SUBST(LIBXENBACKEND_MINOR_VERSION)
AC_SUBST(LIBXENBACKEND_MICRO_VERSION)
AC_SUBST(LIBXENBACKEND_VERSION)

AC_SUBST(LIBXENBACKEND_INTERFACE_VERSION_MIN)
AC_SUBST(LIBXENBACKEND_INTERFACE_VERSION_MAX)
AC_SUBST(LIBXENBACKEND_INTERFACE_REVISION)

LT_RELEASE=$LIBXENBACKEND_MAJOR_VERSION.$LIBXENBACKEND_MINOR_VERSION
LT_CURRENT=$LIBXENBACKEND_INTERFACE_VERSION_MAX
LT_REVISION=$LIBXENBACKEND_MICRO_VERSION
LT_AGE=`expr $LIBXENBACKEND_INTERFACE_VERSION_MAX - $LIBXENBACKEND_INTERFACE_VERSION_MIN`

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

VERSION=$LIBXENBACKEND_VERSION
PACKAGE=libxenbackend

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
AC_CHECK_HEADERS([pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for libraries.

ORIG_LIBS="${LIBS}"
LIBS=""
AC_SEARCH_LIBS([pthread_mutex_lock], [pthread])
PTHREAD_LIB="${LIBS}"
LIBS="${ORIG_LIBS}"
AC_SUBST(PTHREAD_LIB)


AC_ARG_WITH(libxc,
            AC_HELP_STRING([--with-libxc=PATH],
                           [Path to prefix where where libxc and xen were installed]),
            LIBXC_PREFIX=$with_libxc, [])

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

ORIG_LDFLAGS="${LDFLAGS}"
ORIG_CPPFLAGS="${CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${LIBXC_LIB}"
CPPFLAGS="${CPPFLAGS} ${LIBXC_INC}"
AC_CHECK_HEADERS([xenctrl.h])
AC_CHECK_FUNCS([xc_version xc_domain_iommu_x_mapping])
LDFLAGS="${ORIG_LDFLAGS}"
CPPFLAGS="${ORIG_CPPFLAGS}"

# Xenstore

AC_ARG_WITH([libxenstore],
            AC_HELP_STRING([--with-libxenstore=PATH], [Path to prefix where libxenstore was installed.]),
            [LIBXENSTORE_PREFIX=$with_libxenstore], [])

case "x$LIBXENSTORE_PREFIX" in
        x|xno)
                LIBXENSTORE_INC=""
                LIBXENSTORE_LIB="-lxenstore"
                ;;
        xyes)
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

# Output files.
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(src/config.h)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/xenbackend-head.h
                 libxenbackend.pc.src])
AC_CONFIG_FILES([libxenbackend-config.src],
                [chmod +x libxenbackend-config.src])

AC_OUTPUT
