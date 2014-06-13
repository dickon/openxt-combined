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

SUBDIRS=. rpc src server client test 


BUILT_SOURCES=version.h

bin_SCRIPTS = libicbinn-config 

pkgconfigdir=${libdir}/pkgconfig
pkgconfig_DATA= libicbinn.pc 

libicbinn.pc.src:libicbinn.pc.src.in
libicbinn.pc:libicbinn.pc.src
	@SED@ -f version.sed < libicbinn.pc.src > libicbinn.pc || rm -f libicbinn.pc


libicbinn-config.src:libicbinn-config.src.in
libicbinn-config:libicbinn-config.src version.sed
	@SED@ -f version.sed < libicbinn-config.src > libicbinn-config || rm -f libicbinn-config
	chmod +x $@
	
VNUM=${shell git rev-parse HEAD}

do_test: all
	@if [ `id -u` -ne 0 ]; then echo ""; echo "*** Error: must run as root" ; false ; fi
	server/icbinn_svc -p 4878 -p 4879 & SPID=$$!; \
	HAPPY=true; \
	test/test -h localhost -p 4878 || HAPPY=false; \
	test/test -h localhost -p 4879 || HAPPY=false; \
	kill -9 $$SPID || true ; \
	$$HAPPY

version.sed: never-happy
	if git diff --quiet --exit-code; then \
		echo "s/%VERSION%/${VNUM}/g" > version.sed; \
	else \
		echo "s/%VERSION%/${VNUM}+diffs/g" > version.sed; \
	fi

version.h: never-happy
	if git diff --quiet --exit-code; then \
		echo "#define FULL_VERSION \"libicbinn Version ${VNUM}\"" > version.h; \
	else \
		echo "#define FULL_VERSION \"libicbinn Version ${VNUM}+diffs\"" > version.h; \
	fi

never-happy:

