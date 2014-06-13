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
#include <libv4v.h>
#include <string.h>

#include "dmbus.h"
#include "util.h"

/* Time to wait between 2 reconnection in ms */
#define DMBUS_TIMEOUT 1000

struct service
{
    int fd;
    v4v_addr_t peer;
    struct dmbus_conn_prologue prologue;

    char buff[DMBUS_MAX_MSG_LEN];
    uint32_t len;
    struct event ev;
    struct event ev_reconnect;
    struct device_model *devmodel;
    int first_connection;
    int service;
    DeviceType devtype;
    dmbus_state_cb_t state;
};

static void handle_message (struct service *s, union dmbus_msg *m)
{
    enum dmbus_state state;

    (void) s;

    devmodel_info (s->devmodel, "handle message %d %s",
                   m->hdr.msg_type, (m->hdr.msg_type == DMBUS_MSG_DEVICE_MODEL_READY)
                    ? "ready" : "");

    switch (m->hdr.msg_type)
    {
    case DMBUS_MSG_DEVICE_MODEL_READY:
        state = s->first_connection ? DMBUS_CONNECT : DMBUS_RECONNECT;
        s->state (s->devmodel, state);
        s->first_connection = 0;
        break;
    default:
        devmodel_warning (s->devmodel, "unrecognized message ID: %d",
                          m->hdr.msg_type);
    }
}

static void pop_message (struct service *s)
{
    union dmbus_msg *m = (union dmbus_msg *)s->buff;
    uint32_t len = m->hdr.msg_len;

    if ((s->len < sizeof (struct dmbus_msg_hdr)) || (s->len < len))
        return;

    memmove(s->buff,  s->buff + len,  s->len - len);
    s->len -= len;
}

static void handle_disconnect (struct service *s)
{
    struct timeval time;

    event_del (&s->ev);
    v4v_close (s->fd);
    s->fd = -1;

    devmodel_warning (s->devmodel, "remote service disconnected");

    s->state (s->devmodel, DMBUS_DISCONNECT);
    /**
     * FIXME: hack for xengfx device when surfman died. Don't try to
     * reconnect.
     */
    if (s->devmodel->status == DEVMODEL_DIED)
        return;

    devmodel_info (s->devmodel, "try to reconnect in %u ms", DMBUS_TIMEOUT);
    time.tv_sec = DMBUS_TIMEOUT / 1000;
    time.tv_usec = (DMBUS_TIMEOUT % 1000) * 1000;
    event_add (&s->ev_reconnect, &time);
}

static union dmbus_msg *sync_recv (struct service *s)
{
    int rc;
    union dmbus_msg *m = (union dmbus_msg *)s->buff;

    while ((s->len < sizeof (struct dmbus_msg_hdr))
           || (s->len < m->hdr.msg_len))
    {
        rc = v4v_recv (s->fd, s->buff + s->len, sizeof (s->buff) - s->len, 0);
        switch (rc) {
        case 0:
            handle_disconnect (s);
            return NULL;
        case -1:
            if (errno == EINTR)
                continue;
            devmodel_error (s->devmodel, "recv error: %s", strerror (errno));
            return NULL;
        default:
            s->len += rc;
        }
    }

    return m;
}

static void dmbus_fd_handler (int fd, short ev, void *opaque)
{
    int rc;
    struct service *s = opaque;
    union dmbus_msg *m = (union dmbus_msg *)s->buff;

    (void) fd;
    (void) ev;

    do {
        rc = v4v_recv (s->fd, s->buff + s->len, sizeof (s->buff) - s->len,
                       MSG_DONTWAIT);
        switch (rc) {
        case 0:
            handle_disconnect (s);
            return;
        case -1:
            if (errno == EINTR)
                continue;
            devmodel_error (s->devmodel, "recv error: %s", strerror (errno));
            return;
        default:
            s->len += rc;
        }
    } while (rc <= 0);

    m = sync_recv (s);
    if (!m)
        return;

    while ((s->len >= sizeof (struct dmbus_msg_hdr))
           && (s->len >= m->hdr.msg_len))
    {
        handle_message (s, m);
        pop_message (s);
    }
}

static bool dmbus_connect (struct service *s)
{
    int rc = 0;

    s->fd = v4v_socket (SOCK_STREAM);
    if (s->fd == -1)
    {
        devmodel_error (s->devmodel, "failed to create v4v socket: %s ",
                        strerror (errno));
        return false;
    }
    rc = v4v_connect (s->fd, &s->peer);
    if (rc == -1)
    {
        devmodel_error (s->devmodel, "failed to connect v4v socket: %s",
                        strerror (errno));
        v4v_close (s->fd);
        return false;
    }

    rc = v4v_send (s->fd, &s->prologue, sizeof (s->prologue), 0);
    if (rc != sizeof (s->prologue))
    {
        devmodel_error (s->devmodel, "failed to initialize dmbus connection: %s",
                        strerror (errno));
        v4v_close (s->fd);
        return false;
    }

    devmodel_info (s->devmodel, "Connected");

    return true;
}

