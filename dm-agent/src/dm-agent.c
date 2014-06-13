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
#include <event.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "device-model.h"
#include "device.h"
#include "dm-agent.h"
#include "domain.h"
#include "spawn.h"
#include "util.h"
#include "xenstore.h"

static s_dm_agent dm_agent;

static void dm_agent_release (s_dm_agent *dm_agent, bool killall)
{
    xenstore_dm_agent_destroy (dm_agent, killall);
}

static bool dm_agent_init (s_dm_agent *dm_agent)
{
    char *sdomid = xenstore_read ("domid");
    char *buff = NULL;

    if (sdomid)
        dm_agent->domid = atoi (sdomid);
    free (sdomid);

    /* Set the right prefix */
    if (asprintf (&buff, "dm-agent-%u" , dm_agent->domid) == -1)
        return false;

    prefix_set (buff);
    free (buff);

    if (!xenstore_dm_agent_create (dm_agent))
        return false;

    return true;
}

const char *dm_agent_get_path (void)
{
    return dm_agent.dmapath;
}

struct event_base *dm_agent_get_ev_base (void)
{
    return dm_agent.ev_base;
}

domid_t dm_agent_get_target (void)
{
    return dm_agent.target_domid;
}

void dm_agent_destroy (bool killall)
{
    domains_destroy (killall);
    dm_agent_release (&dm_agent, killall);
    xenstore_release ();
    if (dm_agent.ev_base)
        event_base_free (dm_agent.ev_base);
}

static void signal_handler (int sig)
{
    (void) sig;

    info ("signal = %d", sig);

    // Ugly hack to remove capabilities directory
    if (dm_agent.dmapath)
        xenstore_rm ("%s/%s", dm_agent.dmapath, "capabilities");

    dm_agent_destroy (false);

    // if dm-agent is in a stubdomain, be sure that all logs are retrieved by xenconsoled
    if (dm_agent_in_stubdom ())
        sleep (1);

    exit (0);
}

bool dm_agent_in_stubdom (void)
{
    return dm_agent.domid != 0;
}

static void usage (const char *progname)
{
    fprintf (stderr, "Usage: %s [-n] [-h|--help]\n"
             "\t-h,--help   Show this help screen\n"
             "\t-n          Do not deamonize\n"
             "\t-t domid    Dm-agent is handle request only for domid\n",
             progname);
    exit (1);
}

int main (int argc, char **argv)
{
    int c;
    int dont_detach = 0;
    /* By default use ioemu */
    int enable_qemu = false;

    while ((c = getopt (argc, argv, "nqt:h|help")) != -1)
    {
        switch (c)
        {
        case 'n':
            dont_detach++;
            break;
        case 't':
            dm_agent.target_domid = atoi (optarg);
            break;
        case 'q':
            enable_qemu = true;
            break;
        default:
            usage(argv[0]);
        }
    }

    /* Dummy prefix, the right prefix will be set during dm-agent initialization */
    prefix_set ("dm-agent-init");

    if (!dont_detach)
    {
        info ("Daemon");
        if (daemon (0, 0))
            fatal ("daemon(0, 0) failed: %s", strerror (errno));
        logging (LOG_SYSTEM);
    }
    else
        logging (LOG_STDERR);

    signal (SIGTERM, signal_handler);
    signal (SIGINT, signal_handler);

    lockfile_lock ();

    /* Quickly fix to try easily qemu upstream */
    devices_group_disable (DEVMODEL_SPAWN, SPAWN_QEMU_OLD, enable_qemu);
    devices_group_disable (DEVMODEL_SPAWN, SPAWN_QEMU, !enable_qemu);

    dm_agent.ev_base = event_init ();

    if (!dm_agent.ev_base)
        fatal ("Unable to initialize libevent");

    if (xenstore_init ())
        fatal ("Unable to initialize xenstore");

    if (!dm_agent_init (&dm_agent))
        fatal ("Unable to initialize dm-agent");

    info ("dm-agent running in domain %u", dm_agent.domid);
    if (dm_agent.target_domid)
        info ("dm-agent only handles request for domain %u",
              dm_agent.target_domid);

    info ("Dispatching events (event lib v%s. Method %s)",
          event_get_version (), event_get_method ());

    device_models_init ();

    if (!domains_create ())
        fatal ("Unable to check xenstore dms path");

    event_dispatch ();

    return 0;
}
