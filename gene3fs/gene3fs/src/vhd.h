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

#include <stdint.h>


typedef struct VHD_struct VHD;

typedef struct
{
  uint8_t uuid[8];
} vhd_uuid;

typedef struct
{
  uint8_t locator[8];
} vhd_locator;

typedef struct
{
  uint8_t cookie[8];
  uint64_t data_offset;
  uint64_t table_offset;
  uint32_t header_version;
  uint32_t max_table_entries;
  uint32_t block_size;
  uint32_t checksum;
  vhd_uuid parent_unique_id;
  uint32_t parent_time_stamp;
  uint32_t reserved1;
  uint16_t parent_name[256];
  vhd_locator parent_locator[8];
  uint8_t reserved2[256];
} vhd_header;

typedef struct
{
  uint16_t cylinders;
  uint8_t heads;
  uint8_t sectors;
} vhd_geometry;


typedef struct
{
  uint8_t cookie[8];
  uint32_t features;
  uint32_t file_format_version;
  uint64_t data_offset;
  uint32_t time_stamp;
  uint8_t creator_application[4];
  uint32_t creator_version;
  uint32_t creator_host_os;
  uint64_t original_size;
  uint64_t current_size;
  vhd_geometry disk_geometry;
  uint32_t disk_type;
  uint32_t checksum;
  vhd_uuid unique_id;
  uint8_t saved_state;
  uint8_t reserved[427];
} vhd_footer;

#define VHD_COOKIE "conectix"
#define VHD_COOKIE2 "cxsparse"
#define VHD_COOKIE_LEN 8

#define VHD_FEATURES_TEMPORARY 0x1
#define VHD_FEATURES_RESERVED  0x2

#define VHD_FILE_FORMAT_VERSION 0x0010000

#define VHD_CREATOR_HOST_OS_WINDOWS 0x5769326B
#define VHD_CREATOR_HOST_OS_MACINTOSH 0x4D616320

#define VHD_DISK_TYPE_NONE	0
#define VHD_DISK_TYPE_FIXED_HARD_DISK	2
#define VHD_DISK_TYPE_DYNAMIC_HARD_DISK 3
#define VHD_DISK_TYPE_DIFFERENCING_HARD_DISK 4

#define VHD_HEADER_VERSION 0x00010000


extern void vhd_close (VHD * v);
extern VHD *vhd_open (char *name, int ro);
extern VHD *vhd_create (char *name, uint64_t size);
extern uint32_t vhd_read (VHD * v, void *_buf, uint64_t sector,
                          uint32_t nsectors);
extern uint32_t vhd_write (VHD * v, const void *_buf, uint64_t sector,
                           uint32_t nsectors);
extern uint64_t vhd_size (VHD * v);
extern void vhd_flush (VHD * v);
