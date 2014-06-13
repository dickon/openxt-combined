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

#include "device.h"
#include "dmbus.h"
#include "util.h"

#define SUBTYPE(s, dev) ((s) << 8) | (dev)

static void xengfx_state (struct device_model *devmodel,
                          enum dmbus_state state)
{
    switch (state)
    {
    case DMBUS_CONNECT:
        devmodel_info (devmodel, "dmbus change state %u", state);
        device_model_status (devmodel, DEVMODEL_RUNNING);
        break;
    /* For the moment we can support surfman reconnection */
    case DMBUS_DISCONNECT:
        devmodel_warning (devmodel, "Can't retry to connect to surfman \
                          with xengfx device");
        device_model_status (devmodel, DEVMODEL_DIED);
        break;
    default:
        devmodel_info (devmodel, "dmbus change state %u", state);
    }
}

static void xenfb_state (struct device_model *devmodel,
                         enum dmbus_state state)
{
    switch (state)
    {
    case DMBUS_RECONNECT:
    case DMBUS_CONNECT:
        devmodel_info (devmodel, "dmbus change state %u", state);
        device_model_status (devmodel, DEVMODEL_RUNNING);
        break;
    default:
        devmodel_info (devmodel, "dmbus change state %u", state);
    }
}

static void input_state (struct device_model *devmodel,
                         enum dmbus_state state)
{
    struct msg_switcher_abs msg;

    switch (state)
    {
    case DMBUS_CONNECT:
    case DMBUS_RECONNECT:
        devmodel_info (devmodel, "dmbus change state %u", state);
        msg.enabled = 0;
        dmbus_send (devmodel, DMBUS_MSG_SWITCHER_ABS, &msg, sizeof (msg));
        device_model_status (devmodel, DEVMODEL_RUNNING);
        break;
    default:
        devmodel_info (devmodel, "dmbus change state %u", state);
    }
}

dmbus_device_init (xengfx, DMBUS_SERVICE_SURFMAN, DEVICE_TYPE_XENGFX,
                   xengfx_state)
dmbus_device_init (xenfb, DMBUS_SERVICE_SURFMAN, DEVICE_TYPE_XENFB,
                   xenfb_state)
dmbus_device_init (input, DMBUS_SERVICE_INPUT, DEVICE_TYPE_INPUT,
                   input_state)
