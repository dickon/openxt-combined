/*
 * Copyright (c) 2014 Citrix Systems, Inc.
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

/* USB policy daemon
 * Growable text buffers
 */

#ifndef BUFFER_H  /* include this file only once */
#define BUFFER_H

/* 0 terminated character based buffers (for strings) */

typedef struct {
    char *data;
    int len;
} Buffer;


/* Length recorded integer based buffers */

typedef struct {
    int *data;
    int len;
    int allocated;
} IBuffer;


#endif  /* BUFFER_H */
