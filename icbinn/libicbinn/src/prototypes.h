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

/* client.c */
ICBINN *icbinn_clnt_create_tcp(const char *host, int port);
ICBINN *icbinn_clnt_create_v4v(int domid, int port);
void icbinn_clnt_destroy(ICBINN *icb);
/* close.c */
int icbinn_close(ICBINN *icb, int fd);
/* lock.c */
int icbinn_lock(ICBINN *icb, int fd, int type);
/* mkdir.c */
int icbinn_mkdir(ICBINN *icb, const char *path);
/* null.c */
int icbinn_null(ICBINN *icb);
/* open.c */
int icbinn_open(ICBINN *icb, const char *name, int mode);
/* pread.c */
ssize_t icbinn_pread(ICBINN *icb, int fd, void *_buf, size_t count, uint64_t offset);
/* pwrite.c */
ssize_t icbinn_pwrite(ICBINN *icb, int fd, const void *_buf, size_t count, uint64_t offset);
/* readdir.c */
ssize_t icbinn_readdir(ICBINN *icb, const char *dir, size_t offset, size_t count, struct icbinn_dirent *buf);
int icbinn_readent(ICBINN *icb, const char *dir, size_t offset, struct icbinn_dirent *buf);
/* rename.c */
int icbinn_rename(ICBINN *icb, const char *from, const char *to);
/* rmdir.c */
int icbinn_rmdir(ICBINN *icb, const char *path);
/* stat.c */
int icbinn_stat(ICBINN *icb, const char *path, struct icbinn_stat *buf);
/* unlink.c */
int icbinn_unlink(ICBINN *icb, const char *path);
/* util.c */
void *xmalloc(size_t size);
/* version.c */
char *libicbinn_get_version(void);
/* ftruncate.c */
int icbinn_ftruncate(ICBINN *icb, int fd, uint64_t length);
/* fallocate.c */
int icbinn_fallocate(ICBINN *icb, int fd, int mode, uint64_t offset, uint64_t length);
/* rand.c */
ssize_t icbinn_rand(ICBINN *icb, int src, void *_buf, size_t count);
/* readlink.c */
ssize_t icbinn_readlink(ICBINN *icb, const char *path, char *buf, size_t bufsz);
/* symlink.c */
int icbinn_symlink(ICBINN *icb, const char *oldpath, const char *newpath);
/* statfs.c */
int icbinn_statfs(ICBINN *icb, const char *path, struct icbinn_statfs *buf);
/* fsync.c */
int icbinn_fsync(ICBINN *icb, int fd);
/* canary.c */
void icbinn_canary(void);
