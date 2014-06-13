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

#include <assert.h>
#include <errno.h>
#include <event.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "dm-agent.h"
#include "spawn.h"
#include "xenstore.h"
#include "util.h"

#define ARGS_BATCH 10
#define QEMU_NEW_PATH "/usr/bin/qemu-system-i386"
#define IOEMU_PATH "/usr/sbin/svirt-interpose"
#define IOEMU_STUBDOM_PATH "/usr/lib/xen/bin/qemu-dm"
#define WAITPID_TIMER 2000 /* In millisecond */

struct spawn
{
    struct
    {
        size_t size; /* Array size */
        size_t pos;
        char **args;
    } arguments;

    struct device_model *devmodel;
    enum spawn_type type;
    /* Where the device model will write its status */
    char *dmpath;

    pid_t pid;

    LIST_ENTRY (struct spawn) link;
};

static LIST_HEAD (, struct spawn)
    spawn_list = LIST_HEAD_INITIALIZER;

static struct event ev_waitpid;

static struct spawn *spawn_by_pid (pid_t pid)
{
    struct spawn *spawn;

    LIST_FOREACH (spawn, &spawn_list, link)
    {
        if (spawn->pid == pid)
            return spawn;
    }

    return NULL;
}

static void spawn_waitpid (int fd, short ev, void *opaque)
{
    struct timeval time;
    pid_t pid;
    int status = 0;
    struct spawn *spawn = NULL;

    (void) fd;
    (void) ev;
    (void) opaque;

    pid = waitpid (-1, &status, WNOHANG);
    if (pid == -1 || pid == 0)
        goto end_waitpid;

    info ("child %d died", pid);

    spawn = spawn_by_pid (pid);
    if (!spawn)
    {
        info ("child %d not associated to a device model", pid);
        goto end_waitpid;
    }

    devmodel_info (spawn->devmodel, "died");
    spawn->pid = -1;
    LIST_REMOVE (spawn, link);
    device_model_status (spawn->devmodel, DEVMODEL_DIED);

end_waitpid:
    time.tv_sec = WAITPID_TIMER / 1000;
    time.tv_usec = (WAITPID_TIMER % 1000) * 1000;
    event_add (&ev_waitpid, &time);
}

static bool spawn_init (void)
{
    struct timeval time;

    event_set (&ev_waitpid, -1, EV_TIMEOUT, spawn_waitpid, NULL);

    time.tv_sec = WAITPID_TIMER / 1000;
    time.tv_usec = (WAITPID_TIMER % 1000) * 1000;

    event_add (&ev_waitpid, &time);

    return true;
}

static bool spawn_qemu_args (struct device_model *devmodel)
{
    SPAWN_ADD_ARG (devmodel, QEMU_NEW_PATH);

    SPAWN_ADD_ARG (devmodel, "-xen-domid");
    SPAWN_ADD_ARG (devmodel, "%u", devmodel->domain->domid);

    SPAWN_ADD_ARG (devmodel, "-nodefaults");

    SPAWN_ADD_ARG (devmodel, "-name");
    SPAWN_ADD_ARG (devmodel, "qemu-%u.%u",
                   devmodel->domain->domid,
                   devmodel->dmid);

    SPAWN_ADD_ARG (devmodel, "-machine");
    SPAWN_ADD_ARG (devmodel, "xenfv,xen_dmid=%u,xen_default_dev=on",
                   devmodel->dmid);

    SPAWN_ADD_ARG (devmodel, "-m");
    SPAWN_ADD_ARG (devmodel, "%lu", devmodel->domain->memkb >> 10);

    SPAWN_ADD_ARG (devmodel, "-smp");
    SPAWN_ADD_ARG (devmodel, "%u", devmodel->domain->vcpus);

    return true;
}

