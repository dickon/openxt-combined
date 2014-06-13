/*
 * Copyright (c) 2013 Citrix Systems, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef IO_H_
# define IO_H_

# include <pthread.h>
# include <stdint.h>
# include <xenctrl.h>
# include <xen/hvm/ioreq.h>
# include "libpciemu_int.h"
# include "list.h"
# include "mapcache.h"

/* Mask a value to avoid spurious byte */
#define BMASK(data, bsize)                                                  \
    ((data) & ((-(uint64_t)1) >> ((sizeof (uint64_t) * 8) - (bsize) * 8)))

/* Handle to xen control */
extern xc_interface *xch;

/* Describe an I/O range */
struct iorange
{
    /* Beginning and end (inclusive) of the range */
    uint64_t start;
    uint64_t end;
    /* Is it a PIO or MMIO range ? */
    int mmio;
    /* Private data */
    void *priv;

    /* Read/Write callback when an access is made in this range */
    libpciemu_io_ops_t ops;

    /* Link to the next iorange */
    LIST_ENTRY(struct iorange) link;
};

/* Describe I/O handle for a domain */
struct iohandle
{
    /* Shared page used to communicate with Xen */
    shared_iopage_t *shpage_ptr;
    buffered_iopage_t *buffered_iopage;
    /* Event channel used to notify a new event */
    xc_evtchn *evtchn;
    int fd;
    unsigned int ports[HVM_MAX_VCPUS];
    int nports;
    /* Server id */
    ioservid_t serverid;
    /* Mapcache, used to avoid lots of mapping */
    mapcache_t mapcache;
    /* Domain id */
    domid_t domid;
    /* List of I/O trapped */
    LIST_HEAD (, struct iorange) iorange_list;
    pthread_mutex_t iorange_lock;
    /* List of PCI associated */
    LIST_HEAD (, struct s_pci) pci_list;
    pthread_mutex_t pci_lock;
};

#endif /* !IO_H_ */
