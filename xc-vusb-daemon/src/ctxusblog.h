/*********************************************************************
 * $Id: //icaclient/develop/main/src/XenDesktop/usb/unix/include/ctxusblog.h#1 $
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

#ifndef _CTXUSBLOG_H_
#define _CTXUSBLOG_H_

#include <syslog.h>
#include <stdarg.h>

void LogOpen(const char *ident);                           // Start logging system as "ident"
void LogClose(void);                                       // Finish logging system

#ifdef DEBUG
void LogBuffer(const char *preamble, void *pbuf, int size) // debug-buid only log of buffer content
    __attribute__((nonnull));
#else
#define LogBuffer(preamble, pbuf, size)
#endif


void LogGeneral(const char *file, int line, const char *func, int level, const char *format, ...)
    __attribute__((format(printf, 5, 6), nonnull));

#if defined(__STDC_VERSION__)  && (__STDC_VERSION__ >= 199901L)

#define LogHelpFn(level, ...) LogGeneral(__FILE__, __LINE__, __func__, level, __VA_ARGS__)
#define LogError(...)         LogHelpFn(LOG_ERR, __VA_ARGS__)
#define LogPerror(format, ...) LogError(format ": %m", ## __VA_ARGS__)
#define LogInfo(...)          LogHelpFn(LOG_INFO, __VA_ARGS__)
#define LogDebug(...)         LogHelpFn(LOG_DEBUG, __VA_ARGS__)

#ifdef DEBUG
#define LogNotRetail(...)     LogHelpFn(LOG_DEBUG, __VA_ARGS__)
#define LogTracer()           LogNotRetail("%s:%d - %s()\n", __FILE__, __LINE__, __func__)
#else
#define LogNotRetail(...)
#define LogTracer()
#endif // DEBUG

#elif defined(__GNUC__)

#define LogHelpFn(level, format...) LogGeneral(__FILE__, __LINE__, __func__, level, format)
#define LogError(format...)         LogHelpFn(LOG_ERR, format)
#define LogPerror(format, args...) LogError(format ": %m", ## args)
#define LogInfo(format...)          LogHelpFn(LOG_INFO, format)
#define LogDebug(format...)         LogHelpFn(LOG_DEBUG, format)

#ifdef DEBUG
#define LogNotRetail(format...)     LogHelpFn(LOG_DEBUG, format)
#define LogTracer()           LogNotRetail("%s:%d - %s()\n", __FILE__, __LINE__, __func__)
#else
#define LogNotRetail(format...)
#define LogTracer()
#endif // DEBUG

#else  // defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

//
//
#error Only C99 __VA_ARGS__ macro substitutions implemented.
//
//

#endif // defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

#endif // _CTXUSBLOG_H_
