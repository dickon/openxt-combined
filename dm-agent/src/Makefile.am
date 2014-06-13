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
		   ${LIBEVENT_INC} ${LIBXC_INC} ${LIBXCXENSTORE_INC}

SRCS = 						\
		device.c			\
		device-model.c		\
		dm-agent.c			\
		domain.c			\
		ioemu-device.c		\
		lockfile.c			\
		qemu-device.c		\
		spawn.c				\
		util.c				\
		xenstore.c

if DMBUSDEV
SRCS += dmbus-device.c dmbus.c
endif

noinst_HEADERS =					\
					device.h		\
					device-model.h	\
					dm-agent.h		\
					dmbus.h			\
					domain.h		\
					list.h			\
					spawn.h			\
					types.h			\
					util.h			\
					xenstore.h

bin_PROGRAMS = dm-agent
dm_agent_SOURCES = ${SRCS}
dm_agent_CFLAGS = -W -Werror -g -Wall
dm_agent_LDADD = ${LIBDMBUS_LIB} ${LIBV4V_LIB} \
				 ${LIBEVENT_LIB} ${LIBPTHREAD_LIB} ${LIBXC_LIB} \
				 ${LIBXCXENSTORE_LIB} ${LIBXENSTORE_LIB}
