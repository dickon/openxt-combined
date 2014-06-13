/*
 * util.c:
 *
 * Copyright (c) 2009 James McKenzie <20@madingley.org>,
 * All rights reserved.
 *
 */

/*
 * Copyright (c) 2010 Citrix Systems, Inc.
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


static char rcsid[] = "$Id:$";

/*
 * $Log:$
 */


#include "project.h"

void
message (int flags, const char *file, const char *function, int line,
         const char *fmt, ...)
{
  char *level = NULL;
  va_list ap;

  if (flags & MESSAGE_INFO)
    {
      level = "Info";
    }
  else if (flags & MESSAGE_WARNING)
    {
      level = "Warning";
    }
  else if (flags & MESSAGE_ERROR)
    {
      level = "Error";
    }
  else if (flags & MESSAGE_FATAL)
    {
      level = "Fatal";
    }

  fprintf (stderr, "%s:%s:%s:%d:", level, file, function, line);

  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  fprintf (stderr, "\n");
  fflush (stderr);

  if (flags &
      (MESSAGE_INFO | MESSAGE_WARNING | MESSAGE_ERROR | MESSAGE_FATAL))
    {
      syslog (LOG_ERR, "%s:%s:%s:%d:", level, file, function, line);

      va_start (ap, fmt);
      vsyslog (LOG_ERR, fmt, ap);
      va_end (ap);
    }

  if (flags & MESSAGE_FATAL)
      abort ();
}
