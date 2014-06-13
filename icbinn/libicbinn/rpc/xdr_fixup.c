/*
 * Copyright (c) 2012 Citrix Systems, Inc.
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

#include <rpc/rpc.h>

/* VILE HACK - rpcgen from libc, specificies to call xdr_quad_t when dealing with hypers */
/* unfortunately that symbol isn't defined in tirpc - which instead uses xdr_hyper, thus */
/* we call a confused an unitizalized libc fuction which promptly explodes. Override the */
/* weak aliases with the correct symbols from libtirpc */

bool_t
xdr_quad_t (XDR * x, quad_t * q)
{
  return xdr_hyper (x, q);
}

bool_t
xdr_u_quad_t (XDR * x, u_quad_t * q)
{
  return xdr_u_hyper (x, q);
}
