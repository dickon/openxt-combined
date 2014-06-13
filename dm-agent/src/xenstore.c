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

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "device-model.h"
#include "device.h"
#include "dm-agent.h"
#include "domain.h"
#include "xenstore.h"
#include "util.h"

/* Create the capabilities in xenstore */
static bool xenstore_capabilities_create (s_dm_agent *dm_agent)
{
    const struct device *device;
    const struct device *devtype = NULL;
    unsigned int i = 0;

    // Remove the old capabilities directory
    xenstore_rm ("%s/%s", dm_agent->dmapath, "capabilities");

    if (!xenstore_mkdir ("%s/%s", dm_agent->dmapath, "capabilities"))
        return false;

    device_foreach (device)
    {
        if (device->disabled)
            continue;
        if (!devtype || devtype->type != device->type
            || devtype->subtype != device->subtype)
        {
            if (!xenstore_mkdir ("%s/%s/%u-%u", dm_agent->dmapath,
                                 "capabilities", device->type, device->subtype))
                return false;
            devtype = device;
        }

        if (!xenstore_mkdir ("%s/%s/%u-%u/%s", dm_agent->dmapath,
                             "capabilities", device->type,
                             device->subtype, device->name))
            return false;

        /* Create dependencies */
        if (device->depends[0])
        {
            if (!xenstore_mkdir ("%s/capabilities/%u-%u/%s/depends",
                                 dm_agent->dmapath,
                                 device->type,
                                 device->subtype, device->name))
                return false;

            for (i = 0; i < MAX_DEPENDS && device->depends[i]; i++)
            {
                if (!xenstore_write ("", "%s/capabilities/%u-%u/%s/depends/%s",
                                     dm_agent->dmapath, device->type,
                                     device->subtype, device->name,
                                     device->depends[i]))
                    return false;
            }
        }

    }

    return true;
}

/* Create the node target if needed */
static bool xenstore_write_target (s_dm_agent *dm_agent)
{
    char *buf = NULL;
    bool res = false;

    if (!dm_agent->target_domid)
        return true;

    if (asprintf (&buf, "%u", dm_agent->target_domid) == -1)
        return false;

    res = xenstore_write (buf, "%s/target", dm_agent->dmapath);
    free (buf);

    return res;
}

bool xenstore_dm_agent_create (s_dm_agent *dm_agent)
{
    char *dompath = NULL;
    char *tmp = NULL;
    int res = 0;

    dompath = xenstore_get_domain_path (dm_agent->domid);

    if (!dompath)
        return false;

    res = asprintf (&tmp, "%s/dm-agent", dompath);

    free (dompath);

    if (res == -1)
        return false;

    dm_agent->dmapath = tmp;

restart_transaction:
    if (!xenstore_transaction_start ())
        return false;

    if (!xenstore_mkdir (dm_agent->dmapath))
        goto fail_transaction;

    if (!xenstore_mkdir ("%s/dms", dm_agent->dmapath))
        goto fail_transaction;

    if (!xenstore_write_target (dm_agent))
        goto fail_transaction;

    if (!xenstore_capabilities_create (dm_agent))
        goto fail_transaction;

    if (!xenstore_transaction_end (false))
    {
       if (errno == EAGAIN)
            goto restart_transaction;
        else
            return false;
    }

    /* Watch device models creation */
    if (!xenstore_watch_dir (domains_watch, dm_agent,
                             "%s/dms", dm_agent->dmapath))
        return false;

    return true;

fail_transaction:
    xenstore_transaction_end (true);

    return false;
}

void xenstore_dm_agent_destroy (s_dm_agent *dm_agent, bool killall)
{
    if (!dm_agent->dmapath)
        return;

    free (dm_agent->dmapath);

    if (!killall)
        return;

    if (!xenstore_rm (dm_agent->dmapath))
        warning ("Unable to remove '%s' in xenstore", dm_agent->dmapath);
}
