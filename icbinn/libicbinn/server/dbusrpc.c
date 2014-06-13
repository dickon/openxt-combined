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

#include "project.h"

#ifdef HAVE_LIBXCDBUS
#include "xenmgr_vm_client.h"
#include "xenmgr_client.h"
#endif


int
get_icbinn_path (int domid, char *path_ptr, int buf_size, int path_index)
{
#ifdef HAVE_LIBXCDBUS
  xcdbus_conn_t *c_xcdbus = NULL;
  char *obj_path = NULL;
  char *value = NULL;
  char *ptr;
  int ret = -1;
  int i = path_index;
  const char delim = ',';

  /* have to initialise glib type system */
  g_type_init ();

  c_xcdbus = xcdbus_init (NULL);
  if (!c_xcdbus)
    {
      error ("failed to init xcdbus connection");
      goto err;
    }

  if (!com_citrix_xenclient_xenmgr_find_vm_by_domid_
      (c_xcdbus, "com.citrix.xenclient.xenmgr", "/", domid, &obj_path))
    {
      error ("failed to find vm by domid %d", domid);
      goto err;
    }

  if (!property_get_com_citrix_xenclient_xenmgr_vm_icbinn_path_
      (c_xcdbus, "com.citrix.xenclient.xenmgr", obj_path, &value))
    {
      error ("failed to get icbinn-path for domid %d", domid);
      goto err;
    }

  if (value == NULL)
    {
      error
        ("null value received when trying to get icbinn-path for domid %d",
         domid);
      goto err;
    }

  ptr = value;
  /* Parse value to get the path at index */
  while (i--)
    {
      ptr = index (ptr, ',');
      if (!ptr)
        {
          error
            ("not enough fields in icbinn-path to find number %d for domid %d",
             path_index, domid);
          goto err;
        }
      ptr++;
    }


  value = index (ptr, ',');
  if (value)
    *value = 0;




  if (strlen (ptr) >= buf_size)
    {
      error ("icbinn-path number %d for domid %d too long", path_index,
             domid);
      goto err;
    }

  if (!*ptr)
    {
      error ("failed to get icbinn-path number %d for domid %d", path_index,
             domid);
      goto err;
    }

  strncpy (path_ptr, ptr, buf_size);
  path_ptr[buf_size - 1] = '\0'; //XXX: not needed
  ret = 0;

err:
  xcdbus_shutdown (c_xcdbus);
  return ret;
#else
  strncpy (path_ptr, "./", buf_size);
  return 0;
#endif
}
