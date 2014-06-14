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

#include <errno.h>
#include <event.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#ifdef SYSLOG
# include <syslog.h>
#endif
#include <sys/wait.h>
#include <unistd.h>
#include <xenstore.h>
#include "arguments.h"
#include "qemu-wrapper.h"
#include "util.h"

# define QEMU_OLD_PATH "/usr/bin/qemu-dm-wrapper-old"
# define QEMU_OLD_STUBDOMAIN_PATH "/usr/lib/xen/bin/qemu-dm"
# define QEMU_NEW_PATH "/usr/bin/qemu-system-i386"

/**
 * FIXME:
 *  - destroy dmbus connection on fork
 *  - check the command line. For the moment we assume that it's valid
 */

static struct
{
    /* Xen store handle */
    struct xs_handle *xsh;
    /* Xen store event */
    struct event xs_ev;
    char *dompath;
    unsigned int domid;
    unsigned int num_watch;
    /* Number of QEMU spawned */
    unsigned int num_dm;
    int shutting_down;
    /* Don't spawn multiple qemu if it's an HVM */
    int is_hvm;
    int in_stubdomain;
    /* first pci slot available */
    uint8_t pcidev_slot;
} g_info;

static uint8_t pcidev_get_slot (void)
{
    return g_info.pcidev_slot++;
}

static int dm_has_cap (const s_spawn_dm *dm, cap_t cap)
{
    return (dm->cap & cap);
}

static char * const *build_args_qemu_old (const s_spawn_dm *dm,
                                          int argc, char * const *argv)
{
    s_arguments args;
    int i = 0;
    int has_xengfx = 0;

    (void) dm;

    arguments_init (&args);

    /* Don't use a wrapper if we are in stubdomain */
    arguments_add (&args, g_info.in_stubdomain ? QEMU_OLD_STUBDOMAIN_PATH : QEMU_OLD_PATH);

    if (find_arg (argv, "-xengfx"))
        has_xengfx = 1;

    for (i = 0; i < argc; i++)
    {
        if (i == 0 && g_info.in_stubdomain) /* Skip domid */
            continue;
        else if (!strcmp ("-surfman", argv[i]) && has_xengfx)
        {
            arguments_add (&args, "-vga");
            arguments_add (&args, "none");
        }
        else if (!strcmp ("-net", argv[i])
                 && dm_has_cap (dm, DM_CAP_NETWORK) != DM_CAP_NETWORK)
            i++;
        else if (!strcmp ("-videoram", argv[i]) && has_xengfx)
            i++;
        else
            arguments_add (&args, argv[i]);
    }

    if (dm_has_cap (dm, DM_CAP_NETWORK) != DM_CAP_NETWORK)
    {
        arguments_add (&args, "-net");
        arguments_add (&args, "none");
    }

    return arguments_get (&args);
}

static int qemu_new_net (const s_spawn_dm *dm, char * const *argv,
                         s_arguments *args)
{
    char *tmp;
    char *type;
    char *value;
    char *opt;
    char *id = NULL;
    char *mac = NULL;
    char *model = NULL;
    char *ifname = NULL;
    int res = 1;
    char str[200];

    if (strcmp (argv[0], "-net"))
        return 0;
    if (!argv[1])
        return 0;

    tmp = xstrdup (argv[1]);

    type = strtok (tmp, ",");

    if (!type)
        return 0;

    while ((opt = strtok (NULL, ",")))
    {
        value = strchr (opt,'=');

        if (!value)
            continue;

        *value = '\0';
        value ++;

        if (!strcmp ("vlan", opt))
            id = value;
        else if (!strcmp ("macaddr", opt))
            mac = value;
        else if (!strcmp ("model", opt))
            model = value;
        else if (!strcmp ("ifname", opt))
            ifname = value;
    }

    info ("%s id = %s cap = 0x%x", dm->name, id, dm->cap);

    if (!id || (atoi(id) && !dm_has_cap (dm, DM_CAP_WIFI))
        || (!atoi(id) && !dm_has_cap (dm, DM_CAP_WIRED)))
    {
        info ("Out");
        res = 0;
    }
    else if (!strcmp ("nic", type) && model && mac)
    {
        arguments_add (args, "-device");
        snprintf (str, sizeof (str), "%s,id=%s%s,netdev=net%s,mac=%s,addr=%u",
                  model, atoi(id) ? "vwif" : "vif", id, id, mac,
                  pcidev_get_slot ());
        arguments_add (args, str);
    }
    else if (!strcmp ("tap", type) && ifname)
    {
        arguments_add (args, "-netdev");
        snprintf (str, sizeof (str), "type=tap,id=net%s,ifname=%s,script=no,downscript=no",
                  id, ifname);
        arguments_add (args, str);
    }
    else
        res = 0;

    free (tmp);

    return res;
}

