#
#
# Makefile.am:
#
# Copyright (c) 2011 Citrix Systems Inc.,
# All rights reserved.
#
# $Id:$
#
# $Log:$
#
#
#

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


INCLUDES = 

SRCS= libedid.c version.c parse_edid.c util.c gen_edid.c
CPROTO=cproto

EDIDSRCS=${SRCS}

noinst_HEADERS= project.h prototypes.h edid-tail.h ext_prototypes.h

libedid_la_SOURCES = ${EDIDSRCS}

BUILT_SOURCES = version.h edid.h

lib_LTLIBRARIES=libedid.la

include_HEADERS=edid.h

AM_CFLAGS=-g

libedid_la_LDFLAGS = \
        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
        -release $(LT_RELEASE) \
        -export-dynamic 

VFD=${srcdir}/..
VF=${shell cat ${VFD}/version-files}
VFS=${VF:%=${VFD}/%}
VCHK=${shell cat ${VFS} | @MD5SUM@ | @AWK@ '{print $$1 }' }
VNUM=${shell @GREP@ ${VCHK} ${VFD}/version-md5sums | @AWK@ '{ print $$2 }'  }
VDEF=${shell echo `cat ${VFD}/version-major`.`cat ${VFD}/version-minor`.`cat ${VFD}/version-micro` }


edid.h:edid-head.h ext_prototypes.h edid-tail.h
	cat edid-head.h ext_prototypes.h edid-tail.h > edid.h

protos: 
	echo > prototypes.h
	echo > ext_prototypes.h
	cat edid-head.h ext_prototypes.h edid-tail.h > edid.h
	${CPROTO} -v -DINT_PROTOS ${INCLUDES} ${SRCS} > prototypes.tmp
	mv -f prototypes.tmp prototypes.h
	${CPROTO} -v -DEXT_PROTOS ${INCLUDES} ${SRCS} > ext_prototypes.h
	cat edid-head.h ext_prototypes.h edid-tail.h > edid.h

version.h: $(VFD)/version-files $(VFD)/version-major \
	$(VFD)/version-minor $(VFD)/version-micro \
	$(VFD)/version-md5sums ${VFS} Makefile
	if [ .${VNUM} = . ]; then \
		echo "#define FULL_VERSION \"libedid Version ${VDEF} + Edits\"" > version.h; \
		echo ${VDEF}-E > version-num; \
	else \
		echo "#define FULL_VERSION \"libedid Version ${VNUM}\"" > version.h; \
		echo ${VNUM} > version-num; \
	fi

