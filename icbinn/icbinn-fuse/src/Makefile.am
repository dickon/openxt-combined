#
# Copyright (c) 2013 Citrix Systems, Inc.
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

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = icbinn-fs
bin_SCRIPTS = icbinn-fs-domU

icbinn_fs_CFLAGS  = -D_FILE_OFFSET_BITS=64 @LIBV4V_CFLAGS@ @FUSE_CFLAGS@ @LIBICBINN_CFLAGS@ 
icbinn_fs_LDADD   = @LIBV4V_LIBS@ @FUSE_LIBS@ @LIBICBINN_LIBS@
icbinn_fs_SOURCES = icbinn-fs.c