static bool spawn_ioemu_args (struct device_model *devmodel)
{
    if (dm_agent_in_stubdom ())
    {
        SPAWN_ADD_ARG (devmodel, IOEMU_STUBDOM_PATH);
        SPAWN_ADD_ARG (devmodel, "-stubdom");
    }
    else
    {
        SPAWN_ADD_ARG (devmodel, IOEMU_PATH);
        SPAWN_ADD_ARG (devmodel, "%u", devmodel->domain->domid);

        /* Unused param but shift by the wrapper */
        SPAWN_ADD_ARG (devmodel, "/tmp/qemu.%u", devmodel->domain->domid);
    }

    SPAWN_ADD_ARG (devmodel, "-d");
    SPAWN_ADD_ARG (devmodel, "%u", devmodel->domain->domid);

    SPAWN_ADD_ARG (devmodel, "-m");
    SPAWN_ADD_ARG (devmodel, "%lu", devmodel->domain->memkb >> 10);

    SPAWN_ADD_ARG (devmodel, "-vcpus");
    SPAWN_ADD_ARG (devmodel, "%u", devmodel->domain->vcpus);

    /* Dunno why we need both vcpus and smp. It's quiet useless */
    if (devmodel->domain->vcpus > 1)
    {
        SPAWN_ADD_ARG (devmodel, "-smp");
        SPAWN_ADD_ARG (devmodel, "%u", devmodel->domain->vcpus);
    }

    SPAWN_ADD_ARG (devmodel, "-M");
    SPAWN_ADD_ARG (devmodel, "xenfv");

    if (devmodel->domain->boot && strcmp(devmodel->domain->boot, "")) {
        SPAWN_ADD_ARG (devmodel, "-boot");
	SPAWN_ADD_ARG (devmodel, "%s", devmodel->domain->boot);
    }

    return true;
}

static bool spawn_create (struct device_model *devmodel, unsigned int type)
{
    char *dompath = NULL;
    char *tmp;
    int res;
    struct spawn *spawn = NULL;

    assert (devmodel->type_priv == NULL);

    spawn = calloc (1, sizeof (*spawn));
    if (!spawn)
        return false;

    devmodel->type_priv = spawn;
    spawn->type = type;
    spawn->devmodel = devmodel;
    spawn->pid = -1;

    dompath = xenstore_get_domain_path (devmodel->domain->domid);
    if (!dompath)
        return false;

    if (spawn->type == SPAWN_QEMU_OLD)
        res = asprintf (&tmp, "%s/dms/qemu-old", dompath);
    else
        res = asprintf (&tmp, "%s/dms/%u/state", dompath, devmodel->dmid);
    free (dompath);

    if (res == -1)
        return false;

    spawn->dmpath = tmp;

    spawn->arguments.args = calloc (ARGS_BATCH,
                                    sizeof (*spawn->arguments.args));

    if (!spawn->arguments.args)
        return false;

    spawn->arguments.pos = 1;
    spawn->arguments.size = ARGS_BATCH;
    spawn->arguments.args[0] = NULL;

    /* Add default arguments */
    switch (spawn->type)
    {
    case SPAWN_QEMU:
        return spawn_qemu_args (devmodel);
    case SPAWN_QEMU_OLD:
        return spawn_ioemu_args (devmodel);
    default:
        devmodel_warning (devmodel, "no default arguments for type %u",
                          type);
    }

    return true;
}

static void spawn_kill (struct device_model *devmodel)
{
    struct spawn *spawn = devmodel->type_priv;

    if (spawn->pid > 0)
    {
        devmodel_info (devmodel, "will be kill");
        kill (spawn->pid, SIGTERM);
    }
}

static void spawn_destroy (struct device_model *devmodel, bool killall)
{
    struct spawn *spawn = devmodel->type_priv;
    unsigned int i = 0;

    if (!spawn)
        return;

    if (spawn->pid > 0)
    {
        LIST_REMOVE (spawn, link);
        if (killall)
        {
            devmodel_info (devmodel, "killed without sommation");
            kill (spawn->pid, SIGKILL);
        }
    }

    if (killall)
        xenstore_watch (NULL, NULL, "%s", spawn->dmpath);

    free (spawn->dmpath);

    for (i = 0; i < spawn->arguments.pos; i++)
        free (spawn->arguments.args[i]);

    free (spawn->arguments.args);
    memset (spawn, 0, sizeof (*spawn));
    free (spawn);
}

