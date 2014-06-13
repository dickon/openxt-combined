/*
 * Copyright (c) 2013 Citrix Systems, Inc.
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

#define _BSD_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#ifdef SYSLOG
#include <syslog.h>
#endif
#include "util.h"

static char *prefix = NULL;
static int log_where = LOG_SYSTEM | LOG_STDERR;

void prefix_set (const char *p)
{
#ifdef SYSLOG
    closelog ();
#endif
    if (prefix)
        free (prefix);
    prefix = strdup (p);
    if (!prefix)
        fatal ("Unable to set a new prefix log");
#ifdef SYSLOG
    openlog (prefix, LOG_CONS, LOG_USER);
#endif
}

int logging (int where)
{
    int tmp = log_where;

    log_where = where;

    return tmp;
}

#ifdef SYSLOG
static void message_syslog (loglvl lvl, const char *fmt, va_list ap)
{
    int syslog_lvl = LOG_DEBUG;

    switch (lvl)
    {
    case MESSAGE_DEBUG:
        syslog_lvl = LOG_DEBUG;
        break;
    case MESSAGE_INFO:
        syslog_lvl = LOG_INFO;
        break;
    case MESSAGE_WARNING:
        syslog_lvl = LOG_WARNING;
        break;
    case MESSAGE_ERROR:
        syslog_lvl = LOG_ERR;
        break;
    case MESSAGE_FATAL:
        syslog_lvl = LOG_EMERG;
        break;
    }

    vsyslog (syslog_lvl, fmt, ap);
}
#endif

static void message_stderr (loglvl lvl, const char *fmt, va_list ap)
{
    (void) lvl;

    vfprintf (stderr, fmt, ap);
    fprintf (stderr, "\n");
    fflush (stderr);
}

void message (loglvl lvl, const char *fmt, ...)
{
    va_list ap;

    if (log_where & LOG_STDERR)
    {
        va_start (ap, fmt);
        message_stderr (lvl, fmt, ap);
        va_end (ap);
    }

#ifdef SYSLOG
    if (log_where & LOG_SYSTEM)
    {
        va_start (ap, fmt);
        message_syslog (lvl, fmt, ap);
        va_end (ap);
    }
#endif /* !SYSLOG */

    if (lvl == MESSAGE_FATAL)
        abort ();
}

void *
xcalloc (size_t n, size_t s)
{
  void *ret = calloc (n, s);
  if (!ret)
    fatal ("calloc failed");
  return ret;
}

void *
xmalloc (size_t s)
{
  void *ret = malloc (s);
  if (!ret)
    fatal ("malloc failed");
  return ret;
}

void *
xrealloc (void *p, size_t s)
{
  p = realloc (p, s);
  if (!p)
    fatal ("realloc failed");
  return p;
}

char *
xstrdup (const char *s)
{
  char *ret = strdup (s);
  if (!ret)
    fatal ("strdup failed");
  return ret;
}

