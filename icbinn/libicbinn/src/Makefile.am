#
#
# Makefile.am:
#
# Copyright (c) 2012 James McKenzie <20@madingley.org>,
# All rights reserved.
#
# $Id:$
#
# $Log:$
#
#
#

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


INCLUDES = @LIBTIRPC_CFLAGS@ @LIBV4V_CFLAGS@

LIBV4V_LIBS_NO_DL=${LIBV4V_LIBS:-ldl=}

PROTOSRCS=client.c close.c lock.c mkdir.c null.c open.c pread.c pwrite.c readdir.c rename.c rmdir.c stat.c unlink.c util.c version.c ftruncate.c fallocate.c rand.c readlink.c symlink.c statfs.c fsync.c canary.c

ICBINNSRCS=../rpc/icbinn_prot_clnt.c ../rpc/icbinn_prot_xdr.c  ${PROTOSRCS} ../rpc/xdr_fixup.c

noinst_HEADERS=project.h prototypes.h icbinn-tail.h ext_prototypes.h 

libicbinn_a_SOURCES =  ${ICBINNSRCS}
libicbinn_la_SOURCES = ${ICBINNSRCS}

libicbinn_a_CFLAGS = ${AM_CFLAGS} ${V4V_EXTRA_CFLAGS}
libicbinn_la_CFLAGS = ${AM_CFLAGS} ${V4V_EXTRA_CFLAGS}

BUILT_SOURCES = icbinn.h 

lib_LIBRARIES=libicbinn.a 
lib_LTLIBRARIES=libicbinn.la

include_HEADERS=icbinn.h 

AM_CFLAGS=-g -Wall  -Wno-unused -fPIC

libicbinn_la_LDFLAGS = \
        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
        -release $(LT_RELEASE) \
        -export-dynamic \
	@LIBTIRPC_LIBS@ @LIBV4V_LIBS@

LDFLAGS+=-Wl,--no-as-needed

icbinn.h:icbinn-head.h ext_prototypes.h icbinn-tail.h
	cat icbinn-head.h ext_prototypes.h icbinn-tail.h > icbinn.h

protos: 
	echo > prototypes.h
	echo > ext_prototypes.h
	cat icbinn-head.h ext_prototypes.h icbinn-tail.h > icbinn.h
	${CPROTO} -v -DINT_PROTOS ${INCLUDES} ${PROTOSRCS} > prototypes.tmp
	mv -f prototypes.tmp prototypes.h
	${CPROTO} -v -DEXT_PROTOS ${INCLUDES} ${PROTOSRCS} > ext_prototypes.h
	cat icbinn-head.h ext_prototypes.h icbinn-tail.h > icbinn.h
