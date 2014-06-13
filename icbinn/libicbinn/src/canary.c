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

#include "project.h"


/* A function which will cause all the relevant parts to the library to
 * be marked as needed, to use with ld -r -u icbinn_canary */

INTERNAL void
icbinn_canary (void)
{
  (void) icbinn_clnt_create_tcp (NULL, 0);
  (void) icbinn_clnt_create_v4v (0, 0);
  icbinn_clnt_destroy (NULL);
  (void) icbinn_close (NULL, 0);
  (void) icbinn_lock (NULL, 0, 0);
  (void) icbinn_mkdir (NULL, NULL);
  (void) icbinn_null (NULL);
  (void) icbinn_open (NULL, NULL, 0);
  (void) icbinn_pread (NULL, 0, NULL, 0, 0);
  (void) icbinn_pwrite (NULL, 0, NULL, 0, 0);
  (void) icbinn_readdir (NULL, NULL, 0, 0, NULL);
  (void) icbinn_readent (NULL, NULL, 0, NULL);
  (void) icbinn_rename (NULL, NULL, NULL);
  (void) icbinn_rmdir (NULL, NULL);
  (void) icbinn_stat (NULL, NULL, NULL);
  (void) icbinn_unlink (NULL, NULL);
  (void) icbinn_ftruncate (NULL, 0, 0);
  (void) icbinn_fallocate (NULL, 0, 0, 0, 0);
  (void) icbinn_rand (NULL, NULL, 0, 0);
}