static void dmbus_reconnect (int fd, short ev, void *opaque)
{
    struct timeval time;
    struct service *s = opaque;

    (void) fd;
    (void) ev;

    devmodel_info (s->devmodel, "try to reconnect to service");

    if (!dmbus_connect (s))
        goto rearm;

    event_set (&s->ev, s->fd, EV_PERSIST | EV_READ, dmbus_fd_handler, s);
    event_add (&s->ev, NULL);

    return;
rearm:
    s->fd = -1;
    warning ("Failed. Try to reconnect in %u ms", DMBUS_TIMEOUT);
    time.tv_sec = DMBUS_TIMEOUT / 1000;
    time.tv_usec = (DMBUS_TIMEOUT % 1000) * 1000;
    event_add (&s->ev_reconnect, &time);
}

static void fill_hash (uint8_t *h)
{
    const char *hash_str = DMBUS_SHA1_STRING;
    size_t i;

    for (i = 0; i < 20; i++) {
        unsigned int c;

        sscanf (hash_str + 2 * i, "%02x", &c);
        h[i] = c;
    }
}

int dmbus_send (struct device_model *devmodel, uint32_t msgtype,
                void *data, size_t len)
{
    struct service *s = devmodel->type_priv;
    struct dmbus_msg_hdr *hdr = data;
    int rc;
    size_t b = 0;

    hdr->msg_type = msgtype;
    hdr->msg_len = len;

    while (b < len)
    {
        rc = v4v_send (s->fd, data + b, len - b, 0);
        if (rc == -1)
        {
            if (errno == ECONNRESET)
                handle_disconnect(s);
            else
                devmodel_error (devmodel, "failed: %s", strerror (errno));
            return -1;
        }

        b += rc;
    }

    return b;
}

static bool dmbus_int_init (void)
{
    return true;
}

static bool dmbus_create (struct device_model *devmodel, unsigned int type)
{
    struct service *s;

    (void) type;

    s = calloc (1, sizeof (*s));
    if (!s)
        return NULL;

    devmodel->type_priv = s;
    s->first_connection = 1;
    s->fd = -1;
    s->devmodel = devmodel;

    event_set (&s->ev_reconnect, -1, EV_TIMEOUT, dmbus_reconnect, s);

    return true;
}

bool dmbus_set_device (struct device_model *devmodel, int service,
                       DeviceType devtype, dmbus_state_cb_t state)
{
    struct service *s = devmodel->type_priv;

    /* Dmbus is able to handle only one device per connexion */
    if (s->peer.port)
        return false;
    if (!state)
        return false;

    s->service = service;
    s->devtype = devtype;
    s->state = state;

    s->peer.port = DMBUS_BASE_PORT + service;
    /* TODO: change to handle service in other domain than dom0 */
    s->peer.domain = 0; /* Dom 0 */
    s->prologue.domain = devmodel->domain->domid;
    s->prologue.type = devtype;
    fill_hash (s->prologue.hash);

    return true;
}

static bool dmbus_start (struct device_model *devmodel)
{
    struct service *s = devmodel->type_priv;
    struct timeval time;

    if (!dmbus_connect (s))
    {
        info ("Try to reconnect in %u ms", DMBUS_TIMEOUT);
        time.tv_sec = DMBUS_TIMEOUT / 1000;
        time.tv_usec = (DMBUS_TIMEOUT % 1000) * 1000;
        event_add (&s->ev_reconnect, &time);
    }
    else
    {
        event_set (&s->ev, s->fd, EV_READ | EV_PERSIST, dmbus_fd_handler, s);
        event_add (&s->ev, NULL);
    }

    return true;
}

static void dmbus_kill (struct device_model *devmodel)
{
    struct service *s = devmodel->type_priv;

    event_del (&s->ev_reconnect);

    if (s->fd != -1)
    {
        event_del (&s->ev);
        v4v_close (s->fd);
    }

    device_model_status (devmodel, DEVMODEL_DIED);
}

static void dmbus_destroy (struct device_model *devmodel, bool killall)
{
    struct service *s = devmodel->type_priv;

    (void) killall;

    if (devmodel->status != DEVMODEL_DIED)
    {
        event_del (&s->ev_reconnect);

        if (s->fd != -1)
        {
            event_del (&s->ev);
            v4v_close (s->fd);
        }
    }

    free (s);
}

static struct device_model_ops devmodel_ops =
{
    .type = DEVMODEL_DMBUS,
    .init = dmbus_int_init,
    .create = dmbus_create,
    .start = dmbus_start,
    .kill = dmbus_kill,
    .destroy = dmbus_destroy,
};

devmodel_init (devmodel_ops)
