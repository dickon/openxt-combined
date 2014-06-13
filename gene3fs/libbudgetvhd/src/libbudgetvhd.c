/*
 * libbudgetvhd.c:
 *
 * Copyright (c) 2012 James McKenzie <20@madingley.org>,
 * All rights reserved.
 *
 */

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


static char rcsid[] = "$Id:$";

/*
 * $Log:$
 *
 */

#include "project.h"



INTERNAL uint32_t
bvhd_checksum (void *_buf, int len)
{
  uint8_t *buf = (uint8_t *) _buf;
  uint32_t ret = 0;

  while (len--)
    ret += *(buf++);


  return ~ret;
}

INTERNAL uint32_t
bvhd_now (void)
{
  time_t t = time (NULL);

  return t - 946684800;
}

INTERNAL bvhd_uuid
bvhd_new_uid (void)
{
  bvhd_uuid ret;
  int fd;

  fd = open ("/dev/urandom", O_RDONLY);

  read (fd, ret.uuid, sizeof (ret.uuid));
  close (fd);

  return ret;
}


INTERNAL uint64_t
bvhd_round_to_block (BVHD * v, uint64_t ret)
{
  ret += v->block_mask;
  ret &= ~v->block_mask;
  return ret;
}


INTERNAL uint64_t
bvhd_round_to_sector (BVHD * v, uint64_t ret)
{
  ret += 511;
  ret &= ~(uint64_t) 511;
  return ret;
}

/* Implement the CHS calculation in the appendix of the Microsoft standard*/

INTERNAL bvhd_geometry
bvhd_make_geometry (uint64_t sectors)
{
  bvhd_geometry ret;
  uint32_t hc, heads;

  if (sectors > (0xffff * 16 * 255))
    {

      ret.sectors = 255;
      ret.heads = 16;
      ret.cylinders = 0xffff;

    }
  else
    {
      ret.sectors = 17;

      hc = sectors / ret.sectors;

      heads = (hc + 1023) / 1024;

      if (heads < 4)
        heads = 4;

      if ((hc >= (heads * 1023)) || (heads > 16))
        {
          heads = 16;

          ret.sectors = 31;
          hc = sectors / ret.sectors;
        }

      if (hc >= (heads * 1023))
        {
          ret.sectors = 63;
          heads = 16;
          hc = sectors / ret.sectors;
        }

      ret.heads = heads;
      ret.cylinders = hc / heads;
    }

  return ret;
}


INTERNAL int
bvhd_check_footer (bvhd_footer * footer)
{
  uint32_t c, d;

  if (memcmp (footer->cookie, BVHD_COOKIE, BVHD_COOKIE_LEN))
    return -1;
  if (footer->file_format_version != BVHD_FILE_FORMAT_VERSION)
    return -1;
  if (footer->disk_type != BVHD_DISK_TYPE_DYNAMIC_HARD_DISK)
    return -1;

  c = footer->checksum;
  footer->checksum = 0;

  d = bvhd_checksum (footer, sizeof (bvhd_footer));
  footer->checksum = c;

  if (c != d)
    return -1;

  return 0;
}

INTERNAL int
bvhd_check_header (bvhd_header * header)
{
  uint32_t c, d;

  if (memcmp (header->cookie, BVHD_COOKIE2, BVHD_COOKIE_LEN))
    return -1;
  if (header->data_offset != 0xffffffffffffffffULL)
    return -1;
  if (header->header_version != BVHD_HEADER_VERSION)
    return -1;


  c = header->checksum;
  header->checksum = 0;

  d = bvhd_checksum (header, sizeof (bvhd_header));
  header->checksum = c;

  if (c != d)
    return -1;


  return 0;
}


EXTERNAL void
bvhd_close (BVHD * v)
{

  if (v->f)
    fclose (v->f);

  if (v->bitmap)
    free (v->bitmap);
  if (v->bat)
    free (v->bat);

  free (v);
}

