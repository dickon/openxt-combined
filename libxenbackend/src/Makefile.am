#
#
# Makefile.am:
#
# Copyright (c) 2010 Julian Pidancet <julian.pidancet@citrix.com>,
# All rights reserved.
#
# $Id:$
#
# $Log:$
#
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


INCLUDES = ${LIBXENSTORE_INC} ${LIBXC_INC}

SRCS = xs.c state.c backend.c
CPROTO = cproto

XENBACKENDSRCS=${SRCS}

noinst_HEADERS = project.h prototypes.h xenbackend-tail.h ext_prototypes.h

libxenbackend_la_SOURCES = ${XENBACKENDSRCS}
libxenbackend_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	-export-dynamic

BUILT_SOURCES = xenbackend.h

lib_LTLIBRARIES = libxenbackend.la

include_HEADERS = xenbackend.h
AM_CFLAGS = -g -W -Wall

xenbackend.h: xenbackend-head.h ext_prototypes.h xenbackend-tail.h
	cat xenbackend-head.h ext_prototypes.h xenbackend-tail.h > xenbackend.h

protos:
	echo > prototypes.h
	echo > ext_prototypes.h
	cat xenbackend-head.h ext_prototypes.h xenbackend-tail.h > xenbackend.h
	${CPROTO} -E "${CPP} ${CPPFLAGS}" -DINT_PROTOS ${INCLUDES} ${SRCS} > prototypes.tmp
	mv -f prototypes.tmp prototypes.h
	${CPROTO} -E "${CPP} ${CPPFLAGS}" -DEXT_PROTOS ${INCLUDES} ${SRCS} > ext_prototypes.h
	cat xenbackend-head.h ext_prototypes.h xenbackend-tail.h > xenbackend.h

