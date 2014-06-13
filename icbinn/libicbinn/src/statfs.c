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

#include "project.h"

EXTERNAL int
icbinn_statfs (ICBINN * icb, const char *path, struct icbinn_statfs *buf)
{
  icbinn_prot_path argp;
  icbinn_prot_statfsres *res;

  if (!path)
    return -1;
  if (!buf)
    return -1;

  argp = (icbinn_prot_path) path;

  res = icbinn_prot_statfs_1 (&argp, icb->c);

  if (!res)
    return -1;

  if (res->status != ICBINN_PROT_OK)
    return -res->icbinn_prot_statfsres_u.errno_code;
  buf->bsize = res->icbinn_prot_statfsres_u.reply.bsize;
  buf->blocks = res->icbinn_prot_statfsres_u.reply.blocks;
  buf->bfree = res->icbinn_prot_statfsres_u.reply.bfree;
  buf->bavail = res->icbinn_prot_statfsres_u.reply.bavail;

  return 0;
}
