/*
 * gene3fs.c:
 *
 * Copyright (c) 2012 James McKenzie <20@madingley.org>,
 * All rights reserved.
 *
 */

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


static char rcsid[] = "$Id:$";

/*
 * $Log:$
 */



#include "project.h"


void
copy_inode_permissions_from_stat (struct stat *stbuf,
                                  struct ext2_inode *inode)
{
  inode->i_mode &= LINUX_S_IFMT;
  inode->i_mode |= (stbuf->st_mode & ~LINUX_S_IFMT);
  inode->i_atime = stbuf->st_atime;
  inode->i_ctime = stbuf->st_ctime;
  inode->i_mtime = stbuf->st_mtime;

  inode->i_uid_low = stbuf->st_uid & 0xffff;
  ext2fs_set_i_uid_high (*inode, stbuf->st_uid >> 16);

  inode->i_gid_low = stbuf->st_gid & 0xffff;
  ext2fs_set_i_gid_high (*inode, stbuf->st_gid >> 16);
}



void
fill_inode_from_stat (ext2_filsys fs, struct stat *stbuf,
                      struct ext2_inode *inode)
{
  memset (inode, 0, sizeof (*inode));
  inode->i_links_count = 1;
  inode->i_size = stbuf->st_size;

  copy_inode_permissions_from_stat (stbuf, inode);

  if (fs->super->s_feature_incompat & EXT3_FEATURE_INCOMPAT_EXTENTS)
    inode->i_flags |= EXT4_EXTENTS_FL;
}



int
make_inode_in_dir (ext2_filsys fs, char *path, char *leaf, ext2_ino_t dir,
                   ext2_ino_t * ino, int mode, int type)
{
  int ret;


  if (!ext2fs_namei (fs, EXT2_ROOT_INO, dir, leaf, ino))
    {
      fprintf (stderr, "%s/%s already exists\n", path, leaf);
      return -1;
    }

  if ((ret = ext2fs_new_inode (fs, dir, mode, 0, ino)))
    {
      fprintf (stderr, "ext2fs_new_inode gave %d\n", ret);
      return -1;
    }

  ret = ext2fs_link (fs, dir, leaf, *ino, type);

  if (ret == EXT2_ET_DIR_NO_SPACE)
    {
      if ((ret = ext2fs_expand_dir (fs, dir)))
        {
          fprintf (stderr, "ext2fs_expand_dir(%s) gave %d\n", path, ret);
          return -1;
        }

      ret = ext2fs_link (fs, dir, leaf, *ino, type);
    }

  if (ret)
    {
      fprintf (stderr, "ext2fs_link(%s,%s) gave %x\n", path, leaf, ret);
      return -1;
    }

  ext2fs_inode_alloc_stats2 (fs, *ino, +1, 0);

  ext2fs_mark_inode_bitmap (fs->inode_map, *ino);
  ext2fs_mark_ib_dirty (fs);



  return 0;
}



int
copy_file (ext2_filsys fs, char *path, ext2_ino_t ino)
{
  static char buf[8192];
  char *ptr;
  int ret;

  ext2_file_t e2_file;
  int len;
  size_t writ;

  int fd;

  fd = open (path, O_RDONLY);

  if (fd < 0)
    {
      perror ("open");
      return -1;
    }

  if ((ret = ext2fs_file_open (fs, ino, EXT2_FILE_WRITE, &e2_file)))
    {
      close (fd);
      fprintf (stderr, "ext2fs_file_open(%s) gave %d\n", path, ret);
      return -1;
    }

  while ((len = read (fd, buf, sizeof (buf))) > 0)
    {


      for (ptr = buf; len; len -= writ, ptr += writ)
        {

          if ((ret = ext2fs_file_write (e2_file, ptr, len, &writ)))
            {
              fprintf (stderr, "ext2fs_file_write(%s) gave %d\n", path, ret);
              ext2fs_file_close (e2_file);
              close (fd);
              return -1;
            }

        }




    }

  ext2fs_file_close (e2_file);
  close (fd);

  if (len < 0)
    {
      perror ("read");
      return -1;
    }


  return 0;

}


int
write_symlink (ext2_filsys fs, char *path, char *buf, ext2_ino_t ino)
{
  char *ptr;
  int ret;

  ext2_file_t e2_file;
  int len;
  size_t writ;

  len = fs->blocksize;

  if ((ret = ext2fs_file_open (fs, ino, EXT2_FILE_WRITE, &e2_file)))
    {
      fprintf (stderr, "ext2fs_file_open(%s) gave %d\n", path, ret);
      return -1;
    }

  for (ptr = buf; len; len -= writ, ptr += writ)
    {

      if ((ret = ext2fs_file_write (e2_file, ptr, len, &writ)))
        {
          fprintf (stderr, "ext2fs_file_write(%s) gave %d\n", path, ret);
          ext2fs_file_close (e2_file);
          return -1;
        }


    }





  ext2fs_file_close (e2_file);

  return 0;

}



