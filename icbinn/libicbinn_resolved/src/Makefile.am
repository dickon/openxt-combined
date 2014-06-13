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


INCLUDES =

LIBICBINN_LIBS_NO_DL=${LIBICBINN_LIBS:-ldl=}

libicbinn_resolved_a_SOURCES =
libicbinn_resolved_a_LIBADD = libicbinn_prelink.o

#${AM_CFLAGS} ${V4V_EXTRA_CFLAGS}

lib_LIBRARIES=libicbinn_resolved.a #libicbinn_resolved.so


#libicbinn_so_LDFLAGS = \
#        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
#        -release $(LT_RELEASE) \
#        -export-dynamic \
#	@LIBTIRPC_LIBS@ @LIBV4V_LIBS@

LDFLAGS+=-Wl,--no-as-needed

libicbinn_prelink.o: 
	        ${CC} ${CFLAGS} ${LDFLAGS} -nostdlib -nodefaultlibs -Wl,-r -o $@ -Wl,-u -Wl,icbinn_canary -Wl,-Bstatic ${LIBICBINN_LIBS_NO_DL} -Wl,-Bdynamic 



