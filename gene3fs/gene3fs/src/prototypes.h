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

/* gene3fs.c */
void copy_inode_permissions_from_stat(struct stat *stbuf, struct ext2_inode *inode);
void fill_inode_from_stat(ext2_filsys fs, struct stat *stbuf, struct ext2_inode *inode);
int make_inode_in_dir(ext2_filsys fs, char *path, char *leaf, ext2_ino_t dir, ext2_ino_t *ino, int mode, int type);
int copy_file(ext2_filsys fs, char *path, ext2_ino_t ino);
int write_symlink(ext2_filsys fs, char *path, char *buf, ext2_ino_t ino);
int process_dir(ext2_filsys fs, char *path, ext2_ino_t dir);
int main(int argc, char *argv[]);
/* version.c */
/* adler32.c */
uint32_t adler32(uint32_t adler, void *_buf, size_t len);
/* util.c */
int errs;
int verbose;
void *xmalloc(size_t size);
void *xcalloc(size_t n, size_t size);
/* xattr.c */
void handle_xattr(ext2_filsys fs, char *path, ext2_ino_t ino, struct ext2_inode *inode);
/* vhdio.c */
io_manager vhd_io_manager;
