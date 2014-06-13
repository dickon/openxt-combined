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

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <icbinn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <stdarg.h>

#define MAX_SYMLINK_SIZE 512

typedef struct icbinn_data {
  ICBINN *icb;
} icbinn_data_t;

static
ICBINN *get_icb()
{
  return ((icbinn_data_t*)fuse_get_context()->private_data)->icb;
}

#ifdef DEBUG
void debug(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vsyslog(LOG_DEBUG, format, ap);
}
#else
#define debug
#endif

/* convert open flags from open() to icbinn style */
static
int translate_open_flags(int f)
{
  int xf = 0;
  if (f & O_CREAT)
    xf |= ICBINN_CREAT;
  if (f & O_TRUNC)
    xf |= ICBINN_TRUNC;

  if (f & O_WRONLY)
    xf |= ICBINN_WRONLY;
  if (f & O_RDWR)
    xf |= ICBINN_RDWR;
  if (f & O_RDONLY)
    xf |= ICBINN_RDONLY; /* it's 0, so no-op */
  return xf;
      
}

/* convert flock flags from open() to icbinn style */
static
int translate_flock_flags(int f)
{
  int xf = 0;
  if (f & LOCK_SH)
    xf |= ICBINN_LTYPE_RDLCK;
  else if (f & LOCK_EX)
    xf |= ICBINN_LTYPE_WRLCK;
  else if (f & LOCK_UN)
    xf |= ICBINN_LTYPE_UNLCK;
  return xf;
}

static
int icbinnfs_getattr(const char *path, struct stat *stbuf)
{
  struct icbinn_stat ist;
  int rv = icbinn_stat(get_icb(), path, &ist);
  debug("getattr %s = %d", path, rv);
  if (rv < 0)
    return rv;
  memset(stbuf, 0, sizeof(struct stat));
  if (ist.type == ICBINN_TYPE_LINK) {
    stbuf->st_mode = S_IFLNK | 0644;
  } else if (ist.type == ICBINN_TYPE_FILE) {
    stbuf->st_mode = S_IFREG | 0644;
  } else if (ist.type == ICBINN_TYPE_DIRECTORY) {
    stbuf->st_mode = S_IFDIR | 0755;
  } else {
    return -ENOENT;
  }
  stbuf->st_size = ist.size;
  return 0;
}


static
int icbinnfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		     struct fuse_file_info *fi)
{
  ICBINN *icb = get_icb();
  int i = 0;
  debug("readdir %s", path);
  struct icbinn_dirent de;
  while (!icbinn_readent(icb, path, i++, &de)) {
    char e[256];
    struct stat st;
    snprintf(e, sizeof(e), "%s/%s", path, de.name);
    int statrv = icbinnfs_getattr(e, &st);

    if (filler(buf, de.name, statrv != 0 ? NULL : &st, 0) != 0) {
      return -ENOMEM;
    }
  }
  return 0;
}

static
int icbinnfs_open(const char *path, struct fuse_file_info *fi)
{
  int fd = icbinn_open(get_icb(), path, translate_open_flags(fi->flags));
  debug("open %s %d = %d", path, fi->flags, fd);
  if (fd >= 0) {
    fi->fh = fd;
    return 0;
  }
  return fd;
}

static
int icbinnfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  int fd = icbinn_open(get_icb(), path, ICBINN_CREAT|ICBINN_TRUNC|ICBINN_WRONLY);
  debug("create %s %d = %d", path, fi->flags, fd);
  if (fd >= 0) {
    fi->fh = fd;
    return 0;
  }
  return fd;
}

static
int icbinnfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
  int rv = icbinn_pread(get_icb(), fi->fh, buf, size, offset);
  debug("pread %s %d %d = %d", path, size, offset, rv);
  return rv;
}

static
int icbinnfs_write(const char *path, const char *buf, size_t size, off_t offset,
		   struct fuse_file_info *fi)
{
  int rv = icbinn_pwrite(get_icb(), fi->fh, buf, size, offset);
  debug("pwrite %s %d %d = %d", path, size, offset, rv);
  return rv;
}

static
int icbinnfs_truncate(const char *path, off_t offset)
{
  int rv = 0;
  ICBINN *icb = get_icb();
  debug("truncate %s", path);
  int fd = icbinn_open(icb, path, ICBINN_WRONLY);
  if (fd < 0)
    return fd;
  rv = icbinn_ftruncate(icb, fd, offset);
  icbinn_close(icb, fd);
  return rv;
}

static
int icbinnfs_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
  int rv = icbinn_ftruncate(get_icb(), fi->fh, offset);
  debug("ftruncate %s %d = %d", path, offset, rv);
  return rv;
}

static
int icbinnfs_unlink(const char *path)
{
  int rv = icbinn_unlink(get_icb(), path);
  debug("unlink %s = %d", path, rv);
  return rv;
}

static
int icbinnfs_mkdir(const char *path, mode_t mode)
{
  int rv = icbinn_mkdir(get_icb(), path);
  debug("mkdir %s = %d", path, rv);
  return rv;
}

static
int icbinnfs_rmdir(const char *path)
{
  int rv = icbinn_rmdir(get_icb(), path);
  debug("rmdir %s = %d", path, rv);
  return rv;
}

static
int icbinnfs_rename(const char *old, const char *new)
{
  int rv = icbinn_rename(get_icb(), old, new);
  debug("rename %s %s = %d", old, new, rv);
  return rv;
}

static
int icbinnfs_release(const char *path, struct fuse_file_info *fi)
{
  int rv = icbinn_close(get_icb(), fi->fh);
  debug("close %s = %d", path, rv);
  return rv;
}

