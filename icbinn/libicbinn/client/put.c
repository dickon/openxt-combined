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

#include "project.h"

int
cmd_put (int argc, char *argv[])
{
  int fd, ifd;
  static char buf[ICBINN_MAXDATA];
  int red;
  off_t o = 0;

  fd = open (argv[1], O_RDONLY);
  if (fd < 0)
    {
      perror ("open");
      return -1;
    }

  ifd =
    icbinn_open (icb, argv[2], ICBINN_WRONLY | ICBINN_CREAT | ICBINN_TRUNC);
  if (ifd < 0)
    {
      fprintf (stderr, "icbinn_open failed\n");
      close (fd);
      return -1;
    }


  if (icbinn_lock (icb, fd, ICBINN_LTYPE_WRLCK))
    {
      fprintf (stderr, "failed to get icbinn write lock\n");
      icbinn_close (icb, ifd);
      close (fd);
      return -1;
    }

  while ((red = read (fd, buf, sizeof (buf))) > 0)
    {


      if (icbinn_pwrite (icb, ifd, buf, red, o) != red)
        {
          fprintf (stderr, "icbinn_write failed\n");
          icbinn_close (icb, ifd);
          close (fd);
          return -1;
        }
      o += red;

    }

  icbinn_close (icb, ifd);
  close (fd);

  if (red)
    {
      perror ("read");
      return -1;
    }



  return 0;
}
