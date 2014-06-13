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

INCLUDES = ${LIBXC_INC}

SRCS = pciemu.c io.c mapcache.c
CPROTO = cproto

SURFMANSRCS=${SRCS}

noinst_HEADERS =					\
					mapcache.h		\
					libpciemu_int.h \
					list.h			\
					compiler.h		\
					io.h 			\
				 	logging.h

libpciemu_la_SOURCES = ${SRCS}
libpciemu_la_CFLAGS = ${AM_CFLAGS} -Wall -Werror -Wmissing-prototypes
libpciemu_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic

#BUILT_SOURCES = pciemu.h

lib_LTLIBRARIES = libpciemu.la

include_HEADERS = libpciemu.h libpciemu_regs.h

#surfman.h: surfman-head.h ext_prototypes.h surfman-tail.h
#	cat surfman-head.h ext_prototypes.h surfman-tail.h > surfman.h

#protos:
#	echo > prototypes.h
#	echo > ext_prototypes.h
#	cat surfman-head.h ext_prototypes.h surfman-tail.h > surfman.h
#	${CPROTO} -e -v -E "${CPP} ${CPPFLAGS} -I." -DINT_PROTOS ${INCLUDES} ${SRCS} > prototypes.tmp
#	mv -f prototypes.tmp prototypes.h
#	${CPROTO} -e -v -E "${CPP} ${CPPFLAGS} -I." -DEXT_PROTOS ${INCLUDES} ${SRCS} > ext_prototypes.h
#	cat surfman-head.h ext_prototypes.h surfman-tail.h > surfman.h

#version.h: $(VFD)/version-files $(VFD)/version-major \
#	$(VFD)/version-minor $(VFD)/version-micro \
#	$(VFD)/version-md5sums ${VFS} Makefile
#	if [ .${VNUM} = . ]; then \
#		echo "#define FULL_VERSION \"libsurfman Version ${VDEF} + Edits\"" > version.h; \
#		echo ${VDEF}-E > version-num; \
#	else \
#		echo "#define FULL_VERSION \"libsurfman Version ${VNUM}\"" > version.h; \
#		echo ${VNUM} > version-num; \
#	fi
