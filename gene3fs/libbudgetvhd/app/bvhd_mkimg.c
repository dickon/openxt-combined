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

static void
usage (char *prog)
{
  fprintf (stderr, "Usage:\n%s [-s] vhd_file image_file_or_device\n", prog);
  fprintf (stderr,
           "\t-s\tcauses the program to not write blocks which only contain zero\n");

  exit (1);
}

int
main (int argc, char *argv[])
{
  int fd;
  uint64_t off;
  uint64_t len;
  uint64_t blocksize;
  uint64_t ocol;
  int blocks = 0, empty = 0;

  char *buf;
  char *zero;
  BVHD *v;
  int sparse = 0;

  int opt;

  while ((opt = getopt (argc, argv, "sh")) != -1)
    {
      switch (opt)
        {
        case 's':
          sparse++;
          break;
        default:
          usage (argv[0]);
        }
    }

  if ((argc - optind) != 2)
    usage (argv[0]);

  v = bvhd_open (argv[optind], 1);
  if (!v)
    {
      fprintf (stderr, "bvhd_open(%s,1) failed\n", argv[1]);
      return 1;
    }

  fd = open (argv[optind + 1], O_WRONLY | O_CREAT | O_TRUNC, 0666);

  if (fd < 0)
    {
      perror ("open");
      return 1;
    }


  blocksize = bvhd_block_size (v);
  len = bvhd_size (v);

  buf = malloc (blocksize);
  zero = malloc (blocksize);

  if (!buf || !zero)
    {
      fprintf (stderr, "out of memory\n");
      return 1;
    }

  bzero (zero, blocksize);

  (void) ftruncate (fd, len);


  printf ("Copying\n");

  ocol = 0;
  for (off = 0; off < len; off += blocksize)
    {
      int count = blocksize >> 9;
      uint64_t col = (off * 80) / len;

      if (col > 79)
        col = 79;

      if (col != ocol)
        {
          putchar ('.');
          fflush (stdout);
          ocol = col;
        }


      if ((blocksize + off) > len)
        {
          count = (len - off) >> 9;
        }
      blocks++;


      if (!bvhd_bat_filled (v, off >> 9))
        {
          if (!sparse && (pwrite (fd, buf, count << 9, off) != (count << 9)))
            {
              fprintf (stderr, "pwrite gave short write\n");
              return 1;
            }
          empty++;
          continue;
        }


      if (bvhd_read (v, buf, off >> 9, count) != count)
        {
          printf ("%x to %x gives %x\n", (int) (off >> 9), (int) count,
                  (int) ((off >> 9) + count));
          fprintf (stderr, "bvhd_read gave short read\n");
          return 1;
        }


      if (!memcmp (buf, zero, blocksize))
        {
          empty++;
          if (sparse)
            {
              continue;
            }
        }

      if (pwrite (fd, buf, count << 9, off) != (count << 9))
        {
          fprintf (stderr, "pwrite gave short write\n");
          return 1;
        }
    }



  close (fd);
  bvhd_close (v);
  printf ("\nDone %d/%d blocks were empty\n", empty, blocks);
  return 0;
}
