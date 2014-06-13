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

#ifndef SPAWN_H_
# define SPAWN_H_

# include "device-model.h"

enum spawn_type
{
    SPAWN_INVALID,
    SPAWN_QEMU_OLD,
    SPAWN_QEMU,
};

bool spawn_add_argument (struct device_model *devmodel, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));

#define SPAWN_ADD_ARG(dm, fmt, ...)                     \
do                                                      \
{                                                       \
    if (!spawn_add_argument (dm, fmt, ## __VA_ARGS__))  \
        return false;                                   \
} while (0)

#define SPAWN_ADD_ARGL(dm, lbl_err, fmt, ...)           \
do                                                      \
{                                                       \
    if (!spawn_add_argument (dm, fmt, ## __VA_ARGS__))  \
        goto lbl_err;                                   \
} while (0)

#endif /* !SPAWN_H_ */
