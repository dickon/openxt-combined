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

SUBDIRS=. src
BUILT_SOURCES=version.h

bin_SCRIPTS = libicbinn_resolved-config 

pkgconfigdir=${libdir}/pkgconfig
pkgconfig_DATA= libicbinn_resolved.pc 

libicbinn_resolved.pc.src:libicbinn_resolved.pc.src.in
libicbinn_resolved.pc:libicbinn_resolved.pc.src
	@SED@ -f version.sed < libicbinn_resolved.pc.src > libicbinn_resolved.pc || rm -f libicbinn_resolved.pc


libicbinn_resolved-config.src:libicbinn_resolved-config.src.in
libicbinn_resolved-config:libicbinn_resolved-config.src version.sed
	@SED@ -f version.sed < libicbinn_resolved-config.src > libicbinn_resolved-config || rm -f libicbinn_resolved-config
	chmod +x $@
	
VNUM=${shell git rev-parse HEAD}

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