static
int icbinnfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi)
{
  int rv = icbinn_fsync(get_icb(), fi->fh);
  debug("fsync %s = %d", path, rv);
  return rv;
}

static
int icbinnfs_utimens(const char *path, const struct timespec tv[2])
{
  /* not supported by server but needed */
  return 0;
}

#ifndef ENOATTR
# define ENOATTR ENODATA        /* No such attribute */
#endif

static
int icbinnfs_getxattr(const char *path, const char *name, char *value, size_t len)
{
  /* TODO: implement real version. Blktap uses extended attribute to check if encryption
   * is setup on vhd */
  return -ENOATTR;
}

static
int icbinnfs_symlink(const char *oldpath, const char *newpath)
{
  int rv = icbinn_symlink(get_icb(), oldpath, newpath);
  debug("symlink %s %s = %d", oldpath, newpath, rv);
  return rv;
}

static
ssize_t icbinnfs_readlink(const char *path, char *buf, size_t bufsz)
{
  char _buf[MAX_SYMLINK_SIZE];
  ssize_t rv = icbinn_readlink(get_icb(), path, _buf, sizeof(_buf));
  debug("readlink %s = %d", path, bufsz);
  if (rv >= 0) {
    if (rv < MAX_SYMLINK_SIZE)
      _buf[rv] = 0;
    else
      _buf[MAX_SYMLINK_SIZE-1] = 0;
    strncpy(buf, _buf, bufsz);
    return 0;
  }
  return rv;
}

static
int icbinnfs_statfs(const char *path, struct statvfs *buf)
{
  struct icbinn_statfs st;
  int rv = icbinn_statfs(get_icb(), path, &st);
  debug("statfs %s = %d", path, rv);
  if (rv == 0) {
    memset(buf, 0, sizeof(struct statvfs));
    buf->f_bsize = st.bsize;
    buf->f_blocks = st.blocks;
    buf->f_bfree = st.bfree;
    buf->f_bavail = st.bavail;
  }
  return rv;
}

static
int icbinnfs_fallocate(const char *path, int mode, off_t offset, off_t len, struct fuse_file_info *fi)
{
	int rv = icbinn_fallocate(get_icb(), fi->fh, mode, offset, len);
	debug("fallocate %s = %d", path, rv);
	return rv;
}

#if FUSE_USE_VERSION >= 29
static
int icbinnfs_flock(const char *path, struct fuse_file_info *fi, int op)
{
  return icbinn_lock(get_icb(), fi->fh, translate_flock_flags(op));
}
#endif

static struct fuse_operations icbinnfs_oper = {
#if FUSE_USE_VERSION >= 29
    .flock     = icbinnfs_flock,
#endif

    .getattr   = icbinnfs_getattr,
    .getxattr  = icbinnfs_getxattr,
    .getdir    = NULL, /* deprecated */
    .readdir   = icbinnfs_readdir,
    .create    = icbinnfs_create,
    .open      = icbinnfs_open,
    .read      = icbinnfs_read,
    .write     = icbinnfs_write,
    .truncate  = icbinnfs_truncate,
    .ftruncate = icbinnfs_ftruncate,
    .fallocate = icbinnfs_fallocate,
    .unlink    = icbinnfs_unlink,
    .mkdir     = icbinnfs_mkdir,
    .rmdir     = icbinnfs_rmdir,
    .utimens   = icbinnfs_utimens,
    .rename    = icbinnfs_rename,
    .readlink  = icbinnfs_readlink,
    .symlink   = icbinnfs_symlink,
    .statfs    = icbinnfs_statfs,
    .fsync     = icbinnfs_fsync,
    .release   = icbinnfs_release
};

static
void usage(char *name)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s {-d domid|-h host} [-p port] mountpoint [fuseargs]\n", name);
  exit(1);
}

int main(int argc, char *argv[])
{
  int rv, c, i, dflag=0, hflag=0, domid=0, port=0;
  char *host = NULL;
  ICBINN *icb;
  icbinn_data_t data = { 0 };
  while ((c = getopt (argc, argv, "+d:p:h:")) != EOF)
  {
    switch (c)
    {
    case 'd':
      domid = atoi (optarg);
      dflag++;
      break;
    case 'p':
      port = atoi (optarg);
      break;
    case 'h':
      host = optarg;
      hflag++;
      break;
    default:
      break;
    }
  }

  if (!dflag ^ !!hflag)
    usage (argv[0]);
  int fuseargc = 1+argc-(optind-1);
  char **fuseargv = calloc(fuseargc, sizeof(char*));
  fuseargv[0] = argv[0];
  fuseargv[1] = "-s"; /* run single threaded, icbinn access is not safe otherwise */
  for (i = 1; i < fuseargc-1; ++i) {
    fuseargv[1+i] = argv[i+optind-1];
  }

  if (dflag)
    icb = icbinn_clnt_create_v4v(domid, port);
  else
    icb = icbinn_clnt_create_tcp(host, port);
  if (!icb) {
    fprintf(stderr, "failed to connect to server\n");
    return 1;
  }

  data.icb = icb;
  openlog(argv[0], 0, LOG_DAEMON);
#ifdef DEBUG
  char cmd[512] = { 0 };
  for (i = 0; i < fuseargc; ++i) {
    strcat(cmd, fuseargv[i]);
    strcat(cmd, " ");
  }
  debug("starting icbinn filesystem... fuse cmdline=%s", cmd);
#endif
  rv = fuse_main(fuseargc, fuseargv, &icbinnfs_oper, &data);
  icbinn_clnt_destroy(icb);
  return rv;
}
