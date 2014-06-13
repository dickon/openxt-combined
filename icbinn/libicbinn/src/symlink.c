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
icbinn_symlink (ICBINN * icb, const char *oldpath, const char *newpath)
{
  icbinn_prot_symlinkargs argp;
  icbinn_prot_errnostat *res;

  argp.oldpath = (icbinn_prot_path) oldpath;
  argp.newpath = (icbinn_prot_path) newpath;

  res = icbinn_prot_symlink_1 (&argp, icb->c);

  if (!res)
    return -1;

  if (res->status != ICBINN_PROT_OK)
    return -res->icbinn_prot_errnostat_u.errno_code;

  return 0;
}
