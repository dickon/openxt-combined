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
int backend_print(xen_backend_t xenback, int devid, const char *node, const char *fmt, ...);
int backend_scan(xen_backend_t xenback, int devid, const char *node, const char *fmt, ...);
int frontend_scan(xen_backend_t xenback, int devid, const char *node, const char *fmt, ...);
/* state.c */
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
