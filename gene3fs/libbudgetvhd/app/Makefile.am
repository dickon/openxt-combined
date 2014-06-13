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


INCLUDES = -I$(srcdir)/../src 

bin_PROGRAMS = bvhd_mkvhd bvhd_mkimg bvhd_create

bvhd_mkvhd_SOURCES = bvhd_mkvhd.c
bvhd_mkvhd_LDADD = ../src/libbudgetvhd.la  

bvhd_mkimg_SOURCES = bvhd_mkimg.c
bvhd_mkimg_LDADD = ../src/libbudgetvhd.la  

bvhd_create_SOURCES = bvhd_create.c
bvhd_create_LDADD = ../src/libbudgetvhd.la  

AM_CFLAGS=-g -Wall


