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


RPCGEN=@RPCGEN@

PROT = icbinn_prot.x

PROTC=${PROT:%.x=%_clnt.c} 
PROTX=${PROT:%.x=%_xdr.c}
PROTS=${PROT:%.x=%_svc.c}
PROTH=${PROT:%.x=%.h}

#all: ${BUILD_SOURCES}

BUILT_SOURCES = ${PROTH} ${PROTC}  ${PROTS}  ${PROTX}

CLEANFILES = ${PROTH} ${PROTC} ${PROTS}

RPCGENOPS=-C

%.h: %.x
	$(RPCGEN) $(RPCGENOPS) -h $< > $@ || rm -f $@

%_xdr.c: %.x
	$(RPCGEN) $(RPCGENOPS) -c $< > $@ || rm -f $@

%_svc.c: %.x
	$(RPCGEN) $(RPCGENOPS) -m $< > $@ || rm -f $@

%_clnt.c: %.x
	$(RPCGEN) $(RPCGENOPS) -l $< > $@ || rm -f $@


