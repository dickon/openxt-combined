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
#include "device-model.h"
#include "util.h"
#include "xenstore.h"

static LIST_HEAD (, struct device_model_ops)
    devmodel_ops_list = LIST_HEAD_INITIALIZER;

void register_device_model (struct device_model_ops *ops)
{
    LIST_INSERT_HEAD (&devmodel_ops_list, ops, link);
}

bool device_models_init (void)
{
    struct device_model_ops *ops = NULL;

    LIST_FOREACH (ops, &devmodel_ops_list, link)
    {
        if (!ops->init ())
            return false;
    }

    return true;
}

static char *device_model_get_path (struct domain *d, dmid_t dmid)
{
    char *path = NULL;

    if (asprintf (&path, "%s/%u", d->dompath, dmid) == -1)
        return NULL;

    return path;
}

static void device_model_check (struct domain *d, dmid_t dmid)
{
    char *dmpath = device_model_get_path (d, dmid);
    char *buf;
    struct device_model *devmodel;

    if (!dmpath)
        return;

    buf = xenstore_read ("%s", dmpath);
    if (!buf)
        goto end_check;

    free (buf);

    devmodel = device_model_by_dmid (d, dmid);
    if (devmodel)
        goto end_check;

    devmodel = device_model_create (d, dmid);
    if (devmodel)
        devmodel_info (devmodel, "new");

end_check:
    free (dmpath);
    return;
}

bool device_models_create (struct domain *d)
{
    char **devmodels;
    unsigned int num = 0;
    unsigned int i = 0;
    dmid_t dmid;

    devmodels = xenstore_ls (&num, "%s", d->dompath);
    if (!devmodels)
    {
        error ("Domain %u: unable to retrieve the list of device models",
               d->domid);
        return false;
    }

    for (i = 0; i < num; i++)
    {
        dmid = atoi (devmodels[i]);
        device_model_check (d, dmid);
    }

    free (devmodels);

    return true;
}

void device_models_watch (const char *path, void *priv)
{
    struct domain *d = priv;
    char *buff = NULL;
    int res;
    dmid_t dmid;

    res = asprintf (&buff, "%s/%%u", d->dompath);
    if (res == -1)
        return;

    res = sscanf (path, buff, &dmid);
    free (buff);

    if (res != 1)
        return;

    /* Don't fire on status node */
    res = asprintf (&buff, "%s/%u/status", d->dompath, dmid);
    if (res != -1 && (!strncmp (path, buff, res)))
    {
        free (buff);
        return;
    }

    free (buff);

    device_model_check (d, dmid);
}

struct device_model *device_model_by_dmid (struct domain *d, dmid_t dmid)
{
    struct device_model *devmodel = NULL;

    LIST_FOREACH (devmodel, &d->devmodels, link)
    {
        if (devmodel->dmid == dmid)
            return devmodel;
    }

    return NULL;
}

static struct device_model_ops *device_model_ops_by_type (enum devmodel_type type)
{
    struct device_model_ops *ops;

    LIST_FOREACH (ops, &devmodel_ops_list, link)
    {
        if (ops->type == type)
            return ops;
    }

    return NULL;
}

