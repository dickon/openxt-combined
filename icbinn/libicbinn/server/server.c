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
#include <sys/statvfs.h>

static char readbuf[ICBINN_PROT_MAXDATA];
static int random_fd = -1;
static int urandom_fd = -1;

static int get_pathname_len (char *path);
static void set_icbinn_prot_type (mode_t entry_mode,
                                  icbinn_prot_type * entry_type);

static int
get_pathname_len (char *path)
{
  int len;

  len = strlen (path);
  if (len >= ICBINN_PROT_MAXPATHLEN)
    {
      error ("pathname too long");
      return -1;
    }
  return len;
}

static void
set_icbinn_prot_type (mode_t entry_mode, icbinn_prot_type * entry_type)
{
  if (S_ISLNK (entry_mode))
    *entry_type = ICBINN_PROT_LINK;
  else if (S_ISDIR (entry_mode))
    *entry_type = ICBINN_PROT_DIRECTORY;
  else if (S_ISREG (entry_mode))
    *entry_type = ICBINN_PROT_FILE;
  else
    *entry_type = ICBINN_PROT_UNKNOWN;
}

void
init_rand (void)
{
  random_fd = open ("/dev/random", O_RDONLY);
  if (random_fd < 0)
    fatal ("failed to open /dev/random: errno %d", errno);

  urandom_fd = open ("/dev/urandom", O_RDONLY);
  if (urandom_fd < 0)
    fatal ("failed to open /dev/urandom: errno %d", errno);
}

void *
icbinn_prot_null_1_svc (void *argp, struct svc_req *rqstp)
{
  static char *result;

  /*
   * insert server code here
   */

  return (void *) &result;
}

icbinn_prot_readdirres *
icbinn_prot_readdir_1_svc (icbinn_prot_readdirargs * argp,
                           struct svc_req *rqstp)
{
  static icbinn_prot_readdirres result;
  struct stat sbuf;
  DIR *dirp;
  struct dirent *dp = NULL;
  struct icbinn_prot_entry *e, **ep;
  unsigned int doffset, dcount;
  static int init = 1;
  unsigned int dname_len;
  char filepath[ICBINN_PROT_MAXPATHLEN + ICBINN_PROT_MAXNAMELEN + 1]; //Add 1 for insertion of separator
  char *fname, *fpath;

  //Free the previous results
  if (init)
    init = 0;
  else if (result.status == ICBINN_PROT_OK)
    xdr_free ((xdrproc_t) xdr_icbinn_prot_readdirres, (caddr_t) & result);

  //Do some pathname checks here if we decide not to use chroot

  result.status = ICBINN_PROT_ERROR;

  dname_len = get_pathname_len (argp->dir);
  if (dname_len < 0)
    return &result;

  if ((stat (argp->dir, &sbuf) < 0))
    {
      error ("failed to stat %s: errno %d", argp->dir, errno);
      return &result;
    }

  if (!(S_ISDIR (sbuf.st_mode)))
    {
      error ("%s is not a directory", argp->dir);
      return &result;
    }

  if ((dirp = opendir (argp->dir)) == NULL)
    {
      error ("failed to open directory %s: errno %d", argp->dir, errno);
      return &result;
    }

  strcpy (filepath, argp->dir);
  if (filepath[dname_len - 1] != '/')
    {
      filepath[dname_len] = '/';
      filepath[dname_len + 1] = '\0';
      dname_len++;
    }

  doffset = dcount = 0;
  ep = &(result.icbinn_prot_readdirres_u.reply.entries);
  while ((dcount < argp->count) && ((dp = readdir (dirp)) != NULL))
    {
      if (!strcmp (dp->d_name, "."))
        continue;
      if (!strcmp (dp->d_name, ".."))
        continue;

      if (strlen (dp->d_name) >= ICBINN_PROT_MAXNAMELEN)
        continue;

      /* Append the entry name to the original path. */
      fpath = &(filepath[dname_len]);
      fname = dp->d_name;
      while (*fname != '\0')
        *fpath++ = *fname++;
      *fpath = '\0';

      if (lstat (filepath, &sbuf) < 0)
        continue;

      //Now we have a valid entry but we need to skip until offset
      if (doffset++ < argp->offset)
        continue;

      e = *ep =
        (struct icbinn_prot_entry *)
        malloc (sizeof (struct icbinn_prot_entry));
      if (e == NULL)
        goto err_mem;

      e->nextentry = NULL;
      e->name = (char *) malloc (strlen (dp->d_name) + 1);
      if (e->name == NULL)
        goto err_mem;

      strcpy (e->name, dp->d_name);
      set_icbinn_prot_type (sbuf.st_mode, &e->type);
      ep = &e->nextentry;
      dcount++;
    }

