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
#include <byteswap.h>

INTERNAL int
bvhd_swab16 (uint16_t * b, size_t n)
{
  while (n--)
    {
      *b = bswap_16 (*b);
      b++;
    }
  return 0;
}

INTERNAL int
bvhd_swab32 (uint32_t * b, size_t n)
{
  while (n--)
    {
      *b = bswap_32 (*b);
      b++;
    }
  return 0;
}

INTERNAL int
bvhd_swab64 (uint64_t * b, size_t n)
{
  while (n--)
    {
      *b = bswap_64 (*b);
      b++;
    }
  return 0;
}


INTERNAL int
bvhd_swab_header (bvhd_header * h)
{
  //uint8_t cookie[8];
  bvhd_swab64 (&h->data_offset, 1);
  bvhd_swab64 (&h->table_offset, 1);
  bvhd_swab32 (&h->header_version, 1);
  bvhd_swab32 (&h->max_table_entries, 1);
  bvhd_swab32 (&h->block_size, 1);
  bvhd_swab32 (&h->checksum, 1);
//bvhd_uuid parent_unique_id;
  bvhd_swab32 (&h->parent_time_stamp, 1);
  bvhd_swab32 (&h->reserved1, 1);
  bvhd_swab16 (h->parent_name, 256);
  //bvhd_locator parent_locator[8];
  //uint8_t reserved2[256];

  return 0;
}

INTERNAL int
bvhd_swab_geometry (bvhd_geometry * g)
{
  bvhd_swab16 (&g->cylinders, 1);
  return 0;
}

INTERNAL int
bvhd_swab_footer (bvhd_footer * f)
{

  //uint8_t cookie[8];
  bvhd_swab32 (&f->features, 1);
  bvhd_swab32 (&f->file_format_version, 1);
  bvhd_swab64 (&f->data_offset, 1);
  bvhd_swab32 (&f->time_stamp, 1);
  //uint8_t creator_application[4];
  bvhd_swab32 (&f->creator_version, 1);
  bvhd_swab32 (&f->creator_host_os, 1);
  bvhd_swab64 (&f->original_size, 1);
  bvhd_swab64 (&f->current_size, 1);
  bvhd_swab_geometry (&f->disk_geometry);
  bvhd_swab32 (&f->disk_type, 1);
  bvhd_swab32 (&f->checksum, 1);
  //bvhd_uuid unique_id;
  //uint8_t saved_state;
  //uint8_t reserved[427];
  return 0;
}