int
process_dir (ext2_filsys fs, char *path, ext2_ino_t dir)
{
  ext2_ino_t ino;
  struct ext2_inode inode;
  struct stat stbuf;

  char full_path[PATH_MAX];
  int ret;

  DIR *d;
  struct dirent *ent;
  d = opendir (path);
  if (!d)
    {
      perror ("opendir");
      return -1;
    }

  while ((ent = readdir (d)))
    {

      if (!strcmp (ent->d_name, ".") || !strcmp (ent->d_name, ".."))
        continue;


      strcpy (full_path, path);
      strcat (full_path, "/");
      strcat (full_path, ent->d_name);

      if (lstat (full_path, &stbuf))
        {
          perror ("lstat");
          errs++;
          continue;
        }

      if (verbose)
        printf ("%8o %s\n", stbuf.st_mode, full_path);

      switch (stbuf.st_mode & S_IFMT)
        {
        case S_IFDIR:

          if (ext2fs_namei (fs, EXT2_ROOT_INO, dir, ent->d_name, &ino))
            {
              if ((ret =
                   ext2fs_new_inode (fs, dir,
                                     LINUX_S_IFDIR | (07777 & stbuf.st_mode),
                                     0, &ino)))
                {
                  fprintf (stderr, "ext2fs_new_inode gave %d\n", ret);
                  errs++;
                  continue;
                }


              ret = ext2fs_mkdir (fs, dir, ino, ent->d_name);

              if (ret == EXT2_ET_DIR_NO_SPACE)
                {
                  if ((ret = ext2fs_expand_dir (fs, dir)))
                    {
                      fprintf (stderr, "ext2fs_expand_dir(%s) gave %d\n",
                               path, ret);
                      errs++;
                      continue;
                    }

                  ret = ext2fs_mkdir (fs, dir, ino, ent->d_name);
                }

              if (ret)
                {
                  fprintf (stderr, "ext2fs_mkdir(%s) gave %d\n", full_path,
                           ret);
                  errs++;
                  continue;
                }

              if ((ret = ext2fs_read_inode (fs, ino, &inode)))
                {
                  fprintf (stderr, "ext2fs_read_inode(%s) gave %d\n",
                           full_path, ret);
                  errs++;
                  continue;
                }

              copy_inode_permissions_from_stat (&stbuf, &inode);

              if ((ret = ext2fs_write_inode (fs, ino, &inode)))
                {
                  fprintf (stderr, "ext2fs_write_inode(%s) gave %d\n",
                           full_path, ret);
                  errs++;
                  continue;
                }

              handle_xattr (fs, full_path, ino, &inode);


            }

          process_dir (fs, full_path, ino);

          break;
        case S_IFREG:

          if (make_inode_in_dir
              (fs, path, ent->d_name, dir, &ino, LINUX_S_IFREG | 07777,
               EXT2_FT_REG_FILE))
            {
              errs++;
              continue;
            }

          fill_inode_from_stat (fs, &stbuf, &inode);
          inode.i_mode |= LINUX_S_IFREG;
          if ((ret = ext2fs_write_new_inode (fs, ino, &inode)))
            {
              fprintf (stderr, "ext2fs_write_new_inode(%s/%s) gave %d\n",
                       path, ent->d_name, ret);
              errs++;
              continue;
            }

          handle_xattr (fs, full_path, ino, &inode);

          if (copy_file (fs, full_path, ino))
            {
              errs++;
              continue;
            }


          break;
        case S_IFLNK:
          {
            char *buf;
            int len;
            int slow;

            buf = xmalloc (fs->blocksize);
            memset (buf, 0, fs->blocksize);
            len = readlink (full_path, buf, fs->blocksize);

            if (len < 0)
              {
                perror ("readlink");
                free (buf);
                errs++;
                continue;
              }



            if (len == fs->blocksize)
              {
                fprintf (stderr, "%s/%s symlink destination too long\n", path,
                         ent->d_name);

                free (buf);
                errs++;
                continue;
              }

            len++;

            slow = (len > EXT2_N_BLOCKS * 4);

            if (make_inode_in_dir
                (fs, path, ent->d_name, dir, &ino, LINUX_S_IFLNK | 07777,
                 EXT2_FT_SYMLINK))
              {
                errs++;
                continue;
              }
            fill_inode_from_stat (fs, &stbuf, &inode);
            inode.i_size = len - 1;
            inode.i_mode |= LINUX_S_IFLNK;

            if (!slow)
              memcpy (inode.i_block, buf, len);


            if ((ret = ext2fs_write_new_inode (fs, ino, &inode)))
              {
                fprintf (stderr, "ext2fs_write_new_inode(%s/%s) gave %d\n",
                         path, ent->d_name, ret);

                errs++;
                continue;
              }

            handle_xattr (fs, full_path, ino, &inode);

            if (slow)
              {
                if (write_symlink (fs, full_path, buf, ino))
                  {
                    errs++;
                    continue;
                  }
              }

          }

          break;
        case S_IFCHR:
          {
            int major_num = major (stbuf.st_rdev);
            int minor_num = minor (stbuf.st_rdev);


            if (make_inode_in_dir
                (fs, path, ent->d_name, dir, &ino, LINUX_S_IFCHR | 07777,
                 EXT2_FT_CHRDEV))
              {
                errs++;
                continue;
              }

            fill_inode_from_stat (fs, &stbuf, &inode);

            if ((major_num < 256) && (minor_num < 256))
              {
                inode.i_block[0] = major_num * 256 + minor_num;
                inode.i_block[1] = 0;
              }
            else
              {
                inode.i_block[0] = 0;
                inode.i_block[1] =
                  (minor_num & 0xff) | (major_num << 8) | ((minor_num & ~0xff)
                                                           << 12);
              }


            inode.i_mode |= LINUX_S_IFCHR;
            if ((ret = ext2fs_write_new_inode (fs, ino, &inode)))
              {
                fprintf (stderr, "ext2fs_write_new_inode(%s/%s) gave %d\n",
                         path, ent->d_name, ret);
                errs++;
                continue;
              }

            handle_xattr (fs, full_path, ino, &inode);
          }

          break;
        case S_IFBLK:
          {
            int major_num = major (stbuf.st_rdev);
            int minor_num = minor (stbuf.st_rdev);


            if (make_inode_in_dir
                (fs, path, ent->d_name, dir, &ino, LINUX_S_IFBLK | 07777,
                 EXT2_FT_BLKDEV))
              {
                errs++;
                continue;
              }

            fill_inode_from_stat (fs, &stbuf, &inode);

            if ((major_num < 256) && (minor_num < 256))
              {
                inode.i_block[0] = major_num * 256 + minor_num;
                inode.i_block[1] = 0;
              }
            else
              {
                inode.i_block[0] = 0;
                inode.i_block[1] =
                  (minor_num & 0xff) | (major_num << 8) | ((minor_num & ~0xff)
                                                           << 12);
              }


            inode.i_mode |= LINUX_S_IFBLK;

            if ((ret = ext2fs_write_new_inode (fs, ino, &inode)))
              {
                fprintf (stderr, "ext2fs_write_new_inode(%s/%s) gave %d\n",
                         path, ent->d_name, ret);
                errs++;
                continue;
              }

            handle_xattr (fs, full_path, ino, &inode);
          }


          break;
        case S_IFSOCK:
          fprintf (stderr, "%s is a socket, ignored\n", full_path);
          continue;

        case S_IFIFO:
          fprintf (stderr, "%s is a fifo, ignored\n", full_path);
          continue;

        default:
          errs++;
          fprintf (stderr, "haven't handed %s - unknown d_type %d\n",
                   full_path, ent->d_type);
        }
    }


  closedir (d);
  return 0;
}