static char * const *build_args_qemu_new (const s_spawn_dm *dm,
                                          int argc, char * const * argv)
{
    s_arguments args;
    char str[100];
    char * const *arg = NULL;
    int i = 0;

    (void) argc;

    arguments_init (&args);

    arguments_add (&args, QEMU_NEW_PATH);
    arguments_add (&args, "-xen-domid");

    snprintf (str, sizeof (str), "%u", g_info.domid);
    arguments_add (&args, str);

    arguments_add (&args, "-nodefaults");
    arguments_add (&args, "-name");

    snprintf (str, sizeof (str), "%u-%s", g_info.domid, dm->name);
    arguments_add (&args, str);

    arguments_add (&args, "-machine");

    snprintf (str, sizeof (str), "xenfv,xen_dmid=%u,xen_default_dev=off", dm->dmid);
    arguments_add (&args, str);

    arguments_add (&args, "-nographic");

    arg = find_arg (argv, "-m");

    if (arg && arg[1])
    {
        arguments_add (&args, "-m");
        arguments_add (&args, arg[1]);
    }

    for (i = 0; i < argc; i++)
    {
        if (dm_has_cap (dm, DM_CAP_NETWORK))
            i += qemu_new_net (dm, &argv[i], &args);
    }

    return arguments_get (&args);
}

static int is_needed_qemu_old (const s_spawn_dm *dm,
                               int argc, char * const *argv)
{
    (void) dm;
    (void) argc;
    (void) argv;

    return g_info.is_hvm;
}

static int is_needed_qemu (const s_spawn_dm *dm,
                           int argc, char * const *argv)
{
    (void) dm;
    (void) argc;
    (void) argv;

    return 0;
    return g_info.is_hvm && !g_info.in_stubdomain;
}

static int is_needed_surfman (const s_spawn_dm *dm,
                              int argc, char * const *argv)
{
    int i = 0;
    (void) dm;

    /* For pv we use xenfb graphical card */
    if (!g_info.is_hvm)
       return (dm->devtype == DEVICE_TYPE_XENFB);

    for (i = 0; i < argc; i++)
    {
        if (!strcmp ("-xengfx", argv[i]) && dm->devtype == DEVICE_TYPE_XENGFX)
            return 1;
    }

    return 0;
}

static int is_needed_input (const s_spawn_dm *dm,
                            int argc, char * const * argv)
{
    (void) dm;
    (void) argc;
    (void) argv;

    return !g_info.is_hvm;
}

static void dmbus_state_surfman (const s_spawn_dm *dm, e_dmbus_state state)
{
    (void)dm;

    if (state == DMBUS_DISCONNECT && g_info.is_hvm)
        fatal ("Surfman is not able to handle hvm reconnection");
}

static void dmbus_state_input (const s_spawn_dm *dm, e_dmbus_state state)
{
    struct msg_switcher_abs msg;

    switch (state)
    {
    case DMBUS_CONNECT:
    case DMBUS_RECONNECT:
        msg.enabled = 0;
        dmbus_send (dm->serv, DMBUS_MSG_SWITCHER_ABS, &msg, sizeof (msg));
        break;
    default:
        break;
    }
}

/**
 * Some information about the structure:
 *  - We can only support one old QEMU (ie ioemu)
 *  - Device model name must be unique. It's used with xen_watch
 */

static s_spawn_dm dms[] = {
    /* Always spawn qemu-old, it's capabilities is build dynamically */
    SPAWN_QEMU_OLD ("qemu-old", is_needed_qemu_old, build_args_qemu_old),
    SPAWN_QEMU ("qemu-net", is_needed_qemu, build_args_qemu_new, DM_CAP_WIRED),
    SPAWN_QEMU ("qemu-net2", is_needed_qemu, build_args_qemu_new, DM_CAP_WIFI),
    SPAWN_DMBUS ("surfman-xengfx", is_needed_surfman, dmbus_state_surfman,
                 DMBUS_SERVICE_SURFMAN, DEVICE_TYPE_XENGFX),
    SPAWN_DMBUS ("surfman-xenfb", is_needed_surfman, dmbus_state_surfman,
                 DMBUS_SERVICE_SURFMAN, DEVICE_TYPE_XENFB),
    SPAWN_DMBUS ("input-pv", is_needed_input, dmbus_state_input,
                 DMBUS_SERVICE_INPUT, DEVICE_TYPE_INPUT),
    SPAWN_END_OF_LIST (),
};

