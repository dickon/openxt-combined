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

/*
 * vhdio_io.c --- io to a vhd 
 */

#define _XOPEN_SOURCE 600 /* for inclusion of PATH_MAX in Solaris */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <ctype.h>
#include <time.h>
#ifdef __linux__
#include <sys/utsname.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#else
extern char *optarg;
extern int optind;
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif
#include <sys/ioctl.h>
#include <sys/types.h>
#include <libgen.h>
#include <limits.h>


#include "ext2fs/ext2_fs.h"
#include "ext2fs/ext2fs.h"
#include <budgetvhd.h>

#include "vhdio.h"

#define EXT2_ET_MAGIC_VHD_IO_CHANNEL 0x1928419a

struct vhdio_private_data
{
  int magic;
#if 0
  int fd;
#endif
  int sectors_per_block;
  BVHD *vhd;
  struct struct_io_stats io_stats;
};

static struct struct_io_manager struct_vhd_manager;

io_manager vhd_io_manager = &struct_vhd_manager;


static errcode_t
vhdio_get_stats (io_channel channel, io_stats * stats)
{
  struct vhdio_private_data *data;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);


  if (stats)
    *stats = &data->io_stats;


  return 0;
}

static errcode_t
vhdio_open (const char *name, int flags, io_channel * channel)
{

  io_channel io = NULL;
  struct vhdio_private_data *data = NULL;
  errcode_t retval;


  do
    {
      retval = ext2fs_get_mem (sizeof (struct struct_io_channel), &io);
      if (retval)
        break;

      memset (io, 0, sizeof (struct struct_io_channel));
      io->magic = EXT2_ET_MAGIC_IO_CHANNEL;

      retval = ext2fs_get_mem (sizeof (struct vhdio_private_data), &data);
      if (retval)
        break;

      io->manager = vhd_io_manager;

      retval = ext2fs_get_mem (strlen (name) + 1, &io->name);
      if (retval)
        break;

      strcpy (io->name, name);
      io->private_data = data;


      io->block_size = 1024;
      io->read_error = 0;
      io->write_error = 0;
      io->refcount = 1;

      memset (data, 0, sizeof (struct vhdio_private_data));
      data->magic = EXT2_ET_MAGIC_VHD_IO_CHANNEL;
      data->io_stats.num_fields = 2;
      data->sectors_per_block = io->block_size / 512;


#if 0
      data->fd = open ("../test/vhdfs", O_RDWR);
#endif

      data->vhd = bvhd_open (io->name, (flags & IO_FLAG_RW) ? 0 : 1);

      if (!data->vhd)
        break;


      *channel = io;
      return 0;
    }
  while (0);

  if (data)
    ext2fs_free_mem (&data);

  if (io)
    ext2fs_free_mem (&io);
  return EXT2_ET_BAD_DEVICE_NAME;
}

static errcode_t
vhdio_close (io_channel channel)
{
  struct vhdio_private_data *data;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);

  if (--channel->refcount > 0)
    return 0;

  bvhd_close (data->vhd);

  ext2fs_free_mem (&channel->private_data);
  if (channel->name)
    ext2fs_free_mem (&channel->name);
  ext2fs_free_mem (&channel);

  return 0;
}

static errcode_t
vhdio_set_blksize (io_channel channel, int blksize)
{
  struct vhdio_private_data *data;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);

  channel->block_size = blksize;
  data->sectors_per_block = channel->block_size / 512;
  return 0;
}


static errcode_t
vhdio_read_blk64 (io_channel channel, unsigned long long block, int count,
                  void *buf)
{
  struct vhdio_private_data *data;
  int slop = 0;
  uint64_t sector = block;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);

#if 0
  {
    uint32_t size = (count < 0) ? -count : count * channel->block_size;
    uint64_t location = (uint64_t) block * channel->block_size;
    int actual;

    data->io_stats.bytes_read += size;

    if (lseek (data->fd, location, SEEK_SET) != location)
      {
        return EXT2_ET_LLSEEK_FAILED;
      }
    actual = read (data->fd, buf, size);

    if (actual != size)
      return EXT2_ET_SHORT_READ;


    return 0;

  }
#endif


  sector *= data->sectors_per_block;

  if (count < 0)
    {
      count = -count;
      slop = count & 0x1ff;
      count >>= 9;
    }
  else
    {
      count *= data->sectors_per_block;
    }

  if (bvhd_read (data->vhd, buf, sector, count) != count)
    {
      printf ("Short read\n");
      return EXT2_ET_SHORT_READ;
    }

  data->io_stats.bytes_read += count * 512;

  if (slop)
    {
      char tmp[512];
      uint8_t *ptr = buf;

      ptr += count * 512;

      if (bvhd_read (data->vhd, tmp, sector + count, 1) != 1)
        return EXT2_ET_SHORT_READ;

      memcpy (ptr, tmp, slop);
      data->io_stats.bytes_read += slop;

    }

  return 0;
}

