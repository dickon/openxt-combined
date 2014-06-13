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

/* xs.c */
int xs_write_str(const char *base, const char *node, const char *val);
char *xs_read_str(const char *base, const char *node);
int xs_write_int(const char *base, const char *node, int ival);
int xs_read_int(const char *base, const char *node, int *ival);
int xs_write_be_str(struct xen_device *xendev, const char *node, const char *val);
int xs_write_be_int(struct xen_device *xendev, const char *node, int ival);
char *xs_read_be_str(struct xen_device *xendev, const char *node);
int xs_read_be_int(struct xen_device *xendev, const char *node, int *ival);
char *xs_read_fe_str(struct xen_device *xendev, const char *node);
int xs_read_fe_int(struct xen_device *xendev, const char *node, int *ival);
int backend_print(xen_backend_t xenback, int devid, const char *node, const char *fmt, ...);
int backend_scan(xen_backend_t xenback, int devid, const char *node, const char *fmt, ...);
int frontend_scan(xen_backend_t xenback, int devid, const char *node, const char *fmt, ...);
/* state.c */
void backend_changed(struct xen_device *xendev, const char *node);
void frontend_changed(struct xen_device *xendev, const char *node);
void check_state(struct xen_device *xendev);
int check_state_early(struct xen_device *xendev);
/* backend.c */
int backend_init(int backend_domid);
int backend_close(void);
xen_backend_t backend_register(const char *type, int domid, struct xen_backend_ops *ops, backend_private_t priv);
void backend_release(xen_backend_t xenback);
void backend_xenstore_handler(void *unused);
int backend_xenstore_fd(void);
int backend_bind_evtchn(xen_backend_t xenback, int devid);
void backend_unbind_evtchn(xen_backend_t xenback, int devid);
int backend_evtchn_notify(xen_backend_t xenback, int devid);
void *backend_evtchn_priv(xen_backend_t xenback, int devid);
void backend_evtchn_handler(void *priv);
void *backend_map_shared_page(xen_backend_t xenback, int devid);
void backend_unmap_shared_page(xen_backend_t xenback, int devid, void *page);
