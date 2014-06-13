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

/* Small application to discuss with dm-agent */
/* /!\ To be used only for testing */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xenctrl.h>
#include <xs.h>

static char *dmapath = NULL;
static struct xs_handle *xs;
static xs_transaction_t xs_trans = XBT_NULL;
bool is_running = true;
unsigned int dmaid = 0;

typedef unsigned int dmid_t;

# define MAX_DOMAINS 256

static void usage (const char *progname)
{
    xc_interface *xch = xc_interface_open (NULL, NULL, 0);
    xc_dominfo_t doms[MAX_DOMAINS];
    int n, i;
    char *dompath = NULL;
    char *val = NULL;
    char *path = NULL;
    int res;

    fprintf (stderr, "Usage: %s domid\n"
             "domid: target DM-agent\n",
             progname);

    if (!xch)
        goto eusage;

    if ((n = xc_domain_getinfo (xch, 0, MAX_DOMAINS, doms)) == -1)
    {
        goto eusage;
    }

    fprintf (stderr, "list of available dm-agent\n");

    for (i = 0; i < n; i++)
    {
        if (!(dompath = xs_get_domain_path (xs, doms[i].domid)))
            continue;

        res = asprintf (&path, "%s/dm-agent", dompath);
        free (dompath);

        if (res == -1)
            continue;

        val = xs_read (xs, XBT_NULL, path, NULL);
        free (path);

        if (!val)
            continue;

        fprintf (stderr, "\t- %u\n", doms[i].domid);
    }

eusage:
    if (xch)
        xc_interface_close (xch);

    xs_close (xs);

    exit (1);
}

static char *get_path (const char *path)
{
    char *buff = NULL;

    if (asprintf (&buff, "%s/%s", dmapath, path) == -1)
        return NULL;

    return buff;
}

static bool xenstore_mkdir (char *cmdline)
{
    char *dir = NULL;
    char *path = NULL;
    bool res;

    dir = strtok (cmdline, " ");

    if (!dir)
    {
        fprintf (stderr, "Help: mkdir dir\n");
        return false;
    }

    if (!(path = get_path (dir)))
        return false;

    res = xs_mkdir (xs, xs_trans, path);
    if (!res)
        fprintf (stderr, "Unable to create directory '%s'\n", dir);

    return res;
}

static bool xenstore_rm (char *cmdline)
{
    char *key = NULL;
    char *path = NULL;
    bool res;

    key = strtok (cmdline, " ");

    if (!key)
    {
        fprintf (stderr, "Help: rm key\n");
        return false;
    }

    if (!(path = get_path (key)))
        return false;

    res = xs_rm (xs, xs_trans, path);
    if (!res)
        fprintf (stderr, "Unable to remove '%s'\n", key);

    free (path);

    return res;
}

static bool xenstore_ls (char *cmdline)
{
    char *dir = NULL;
    char **list = NULL;
    char *path;
    unsigned int num;
    unsigned int i;

    dir = strtok (cmdline, " ");

    if (!dir)
        path = dmapath;
    else if (!(path = get_path (dir)))
        return false;

    list = xs_directory (xs, xs_trans, path, &num);

    if (dir)
        free (path);

    if (!list)
    {
        fprintf (stderr, "Unable to list directory '%s'\n", dir);
        return false;
    }

    for (i = 0; i < num; i++)
        printf ("%s\n", list[i]);

    free (list);
    return true;
}

static bool xenstore_transaction (char *cmdline)
{
    char *state = NULL;

    state = strtok (cmdline, " ");

    if (!state)
        goto thelp;

    if (!strcmp (state, "begin"))
    {
        if (xs_trans != XBT_NULL)
        {
            fprintf (stderr, "transaction in progress\n"
                     "please finish it before starting a new one\n");
            return false;
        }

        xs_trans = xs_transaction_start (xs);
        if (xs_trans == XBT_NULL)
        {
            fprintf (stderr, "Unable to create a new transaction\n");
            return false;
        }

    }
    else if (!strcmp (state, "commit"))
    {
        if (!xs_transaction_end (xs, xs_trans, 0))
            fprintf (stderr, "Unable to commit the transaction: %s\n", strerror (errno));
        xs_trans = XBT_NULL;
    }
    else if (!strcmp (state, "abort"))
    {
        if (!xs_transaction_end (xs, xs_trans, 1))
            fprintf (stderr, "Unable to abort the transaction\n");
        xs_trans = XBT_NULL;
    }
    else
        goto thelp;

    return true;

thelp:
    fprintf (stderr, "Help: transaction begin|commit|abort\n");
    return false;
}

