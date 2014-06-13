/*
 * Copyright (c) 2014 Citrix Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "project.h"
#include "cmd.h"
#include "owls.h"

#define USBOWLS_PATH "/usr/bin/usbowls"

int owls_domain_attach(int bus, int dev, int domid)
{
    char bus_str[64], dev_str[64], domid_str[64];
    snprintf(bus_str, sizeof(bus_str), "%d", bus);
    snprintf(dev_str, sizeof(dev_str), "%d", dev);
    snprintf(domid_str, sizeof(domid_str), "%d", domid);
    {
        char *argv[] = {
            USBOWLS_PATH, "attach",
            "-b", bus_str, "-d", dev_str,
            "-D", domid_str,
            NULL
        };

        int rv = runcmd(USBOWLS_PATH, argv, NULL, NULL);
        if (rv) {
            LogError("Failed to attach device %d-%d to domain %d", bus, dev, domid);
        }
        return rv;
    }
}

int owls_domain_detach(int bus, int dev, int domid)
{
    char bus_str[64], dev_str[64], domid_str[64];
    snprintf(bus_str, sizeof(bus_str), "%d", bus);
    snprintf(dev_str, sizeof(dev_str), "%d", dev);
    snprintf(domid_str, sizeof(domid_str), "%d", domid);
    {
        char *argv[] = {
            USBOWLS_PATH, "detach",
            "-b", bus_str, "-d", dev_str,
            "-D", domid_str,
            NULL
        };

        int rv = runcmd(USBOWLS_PATH, argv, NULL, NULL);
        if (rv) {
            LogError("Failed to detach device %d-%d from domain %d", bus, dev, domid);
        }
        return rv;
    }
}

int owls_vusb_assign(int bus, int dev)
{
    char bus_str[64], dev_str[64];
    snprintf(bus_str, sizeof(bus_str), "%d", bus);
    snprintf(dev_str, sizeof(dev_str), "%d", dev);
    {
        char *argv[] = {
            USBOWLS_PATH, "assign",
            "-b", bus_str, "-d", dev_str,
            NULL
        };

        int rv = runcmd(USBOWLS_PATH, argv, NULL, NULL);
        if (rv) {
            LogError("Failed to assign device %d-%d to vusb driver", bus, dev);
        }
        return rv;
    }
}

int owls_vusb_unassign(int bus, int dev)
{
    char bus_str[64], dev_str[64];
    snprintf(bus_str, sizeof(bus_str), "%d", bus);
    snprintf(dev_str, sizeof(dev_str), "%d", dev);
    {
        char *argv[] = {
            USBOWLS_PATH, "unassign",
            "-b", bus_str, "-d", dev_str,
            NULL
        };

        int rv = runcmd(USBOWLS_PATH, argv, NULL, NULL);
        if (rv) {
            LogError("Failed to unassign device %d-%d from vusb driver", bus, dev);
        }
        return rv;
    }
}

static int parse_ids(char *str, int str_sz, int *out, int out_sz)
{
    int b = 0, i = 0, numids = 0;
    while (i < str_sz) {
        if (str[i] == 0 || str[i] == '\n') {
            str[i] = 0;
            if (strlen(&str[b]) > 0 && numids < out_sz)
                out[numids++] = atoi(&str[b]);
            b = i + 1;
        }
        ++i;
    }
    return numids;
}

int owls_list_domain_devices(int domid, int *buf_devids, int buf_sz)
{
    char domid_str[64];

    snprintf(domid_str, sizeof(domid_str), "%d", domid);
    {
        char *argv[] = {
            USBOWLS_PATH, "list",
            "-D", domid_str,
            NULL
        };
        char text[4096] = { 0 };
        int text_sz = sizeof(text);

        int rv = runcmd(USBOWLS_PATH, argv, text, &text_sz);
        if (rv) {
            LogError("Failed to list domain %d devices", domid);
            return 0;
        }
        return parse_ids(text, text_sz, buf_devids, buf_sz);
    }
}
