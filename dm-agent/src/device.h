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

#ifndef DEVICE_H_
# define DEVICE_H_

# include "device-model.h"
# include "list.h"

/**
 * return false if something fail
 */
typedef bool (*device_parse_option_cb_t) (struct device_model *devmodel,
                                          const char *device);

/* Maximal number of dependencies */
# define MAX_DEPENDS 10

struct device
{
    const char *name;
    /* Which is the devmodel associate with this device */
    enum devmodel_type type;
    unsigned int subtype;
    /* Parse XenStore */
    device_parse_option_cb_t parse_options;
    LIST_ENTRY (struct device) link;
    /* Is device is disabled ? */
    bool disabled;
    const char *depends[MAX_DEPENDS];
};

#define DEVICE_DEPENDS_END() NULL

typedef LIST_HEAD(, struct device) list_device_t;

/* Register a device in dm-agent */
void register_device (struct device *device);

/* Retrieve a device with its name */
const struct device *device_by_name (const char *name);

/* Disable/Enable device */
void device_disable (const char *name, bool disabled);
void devices_disable (bool disabled);
/* Disable/Enable a group of device */
void devices_group_disable (enum devmodel_type type, unsigned int subtype,
                            bool disabled);

/* Retrieve the list of device. All device models are group by type/subtype */
const list_device_t *device_list (void);

/* List device options */
char **device_list_option (struct device_model *devmodel, const char *dev,
                           unsigned int *num);
/* Retrieve a device option value */
char *device_option (struct device_model *devmodel, const char *dev,
                     const char *option);

/**
 * Take a bdf in string format (ie: 01:a.1) and return the value or ~0
 * if an error occured
 */
uint32_t str_to_bdf (const char *bdf);

#define BDF_BUS(bdf) (((bdf) >> 8) & 0xff)
#define BDF_DEV(bdf) (((bdf) >> 5) & 0x1f)
#define BDF_FUN(bdf) ((bdf) & 0x7)

#define device_foreach(iter) LIST_FOREACH (iter, device_list (), link)

#define device_init(dev)                                                \
static void __attribute__((constructor)) do_device_init##dev (void)     \
{                                                                       \
    register_device (&dev);                                             \
}

#define device_info(devmodel, dev, fmt, ...)                            \
    info ("dom %u, dm %u, dev %s: "fmt,                                 \
          (devmodel)->domain->domid,                                    \
          (devmodel)->dmid, dev, ## __VA_ARGS__)

#define device_error(devmodel, dev, fmt, ...)                           \
    error ("dom %u, dm %u, dev %s: "fmt,                                \
           (devmodel)->domain->domid,                                   \
           (devmodel)->dmid, dev, ## __VA_ARGS__)

#define retrieve_option(devmodel, device, option, label_err)        \
({                                                                  \
    char *_option = NULL;                                           \
                                                                    \
    _option = device_option (devmodel, device, option);             \
    if (!_option)                                                   \
    {                                                               \
        device_error (devmodel, device, "missing option '%s'",      \
                      option);                                      \
        goto label_err;                                             \
    }                                                               \
    _option;                                                        \
})

#endif /* ! DEVICE_H_ */
