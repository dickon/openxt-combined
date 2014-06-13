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

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include "domain.h"
# include "dm-agent.h"
# include "util.h"
# include "xenstore.h"

#define DOMAIN_KILLED_TRY 30
#define DOMAIN_KILLED_TIMER 1000 /* In millisecond */

static LIST_HEAD (, struct domain)
    domain_list = LIST_HEAD_INITIALIZER;

static char *domain_get_path (domid_t domid)
{
    char *path = NULL;

    if (asprintf (&path, "%s/dms/%u", dm_agent_get_path (), domid) == -1)
        return NULL;

    return path;
}

static void domain_check (domid_t domid)
{
    char *dompath = domain_get_path (domid);
    char *buf;
    struct domain *d = NULL;

    if (!dompath)
        return;

    buf = xenstore_read ("%s", dompath);
    if (!buf)
        goto end_check;

    free (buf);

    d = domain_by_domid (domid);
    if (d)
        goto end_check;

    /* Check if destroy node exists or domain is not handle by dm-agent */
    buf = xenstore_read ("%s/destroy", dompath);
    if (buf || (dm_agent_get_target () && dm_agent_get_target () != domid))
    {
        info ("Remove domain %u", domid);
        free (buf);
        xenstore_rm (dompath);
        goto end_check;
    }

    /* Create the domain */
    d = domain_create (domid);
    if (!d)
        goto end_check;

    domain_info (d, "new");

    device_models_create (d);

end_check:
    free (dompath);
    return;
}

/* List xenstore dms directory and check if all domain are created */
bool domains_create (void)
{
    char **doms;
    unsigned int num = 0;
    unsigned int i = 0;
    domid_t domid;

    doms = xenstore_ls (&num, "%s/dms", dm_agent_get_path ());
    if (!doms)
    {
        error ("Unable to retrieve the list of domains");
        return false;
    }

    for (i = 0; i < num; i++)
    {
        domid = atoi (doms[i]);
        domain_check (domid);
    }

    free (doms);

    return true;
}

/* Watch about domain creation/destruction */
void domains_watch (const char *path, void *priv)
{
    domid_t domid;
    char *buff = NULL;
    int res = -1;

    (void) priv;

    res = asprintf (&buff, "%s/dms/%%u", dm_agent_get_path ());
    if (res == -1)
        return;

    res = sscanf (path, buff, &domid);
    free (buff);

    if (res != 1)
        return;

    domain_check (domid);
}

/* When the node is fired destroy the domain */
static void domain_watch_destroy (const char *path, void *priv)
{
    struct domain *d = priv;

    if (!xenstore_read ("%s", path))
        return;

    domain_notify_destroy (d);
}

struct domain *domain_by_domid (domid_t domid)
{
    struct domain *d;

    LIST_FOREACH (d, &domain_list, link)
    {
        if (d->domid == domid)
            return d;
    }

    return NULL;
}

static bool domain_get_info (struct domain *domain)
{
    char *val;
    char *dompath = xenstore_get_domain_path (domain->domid);

    if (!dompath)
        goto err_get_info;

    val = xenstore_read ("%s/memory/static-max", dompath);
    if (!val)
        goto err_get_info;

    domain_info (domain, "retrieve memory size");

    domain->memkb = atoi (val);
    free (val);

    val = xenstore_read ("%s/platform/vcpu_number", dompath);
    if (!val)
        goto err_get_info;

    domain_info (domain, "retrieved vcpu number");

    domain->vcpus = atoi (val);
    free (val);

    val = xenstore_read ("%s/boot_order", dompath);
    if (!val)
        goto err_get_info;

    domain_info (domain, "retrieved boot order");

    domain->boot = val;

    return true;

err_get_info:
    free (dompath);
    domain_error (domain, "unable to retrieve domain info");
    return false;
}

/* Callback used to check if all device models are died */
static void domain_check_destroy (int fd, short ev, void *opaque)
{
    struct domain *d = opaque;
    struct device_model *devmodel;
    bool all_destroyed = true;
    struct timeval time;

    (void) fd;
    (void) ev;

    LIST_FOREACH (devmodel, &d->devmodels, link)
    {
        if (devmodel->status != DEVMODEL_DIED)
        {
            devmodel_info (devmodel, " waiting for dying ...");
            all_destroyed = false;
        }
    }

    domain_info (d, "all_destroyed = %d num_try_destroyed = %u\n",
                 all_destroyed, d->num_try_destroy);

    d->num_try_destroy++;

    if (!all_destroyed && d->num_try_destroy < DOMAIN_KILLED_TRY)
    {
        time.tv_sec = DOMAIN_KILLED_TIMER / 1000;
        time.tv_usec = (DOMAIN_KILLED_TIMER % 1000) * 1000;
        event_add (&d->ev_destroy, &time);
    }
    else
        domain_destroy (d, true);
}

struct domain *domain_create (domid_t domid)
{
    struct domain *d = NULL;

    info ("create domain %u", domid);

    if (domain_by_domid (domid))
    {
        error ("Domain %u: already exists", domid);
        return NULL;
    }

    d = calloc (1, sizeof (*d));
    if (!d)
    {
        error ("Domain %u: can't allocate memory", domid);
        return NULL;
    }

    d->dompath = domain_get_path (domid);
    d->domid = domid;
    d->num_try_destroy = 0;

    if (!d->dompath)
        goto fdomain;

    if (!domain_get_info (d))
        goto fdompath;

    if (!xenstore_watch (domain_watch_destroy, d, "%s/destroy", d->dompath))
        goto fgetinfo;

    if (!xenstore_watch_dir (device_models_watch, d, d->dompath))
        goto fdwatch;

    LIST_INSERT_HEAD (&domain_list, d, link);
    event_set (&d->ev_destroy, -1, EV_TIMEOUT, domain_check_destroy, d);

    return d;

fdwatch:
    xenstore_watch (NULL, NULL, "%s/destroy", d->dompath);
fgetinfo:
    free (d->boot);
fdompath:
    free (d->dompath);
fdomain:
    free (d);
    return NULL;

}

void domain_notify_destroy (struct domain *d)
{
    struct device_model *devmodel, *tmp;
    struct timeval time;

    domain_info (d, "kill all device models");
    d->num_try_destroy = 0;

    time.tv_sec = DOMAIN_KILLED_TIMER / 1000;
    time.tv_usec = (DOMAIN_KILLED_TIMER % 1000) * 1000;
    event_add (&d->ev_destroy, &time);

    LIST_FOREACH_SAFE (devmodel, tmp, &d->devmodels, link)
        device_model_notify_destroy (devmodel);
}

void domain_destroy (struct domain *d, bool killall)
{
    struct device_model *devmodel, *tmp;

    LIST_REMOVE (d, link);

    domain_info (d, "destroy");
    event_del (&d->ev_destroy);

    if (killall)
    {
        /* Use xenstore only when we want to remove all informations */
        xenstore_watch (NULL, NULL, "%s/destroy", d->dompath);
        xenstore_watch_dir (NULL, NULL, d->dompath);
        xenstore_rm ("%s", d->dompath);
    }

    LIST_FOREACH_SAFE (devmodel, tmp, &d->devmodels, link)
        device_model_destroy (devmodel, killall);

    free (d->boot);
    free (d->dompath);
    memset (d, 0, sizeof (*d));
    free (d);
}

void domains_destroy (bool killall)
{
    struct domain *d, *tmp;

    LIST_FOREACH_SAFE (d, tmp, &domain_list, link)
        domain_destroy (d, killall);
}
