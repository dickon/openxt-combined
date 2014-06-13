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

from distutils.core import setup
from distutils.extension import Extension
#from subprocess import check_output
import subprocess;


def fish(x):
	return [y[2:] for y in x.split()]

def check_output(a):
	return subprocess.Popen(a, stdout=subprocess.PIPE).communicate()[0]

libicbinn_include_dirs = fish(check_output(["pkg-config", "--cflags-only-I", "libicbinn_resolved"]))
libicbinn_lib_dirs = fish(check_output(["pkg-config", "--libs-only-L", "libicbinn_resolved"]))
libicbinn_libs = fish(check_output(["pkg-config", "--libs-only-l", "libicbinn_resolved"]))

subprocess.call(["swig","-python","pyicbinn.i"])

ext_modules = [Extension("_pyicbinn", 
			sources=['pyicbinn_wrap.c'],
			library_dirs=libicbinn_lib_dirs,
			libraries=libicbinn_libs,
			include_dirs=libicbinn_include_dirs + [ '.' ])]

setup(name = 'pyicbinn',   version = '0.1',
	description = 'Python bindings for icbinn client using swig',
	ext_modules = ext_modules,
	py_modules = ["pyicbinn"],
	)

