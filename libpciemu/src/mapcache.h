/*
 * Copyright (c) 2013 Citrix Systems, Inc.
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

#ifndef MAPCACHE_H_
# define MAPCACHE_H_

# include <stdint.h>

extern int privcmd_fd;

typedef struct mapcache *mapcache_t;

mapcache_t mapcache_create (int domid);
void mapcache_destroy (mapcache_t mapcache);
int mapcache_invalidate_all (mapcache_t mapcache);
int copy_from_domain (mapcache_t mapcache, void *p, uint64_t addr, size_t sz);
int copy_to_domain (mapcache_t mapcache, uint64_t addr, void *p, size_t sz);
int mapcache_has_addr (mapcache_t mapcache, void *addr);
int mapcache_invalidate_entry (mapcache_t mapcache, uint64_t addr);

#endif /* !MAPCACHE_H_ */
