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

#include "config.h"
#define _XOPEN_SOURCE 500

#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <stdlib.h>

#include "budgetvhd.h"


int
main (int argc, char *argv[])
{
  uint64_t len;
  uint64_t blocksize = 0x200000;
  BVHD *v;

  if (argc != 3)
    {
      fprintf (stderr, "Usage:\n%s vhd_file size_in_blocks_of_1k\n", argv[0]);
      return 1;
    }

  len = atol(argv[2]);
  len *=1024;

  len += blocksize - 1;
  len &= ~(blocksize - 1);

  v = bvhd_create (argv[1], len, blocksize);
  if (!v)
    {
      fprintf (stderr, "bvhd_create(%s,%lld) failed\n", argv[2],
               (long long int) len);
      return 1;
    }

  printf ("created vhd of %lld bytes, blocksize %lld\n", (long long int) len,
          (long long int) blocksize);

  bvhd_close (v);

  return 0;
}
