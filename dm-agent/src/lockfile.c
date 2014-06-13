/*
 * Copyright (c) 2012 Citrix Systems, Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

#define PIDFILE "/var/run/dm-agent.pid"

static void lockfile_remove_stale (char *path)
{
  int fd;
  int pid;
  char apid[20];
  int length;

  fd = open (path, O_RDONLY);
  if (fd < 0)
    return;

  length = read (fd, apid, sizeof (apid) - 1);
  if (length < 0)
    length = 0;
  apid[length] = 0;

  pid = 0;
  if (length == sizeof (pid) || sscanf (apid, "%d", &pid) != 1 || pid == 0)
    {
      int *p = (int *)apid;
      pid = *p;
    }

  close (fd);

  if ((kill (pid, 0) < 0) && (errno == ESRCH))
    {
      warning ("removing stale lock file %s (no pid %d)\n", path, pid);
      unlink (path);
    }
}

static int lockfile_make (char *name)
{
  char buf[1024], tmpfn[1024];
  char *ptr;
  int fd;
  int i;

  strcpy (tmpfn, name);

  ptr = rindex (tmpfn, '/');
  if (!ptr)
    return -1;

  ptr++;

  ptr += sprintf (ptr, "LTMP.%d", getpid ());
  *ptr = 0;

  i = sprintf (buf, "%10d\n", getpid ());

  unlink (tmpfn);
  fd = open (tmpfn, O_WRONLY | O_CREAT | O_TRUNC, 0444);
  if (fd < 0)
    {
      unlink (tmpfn);
      return -1;
    }
  write (fd, buf, i);
  close (fd);

  if (link (tmpfn, name) < 0)
    {
      unlink (tmpfn);
      return -1;
    }

  unlink (tmpfn);
  return 0;
}

void lockfile_lock (void)
{

  lockfile_remove_stale (PIDFILE);
  if (lockfile_make (PIDFILE))
      fatal ("Another instance of dm-agent seems to be already running.");
}
