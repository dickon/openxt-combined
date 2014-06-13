/*
 * Copyright (c) 2013 Citrix Systems, Inc.
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

#ifndef LOGGING_H_
# define LOGGING_H_

# include <inttypes.h>
# include "libpciemu.h"

extern libpciemu_logging_t logging_cb;

#define __log(level, level_string, fmt, ...)                                \
    logging_cb (level, "%s:%s:%s:%d: "fmt"\n", level_string,                \
                __FILE__, __PRETTY_FUNCTION__, __LINE__, ## __VA_ARGS__)

# define info(a...) __log (LIBPCIEMU_MESSAGE_INFO, "Info", a)
# define warning(a...) __log (LIBPCIEMU_MESSAGE_WARNING, "Warning", a)
# define fatal(a...) __log (LIBPCIEMU_MESSAGE_FATAL, "Fatal", a)
# define error(a...) __log (LIBPCIEMU_MESSAGE_ERROR, "Error", a)
# define debug(a...) __log (LIBPCIEMU_MESSAGE_DEBUG, "Debug", a)

#endif /* !LOGING_H_ */