static errcode_t
vhdio_read_blk (io_channel channel, unsigned long block, int count, void *buf)
{
  return vhdio_read_blk64 (channel, block, count, buf);
}


static errcode_t
vhdio_write_blk64 (io_channel channel, unsigned long long block, int count,
                   const void *buf)
{
  struct vhdio_private_data *data;
  int ret;
  int slop = 0;
  uint64_t sector = block;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);

#if 0
  {
    uint32_t size = (count < 0) ? -count : count * channel->block_size;
    uint64_t location = (uint64_t) block * channel->block_size;
    int actual;

    data->io_stats.bytes_read += size;

    if (lseek (data->fd, location, SEEK_SET) != location)
      {
        return EXT2_ET_LLSEEK_FAILED;
      }
    actual = write (data->fd, buf, size);

    if (actual != size)
      return EXT2_ET_SHORT_WRITE;


    return 0;

  }
#endif

  if (count < 0)
    {
      count = -count;
      slop = count & 0x1ff;
      count >>= 9;
    }
  else
    {
      count *= data->sectors_per_block;
    }

  sector *= data->sectors_per_block;

  if ((ret = bvhd_write (data->vhd, buf, sector, count)) != count)
    {
      return EXT2_ET_SHORT_WRITE;
    }

  data->io_stats.bytes_written += count * 512;

  if (slop)
    {
      char tmp[512];
      const uint8_t *ptr = buf;

      ptr += count * 512;

      if (bvhd_read (data->vhd, tmp, sector + count, 1) != 1)
        return EXT2_ET_SHORT_WRITE;

      memcpy (tmp, ptr, slop);

      if (bvhd_write (data->vhd, tmp, sector + count, 1) != 1)
        return EXT2_ET_SHORT_WRITE;

      data->io_stats.bytes_written += slop;
    }


  return 0;
}


static errcode_t
vhdio_write_blk (io_channel channel, unsigned long block,
                 int count, const void *buf)
{
  return vhdio_write_blk64 (channel, block, count, buf);
}

/* This is unnecessarily miserable */

static errcode_t
vhdio_write_byte (io_channel channel, unsigned long offset, int size,
                  const void *buf)
{
  struct vhdio_private_data *data;
  uint64_t location = (uint64_t) offset;
  const uint8_t *ptr = buf;
  uint64_t sector = offset >> 9;
  uint32_t slop = offset & 0x1ff;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);

#if 0
  {
    int actual;

    data->io_stats.bytes_written += size;

    if (lseek (data->fd, location, SEEK_SET) != location)
      {
        return EXT2_ET_LLSEEK_FAILED;
      }
    actual = write (data->fd, buf, size);

    if (actual != size)
      return EXT2_ET_SHORT_WRITE;

    return 0;
  }
#endif


  while (size)
    {
      uint32_t count = 512 - slop;
      if (count > size)
        count = size;

      if (!slop && (count == 512))
        {
          if (bvhd_write (data->vhd, ptr, sector, 1) != 1)
            {
              return EXT2_ET_SHORT_WRITE;
            }
        }
      else
        {
          char tmp[512];
          if (bvhd_read (data->vhd, tmp, sector, 1) != 1)
            {
              return EXT2_ET_SHORT_WRITE;
            }
          memcpy (tmp + slop, ptr, count);
          if (bvhd_write (data->vhd, tmp, sector, 1) != 1)
            {
              return EXT2_ET_SHORT_WRITE;
            }
        }

      slop = 0;
      sector++;
      ptr += count;
      size -= count;
      data->io_stats.bytes_written += count;
    }

  return 0;
}



static errcode_t
vhdio_flush (io_channel channel)
{
  struct vhdio_private_data *data;

  EXT2_CHECK_MAGIC (channel, EXT2_ET_MAGIC_IO_CHANNEL);
  data = (struct vhdio_private_data *) channel->private_data;
  EXT2_CHECK_MAGIC (data, EXT2_ET_MAGIC_VHD_IO_CHANNEL);

  bvhd_flush (data->vhd);

  return 0;
}


static errcode_t
vhdio_set_option (io_channel channel, const char *option, const char *arg)
{
  return EXT2_ET_INVALID_ARGUMENT;
}

#ifdef HAVE_DISCARD

static errcode_t
vhdio_discard (io_channel channel, unsigned long long block,
               unsigned long long count)
{
  return EXT2_ET_UNIMPLEMENTED;
}
#endif


static struct struct_io_manager struct_vhd_manager = {
  EXT2_ET_MAGIC_IO_MANAGER,
  "VHD I/O Manager",
  vhdio_open,
  vhdio_close,
  vhdio_set_blksize,
  vhdio_read_blk,
  vhdio_write_blk,
  vhdio_flush,
  vhdio_write_byte,
  vhdio_set_option,
  vhdio_get_stats,
  vhdio_read_blk64,
  vhdio_write_blk64,
#if HAVE_DISCARD
  vhdio_discard,
#endif
};
