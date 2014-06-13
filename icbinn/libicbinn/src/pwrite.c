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


static ssize_t
icbinn_pwrite_worker (ICBINN * icb, int fd, const void *buf, size_t count,
                      uint64_t offset)
{
  icbinn_prot_pwriteargs argp;
  icbinn_prot_errnostat *res;

  argp.fd = fd;
  argp.offset = offset;
  argp.data.data_len = count;
  argp.data.data_val = (void *) buf;

  res = icbinn_prot_pwrite_1 (&argp, icb->c);

  if (!res)
    return -1;

  if (res->status != ICBINN_PROT_OK)
    return -res->icbinn_prot_errnostat_u.errno_code;

  return count;
}

EXTERNAL ssize_t
icbinn_pwrite (ICBINN * icb, int fd, const void *_buf, size_t count,
               uint64_t offset)
{
  ssize_t ret = 0;
  const uint8_t *buf = (uint8_t *) _buf;

  while (count)
    {
      size_t to_read = count > ICBINN_MAXDATA ? ICBINN_MAXDATA : count;
      ssize_t semi_ret = icbinn_pwrite_worker (icb, fd, buf, to_read, offset);

      if (semi_ret > 0)
        ret += semi_ret;

      if (semi_ret != to_read)
        return ret ? ret : semi_ret;

      count -= to_read;
      offset += to_read;
      buf += to_read;
    }

  return ret;
}