EXTERNAL BVHD *
bvhd_open (char *name, int ro)
{
  uint32_t s;
  bvhd_footer footer2;
  BVHD *ret;
  int i;

  ret = (BVHD *) malloc (sizeof (BVHD));
  ret->bat = NULL;
  ret->bitmap = NULL;

  ret->ro = ro;

  ret->f = fopen (name, ro ? "r" : "r+");
  if (!ret->f)
    {
      bvhd_close (ret);
      return NULL;
    }


  bzero (&ret->footer, sizeof (bvhd_footer));

  fseek (ret->f, 0L, SEEK_SET);
  if ((fread (&ret->footer, sizeof (bvhd_footer), 1, ret->f) != 1)
      || bvhd_swab_footer (&ret->footer) || bvhd_check_footer (&ret->footer))
    {
      bvhd_close (ret);
      return NULL;
    }

  bzero (&ret->header, sizeof (bvhd_header));

  if (fseek (ret->f, ret->footer.data_offset, SEEK_SET)
      || (fread (&ret->header, sizeof (bvhd_header), 1, ret->f) != 1)
      || bvhd_swab_header (&ret->header) || bvhd_check_header (&ret->header))
    {
      bvhd_close (ret);
      return NULL;
    }

  s = ret->block_size = ret->header.block_size;

  ret->block_shift = 0;
  while (s != 1)
    {
      if (s & 1)
        {
          bvhd_close (ret);
          return NULL;
        }
      s >>= 1;
      ret->block_shift++;
    }


  ret->block_mask = ret->block_size - 1;

  ret->block_sector_shift = ret->block_shift - 9;
  ret->block_sector_size = ret->block_size >> 9;
  ret->block_sector_mask = ret->block_sector_size - 1;

  ret->bitmap_size = ((ret->block_size / 512) + 7) / 8;
  ret->bitmap_size = bvhd_round_to_sector (ret, ret->bitmap_size);
  ret->bitmap = malloc (ret->bitmap_size);

  for (i = 0; i < ret->bitmap_size; ++i)
    ret->bitmap[i] = 0xff;

  ret->size = ret->footer.current_size;

  ret->bat_ents = (ret->size + ret->block_mask) >> ret->block_shift;

  ret->bat_offset = ret->header.table_offset;

  ret->bat = malloc (sizeof (uint32_t) * ret->bat_ents);

  if (fseek (ret->f, ret->bat_offset, SEEK_SET)
      || (fread (ret->bat, sizeof (uint32_t), ret->bat_ents, ret->f) !=
          ret->bat_ents))
    {
      bvhd_close (ret);
      return NULL;
    }

  bvhd_swab32 (ret->bat, ret->bat_ents);

  fseek (ret->f, 0, SEEK_END);
  ret->current_tail = ftell (ret->f);

  if (fseek (ret->f, ret->current_tail - sizeof (bvhd_footer), SEEK_SET)
      || (fread (&footer2, sizeof (footer2), 1, ret->f) != 1))
    {
      bvhd_close (ret);
      return NULL;
    }

  bvhd_swab_footer (&footer2);

  if (!memcmp (&ret->footer, &footer2, sizeof (bvhd_footer)))
    {
      ret->current_tail -= sizeof (bvhd_footer);
    }

  ret->current_tail = bvhd_round_to_sector (ret, ret->current_tail);

  return ret;
}



INTERNAL void
bvhd_create_footer (BVHD * v, bvhd_footer * footer)
{
  bzero (footer, sizeof (bvhd_footer));

  memcpy (footer->cookie, BVHD_COOKIE, BVHD_COOKIE_LEN);
  footer->features = BVHD_FEATURES_RESERVED;
  footer->file_format_version = BVHD_FILE_FORMAT_VERSION;
  footer->data_offset = 512;
  footer->time_stamp = bvhd_now ();
  memcpy (footer->creator_application, "XBVHD", 4);
  footer->creator_version = 0x00010000;
  footer->creator_host_os = BVHD_CREATOR_HOST_OS_WINDOWS;
  footer->original_size = v->size;
  footer->current_size = v->size;
  footer->disk_geometry = bvhd_make_geometry (v->size >> 9);
  footer->disk_type = BVHD_DISK_TYPE_DYNAMIC_HARD_DISK;
  footer->unique_id = bvhd_new_uid ();
  footer->saved_state = 0;

  footer->unique_id = bvhd_new_uid ();

  footer->checksum = bvhd_checksum (footer, sizeof (bvhd_footer));
}