static bool device_model_parse_config (struct device_model *devmodel)
{
    char **devices = NULL;
    unsigned int num_device = 0;
    unsigned int i;
    const struct device *dev = NULL;
    bool first = true;
    enum devmodel_type type;
    unsigned int subtype;
    char *devtype;

    devices = xenstore_ls (&num_device, "%s/infos", devmodel->dmpath);

    if (!devices)
    {
        devmodel_error (devmodel, "unable to retrieve configuration");
        return false;
    }

    devmodel_info (devmodel, "num devices = %u", num_device);
    if (!num_device)
    {
        devmodel_error (devmodel, "can't create device model with 0 device");
        return false;
    }

    devmodel_info (devmodel, "list devices");

    for (i = 0; i < num_device; i++)
    {
        devmodel_info (devmodel, "%s", devices[i]);
        /* Retrieve the type of device */
        devtype = xenstore_read ("%s/infos/%s/type", devmodel->dmpath,
                                 devices[i]);
        if (!devtype)
        {
            device_error (devmodel, devices[i], "unable to retrieve the type");
            goto parserr;
        }

        dev = device_by_name (devtype);
        if (!dev)
        {
            device_error (devmodel, devices[i], "unable to find device type %s",
                          devtype);
            free (devtype);
            goto parserr;
        }

        free (devtype);

        if (first)
        {
            devmodel->ops = device_model_ops_by_type (dev->type);
            if (!devmodel->ops)
            {
                devmodel_error (devmodel, "can't find ops for the device model");
                goto parserr;
            }
            if (!devmodel->ops->create (devmodel, dev->subtype))
            {
                devmodel_error (devmodel, "can't prepare the device model");
                goto parserr;
            }
            type = dev->type;
            subtype = dev->subtype;
            first = false;
        }
        else if (type != dev->type || subtype != dev->subtype)
        {
            device_error (devmodel, devices[i],
                          "the device is incompatible with this device model");
            goto parserr;
        }

        if (!dev->parse_options (devmodel, devices[i]))
        {
            device_error (devmodel, devices[i], "unable to parse");
            goto parserr;
        }
    }

    free (devices);

    return true;

parserr:
    free (devices);
    return false;
}

struct device_model *device_model_create (struct domain *d, dmid_t dmid)
{
    struct device_model *devmodel = NULL;

    if (device_model_by_dmid (d, dmid))
    {
        domain_error (d, "device model %u already exists", dmid);
        return NULL;
    }

    devmodel = calloc (1, sizeof (*devmodel));

    if (!devmodel)
    {
        domain_error (d, "Devmodel %u: can't allocate memory", dmid);
        return NULL;
    }

    devmodel->status = DEVMODEL_STARTING;
    devmodel->dmid = dmid;
    devmodel->domain = d;
    LIST_INSERT_HEAD (&d->devmodels, devmodel, link);

    devmodel->dmpath = device_model_get_path (d, dmid);

    if (!devmodel->dmpath)
        goto errdmpath;

    /* Parse the configuration */
    if (!device_model_parse_config (devmodel))
        goto errparse;

    /* start the device model */
    if (!devmodel->ops->start (devmodel))
        goto errparse;

    return devmodel;

errparse:
    device_model_status (devmodel, DEVMODEL_DIED);
    if (devmodel->ops)
        devmodel->ops->destroy (devmodel, true);
    free (devmodel->dmpath);
errdmpath:
    LIST_REMOVE (devmodel, link);
    free (devmodel);

    return NULL;
}

bool device_model_status (struct device_model *devmodel,
                          enum devmodel_status status)
{
    const char *str;
    bool res;

    switch (status)
    {
    case DEVMODEL_STARTING:
        str = "starting";
        break;
    case DEVMODEL_RUNNING:
        str = "running";
        break;
    case DEVMODEL_DIED:
        str = "died";
        break;
    case DEVMODEL_KILLED:
        str = "killed";
        break;
    default:
        devmodel_warning (devmodel, "unhandle status %d", status);
        return false;
    }

    devmodel->status = status;

    devmodel_info (devmodel, "change status to %s", str);

    res = xenstore_write (str, "%s/status", devmodel->dmpath);

    return res;
}

void device_model_notify_destroy (struct device_model *devmodel)
{
    /* Kill the device model */
    devmodel->ops->kill (devmodel);
}

void device_model_destroy (struct device_model *devmodel, bool killall)
{
    LIST_REMOVE (devmodel, link);

    devmodel_info (devmodel, "destroy");

    free (devmodel->dmpath);
    if (devmodel->ops)
        devmodel->ops->destroy (devmodel, killall);

    memset (devmodel, 0, sizeof (*devmodel));
    free (devmodel);
}