  *ep = NULL;
  closedir (dirp);
  result.icbinn_prot_readdirres_u.reply.eof = (dp == NULL);
  result.status = ICBINN_PROT_OK;
  return &result;

err_mem:
  /* exit here */
  fatal ("failed to allocate memory");
}

icbinn_prot_statres *
icbinn_prot_stat_1_svc (icbinn_prot_path * argp, struct svc_req * rqstp)
{
  static icbinn_prot_statres result;
  struct stat sbuf;

  result.status = ICBINN_PROT_ERROR;

  if (get_pathname_len (*argp) < 0) {
    result.icbinn_prot_statres_u.errno_code = ENOENT;
    return &result;
  }

  if (lstat (*argp, &sbuf) < 0)
    {
      result.icbinn_prot_statres_u.errno_code = errno;
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  set_icbinn_prot_type (sbuf.st_mode,
                        &result.icbinn_prot_statres_u.reply.type);
  result.icbinn_prot_statres_u.reply.size = (unsigned long long) sbuf.st_size;

  return &result;
}

icbinn_prot_statfsres *
icbinn_prot_statfs_1_svc (icbinn_prot_path *argp, struct svc_req *rqstp)
{
  static icbinn_prot_statfsres result;
  struct statvfs sbuf;

  result.status = ICBINN_PROT_ERROR;

  if (get_pathname_len (*argp) < 0) {
    result.icbinn_prot_statfsres_u.errno_code = ENOENT;
    return &result;
  }

  if (statvfs (*argp, &sbuf) < 0)
    {
      result.icbinn_prot_statfsres_u.errno_code = errno;
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  result.icbinn_prot_statfsres_u.reply.bsize = sbuf.f_bsize;
  result.icbinn_prot_statfsres_u.reply.blocks = sbuf.f_blocks;
  result.icbinn_prot_statfsres_u.reply.bfree = sbuf.f_bfree;
  result.icbinn_prot_statfsres_u.reply.bavail = sbuf.f_bavail;

  return &result;
}

icbinn_prot_openres *
icbinn_prot_open_1_svc (icbinn_prot_openargs * argp, struct svc_req * rqstp)
{
  static icbinn_prot_openres result;
  int flags = 0;
  int fd;

  result.status = ICBINN_PROT_ERROR;

  if (get_pathname_len (argp->name) < 0) {
    result.icbinn_prot_openres_u.errno_code = ENOENT;
    return &result;
  }


  if (argp->mode & ICBINN_PROT_RDONLY)
    flags |= O_RDONLY;

  if (argp->mode & ICBINN_PROT_WRONLY)
    flags |= O_WRONLY;

  if (argp->mode & ICBINN_PROT_RDWR)
    flags |= O_RDWR;

  if (argp->mode & ICBINN_PROT_TRUNC)
    flags |= O_TRUNC;

  if (argp->mode & ICBINN_PROT_CREAT)
    flags |= O_CREAT;

  /* Set permissions to 0600, owned by uid/gid of server. */
  fd = open (argp->name, flags, S_IRUSR | S_IWUSR);
  if (fd < 0)
    {
      result.icbinn_prot_openres_u.errno_code = errno;
      error ("failed to open %s: errno %d", argp->name, errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  result.icbinn_prot_openres_u.reply.fd = fd;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_lock_1_svc (icbinn_prot_lockargs * argp, struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;
  struct flock lock;

  result.status = ICBINN_PROT_ERROR;
  switch (argp->type)
    {
    case ICBINN_PROT_LTYPE_RDLCK:
      lock.l_type = F_RDLCK;
      break;
    case ICBINN_PROT_LTYPE_WRLCK:
      lock.l_type = F_WRLCK;
      break;
    case ICBINN_PROT_LTYPE_UNLCK:
      lock.l_type = F_UNLCK;
      break;
    default:
      result.icbinn_prot_errnostat_u.errno_code = EINVAL;
      error ("invalid lock type %d", argp->type);
      return &result;
    }

  /* Lock the entire file. */
  lock.l_whence = SEEK_SET;
  lock.l_start = 0;
  lock.l_len = 0;

  if (fcntl (argp->fd, F_SETLK, &lock) < 0)
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("fcntl failed: errno %d", errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_readlinkres *
icbinn_prot_readlink_1_svc (icbinn_prot_readlinkargs *argp, struct svc_req *rqstp)
{
  static icbinn_prot_readlinkres result;
  ssize_t retcount;

  result.status = ICBINN_PROT_ERROR;

  if (get_pathname_len (argp->path) < 0) {
    result.icbinn_prot_readlinkres_u.errno_code = ENOENT;
    return &result;
  }

  if (argp->bufsz > ICBINN_PROT_MAXDATA) {
    result.icbinn_prot_readlinkres_u.errno_code = EINVAL;
    error("invalid readlink request");
    return &result;
  }

  retcount = readlink(argp->path, readbuf, (size_t) argp->bufsz);
  if (retcount < 0) {
    result.icbinn_prot_readlinkres_u.errno_code = errno;
    error ("failed to readlink: errno %d", errno);
    return &result;
  }

  result.status = ICBINN_PROT_OK;
  result.icbinn_prot_readlinkres_u.reply.data.data_len = retcount;
  result.icbinn_prot_readlinkres_u.reply.data.data_val = readbuf;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_symlink_1_svc (icbinn_prot_symlinkargs *argp, struct svc_req *rqstp)
{
  static icbinn_prot_errnostat result;
  int ret;

  result.status = ICBINN_PROT_ERROR;

  if (get_pathname_len (argp->oldpath) < 0) {
    result.icbinn_prot_errnostat_u.errno_code = ENOENT;
    return &result;
  }

  if (get_pathname_len (argp->newpath) < 0) {
    result.icbinn_prot_errnostat_u.errno_code = ENOENT;
    return &result;
  }

  ret = symlink(argp->oldpath, argp->newpath);
  if (ret < 0) {
    result.icbinn_prot_errnostat_u.errno_code = errno;
    error ("failed to symlink: errno %d", errno);
    return &result;
  }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_preadres *
icbinn_prot_pread_1_svc (icbinn_prot_preadargs * argp, struct svc_req * rqstp)
{
  static icbinn_prot_preadres result;
  ssize_t retcount;

  result.status = ICBINN_PROT_ERROR;
  if (argp->count > ICBINN_PROT_MAXDATA)
    {
      result.icbinn_prot_preadres_u.errno_code = EINVAL;
      error ("invalid read request");
      return &result;
    }

  retcount =
    pread (argp->fd, readbuf, (size_t) argp->count, (off_t) argp->offset);
  if (retcount < 0)
    {
      result.icbinn_prot_preadres_u.errno_code = errno;
      error ("failed to read from file: errno %d", errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  result.icbinn_prot_preadres_u.reply.data.data_len = retcount;
  result.icbinn_prot_preadres_u.reply.data.data_val = readbuf;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_ftruncate_1_svc (icbinn_prot_ftruncateargs * argp,
                             struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;
  ssize_t retcount;

  result.status = ICBINN_PROT_ERROR;

  if (ftruncate (argp->fd, (off_t) argp->length))
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("ftruncate failed: errno %d", errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_fallocate_1_svc (icbinn_prot_fallocateargs * argp,
                             struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  result.status = ICBINN_PROT_ERROR;

  if (fallocate64 (argp->fd, argp->mode, argp->offset, argp->length))
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("fallocate failed: errno %d", errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_pwrite_1_svc (icbinn_prot_pwriteargs * argp,
                          struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;
  ssize_t retcount;

  result.status = ICBINN_PROT_ERROR;
  if (argp->data.data_len > ICBINN_PROT_MAXDATA)
    {
      result.icbinn_prot_errnostat_u.errno_code = EINVAL;
      error ("invalid write request");
      return &result;
    }

  retcount =
    pwrite (argp->fd, argp->data.data_val, (size_t) argp->data.data_len,
            (off_t) argp->offset);
  if (retcount < 0)
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to write to file: errno %d", errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_close_1_svc (icbinn_prot_fd * argp, struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  if (close (*argp) < 0)
    {
      result.status = ICBINN_PROT_ERROR;
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to close file: errno %d", errno);
    }
  else
    result.status = ICBINN_PROT_OK;

  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_fsync_1_svc (icbinn_prot_fd * argp, struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  if (fsync (*argp) < 0)
    {
      result.status = ICBINN_PROT_ERROR;
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to fsync file: errno %d", errno);
    }
  else
    result.status = ICBINN_PROT_OK;

  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_mkdir_1_svc (icbinn_prot_path * argp, struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  result.status = ICBINN_PROT_ERROR;
  if (get_pathname_len (*argp) < 0) {
    result.icbinn_prot_errnostat_u.errno_code = ENOENT;
    return &result;
  }

  /* Set permissions to 0700, owned by uid of server. */
  if (mkdir (*argp, S_IRWXU) < 0)
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to create directory %s: errno %d", *argp, errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_rmdir_1_svc (icbinn_prot_path * argp, struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  result.status = ICBINN_PROT_ERROR;
  if (get_pathname_len (*argp) < 0) {
    result.icbinn_prot_errnostat_u.errno_code = ENOENT;
    return &result;
  }

  if (rmdir (*argp) < 0)
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to remove directory %s: errno %d", *argp, errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_rename_1_svc (icbinn_prot_renameargs * argp,
                          struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  result.status = ICBINN_PROT_ERROR;
  if ((get_pathname_len (argp->from) < 0)
      || (get_pathname_len (argp->to) < 0)) {
    result.icbinn_prot_errnostat_u.errno_code = ENOENT;
    return &result;
  }

  if (rename (argp->from, argp->to) < 0)
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to rename %s: errno %d", argp->from, errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_errnostat *
icbinn_prot_unlink_1_svc (icbinn_prot_path * argp, struct svc_req * rqstp)
{
  static icbinn_prot_errnostat result;

  result.status = ICBINN_PROT_ERROR;
  if (get_pathname_len (*argp) < 0) {
    result.icbinn_prot_errnostat_u.errno_code = ENOENT;
    return &result;
  }

  if (unlink (*argp) < 0)
    {
      result.icbinn_prot_errnostat_u.errno_code = errno;
      error ("failed to unlink %s: errno %d", *argp, errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  return &result;
}

icbinn_prot_randres *
icbinn_prot_rand_1_svc (icbinn_prot_randargs * argp, struct svc_req * rqstp)
{
  static icbinn_prot_randres result;
  int fd;
  ssize_t retcount, n;

  result.status = ICBINN_PROT_ERROR;
  if (argp->count > ICBINN_PROT_MAXDATA)
    {
      error ("invalid read request");
      return &result;
    }

  switch (argp->src)
    {
    case ICBINN_PROT_RANDOM:
      fd = random_fd;
      break;
    case ICBINN_PROT_URANDOM:
      fd = urandom_fd;
      break;
    default:
      error ("invalid random source %d", argp->src);
      return &result;
    }

  retcount = read (fd, readbuf, (size_t) argp->count);
  if (retcount < 0)
    {
      error ("failed to read from random device: errno %d", errno);
      return &result;
    }

  result.status = ICBINN_PROT_OK;
  result.icbinn_prot_randres_u.reply.data.data_len = retcount;
  result.icbinn_prot_randres_u.reply.data.data_val = readbuf;
  return &result;
}
