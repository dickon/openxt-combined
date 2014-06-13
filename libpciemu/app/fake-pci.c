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

#include <event.h>
#include <inttypes.h>
#include <libpciemu.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PCI_VENDOR_ID_XEN 0x5853
#define PCI_DEVICE_ID_FAKEPCI 0xc148

static void pci_bar_update (uint8_t region, uint64_t addr, void *priv)
{
    (void) priv;

    if (addr == PCI_BAR_UNMAPPED)
        printf ("Bar %u unmapped\n", region);
    else
        printf ("Bar %u mapped to 0x%"PRIx64"\n", region, addr);
}

static uint64_t pci_mmio_read (uint64_t addr, uint32_t size, void *priv)
{
    (void) priv;

    fprintf (stderr, "pci_mmio_read addr=0x%"PRIx64" size=%u\n", addr, size);

    return 0x42;
}

static void pci_mmio_write (uint64_t addr, uint64_t data, uint32_t size,
                            void *priv)
{
    (void) priv;

    fprintf (stderr, "pci_mmio_write addr=0x%"PRIx64" data=0x%"PRIx64" size=%u\n",
             addr, data, size);
}

static libpciemu_io_ops_t pci_mmio_ops = {
    .read = pci_mmio_read,
    .write = pci_mmio_write,
};

static libpciemu_pci_t pci_init (int domid, libpciemu_handle_t iohandle)
{
    libpciemu_pci_info_t info;
    libpciemu_pci_t pci;

    memset (&info, 0, sizeof (info));

    info.vendor_id = PCI_VENDOR_ID_XEN;
    info.device_id = PCI_DEVICE_ID_FAKEPCI;
    info.subvendor_id = PCI_VENDOR_ID_XEN;
    info.subdevice_id = PCI_DEVICE_ID_FAKEPCI;
    /* Use 0:08.0 */
    info.bus = 0;
    info.device = 8;
    info.function = 0;
    info.bar_update = pci_bar_update;

    pci = libpciemu_pci_device_init (&info, NULL);

    if (!pci)
        return NULL;

    libpciemu_pci_register_bar (pci, 0, 1, PCI_BAR_TYPE_PREFETCH, 1024,
                                &pci_mmio_ops);

    if (libpciemu_pci_device_register (iohandle, pci))
    {
        fprintf (stderr, "Unable to register pci device\n");
        libpciemu_pci_device_release (pci);
        return NULL;
    }

    return pci;
}

static void shutdown (int signal)
{
    printf("Kill handlepci\n");
    /* FIXME: Need some cleanup here */
    exit(0);
}

static void usage (const char *prog)
{
    fprintf (stderr, "Usage: %s domid\n", prog);
}

static void pci_io (int fd, short event, void *opaque)
{
    libpciemu_handle_t iohandle = opaque;

    (void) fd;
    (void) event;

    libpciemu_handle (iohandle);
}

static void fake_pci_logging (libpciemu_loglvl lvl, const char *fmt, ...)
{
    va_list ap;

    (void) lvl;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

int main (int argc, char **argv)
{
    int domid;
    libpciemu_handle_t iohandle;
    int ret = 1;
    libpciemu_pci_t pci;
    struct event ioevent;

    if (argc != 2)
    {
        usage (argv[0]);
        return 1;
    }

    domid = atoi (argv[1]);
    if (domid <= 0)
        return 0;

    event_init ();
    signal (SIGKILL, shutdown);
    signal (SIGINT, shutdown);

    libpciemu_init (fake_pci_logging);

    if (!(iohandle = libpciemu_handle_create (domid)))
    {
        fprintf (stderr, "Unable to create iohandle\n");
        goto pcicleanup;
    }

    event_set (&ioevent, libpciemu_handle_get_fd (iohandle),
               EV_READ | EV_PERSIST, pci_io, iohandle);
    event_add (&ioevent, NULL);

    if (!(pci = pci_init (domid, iohandle)))
    {
        fprintf (stderr, "Unable to create pci\n");
        goto iohandlecleanup;
    }

    event_dispatch ();

iohandlecleanup:
    libpciemu_handle_destroy (iohandle);
pcicleanup:
    libpciemu_cleanup ();
    return ret;
}
