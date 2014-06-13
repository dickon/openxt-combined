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

#ifndef DMBUS_H_
# define DMBUS_H_

# include <libdmbus.h>
# include "device-model.h"
# include "types.h"

enum dmbus_state
{
    DMBUS_CONNECT,
    DMBUS_RECONNECT,
    DMBUS_DISCONNECT,
};

/* Dmbus callback when state changed */
typedef void (*dmbus_state_cb_t) (struct device_model *devmodel,
                                  enum dmbus_state state);

bool dmbus_set_device (struct device_model *devmodel, int service,
                       DeviceType devtype, dmbus_state_cb_t state);

int dmbus_send (struct device_model *devmodel, uint32_t msgtype,
                void *data, size_t len);

#define dmbus_device_init(devname, service, devtype, state_func)            \
static bool devname##_parse_options (struct device_model *devmodel,         \
                                     const char *device)                    \
{                                                                           \
    (void) device;                                                          \
                                                                            \
    return dmbus_set_device (devmodel, (service), (devtype), (state_func)); \
}                                                                           \
                                                                            \
static struct device dev_##devname =                                        \
{                                                                           \
    .name = #devname,                                                       \
    .type = DEVMODEL_DMBUS,                                                 \
    .subtype = ((service) << 8) | (devtype),                                \
    .parse_options = devname##_parse_options,                               \
};                                                                          \
                                                                            \
device_init (dev_##devname)


#endif /* !DMBUS_H_ */
