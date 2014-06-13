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

.PHONY: all
all: bootage
bootage: bootage.o

clean:
	rm -f bootage{,.s,.i,.o}

#CFLAGS=-ggdb
LDFLAGS += -lrt

.PHONY: install
install: all
	install -d $(DESTDIR)/sbin
	install -m 755 bootage $(DESTDIR)/sbin/bootage

.PHONY: test
test: CFLAGS += -DBOOTAGE_TEST_MODE
test: clean bootage
	for n in `ls ./tests/`; do \
		for l in `seq 0 5`; do \
			mkdir -pv ./tests/$$n/etc/rc$$l.d/; \
		done; \
	done
	./run_tests