static void
usage (void)
{
  fprintf (stderr, "gene3fs -i image_file -d source_dir [-V] [-v]\n");
  exit (1);
}


void
set_inode_xattrs(ext2_filsys fs, char *path, ext2_ino_t ino)
{
  int ret;
  struct ext2_inode inode;
  if ((ret = ext2fs_read_inode (fs, ino, &inode)))
	{
	  fprintf (stderr, "ext2fs_read_inode(%s) gave %d\n",
			   path, ret);
	  errs++;
	  return;
	}
  handle_xattr (fs, path, ino, &inode);
}

int
main (int argc, char *argv[])
{
  char *dflag = NULL;
  char *iflag = NULL;
  int opt;
  int ret;
  int vhd = 0;
  ext2_filsys fs;
  while ((opt = getopt (argc, argv, "Vvd:i:h")) != -1)
    {
      switch (opt)
        {
        case 'd':
          dflag = optarg;
          break;
        case 'i':
          iflag = optarg;
          break;
        case 'v':
          verbose++;
          break;
        case 'V':
          vhd++;
          break;
        default:
          usage ();
        }
    }

  if (!iflag || !dflag)
    usage ();
  if ((ret =
       ext2fs_open (iflag, EXT2_FLAG_RW, 0, 0,
                    vhd ? vhd_io_manager : unix_io_manager, &fs)))
    {
      fprintf (stderr, "ext2fs_open %d\n", ret);
      exit (1);
    }

  if ((ret = ext2fs_read_inode_bitmap (fs)))
    {
      fprintf (stderr, "ext2fs_read_inode_bitmap %d\n", ret);
      exit (1);
    }

  if ((ret = ext2fs_read_block_bitmap (fs)))
    {
      fprintf (stderr, "ext2fs_read_block_bitmap %d\n", ret);
      exit (1);
    }

  // set xattrs for root inode
  set_inode_xattrs(fs, dflag, EXT2_ROOT_INO);
  process_dir (fs, dflag, EXT2_ROOT_INO);
  ext2fs_close (fs);

  return errs ? 1 : 0;
}
