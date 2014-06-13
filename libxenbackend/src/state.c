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

#include "project.h"
#include "backend.h"

INTERNAL void
backend_changed(struct xen_device *xendev, const char *node)
{
    struct xen_backend *xenback = xendev->backend;

    if (node == NULL || !strcmp(node, "online")) {
        if (xs_read_be_int(xendev, "online", &xendev->online))
            xendev->online = 0;
    }

    if (node) {
        char *val = xs_read_be_str(xendev, node);

        if (xenback->ops->backend_changed)
            xenback->ops->backend_changed(xendev->dev, node, val);
        free(val);
    }
}

INTERNAL void
frontend_changed(struct xen_device *xendev, const char *node)
{
    struct xen_backend *xenback = xendev->backend;

    if (node == NULL || !strcmp(node, "state")) {
        if (xs_read_fe_int(xendev, "state", (int *)&xendev->fe_state))
            xendev->fe_state = XenbusStateUnknown;
    }

    if (node == NULL || !strcmp(node, "protocol")) {
        if (xendev->protocol)
            free(xendev->protocol);

        xendev->protocol = xs_read_fe_str(xendev, "protocol");
    }

    if (node) {
        char *val = xs_read_fe_str(xendev, node);

        if (xenback->ops->frontend_changed)
            xenback->ops->frontend_changed(xendev->dev, node, val);
        free(val);
    }
}

static int set_state(struct xen_device *xendev, enum xenbus_state state)
{
    int rc;

    rc = xs_write_be_int(xendev, "state", state);
    if (rc < 0)
	return rc;
    xendev->be_state = state;
    return 0;
}


static int try_setup(struct xen_device *xendev)
{
    int be_state;
    char token[TOKEN_BUFSZ];
    int rc;

    rc = xs_read_be_int(xendev, "state", &be_state);
    if (rc == -1)
        return -1;

    if (be_state != XenbusStateInitialising)
        return -1;

    xendev->fe = xs_read_be_str(xendev, "frontend");
    if (xendev->fe == NULL)
        return -1;

    rc = snprintf(token, TOKEN_BUFSZ, MAGIC_STRING"%p", xendev);
    if (rc < 0 || rc >= PATH_BUFSZ)
        return -1;

    if (!xs_watch(xs_handle, xendev->fe, token))
        return -1;

    set_state(xendev, XenbusStateInitialising);

    backend_changed(xendev, NULL);
    frontend_changed(xendev, NULL);

    return 0;
}

static int try_init(struct xen_device *xendev)
{
    struct xen_backend *xenback = xendev->backend;
    int rc = 0;

    if (!xendev->online)
        return -1;

    if (xenback->ops->init) {
        rc = xenback->ops->init(xendev->dev);
        if (rc)
            return rc;
    }

    xs_write_be_str(xendev, "hotplug-status", "connected");

    set_state(xendev, XenbusStateInitWait);
    return 0;
}

static int try_connect(struct xen_device *xendev)
{
    struct xen_backend *xenback = xendev->backend;
    int rc = 0;

    if (xendev->fe_state != XenbusStateInitialised &&
        xendev->fe_state != XenbusStateConnected) {
        return -1;
    }

    if (xenback->ops->connect) {
        rc = xenback->ops->connect(xendev->dev);
        if (rc)
            return rc;
    }

    set_state(xendev, XenbusStateConnected);
    return 0;
}

static int try_reset(struct xen_device *xendev)
{
    if (xendev->fe_state != XenbusStateInitialising)
        return -1;

    set_state(xendev, XenbusStateInitialising);
    return 0;
}

static void disconnect(struct xen_device *xendev, enum xenbus_state state)
{
    struct xen_backend *xenback = xendev->backend;

    if (xendev->be_state != XenbusStateClosing &&
	xendev->be_state != XenbusStateClosed) {

        if (xenback->ops->disconnect)
            xenback->ops->disconnect(xendev->dev);
    }

    if (xendev->be_state != state)
        set_state(xendev, state);

}

INTERNAL void
check_state(struct xen_device *xendev)
{
    int rc = 0;

    if (xendev->fe_state == XenbusStateClosing ||
	xendev->fe_state == XenbusStateClosed) {
	disconnect(xendev, xendev->fe_state);
	return;
    }

    for (;;) {
	switch (xendev->be_state) {
	case XenbusStateUnknown:
	    rc = try_setup(xendev);
	    break;
	case XenbusStateInitialising:
	    rc = try_init(xendev);
	    break;
	case XenbusStateInitWait:
	    rc = try_connect(xendev);
	    break;
        case XenbusStateClosed:
            rc = try_reset(xendev);
            break;
	default:
	    rc = -1;
	}
	if (0 != rc)
	    break;
    }

}

INTERNAL int
check_state_early(struct xen_device *xendev)
{
    int rc;
    int be_state;

    rc = xs_read_be_int(xendev, "state", &be_state);
    if (rc == -1)
        return -1;

    if (be_state == XenbusStateConnected) {
        set_state(xendev, XenbusStateInitialising);
        xendev->be_state = XenbusStateUnknown;
    }

    return rc;
}
