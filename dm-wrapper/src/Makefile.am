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


INCLUDES = ${LIBXENSTORE_INC} ${LIBDMBUS_INC} ${LIBV4V_INC} ${LIBPTHREAD_INC} \
		   ${LIBEVENT_INC}

SRCS = 						\
		arguments.c			\
		dmbus.c				\
		qemu-wrapper.c		\
		util.c

noinst_HEADERS = 			\
		arguments.h			\
		qemu-wrapper.h		\
		util.h

bin_PROGRAMS = qemu-dm-wrapper
qemu_dm_wrapper_SOURCES = ${SRCS}
qemu_dm_wrapper_CFLAGS = -W -Werror -Wno-unused -g -Wall
qemu_dm_wrapper_LDADD = ${LIBXENSTORE_LIB} ${LIBDMBUS_LIB} ${LIBV4V_LIB} \
					   	${LIBEVENT_LIB} ${LIBPTHREAD_LIB}