static void spawn_ready (const char *path, void *priv)
{
    struct spawn *spawn = priv;
    char *res = NULL;

    (void) path;

    res = xenstore_read ("%s", spawn->dmpath);

    if (!res)
        return;

    device_model_status (spawn->devmodel, DEVMODEL_RUNNING);
}

static void spawn_dump_cmdline (struct device_model *devmodel, char **argv)
{
    char *buf = NULL;
    char *tmp = NULL;

    while (*argv)
    {
        if (buf)
        {
            if (asprintf (&tmp, "%s %s", buf, *argv) == -1)
            {
                free (buf);
                return;
            }
            free (buf);
            buf = tmp;
        }
        else
        {
            buf = strdup (*argv);
            if (!buf)
                return;
        }
        argv++;
    }

    if (buf)
        devmodel_info (devmodel, "cmdline: %s", buf);

    free (buf);
}

static bool spawn_start (struct device_model *devmodel)
{
    struct spawn *spawn = devmodel->type_priv;
    char **argv = spawn->arguments.args;
    bool log_saved;

    if (!xenstore_watch (spawn_ready, spawn, "%s", spawn->dmpath))
        return false;

    spawn_dump_cmdline (devmodel, argv);

    spawn->pid = fork ();

    if (spawn->pid == 0) /* Child */
    {
        spawn->arguments.args = NULL;
        spawn->arguments.pos = 0;
        signal (SIGTERM, SIG_DFL);
        signal (SIGINT, SIG_DFL);

        // change log prefix
        prefix_set ("dm-agent-fork");

        // Some informations are shared with the father, so we need to reinit
        if (event_reinit (dm_agent_get_ev_base ()))
            fatal ("Failed to reinit event");

        /* Disable logging during destroy otherwise logs are confused */
        log_saved = logging (LOG_NONE);
        dm_agent_destroy (false);
        logging (log_saved);

        /**
         * FIXME: it seems that libevent segault with event_base_free
         * I don't know why ...
         **/
//        event_base_free (dm_agent_get_ev_base ());

        info ("Exec\n");

        /* FIX: free memory + close dmbus connexion */
        execv (argv[0], argv);
        fatal ("Failed to exec '%s': %s", argv[0], strerror (errno));
    }
    else if (spawn->pid == -1)
    {
        devmodel_error (devmodel, "failed to fork: %s", strerror (errno));
        xenstore_watch (NULL, NULL, "%s", spawn->dmpath);
        return false;
    }

    devmodel_info (devmodel, " spawn");

    LIST_INSERT_HEAD (&spawn_list, spawn, link);

    return true;
}

bool spawn_add_argument (struct device_model *devmodel, const char *fmt, ...)
{
    char **tmp = NULL;
    char *str = NULL;
    va_list arg;
    int ret;
    struct spawn *spawn = devmodel->type_priv;

    if (spawn->arguments.pos == spawn->arguments.size)
    {
        spawn->arguments.size += ARGS_BATCH;
        tmp = realloc (spawn->arguments.args,
                       spawn->arguments.size * sizeof (*spawn->arguments.args));
        if (!tmp)
            return false;
        spawn->arguments.args = tmp;
    }

    va_start (arg, fmt);
    ret = vasprintf (&str, fmt, arg);
    va_end (arg);

    if (ret == -1)
        return false;

    spawn->arguments.args[spawn->arguments.pos - 1] = str;
    spawn->arguments.args[spawn->arguments.pos++] = NULL;

    return true;
}

static struct device_model_ops devmodel_ops =
{
    .type = DEVMODEL_SPAWN,
    .init = spawn_init,
    .create = spawn_create,
    .start = spawn_start,
    .kill = spawn_kill,
    .destroy = spawn_destroy,
};

devmodel_init (devmodel_ops)
