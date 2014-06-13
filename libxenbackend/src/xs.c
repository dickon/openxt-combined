/*
 * Copyright (c) 2011 Citrix Systems, Inc.
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

#include <stdarg.h>

#include "project.h"
#include "backend.h"


INTERNAL int
xs_write_str(const char *base, const char *node, const char *val)
{
    char abspath[PATH_BUFSZ];

    snprintf(abspath, sizeof(abspath), "%s/%s", base, node);
    if (!xs_write(xs_handle, 0, abspath, val, strlen(val)))
	return -1;
    return 0;
}

INTERNAL char *
xs_read_str(const char *base, const char *node)
{
    char abspath[PATH_BUFSZ];
    unsigned int len;

    snprintf(abspath, sizeof(abspath), "%s/%s", base, node);
    return xs_read(xs_handle, 0, abspath, &len);
}

INTERNAL int
xs_write_int(const char *base, const char *node, int ival)
{
    char val[32];

    snprintf(val, sizeof(val), "%d", ival);
    return xs_write_str(base, node, val);
}

INTERNAL int
xs_read_int(const char *base, const char *node, int *ival)
{
    char *val;
    int rc = -1;

    val = xs_read_str(base, node);
    if (val && 1 == sscanf(val, "%d", ival))
	rc = 0;
    free(val);
    return rc;
}

INTERNAL int
xs_write_be_str(struct xen_device *xendev, const char *node, const char *val)
{
    return xs_write_str(xendev->be, node, val);
}

INTERNAL int
xs_write_be_int(struct xen_device *xendev, const char *node, int ival)
{
    return xs_write_int(xendev->be, node, ival);
}

INTERNAL char *
xs_read_be_str(struct xen_device *xendev, const char *node)
{
    return xs_read_str(xendev->be, node);
}

INTERNAL int
xs_read_be_int(struct xen_device *xendev, const char *node, int *ival)
{
    return xs_read_int(xendev->be, node, ival);
}

INTERNAL char *
xs_read_fe_str(struct xen_device *xendev, const char *node)
{
    return xs_read_str(xendev->fe, node);
}

INTERNAL int
xs_read_fe_int(struct xen_device *xendev, const char *node, int *ival)
{
    return xs_read_int(xendev->fe, node, ival);
}

EXTERNAL int
backend_print(xen_backend_t xenback, int devid, const char *node,
              const char *fmt, ...)
{
    char buff[1024];
    struct xen_device *xendev = &xenback->devices[devid];
    va_list ap;
    int rc;

    va_start(ap, fmt);
    rc = vsnprintf(buff, 1024, fmt, ap);
    va_end(ap);

    if (rc >= 1024)
        return 0;

    if (xs_write_be_str(xendev, node, buff))
        return 0;

    return rc;
}

EXTERNAL int
backend_scan(xen_backend_t xenback, int devid, const char *node,
             const char *fmt, ...)
{
    struct xen_device *xendev = &xenback->devices[devid];
    va_list ap;
    int rc;
    char *buff;

    buff = xs_read_be_str(xendev, node);
    if (!buff)
        return EOF;

    va_start(ap, fmt);
    rc = vsscanf(buff, fmt, ap);
    va_end(ap);

    free(buff);

    return rc;
}

EXTERNAL int
frontend_scan(xen_backend_t xenback, int devid, const char *node,
              const char *fmt, ...)
{
    struct xen_device *xendev = &xenback->devices[devid];
    va_list ap;
    int rc;
    char *buff;

    buff = xs_read_fe_str(xendev, node);
    if (!buff)
        return EOF;

    va_start(ap, fmt);
    rc = vsscanf(buff, fmt, ap);
    va_end(ap);

    free(buff);

    return rc;
}

