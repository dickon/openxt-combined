#
#
# Makefile.am:
#
# Copyright (c) 2011 Citrix Systems Inc.,
# All rights reserved.
#
# $Id:$
#
# $Log:$
#
#
#

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

SUBDIRS=src test app

EXTRA_DIST= version-major version-minor version-micro version-files version-md5sums

bin_SCRIPTS = libedid-config

pkgconfigdir=${libdir}/pkgconfig
pkgconfig_DATA= libedid.pc

libedid.pc.src:libedid.pc.src.in
libedid.pc:libedid.pc.src
	@SED@ -f version.sed < libedid.pc.src > libedid.pc || rm -f libedid.pc

libedid-config.src:libedid-config.src.in
libedid-config:libedid-config.src version.sed
	@SED@ -f version.sed < libedid-config.src > libedid-config || rm -f libedid-config
	chmod +x $@
	

VFD=${srcdir}
VF=${shell cat ${VFD}/version-files}
VFS=${VF:%=${VFD}/%}
VCHK=${shell cat ${VFS} | @MD5SUM@ | @AWK@ '{print $$1 }' }
VNUM=${shell @GREP@ ${VCHK} ${VFD}/version-md5sums | @AWK@ '{ print $$2 }'  }
VDEF=${shell echo `cat ${VFD}/version-major`.`cat ${VFD}/version-minor`.`cat ${VFD}/version-micro` }

version.sed: $(VFD)/version-files $(VFD)/version-major \
	$(VFD)/version-minor $(VFD)/version-micro \
	$(VFD)/version-md5sums ${VFS} Makefile
	if [ .${VNUM} = . ]; then \
		echo "s/%VERSION%/${VDEF}-E/g" > version.sed; \
	else \
		echo "s/%VERSION%/${VNUM}/g" > version.sed; \
	fi