static void kill_dms ()
{
    const s_spawn_dm *dm;

    FOREACH_DMS (dm)
    {
        switch (dm->type)
        {
        case SPAWN_QEMU_OLD:
        case SPAWN_QEMU:
            if (dm->pid > 0 && kill (dm->pid, SIGTERM) == -1)
                warning ("Unable to kill pid %d", dm->pid);
            else if (dm->pid > 0)
                info ("kill \"%s\"", dm->name);
            break;
        case SPAWN_DMBUS:
            if (dm->serv)
            {
                dmbus_service_disconnect (dm->serv);
                info ("kill \"%s\"", dm->name);
            }
            break;
        default:
            error ("Unknow how to kill type %u", dm->type);
        }
    }
}

static void handle_signal (int signal)
{
    pid_t child;
    int status;
    s_spawn_dm *dm;

    if (g_info.shutting_down)
        return;

    g_info.shutting_down = 1;

    switch (signal)
    {
    case SIGABRT:
        warning ("abort");
        break;
    case SIGCHLD:
        child = wait (&status);
        if (child != -1)
        {
            FOREACH_DMS (dm)
            {
                if (dm->pid != child)
                    continue;

                if (!dm->running)
                    warning ("child \"%s\" died during startup signal", dm->name);
                else
                    warning ("child \"%s\" died", dm->name);
                if (WIFSIGNALED (status))
                    warning ("signal = %d", WTERMSIG(status));

                dm->pid = -1;

                break;
            }
        }
        else
            warning ("unknow child died");
        break;
    case SIGTERM:
        info ("terminated");
        break;
    }

    /**
     * Kill all children. We can't let a child alive cause something
     * goes wrong.
     **/
    kill_dms ();

    if (g_info.xsh)
        xs_close (g_info.xsh);

    if (g_info.dompath)
        free (g_info.dompath);

    if (g_info.in_stubdomain)
        sleep (2); /* Sleep few seconds to be sure that consoled retrieve all logs */
    exit (0);
}

static const char *dm_xs_path (s_spawn_dm *dm, char *path, unsigned int len)
{
    if (dm->type == SPAWN_QEMU_OLD)
        snprintf (path, len, "%s/dms/qemu-old", g_info.dompath);
    else
        snprintf (path, len, "%s/dms/%u/state", g_info.dompath, dm->dmid);

    return path;
}

static void dump_cmdline (s_spawn_dm *dm, char * const *argv)
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
        if (buf && asprintf (&tmp, "%s %s", buf, *argv) == -1)
        {
            free (buf);
            return;
        }
        argv++;
    }

    if (buf)
        info ("%s cmdline: %s", dm->name, buf);

    free (buf);
}

static void spawn_process (s_spawn_dm *dm, int argc, char * const *argv)
{
    char path[100];
    int i = 0;

    info ("fork \"%s\"", dm->name);

    if (dm->type != SPAWN_QEMU_OLD)
        dm->dmid = g_info.num_dm++;

    /* Build command line before fork. Otherwise the bdf will be wrong */
    argv = dm->build_args (dm, argc, argv);

    dm->pid = fork ();
    if (dm->pid == 0)
    {
        if (g_info.xsh)
            xs_close (g_info.xsh);
        if (g_info.dompath)
            free (g_info.dompath);
        info ("exec \"%s\"", dm->name);
        dump_cmdline (dm, argv);
        execv (argv[0], argv);
        fatal ("failed to exec \"%s\" path \"%s\"", dm->name, argv[0]);
    }
    else if (dm->pid == -1)
        fatal ("failed to fork \"%s\" errno = %d\n", dm->name, errno);

    if (!xs_watch (g_info.xsh, dm_xs_path (dm, path, sizeof (path)), dm->name))
        fatal ("failed to watch \"%s\" in xenstore", dm->name);

    for (i = 0; argv[i]; i++)
        free (argv[i]);

    free ((void *)argv);
}

static void spawn_dmbus (s_spawn_dm *dm)
{
    info ("connect to dmbus for \"%s\"", dm->name);
    dm->serv = dmbus_service_connect (dm->service, dm->devtype,
                                      g_info.domid, dm);
    if (!dm->serv)
      fatal ("failed to connect to service \"%s\"", dm->name);
}

static s_spawn_dm *find_dm (const char *name)
{
    s_spawn_dm *dm;

    FOREACH_DMS (dm)
    {
        if (!strcmp (dm->name, name))
            return dm;
    }

    return NULL;
}

static void dm_ready (s_spawn_dm *dm, int reconnect)
{
    char path[100];

    if (!dm->running)
    {
        info ("Dm \"%s\" is ready", dm->name);
        if (!reconnect)
            g_info.num_watch--;
    }

    dm->running = 1;

    if (!g_info.num_watch && !reconnect)
    {
        info ("All device models are now ready");
        snprintf (path, sizeof (path), "%s/device-misc/dm-ready",
                  g_info.dompath);
        if (!xs_write (g_info.xsh, XBT_NULL, path, "1", 1))
            fatal ("Failed to write in xenstore");
    }
}

