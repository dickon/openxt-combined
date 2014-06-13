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

EXTERNAL ssize_t
icbinn_readlink (ICBINN * icb, const char *path, char *buf, size_t bufsz)
{
  ssize_t bytes = 0;
  icbinn_prot_readlinkargs argp;
  icbinn_prot_readlinkres *res;

  if (!path)
    return -1;
  if (!buf)
    return -1;

  argp.path  = (icbinn_prot_path)path;
  argp.bufsz = bufsz;
  res = icbinn_prot_readlink_1(&argp, icb->c);
  if (!res)
    return -1;
  if (res->status != ICBINN_PROT_OK)
    return -res->icbinn_prot_readlinkres_u.errno_code;
  bytes = res->icbinn_prot_readlinkres_u.reply.data.data_len;
  if (bytes > bufsz) {
    (void) clnt_freeres (icb->c, (xdrproc_t) xdr_icbinn_prot_readlinkres,
			 (caddr_t) res);
    return -1;
  }

  memcpy (buf, res->icbinn_prot_readlinkres_u.reply.data.data_val, bytes);
  (void) clnt_freeres (icb->c, (xdrproc_t) xdr_icbinn_prot_readlinkres,
		       (caddr_t) res);
  return bytes;
}
