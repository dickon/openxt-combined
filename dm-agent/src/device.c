/*
 * Copyright (c) 2012 Citrix Systems, Inc.
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

#include <stdio.h>
#include <string.h>
#include "device.h"
#include "util.h"
#include "xenstore.h"

static list_device_t dev_list = LIST_HEAD_INITIALIZER;

void register_device (struct device *device)
{
    struct device *dev = NULL;

    /* By default enable device */
    device->disabled = false;

    LIST_FOREACH (dev, &dev_list, link)
    {
        if (dev->type == device->type && dev->subtype == device->subtype)
            break;
    }

    if (dev)
        LIST_INSERT_BEFORE (dev, device, link);
    else
        LIST_INSERT_HEAD (&dev_list, device, link);
}

const struct device *device_by_name (const char *name)
{
    const struct device *dev;

    LIST_FOREACH (dev, &dev_list, link)
    {
        if (!strcmp (name, dev->name) && !dev->disabled)
            return dev;
    }

    return NULL;
}

void device_disable (const char *name, bool disabled)
{
    struct device *dev;

    LIST_FOREACH (dev, &dev_list, link)
    {
        if (strcmp (name, dev->name))
            continue;

        dev->disabled = disabled;
    }
}

void devices_disable (bool disabled)
{
    struct device *dev;

    LIST_FOREACH (dev, &dev_list, link)
        dev->disabled = disabled;
}

void devices_group_disable (enum devmodel_type type, unsigned int subtype,
                            bool disabled)
{
    struct device *dev = NULL;


    info ("Disable=%d group %d-%d", disabled, type, subtype);

    LIST_FOREACH (dev, &dev_list, link)
    {
        if (dev->type == type && dev->subtype == subtype)
            dev->disabled = disabled;
    }
}

const list_device_t *device_list (void)
{
    return &dev_list;
}

char **device_list_option (struct device_model *devmodel, const char *dev,
                           unsigned int *num)
{
    return xenstore_ls (num, "%s/%s", devmodel->dmpath, dev);
}

char *device_option (struct device_model *devmodel, const char *dev,
                     const char *option)
{
    return xenstore_read ("%s/infos/%s/%s", devmodel->dmpath, dev, option);
}

uint32_t str_to_bdf (const char *bdf)
{
    unsigned int bus, dev, fn;
    int res;

    res = sscanf (bdf, "%x:%x.%d", &bus, &dev, &fn);

    if (res != 3 || bus != 0 || dev > 0x1f || fn > 0x7)
        return ~0;

    return (bus << 8 | dev << 5 | fn);
}