static void dm_ready_xs (int fd, short ev, void *opaque)
{
    unsigned int count;
    char **vec = xs_read_watch (g_info.xsh, &count);
    s_spawn_dm *dm = NULL;
    char *res;
    char path[100];
    unsigned int len;

    (void) fd;
    (void) ev;
    (void) opaque;

    if (!vec)
        return;

    if (!(dm = find_dm (vec[XS_WATCH_TOKEN])))
    {
        warning ("invalid token %s", vec[XS_WATCH_TOKEN]);
        goto end_dm_ready;
    }

    res = xs_read (g_info.xsh, XBT_NULL, dm_xs_path (dm, path, sizeof (path)),
                   &len);

    if (!res)
        goto end_dm_ready;

    free (res);

    /* Fix: perhaps we should check the value */

    if (!dm->running)
        xs_unwatch (g_info.xsh, dm_xs_path (dm, path, sizeof (path)),
                    dm->name);

    dm_ready (dm, 0);

end_dm_ready:
    free (vec);
}

void dm_ready_dmbus (s_spawn_dm *dm, int reconnect)
{
    dm_ready (dm, reconnect);
}

static void qemu_old_build_cap (int argc, char * const *argv)
{
    s_spawn_dm *qemu_old = NULL;
    s_spawn_dm *dm = dms;
    cap_t cap = 0;

    FOREACH_DMS (dm)
    {
        if (dm->is_needed && !dm->is_needed (dm, argc, argv))
            continue;

        switch (dm->type)
        {
        case SPAWN_QEMU_OLD:
            qemu_old = dm;
            break;
        default:
            cap |= dm->cap;
        }
    }

    /* The old QEMU emulate all devices that the others dms don't handle */
    if (qemu_old)
        qemu_old->cap = ~cap;
}

static void initialize_wrapper (int argc, char * const *argv)
{
    int i = 0;

    /* First pci slot available is 00:0b.0 */
    g_info.pcidev_slot = 0xb;

    g_info.is_hvm = 1;

    for (i = 0; i < argc; i++)
    {
        if (!strcmp ("-M", argv[i]))
        {
            if ((i + 1) < argc && !strcmp ("xenpv", argv[i + 1]))
                g_info.is_hvm = 0;
            i++;
        }
        else if (!strcmp ("-stubdom", argv[i]))
            g_info.in_stubdomain = 1;
    }

    g_info.xsh = xs_open (0);
    if (!g_info.xsh)
        fatal ("Unable to open Xenstore");

    if (!(g_info.dompath = xs_get_domain_path (g_info.xsh, g_info.domid)))
        fatal ("Unable to retrieve domain path");

    qemu_old_build_cap (argc, argv);
}

void write_pid (void)
{
    char path[100];
    char pid[20];

    snprintf (path, sizeof (path), "%s/qemu-pid", g_info.dompath);
    snprintf (pid, sizeof (pid), "%u", getpid ());

    if (!xs_write (g_info.xsh, XBT_NULL, path, pid, strlen (pid)))
        fatal ("Unable to write PID in Xenstore");
}

int main (int argc, char **argv)
{
    s_spawn_dm *dm;

    g_info.domid = atoi (argv[1]);

    argv++;
    argc--;

    signal (SIGABRT, handle_signal);
    signal (SIGCHLD, handle_signal);
    signal (SIGTERM, handle_signal);


#ifdef SYSLOG
    {
        char path[100];

        snprintf (path, sizeof (path), "dm-wrapper-%u", g_info.domid);
        openlog (path, LOG_CONS, LOG_USER);
    }
#endif

    initialize_wrapper (argc, argv);

    if (!g_info.in_stubdomain)
        write_pid ();

    event_init ();

    /* spawn device model */
    FOREACH_DMS (dm)
    {
        if (dm->is_needed && !dm->is_needed (dm, argc, argv))
            continue;
        info ("spawn dm \"%s\"", dm->name);

        switch (dm->type)
        {
        case SPAWN_QEMU_OLD:
        case SPAWN_QEMU:
            spawn_process (dm, argc, argv);
            break;
        case SPAWN_DMBUS:
            spawn_dmbus (dm);
            break;
        default:
            warning ("Invalid type %u for dm \"%s\"", dm->type, dm->name);
        }
        g_info.num_watch++;
    }

    event_set (&g_info.xs_ev, xs_fileno (g_info.xsh), EV_READ | EV_PERSIST,
               dm_ready_xs, dms);
    event_add (&g_info.xs_ev, NULL);

    event_dispatch ();

    fatal ("must never leave");

    return 0;
}
