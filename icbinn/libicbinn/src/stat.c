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
icbinn_stat (ICBINN * icb, const char *path, struct icbinn_stat *buf)
{
  icbinn_prot_path argp;
  icbinn_prot_statres *res;

  if (!path)
    return -1;
  if (!buf)
    return -1;

  argp = (icbinn_prot_path) path;

  res = icbinn_prot_stat_1 (&argp, icb->c);

  if (!res)
    return -1;

  if (res->status != ICBINN_PROT_OK)
    return -res->icbinn_prot_statres_u.errno_code;

  switch (res->icbinn_prot_statres_u.reply.type)
    {
    case ICBINN_PROT_FILE:
      buf->type = ICBINN_TYPE_FILE;
      break;
    case ICBINN_PROT_DIRECTORY:
      buf->type = ICBINN_TYPE_DIRECTORY;
      break;
    case ICBINN_PROT_LINK:
      buf->type = ICBINN_TYPE_LINK;
      break;
    case ICBINN_PROT_UNKNOWN:
      buf->type = ICBINN_TYPE_UNKNOWN;
      break;
    default:
      return -1;
    }

  buf->size = res->icbinn_prot_statres_u.reply.size;

  return 0;
}
