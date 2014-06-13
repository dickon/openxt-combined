/*
 * util.c
 *
 * Copyright (c) 2011 Citrix Systems Inc.,
 * All rights reserved.
 *
 */

/*
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


#include <stdarg.h>
#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include "util.h"

void
message (int flags, const char *file, const char *function, int line,
         const char *fmt, ...)
{
  const char *level;
  char buf[1024] = { 0 };
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
  else
    {
      level = "Unknown";
    }

  va_start (ap, fmt);
  vsnprintf (buf, sizeof (buf), fmt, ap);
  va_end (ap);

  syslog (LOG_ERR, "%s:%s:%s:%d:%s", level, file, function, line, buf);

  if (flags & MESSAGE_FATAL)
    {
      exit (1);
    }
}
