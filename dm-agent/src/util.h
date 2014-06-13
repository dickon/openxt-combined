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

#ifndef UTIL_H_
# define UTIL_H_

# include <stdbool.h>
# include <stdlib.h>

/*
 ** Utils and debug tools.
 */
typedef enum
{
    MESSAGE_FATAL = 0,
    MESSAGE_ERROR = 3,
    MESSAGE_WARNING = 4,
    MESSAGE_INFO = 6,
    MESSAGE_DEBUG = 7,
} loglvl;

# define LOG_NONE (0)
# define LOG_SYSTEM (1 << 0)
# define LOG_STDERR (2 << 0)

#define __log(level, level_string, fmt, ...)                            \
    message (level, "%s:%s:%s:%d: " fmt, level_string, __FILE__,   \
             __PRETTY_FUNCTION__, __LINE__, ## __VA_ARGS__)

# define info(a...) __log (MESSAGE_INFO, "Info", a)
# define warning(a...) __log (MESSAGE_WARNING, "Warning", a)
# define fatal(a...) __log (MESSAGE_FATAL, "Fatal", a)
# define error(a...) __log (MESSAGE_ERROR, "Error", a)
# define debug(a...) __log (MESSAGE_DEBUG, "Debug", a)

void prefix_set (const char *p);
/*  Turn off/on log. Return the previous status */
int logging (int where);
void message (loglvl lvl, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

void *xcalloc (size_t n, size_t s);
void *xmalloc (size_t s);
void *xrealloc (void *p, size_t s);
char *xstrdup (const char *s);
void lockfile_lock (void);


#endif /* !UTIL_H_ */
