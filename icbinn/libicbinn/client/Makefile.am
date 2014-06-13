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


INCLUDES = @LIBV4V_CFLAGS@  -I../src

PROTOSRCS = client.c get.c ls.c mkdir.c null.c put.c rename.c rm.c rmdir.c stat.c util.c

bin_PROGRAMS = icbinn_ftp

icbinn_ftp_SOURCES = ${PROTOSRCS} 

icbinn_ftp_CFLAGS = @V4V_EXTRA_CFLAGS@ 

icbinn_ftp_LDADD =  ../src/libicbinn.la @LIBV4V_LIBS@ 

#AM_CFLAGS=-g -Wall

LDFLAGS+=-Wl,--no-as-needed

protos: 
	echo > prototypes.h
	${CPROTO} -v ${INCLUDES} ${PROTOSRCS} > prototypes.tmp
	mv -f prototypes.tmp prototypes.h