static bool xenstore_write (char *cmdline)
{
    char *key = NULL;
    char *value = NULL;
    char *path = NULL;
    bool res;

    key = strtok (cmdline, " ");
    value = strtok (NULL, "");

    if (!key || !value)
    {
        fprintf (stderr, "Help: write key value\n");
        return false;
    }

    if (!(path = get_path (key)))
        return false;

    res = xs_write (xs, xs_trans, path, value, strlen (value));
    if (!res)
        fprintf (stderr, "Unable to write in '%s'\n", key);

    free (path);
    return res;
}

static bool xenstore_touch (char *cmdline)
{
    char *key = NULL;
    char *path = NULL;
    bool res;

    key = strtok (cmdline, " ");

    if (!key)
    {
        fprintf (stderr, "Help: touch key value\n");
        return false;
    }

    if (!(path = get_path (key)))
        return false;

    res = xs_write (xs, xs_trans, path, "1", 1);

    if (!res)
        fprintf (stderr, "Unable to write in '%s'\n", key);

    free (path);

    return res;
}

static bool xenstore_read (char *cmdline)
{
    char *key = NULL;
    char *value = NULL;
    char *path = NULL;

    key = strtok (cmdline, " ");

    if (!key)
    {
        fprintf (stderr, "Help: read key\n");
        return false;
    }

    if (!(path = get_path (key)))
        return false;

    value = xs_read (xs, xs_trans, path, NULL);
    free (path);

    if (!value)
    {
        fprintf (stderr, "Unable to read '%s'\n", key);
        return false;
    }

    printf ("%s = %s\n", key, value);

    free (value);

    return true;
}

static bool xenstore_exist (char *key)
{
    char *value = NULL;
    char *path = NULL;
    bool res;

    if (!(path = get_path (key)))
        return false;

    value = xs_read (xs, xs_trans, path, NULL);
    free (path);

    res = (value != NULL);
    free (value);

    return res;
}

static bool domain_exist (domid_t domid)
{
    char *path = NULL;
    bool res = false;

    if (asprintf (&path, "dms/%u", domid) == -1)
        return false;

    res = xenstore_exist (path);
    free (path);

    return res;
}

static bool domain_create (char *cmdline)
{
    char *sdom = strtok (cmdline, " ");
    domid_t domid;
    char *path = NULL;
    bool res = false;

    if (!sdom || !(domid = atoi (sdom)))
    {
        fprintf (stderr, "Help: domcreate domid\n");
        return false;
    }

    if (domain_exist (domid))
    {
        fprintf (stderr, "Domain %u already exists\n", domid);
        return false;
    }

    if (asprintf (&path, "dms/%u", domid) == -1)
        return false;

    res = xenstore_mkdir (path);
    free (path);

    if (res)
        printf ("Domain %u created\n", domid);

    return res;
}

static bool domain_destroy (char *cmdline)
{
    char *sdom = strtok (cmdline, " ");
    domid_t domid;
    char *path = NULL;
    bool res = false;

    if (!sdom || !(domid = atoi (sdom)))
    {
        fprintf (stderr, "Help: domdestroy domid\n");
        return false;
    }

    if (!domain_exist (domid))
    {
        fprintf (stderr, "Unknown domain %u\n", domid);
        return false;
    }

    if (asprintf (&path, "dms/%u/destroy", domid) == -1)
        return false;

    res = xenstore_touch (path);

    if (res)
        printf ("Domain %u destroyed\n", domid);

    return res;
}

static bool devmodel_exist (domid_t domid, dmid_t dmid)
{
    char *path = NULL;
    bool res = false;

    if (asprintf (&path, "dms/%u/%u", domid, dmid) == -1)
        return false;

    res = xenstore_exist (path);
    free (path);

    return res;
}

