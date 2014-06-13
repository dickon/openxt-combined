#! /usr/bin/env python

from sys import argv
from subprocess import check_call, check_output
from tempfile import mkdtemp
repo = argv[1]

try:
    workd = mkdtemp()
    check_call(['git', 'clone', 'https://github.com/OpenXT/'+repo+'.git', workd])
    version = check_output(['git', 'rev-parse', 'HEAD'])
    print 'verison', version
    check_call(['mkdir', '-p', repo])
    check_call(['rsync', '-rE', '--exclude', '.git', workd+'/', repo])
    check_call(['git', 'add', repo])
    name = check_output(['git', 'config', 'user.name']).strip()
    email = check_output(['git', 'config', 'user.email']).strip()
    try:
        check_call(['git', 'config', 'user.name', 'Open XT'])
        check_call(['git', 'config', 'user.email', 'openxtprime@gmail.com'])
        check_call(['git', 'commit', '-m', 'import '+repo+' version '+version])
    finally:
        check_call(['git', 'config', 'user.name', name])
        check_output(['git', 'config', 'user.email', email])
finally:
    check_call(['rm', '-rf', workd])
