/*
 * util.h:
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


/*
 * $Id:$
 */

/*
 * $Log:$
 */

#ifndef __UTIL_H__
#define __UTIL_H__


#define MESSAGE_INFO    (1UL <<  0)
#define MESSAGE_WARNING (1UL <<  1)
#define MESSAGE_ERROR   (1UL <<  2)
#define MESSAGE_FATAL   (1UL <<  3)

#define info(a...) message(MESSAGE_INFO,__FILE__,__PRETTY_FUNCTION__,__LINE__,a)
#define warning(a...) message(MESSAGE_WARNING,__FILE__,__PRETTY_FUNCTION__,__LINE__,a)
#define error(a...) message(MESSAGE_ERROR,__FILE__,__PRETTY_FUNCTION__,__LINE__,a)
#define fatal(a...) message(MESSAGE_FATAL,__FILE__,__PRETTY_FUNCTION__,__LINE__,a)



#endif /* __UTIL_H__ */
