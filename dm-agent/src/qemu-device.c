/*
 * Copyright (c) 2013 Citrix Systems, Inc.
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

#include <stdlib.h>
#include <string.h>
#include "device.h"
#include "dm-agent.h"
#include "spawn.h"
#include "util.h"

#define qemu_device_init(devname, parse)                        \
static struct device dev_##devname =                            \
{                                                               \
    .name = #devname,                                           \
    .type = DEVMODEL_SPAWN,                                     \
    .subtype = SPAWN_QEMU,                                      \
    .parse_options = parse,                                     \
};                                                              \
                                                                \
device_init (dev_##devname)

static bool serial_device_parse_options (struct device_model *devmodel,
                                         const char *device)
{
    char *serial = device_option (devmodel, device, "device");
    bool res = true;

    if (!serial)
    {
        device_error (devmodel, device, "missing device option to create serial device");
        return false;
    }

    /* Disable pty serial in stubdomain: fails */
    if (dm_agent_in_stubdom () && !strcmp (serial, "pty"))
        goto end_serial;

    res = spawn_add_argument (devmodel, "-serial");
    if (!res)
        goto end_serial;

    res = spawn_add_argument (devmodel, serial);

end_serial:
    free (serial);

    return res;
}

qemu_device_init (serial, serial_device_parse_options)

static bool audio_device_parse_options (struct device_model *devmodel,
                                        const char *device)
{
    char *audio;
    bool res = false;


    audio = retrieve_option (devmodel, device, "device", audiodev);

    SPAWN_ADD_ARGL (devmodel, end_audio, "-soundhw");
    SPAWN_ADD_ARGL (devmodel, end_audio, audio);

    res = true;

end_audio:
    free (audio);
audiodev:
    return res;
}

qemu_device_init (audio, audio_device_parse_options)

static bool acpi_device_parse_options (struct device_model *devmodel,
                                       const char *device)
{
    return true;

    (void) device;

    return spawn_add_argument (devmodel, "-acpi");
}

qemu_device_init (acpi, acpi_device_parse_options);

static bool svga_device_parse_options (struct device_model *devmodel,
                                       const char *device)
{
    return true;

    (void) device;

    SPAWN_ADD_ARG (devmodel, "-videoram");
    SPAWN_ADD_ARG (devmodel, "16");
    SPAWN_ADD_ARG (devmodel, "-surfman");
    SPAWN_ADD_ARG (devmodel, "-std-vga");

    return true;
}

qemu_device_init (svga, svga_device_parse_options);

static bool cdrom_device_parse_options (struct device_model *devmodel,
                                        const char *device)
{
    char *cdrom = device_option (devmodel, device, "device");
    bool res = true;

    /* Skip cdrom for the moment */
    return true;

    if (!cdrom)
    {
        device_error (devmodel, device, "missing device option to create cdrom device");
        return false;
    }

    res = spawn_add_argument (devmodel, "-cdrom");
    if (!res)
        goto end_cdrom;

    res = spawn_add_argument (devmodel, cdrom);

end_cdrom:
    free (cdrom);

    return res;
}

qemu_device_init (cdrom, cdrom_device_parse_options);

static bool net_device_parse_options (struct device_model *devmodel,
                                      const char *device)
{
    char *id;
    char *model;
    char *bridge;
    char *mac;
    char *name;
    bool res = false;

    /* Fix need to handle bdf */

    id = retrieve_option (devmodel, device, "id", netid);
    model = retrieve_option (devmodel, device, "model", netmodel);
    bridge = retrieve_option (devmodel, device, "bridge", netbridge);
    mac = retrieve_option (devmodel, device, "mac", netmac);
    name = retrieve_option (devmodel, device, "name", netname);

    SPAWN_ADD_ARGL (devmodel, end_net, "-device");
    SPAWN_ADD_ARGL (devmodel, end_net, "%s,id=%s,netdev=%s,mac=%s",
                    model, name, device, mac);

    SPAWN_ADD_ARGL (devmodel, end_net, "-netdev");
    SPAWN_ADD_ARGL (devmodel, end_net, "type=bridge,id=%s,br=%s",
                    device, bridge);

    res = true;

end_net:
    free (name);
netname:
    free (mac);
netmac:
    free (bridge);
netbridge:
    free (model);
netmodel:
    free (id);
netid:
    return res;
}

qemu_device_init (net, net_device_parse_options);

static  bool gfx_device_parse_options (struct device_model *devmodel,
                                       const char *device)
{
    return true;

    (void) device;

    SPAWN_ADD_ARG (devmodel, "-gfx_passthru");
    SPAWN_ADD_ARG (devmodel, "-surfman");

    return true;
}

qemu_device_init (gfx, gfx_device_parse_options);

static bool vgpu_device_parse_options (struct device_model *devmodel,
                                       const char *device)
{
    return true;

    (void) device;

    SPAWN_ADD_ARG (devmodel, "-vgpu");
    SPAWN_ADD_ARG (devmodel, "-surfman");

    return true;
}

qemu_device_init (vgpu, vgpu_device_parse_options);

static bool drive_device_parse_options (struct device_model *devmodel,
                                        const char *device)
{
    char *file;
    char *media;
    char *format;
    char *index;
    bool res = false;

    file = retrieve_option (devmodel, device, "file", drivefile);
    media = retrieve_option (devmodel, device, "media", drivemedia);
    format = retrieve_option (devmodel, device, "format", driveformat);
    index = retrieve_option (devmodel, device, "index", driveindex);

    SPAWN_ADD_ARGL (devmodel, end_drive, "-drive");
    SPAWN_ADD_ARGL (devmodel, end_drive,
                    "file=%s,if=ide,index=%s,media=%s,format=%s",
                    file, index, media, format);

    res = true;

end_drive:
    free (index);
driveindex:
    free (format);
driveformat:
    free (media);
drivemedia:
    free (file);
drivefile:
    return res;
}

qemu_device_init (drive, drive_device_parse_options);