INTERNAL void
bvhd_create_header (BVHD * v, bvhd_header * header)
{
  bzero (header, sizeof (bvhd_header));

  memcpy (header->cookie, BVHD_COOKIE2, BVHD_COOKIE_LEN);
  header->data_offset = 0xffffffffffffffffULL;
  header->table_offset = v->bat_offset;
  header->header_version = BVHD_HEADER_VERSION;
  header->max_table_entries = v->bat_ents;
  header->block_size = v->block_size;

  header->checksum = bvhd_checksum (header, sizeof (bvhd_header));
}



EXTERNAL BVHD *
bvhd_create (char *name, uint64_t size, uint64_t blocksize)
{
  BVHD *ret;
  int i;

  ret = (BVHD *) malloc (sizeof (BVHD));
  ret->bat = NULL;
  ret->bitmap = NULL;

  ret->block_size = blocksize ? blocksize : 2097152;

  ret->block_shift = 0;
  while (blocksize != 1)
    {
      if (blocksize & 1)
        {
          bvhd_close (ret);
          return NULL;
        }
      blocksize >>= 1;
      ret->block_shift++;
    }


  do
    {
      ret->block_mask = ret->block_size - 1;

      ret->block_sector_shift = ret->block_shift - 9;
      ret->block_sector_size = ret->block_size >> 9;
      ret->block_sector_mask = ret->block_sector_size - 1;

      ret->bitmap_size = ((ret->block_size / 512) + 7) / 8;
      ret->bitmap_size = bvhd_round_to_sector (ret, ret->bitmap_size);
      ret->bitmap = malloc (ret->bitmap_size);

      if (!ret->bitmap)
        break;

      for (i = 0; i < ret->bitmap_size; ++i)
        ret->bitmap[i] = 0xff;


#if 1
      size = bvhd_round_to_block (ret, size);
#endif

      ret->size = size;

      ret->bat_ents = (ret->size + ret->block_mask) >> ret->block_shift;

      ret->bat_offset = 2048;

      ret->bat = malloc (sizeof (uint32_t) * ret->bat_ents);
      if (!ret->bat)
        break;

      for (i = 0; i < ret->bat_ents; ++i)
        ret->bat[i] = 0xffffffff;

      bvhd_create_footer (ret, &ret->footer);

      bvhd_create_header (ret, &ret->header);

      ret->f = fopen (name, "w");
      if (!ret->f)
        break;

      bvhd_swab_footer (&ret->footer);

      if (fseek (ret->f, 0L, SEEK_SET)
          || (fwrite (&ret->footer, sizeof (bvhd_footer), 1, ret->f) != 1))
        break;

      bvhd_swab_footer (&ret->footer);

      bvhd_swab_header (&ret->header);

      if (fseek (ret->f, ret->footer.data_offset, SEEK_SET)
          || (fwrite (&ret->header, sizeof (bvhd_header), 1, ret->f) != 1))
        break;

      bvhd_swab_header (&ret->header);


      /* No need to swab the bat to and from as it's all -1 */

      if (fseek (ret->f, ret->bat_offset, SEEK_SET)
          || (fwrite (ret->bat, sizeof (uint32_t), ret->bat_ents, ret->f) !=
              ret->bat_ents))
        break;

      ret->current_tail =
        ret->bat_offset + (sizeof (uint32_t) * ret->bat_ents);

      ret->current_tail = bvhd_round_to_sector (ret, ret->current_tail);


      bvhd_swab_footer (&ret->footer);

      if (fseek (ret->f, ret->current_tail, SEEK_SET)
          || (fwrite (&ret->footer, sizeof (bvhd_footer), 1, ret->f) != 1))
        break;

      bvhd_swab_footer (&ret->footer);


      return ret;
    }
  while (0);

  bvhd_close (ret);
  return NULL;
}


INTERNAL uint32_t
bvhd_new_block (BVHD * v)
{
  uint64_t block_offset = v->current_tail;

  if (fseek (v->f, block_offset, SEEK_SET)
      || (fwrite (v->bitmap, v->bitmap_size, 1, v->f) != 1))
    {
      return 0xffffffff;
    }

  v->current_tail += v->bitmap_size;
  v->current_tail += v->block_size;

  bvhd_swab_footer (&v->footer);

  if (fseek (v->f, v->current_tail, SEEK_SET)
      || (fwrite (&v->footer, sizeof (bvhd_footer), 1, v->f) != 1))
    {
      return 0xffffffff;
    }

  bvhd_swab_footer (&v->footer);

  return block_offset >> 9;
}


