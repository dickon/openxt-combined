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
icbinn_readdir (ICBINN * icb, const char *dir, size_t offset,
                size_t count, struct icbinn_dirent * buf)
{
  icbinn_prot_readdirargs argp;
  icbinn_prot_readdirres *res;
  icbinn_prot_entry *e;
  int n = 0;

  argp.dir = (icbinn_prot_path) dir;
  argp.offset = offset;
  argp.count = count;

  res = icbinn_prot_readdir_1 (&argp, icb->c);

  if (!res)
    return -1;

  if (res->status != ICBINN_PROT_OK)
    {
      (void) clnt_freeres (icb->c, (xdrproc_t) xdr_icbinn_prot_readdirres,
                           (caddr_t) res);
      return -1;
    }

  for (e = res->icbinn_prot_readdirres_u.reply.entries; e; e = e->nextentry)
    {

      strncpy (buf[n].name, e->name, ICBINN_PROT_MAXNAMELEN);
      buf[n].name[ICBINN_PROT_MAXNAMELEN] = '\0';

      switch (e->type)
        {
        case ICBINN_PROT_FILE:
          buf[n].type = ICBINN_TYPE_FILE;
          break;
        case ICBINN_PROT_DIRECTORY:
          buf[n].type = ICBINN_TYPE_DIRECTORY;
          break;
	case ICBINN_PROT_LINK:
	  buf[n].type = ICBINN_TYPE_LINK;
	  break;
        case ICBINN_PROT_UNKNOWN:
          buf[n].type = ICBINN_TYPE_UNKNOWN;
          break;
        default:
          buf[n].type = -1;
        }

      n++;
    }

  (void) clnt_freeres (icb->c, (xdrproc_t) xdr_icbinn_prot_readdirres,
                       (caddr_t) res);

  return n;
}

EXTERNAL int
icbinn_readent (ICBINN * icb, const char *dir, size_t offset,
                struct icbinn_dirent *buf)
{
  return (icbinn_readdir (icb, dir, offset, 1, buf) == 1) ? 0 : -1;
}
