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


#include "project.h"

INTERNAL void
message (int flags, const char *file, const char *function, int line,
         const char *fmt, ...)
{
  const char *level;
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
  if (flags & MESSAGE_FATAL)
    {
      abort ();
    }
}

INTERNAL void *
xmalloc (size_t s)
{
  void *ret = malloc (s);
  if (!ret)
    fatal ("malloc failed");
  return ret;
}

INTERNAL void *
xrealloc (void *p, size_t s)
{
  p = realloc (p, s);
  if (!p)
    fatal ("realloc failed");
  return p;
}

INTERNAL char *
xstrdup (const char *s)
{
  char *ret = strdup (s);
  if (!ret)
    fatal ("strdup failed");
  return ret;
}
