/*
 * Copyright (c) 2014 Citrix Systems, Inc.
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

#ifndef OWLS_H_
#define OWLS_H_

int owls_domain_attach(int bus, int dev, int domid);
int owls_domain_detach(int bus, int dev, int domid);
int owls_vusb_assign(int bus, int dev);
int owls_vusb_unassign(int bus, int dev);
int owls_list_domain_devices(int domid, int *buf_devids, int buf_sz);

#endif