INTERNAL int
bvhd_update_bat (BVHD * v, uint32_t block, uint32_t offset)
{
  v->bat[block] = offset;

  bvhd_swab32 (&v->bat[block], 1);

  if (fseek (v->f, v->bat_offset + (block * sizeof (uint32_t)), SEEK_SET)
      || (fwrite (&v->bat[block], sizeof (uint32_t), 1, v->f) != 1))
    return -1;

  bvhd_swab32 (&v->bat[block], 1);

  return 0;
}

EXTERNAL int
bvhd_read_sector (BVHD * v, void *buf, uint64_t sector)
{
  uint32_t block = sector >> v->block_sector_shift;
  //uint64_t os = sector;

  if (v->bat[block] == 0xffffffff)
    {
      bzero (buf, 512);
      return 0;
    }
  sector &= v->block_sector_mask;
  sector += (uint64_t) v->bat[block];
  sector <<= 9;
  sector += v->bitmap_size;


#if 0
  if (fseek (v->f, sector, SEEK_SET) || (fread (buf, 512, 1, v->f) != 1))
    {
      printf ("\nret->block_sector_mask=%llx, ret->block_sector_size=%llx\n",
              (long long int) v->block_sector_mask,
              (long long int) v->block_sector_size);
      printf ("Requested sector %llx\n", (long long int) os);
      printf ("block %x, offset %llx\n", block,
              (long long int) (os & v->block_sector_mask));
      printf ("bat[%x]=%x\n", block, v->bat[block]);
      printf ("bitmap_size=%llx\n", v->bitmap_size);
      printf ("offfset=>%llx\n", (long long int) sector);

      return -1;
    }
#else
  if (fseek (v->f, sector, SEEK_SET) || (fread (buf, 512, 1, v->f) != 1))
    return -1;
#endif

  return 0;
}



EXTERNAL uint32_t
bvhd_read (BVHD * v, void *_buf, uint64_t sector, uint32_t nsectors)
{
  uint8_t *buf = (uint8_t *) _buf;
  uint32_t count = 0;

  while (nsectors--)
    {
      if (bvhd_read_sector (v, buf, sector))
        {
          return count;
        }
      buf += 512;
      count++;
      sector++;
    }

  return count;
}


EXTERNAL int
bvhd_write_sector (BVHD * v, const void *buf, uint64_t sector)
{
  uint32_t block = sector >> v->block_sector_shift;

  if (v->ro)
    return -1;

  if (v->bat[block] == 0xffffffff)
    {
      uint32_t offset = bvhd_new_block (v);

      if (offset == 0xffffffff)
        return -1;

      if (bvhd_update_bat (v, block, offset))
        return -1;
    }

  sector &= v->block_sector_mask;
  sector += (uint64_t) v->bat[block];
  sector <<= 9;
  sector += v->bitmap_size;

  if (fseek (v->f, sector, SEEK_SET) || (fwrite (buf, 512, 1, v->f) != 1))
    return -1;

  return 0;
}

EXTERNAL uint32_t
bvhd_write (BVHD * v, const void *_buf, uint64_t sector, uint32_t nsectors)
{
  const uint8_t *buf = (uint8_t *) _buf;
  uint32_t count = 0;

  while (nsectors--)
    {
      if (bvhd_write_sector (v, buf, sector))
        return count;
      buf += 512;
      count++;
      sector++;
    }

  return count;
}

EXTERNAL uint64_t
bvhd_size (BVHD * v)
{
  return v->size;
}

EXTERNAL void
bvhd_flush (BVHD * v)
{
  fflush (v->f);
  fdatasync (fileno (v->f));
}

EXTERNAL uint64_t
bvhd_block_size (BVHD * v)
{
  return v->block_size;
}

EXTERNAL int
bvhd_bat_filled (BVHD * v, uint64_t sector)
{
  uint32_t block = sector >> v->block_shift;

  return (v->bat[block] == 0xffffffff) ? 0 : 1;
}
