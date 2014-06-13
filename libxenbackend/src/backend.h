/*
 * Copyright (c) 2012 Citrix Systems, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __BACKEND_H__
#define __BACKEND_H__

#define PATH_BUFSZ 1024
#define TOKEN_BUFSZ 64

#define BACKEND_DEVICE_MAX 16

#define MAGIC_STRING "libxenbackend:"

struct xen_device
{
    xen_device_t		dev;

    enum xenbus_state           be_state;
    enum xenbus_state           fe_state;

    char                        *be;
    char                        *fe;

    char                        *protocol;

    struct xen_backend          *backend;

    int                         online;

    xc_evtchn                   *evtchndev;
    int                         local_port;
};

struct xen_backend
{
    struct xen_backend_ops      *ops;
    int                         domid;
    const char                  *type;

    backend_private_t           priv;

    char                        path[PATH_BUFSZ];
    int                         path_len;
    char                        token[TOKEN_BUFSZ];

    struct xen_device           devices[BACKEND_DEVICE_MAX];
};

extern struct xs_handle *xs_handle;

#endif /* __BACKEND_H__ */
