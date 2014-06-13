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


CPROTO=cproto
INCLUDES =  ${LIBE2P_INC} ${LIBEXT2FS_INC}  ${LIBBVHD_INC}

CFLAGS += -Wall

noinst_HEADERS=project.h prototypes.h

bin_PROGRAMS = gene3fs

SRCS=gene3fs.c version.c adler32.c util.c xattr.c vhdio.c
gene3fs_SOURCES = ${SRCS} 
gene3fs_LDADD =  ${LIBE2P_LIB} ${LIBEXT2FS_LIB} ${LIBBVHD_LIB}

AM_CFLAGS=-g

gene3fs_LDFLAGS = 

BUILT_SOURCES = version.h


VFD=${srcdir}/..
VF=${shell cat ${VFD}/version-files}
VFS=${VF:%=${VFD}/%}
VCHK=${shell cat ${VFS} | @MD5SUM@ | @AWK@ '{print $$1 }' }
VNUM=${shell @GREP@ ${VCHK} ${VFD}/version-md5sums | @AWK@ '{ print $$2 }'  }
VDEF=${shell echo `cat ${VFD}/version-major`.`cat ${VFD}/version-minor`.`cat ${VFD}/version-micro` }

protos:
	echo > prototypes.h
	${CPROTO} -v ${INCLUDES} ${SRCS} > prototypes.tmp
	mv -f prototypes.tmp prototypes.h



version.h: $(VFD)/version-files $(VFD)/version-major \
	$(VFD)/version-minor $(VFD)/version-micro \
	$(VFD)/version-md5sums ${VFS} Makefile
	if [ .${VNUM} = . ]; then \
		echo "#define VERSION \"gene3fs Version ${VDEF} + Edits\"" > version.h; \
		echo ${VDEF}-E > version-num; \
	else \
		echo "#define VERSION \"gene3fs Version ${VNUM}\"" > version.h; \
		echo ${VNUM} > version-num; \
	fi


test: gene3fs	
	rm -f ../test/fs
	touch ../test/fs
	mke2fs -F  -j ../test/fs 10240	
	./gene3fs -d . -i ../test/fs
	e2fsck -f -n  ../test/fs

testvhd: gene3fs	
	rm -f ../test/vhdfs
	touch ../test/vhdfs
	mke2fs -F  -j ../test/vhdfs 10240	
	bvhd_mkvhd ../test/vhdfs ../test/vhdfs.vhd
	./gene3fs -v -d . -i ../test/vhdfs.vhd
	bvhd_mkimg ../test/vhdfs.vhd ../test/vhdfs2
	e2fsck -f -n  ../test/vhdfs2
