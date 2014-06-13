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
# Copyright (c) 2012 Citrix Systems, Inc.
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


INCLUDES = @LIBTIRPC_CFLAGS@ @DBUS_CFLAGS@ @DBUS_GLIB_CFLAGS@ @LIBXCDBUS_CFLAGS@ @LIBV4V_CFLAGS@ 

if HAVE_LIBXCDBUS
DBUS_CLIENT_IDLS=xenmgr xenmgr_vm
else
DBUS_CLIENT_IDLS=
endif


DBUS_CLIENT_H=${DBUS_CLIENT_IDLS:%=%_client.h}

PROTOSRCS = server.c svc_main.c util.c dbusrpc.c version.c ../rpc/icbinn_prot_svc.c ../rpc/icbinn_prot_xdr.c ../rpc/xdr_fixup.c

bin_PROGRAMS = icbinn_svc 

icbinn_svc_SOURCES = ${PROTOSRCS} 

icbinn_svc_CFLAGS = @V4V_EXTRA_CFLAGS@ @XCDBUS_EXTRA_CFLAGS@

icbinn_svc_LDADD =  @LIBTIRPC_LIBS@ @LIBV4V_LIBS@ @DBUS_LIBS@ @DBUS_GLIB_LIBS@ @LIBXCDBUS_LIBS@

BUILT_SOURCES = ${DBUS_CLIENT_H}

CLEANFILES = ${DBUS_CLIENT_H}

#AM_CFLAGS=-g -Wall

LDFLAGS+=-Wl,--no-as-needed

%_client.h: ${IDLDIR}/%.xml
	${XC_RPCGEN} -c -o ./ $<

protos: 
	echo > prototypes.h
	${CPROTO} -v ${INCLUDES} ${PROTOSRCS} > prototypes.tmp
	mv -f prototypes.tmp prototypes.h


