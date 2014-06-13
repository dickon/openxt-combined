/*********************************************************************
 * $Id: //icaclient/develop/main/src/XenDesktop/usb/unix/logging/ctxusblog.c#1 $
 * 
 * Copyright 2008 Citrix Systems, Inc. All rights reserved.
 ********************************************************************/

/*
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

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <ctype.h>

#include "ctxusblog.h"

int log_facility = LOG_USER;

void LogOpen(const char *ident)
{
    int option = LOG_PID;
    openlog(ident, option, log_facility);
}

void LogClose(void)
{
    closelog();
}

#ifdef DEBUG
// Never log location in syslog for retail build
//#define LOG_LOCATION
#endif

#ifdef LOG_LOCATION
#define LLP
#else
#define LLP __attribute__ ((__unused__))
#endif

void LogGeneral(LLP const char *file,
                LLP int line, 
                LLP const char *func, 
                int level, 
                const char *format, ...)
{
    va_list params;
    va_start(params, format);
#ifdef LOG_LOCATION
    syslog(log_facility | level, "%s:%d - %s\n", file, line, func);
#endif
    vsyslog(log_facility | level, format, params);    
    va_end(params);
}

#ifdef DEBUG

static const int MAX_DUMP_SIZE=1024;

#define min(a,b) ((a) < (b)) ? (a) : (b)

void LogBuffer(const char *preamble, void *pbuf, int size)
{
    unsigned char *pbyte = (unsigned char *)pbuf;
    int i, j;
    int priority = log_facility | LOG_DEBUG;

    int dumpsize = min (size, MAX_DUMP_SIZE);

    syslog(priority, "%s dumping first %d bytes (of %d) @ 0x%p\n", 
           preamble, dumpsize, size, pbuf);

    char line_buffer[8 + 16*3 + 3 + 16 + 1];
    for (i = 0; i < dumpsize; i += 16) {
        int line_size = sizeof(line_buffer);
        int offset = 0;
        /* 8 chars */
        offset += snprintf(line_buffer+offset, line_size, "0x%04x: ", i);
        if(offset > line_size) 
            return;
        /* 16*3 chars */
        for (j = 0; j < 16 && (i+j) < dumpsize; j++) {
            offset += snprintf(line_buffer+offset, line_size-offset, "%02X ", pbyte[j]);
            if(offset > line_size) 
                return;
        }
        for (; j < 16; j++) {
            offset += snprintf(line_buffer+offset, line_size-offset, "   ");
            if(offset > line_size) 
                return;
        }
        /* 3 chars */
        offset += snprintf(line_buffer+offset, line_size-offset, "   ");
        if(offset > line_size) 
            return;
        /* 16 chars */
        for (j = 0; j < 16 && (i+j) < dumpsize; j++, pbyte++) {
            offset += snprintf(line_buffer+offset, line_size-offset, "%c", isprint(*pbyte) ? *pbyte : '.');
            if(offset > line_size) 
                return;
        }
        syslog(priority, "%s", line_buffer);
    }
}

#endif // DEBUG