static bool devmodel_create (char *cmdline)
{
    char *sdom = strtok (cmdline, " ");
    char *sdm = strtok (NULL, " ");
    char *cmd = NULL;
    domid_t domid;
    dmid_t dmid;
    bool res;

    if (!sdom || !sdm || !(domid = atoi (sdom)))
    {
        fprintf (stderr, "Help: dmcreate domid dmid\n");
        return false;
    }

    dmid = atoi (sdm);

    if (!domain_exist (domid))
    {
        printf ("Unknown domain %u\n", domid);
        return false;
    }

    if (devmodel_exist (domid, dmid))
    {
        fprintf (stderr, "Devmodel %u already exists for domain %u\n",
                 dmid, domid);
        return false;
    }

    if (xs_trans != XBT_NULL)
    {
        fprintf (stderr, "transaction in progress\n"
                 "please finish it before creating a new device model\n");
    }

devmodel_trans:
    xs_trans = xs_transaction_start (xs);
    if (xs_trans == XBT_NULL)
    {
        fprintf (stderr, "Unable to create a new transaction\n");
        return false;
    }

    if (asprintf (&cmd, "dms/%u/%u", domid, dmid) == -1)
        goto devmodel_abort;

    /* Create the device model */
    res = xenstore_mkdir (cmd);
    free (cmd);

    if (!res)
        goto devmodel_abort;

    /* Add a dummy device for test */
    if (asprintf (&cmd, "dms/%u/%u/infos/test%u", domid, dmid, dmid) == -1)
        goto devmodel_abort;
    res = xenstore_mkdir (cmd);
    free (cmd);

    if (!res)
        goto devmodel_abort;

    if (!xs_transaction_end (xs, xs_trans, 0))
    {
        xs_trans = XBT_NULL;
        if (errno == EAGAIN)
        {
            fprintf (stderr, "Retry transaction ...\n");
            goto devmodel_trans;
        }
        else
        {
            fprintf (stderr, "Unable to commit the transaction");
            return false;
        }
    }

    xs_trans = XBT_NULL;

    printf ("Devmodel %u created for domain %u\n", dmid, domid);

    return true;

devmodel_abort:
    fprintf (stderr, "An error occured, devmodel creation aborted\n");
    xs_transaction_end (xs, xs_trans, 1);
    xs_trans = XBT_NULL;
    return false;
}

static bool dmagent_prepare (char *cmdline)
{
    bool res = false;
    char *dir = dmapath;
    struct xs_permissions perms;

    (void) cmdline;

    res = xs_mkdir (xs, xs_trans, dir);

    if (!res)
    {
        fprintf (stderr, "Unable to create directory '%s'\n", dir);
        return false;
    }

    // Set permission on directory
    perms.id = dmaid;
    perms.perms = XS_PERM_READ | XS_PERM_WRITE;

    res = xs_set_permissions (xs, XBT_NULL, dir, &perms, 1);

    if (!res)
        fprintf (stderr, "Unable to set permissions on directory '%s'\n", dir);

    return res;
}

static bool quit (char *cmdline)
{
    (void) cmdline;
    is_running = false;

    return true;
}

static const struct action
{
    const char *action;
    bool (*cb) (char *cmdline);
} actions[] = {
    { "read", xenstore_read },
    { "write", xenstore_write },
    { "ls", xenstore_ls },
    { "rm", xenstore_rm },
    { "touch", xenstore_touch },
    { "mkdir", xenstore_mkdir },
    { "transaction", xenstore_transaction },
    { "domcreate", domain_create },
    { "domdestroy", domain_destroy },
    { "dmcreate", devmodel_create },
    { "prepare", dmagent_prepare },
    { "quit", quit },
    { NULL, NULL },
};

static void help (void)
{
    const struct action *action = actions;

    fprintf (stderr, "Help: action [args]\n"
            "possible actions: \n");

    while (action->action)
    {
        fprintf (stderr, "\t- %s\n", action->action);
        action++;
    }
}

static void execute (char *line)
{
    const struct action *action = actions;
    char *name = strtok (line, " \n");
    char *cmdline = strtok (NULL, "\n");

    if (!name || *name == '\0')
        return;

    while (action->action)
    {
        if (!strcmp (name, action->action))
            break;
        action++;
    }

    if (!action->action && !action->cb)
    {
        fprintf (stderr, "Unknown action: %s\n", name);
        help ();
        return;
    }

    action->cb (cmdline);
}

static void prompt (void)
{
    if (!isatty (STDIN_FILENO))
        return;
    printf ("> ");
    fflush (stdout);
}

int main (int argc, char **argv)
{
    char *line = NULL;
    size_t len = 0;

    if (!(xs = xs_open (0)))
    {
        fprintf (stderr, "Unable to open xenstore");
        return 1;
    }

    if (argc != 2)
        usage (argv[0]);

    dmaid = atoi (argv[1]);

    printf ("Open talk with DM-agent %u\n", dmaid);

    if (!(line = xs_get_domain_path (xs, dmaid)))
    {
        fprintf (stderr, "Unable to retrieve domain path for %u\n", dmaid);
        goto fxs_close;
    }

    if (asprintf (&dmapath, "%s/dm-agent", line) == -1)
        goto fdompath;

    free (line);
    line = NULL;

    prompt ();

    while (is_running && getline (&line, &len, stdin) != -1)
    {
        execute (line);
        if (is_running)
            prompt ();
    }

    free (line);
    free (dmapath);
    xs_close (xs);
    printf ("\n");

    return 0;

fdompath:
    free (line);
fxs_close:
    xs_close (xs);
    return 1;
}
