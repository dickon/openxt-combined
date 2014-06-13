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

#include "budgetvhd.h"


int
main (int argc, char *argv[])
{
  int fd;
  uint64_t off;
  uint64_t len;
  uint64_t blocksize = 0x200000;
  uint64_t ocol;
  int blocks = 0, empty = 0;

  char *buf, *zero;
  BVHD *v;

  if (argc != 3)
    {
      fprintf (stderr, "Usage:\n%s image_file_or_device vhd_file\n", argv[0]);
      return 1;
    }

  fd = open (argv[1], O_RDONLY);

  if (fd < 0)
    {
      perror ("open");
      return 1;
    }


  len = lseek (fd, 0, SEEK_END);
  lseek (fd, 0, SEEK_SET);

  len += blocksize - 1;
  len &= ~(blocksize - 1);


  v = bvhd_create (argv[2], len, blocksize);
  if (!v)
    {
      fprintf (stderr, "bvhd_create(%s,%lld) failed\n", argv[2],
               (long long int) len);
      return 1;
    }

  printf ("created vhd of %lld bytes, blocksize %lld\n", (long long int) len,
          (long long int) blocksize);


  buf = malloc (blocksize);
  zero = malloc (blocksize);

  if (!buf || !zero)
    {
      fprintf (stderr, "out of memory\n");
      return 1;
    }

  bzero (zero, blocksize);

  printf ("Copying\n");

  ocol = 0;
  for (off = 0; off < len; off += blocksize)
    {
      uint64_t col = (off * 80) / len;

      if (col > 79)
        col = 79;

      if (col != ocol)
        {
          putchar ('.');
          fflush (stdout);
          ocol = col;
        }

      if (pread (fd, buf, blocksize, off) != blocksize)
        {
          fprintf (stderr, "pread gave short read\n");
          return 1;
        }

      if (memcmp (buf, zero, blocksize))
        {

          if (bvhd_write (v, buf, off >> 9, blocksize >> 9) !=
              (blocksize >> 9))
            {
              fprintf (stderr, "bvhd_write gave short write\n");
              return 1;
            }
          blocks++;
        }
      else
        {
          empty++;
          blocks++;
        }


    }

  bvhd_close (v);
  close (fd);
  printf ("\nDone %d/%d blocks were empty\n", empty, blocks);

  return 0;
}
