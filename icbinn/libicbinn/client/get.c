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
cmd_get (int argc, char *argv[])
{
  int fd, ifd;
  static char buf[ICBINN_MAXDATA];
  int red;
  off_t o = 0;

  ifd = icbinn_open (icb, argv[1], ICBINN_RDONLY);
  if (ifd < 0)
    {
      fprintf (stderr, "icbinn_open failed\n");
      return -1;
    }

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_RDLCK))
    {
      fprintf (stderr, "failed to get icbinn read lock\n");
      icbinn_close (icb, ifd);
      return -1;
    }

  fd = open (argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    {
      perror ("open");
      icbinn_close (icb, ifd);
      return -1;
    }


  while ((red = icbinn_pread (icb, ifd, buf, sizeof (buf), o)) > 0)
    {

      if (write (fd, buf, red) != red)
        {
          perror ("write");
          close (fd);
          icbinn_close (icb, ifd);
          return -1;
        }
      o += red;
    }

  close (fd);
  icbinn_close (icb, ifd);

  if (red)
    {
      fprintf (stderr, "icbinn_pread failed\n");
      return -1;
    }

  return 0;
}
