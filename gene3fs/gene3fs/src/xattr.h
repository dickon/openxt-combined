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

#ifndef EXT3_XATTR_MAGIC
#define EXT3_XATTR_MAGIC                0xEA020000
#endif

#ifndef EXT3_XATTR_REFCOUNT_MAX
#define EXT3_XATTR_REFCOUNT_MAX         1024
#endif

#ifndef XATTR_SECURITY_PREFIX
#define XATTR_SECURITY_PREFIX   "security."
#define XATTR_SECURITY_PREFIX_LEN (sizeof (XATTR_SECURITY_PREFIX) - 1)
#endif

#ifndef XATTR_TRUSTED_PREFIX
#define XATTR_TRUSTED_PREFIX "trusted."
#define XATTR_TRUSTED_PREFIX_LEN (sizeof (XATTR_TRUSTED_PREFIX) - 1)
#endif


#ifndef XATTR_USER_PREFIX
#define XATTR_USER_PREFIX "user."
#define XATTR_USER_PREFIX_LEN (sizeof (XATTR_USER_PREFIX) - 1)
#endif

#ifndef EXT3_XATTR_INDEX_USER
#define EXT3_XATTR_INDEX_USER                   1
#endif

#ifndef EXT3_XATTR_INDEX_POSIX_ACL_ACCESS
#define EXT3_XATTR_INDEX_POSIX_ACL_ACCESS       2
#endif

#ifndef EXT3_XATTR_INDEX_POSIX_ACL_DEFAULT
#define EXT3_XATTR_INDEX_POSIX_ACL_DEFAULT      3
#endif

#ifndef EXT3_XATTR_INDEX_TRUSTED
#define EXT3_XATTR_INDEX_TRUSTED                4
#endif

#ifndef EXT3_XATTR_INDEX_LUSTRE
#define EXT3_XATTR_INDEX_LUSTRE                 5
#endif

#ifndef EXT3_XATTR_INDEX_SECURITY
#define EXT3_XATTR_INDEX_SECURITY               6
#endif


