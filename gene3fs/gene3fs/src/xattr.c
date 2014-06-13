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

#include "project.h"
#include "xattr.h"


typedef struct xattr
{
  struct xattr *next;
  blk_t blk;
  int refcount;
  int n;
  char **names;
  void **values;
  size_t *lens;
  uint32_t hash;
} xattr;

const static struct prefix_struct
{
  const char *prefix;
  size_t prefix_len;
  uint8_t index;
} prefix_list[] =
{
  {
  XATTR_SECURITY_PREFIX, XATTR_SECURITY_PREFIX_LEN,
      EXT3_XATTR_INDEX_SECURITY},
  {
  XATTR_TRUSTED_PREFIX, XATTR_TRUSTED_PREFIX_LEN, EXT3_XATTR_INDEX_TRUSTED},
  {
  XATTR_USER_PREFIX, XATTR_USER_PREFIX_LEN, EXT3_XATTR_INDEX_USER},
  {
  NULL, 0, 0}
};

static xattr *xattr_list = NULL; //Should use the hash rather than linear search


static int
name_cmp (const void *p1, const void *p2)
{
  return strcmp (*(char *const *) p1, *(char *const *) p2);
}



static void
xattr_free (xattr * a)
{
  int i;
  for (i = 0; i < a->n; ++i)
    {
      free (a->names[i]);
      if (a->values[i])
        free (a->values[i]);
    }

  free (a->names);
  free (a->values);
  free (a->lens);

  free (a);
}


/* This is unnecessarily painful - get a sorted list of all attributes and values for a pathname*/
static xattr *
xattr_read (char *path)
{
  xattr *ret;
  char *list;
  char *ptr;
  int i, j;
  ssize_t len;

  len = llistxattr (path, NULL, 0);

  if (len == 0)
    return 0;

  if (len < 0)
    {
      fprintf (stderr, "llistxattr(%s) returned %d\n", path, errno);
      if (errno != EOPNOTSUPP) errs++;
      return NULL;
    }


  list = xmalloc (len);

  len = llistxattr (path, list, len);

  if (len == 0)
    return NULL;
  if (len < 0)
    {
      fprintf (stderr, "llistxattr(%s) returned %d\n", path, errno);
      free (list);
      if (errno != EOPNOTSUPP) errs++;
      return NULL;
    }

  ret = xmalloc (sizeof (xattr));
  ret->hash = 0;
  ret->refcount = 0;
  ret->next = NULL;
  ret->blk = 0;

  for (i = len, ptr = list, j = 0; i; i--, ptr++)
    {
      if (!*ptr)
        j++;
    }

  ret->n = j;

  ret->names = xcalloc (ret->n, sizeof (char *));
  ret->values = xcalloc (ret->n, sizeof (void *));
  ret->lens = xcalloc (ret->n, sizeof (size_t));

  j = 0;
  for (i = len, ptr = list; i; i--, ptr++)
    {
      if (!ret->names[j])
        ret->names[j] = strdup (ptr);
      if (!*ptr)
        j++;
    }

  free (list);

  qsort (ret->names, ret->n, sizeof (char *), name_cmp);

  for (i = 0; i < ret->n; ++i)
    {

      ret->hash = adler32 (ret->hash, ret->names[i], strlen (ret->names[i]));

      len = lgetxattr (path, ret->names[i], NULL, 0);
      if (len < 0)
        {
          fprintf (stderr, "lgetxattr(%s,%s) returned %d\n", path,
                   ret->names[i], errno);
          xattr_free (ret);
          errs++;
          return NULL;
        }

      if (len)
        ret->values[i] = xmalloc (len);

      ret->lens[i] = len;

      len = lgetxattr (path, ret->names[i], ret->values[i], ret->lens[i]);

      ret->hash = adler32 (ret->hash, ret->values[i], ret->lens[i]);

      if (len < 0)
        {
          fprintf (stderr, "lgetxattr(%s,%s,%p,%d) returned %d\n", path,
                   ret->names[i], ret->values[i], ret->lens[i], errno);
          xattr_free (ret);
          errs++;
          return NULL;
        }
    }
  return ret;
}

static int
xattr_equal (xattr * a, xattr * b) //FIXME should use a hash of all values to do a quick compare
{
  int i;
  if (a->hash != b->hash)
    return 0;
  if (a->n != b->n)
    return 0;

  for (i = 0; i < a->n; ++i)
    {
      if (a->lens[i] != b->lens[i])
        return 0;
      if (strcmp (a->names[i], b->names[i]))
        return 0;
      if (memcmp (a->values[i], b->values[i], a->lens[i]))
        return 0;
    }

  return 1;
}

static xattr *
xattr_find (xattr * new)
{
  xattr *a;

  if (!new)
    return NULL;

  for (a = xattr_list; a; a = a->next)
    {
      if (xattr_equal (a, new))
        {
          return a;
        }
    }

  return NULL;
}

void
handle_xattr (ext2_filsys fs, char *path, ext2_ino_t ino,
              struct ext2_inode *inode)
{
  xattr *a, *b = xattr_read (path);
  struct ext2_inode inode_buf;
  char *buf;
  int i;
  int ptr;
  int value_ptr;
  int ret;


  if (!b)
    return;

  if (!inode) {
	inode=&inode_buf;

              if ((ret = ext2fs_read_inode (fs, ino, inode)))
                {
                  fprintf (stderr, "ext2fs_read_inode(%s) gave %d\n",
                           path, ret);
                  errs++;
		return;
                }
  }
	

  buf = xcalloc (1, fs->blocksize);
  ptr = 0;
  value_ptr = fs->blocksize;    /*There's a bug in e2fsck which means that it won't let you use the last byte in an EA block */

  a = xattr_find (b);


  if (a && a->refcount < EXT3_XATTR_REFCOUNT_MAX)
    {
      /*We've already got a block with these EA already so we just add it to the inode and bump the refcount */
      /*If we've hit the refcount max, then we'll take the other branch and stick another matching block ahead in the list */

      a->refcount++;

      xattr_free (b);


      ext2fs_adjust_ea_refcount (fs, a->blk, buf, +1, NULL);

      inode->i_file_acl = a->blk;
      inode->i_blocks += fs->blocksize / 512;
    }
  else
    {
      struct ext2_ext_attr_header *h =
        (struct ext2_ext_attr_header *) &buf[ptr];

      ptr += sizeof (struct ext2_ext_attr_header);

/* new attributes - add it to the list then start the miserable process of making an attribute block [hilariously the kernel code does EIO if h_block!= 1]*/

      b->refcount++;

      h->h_magic = EXT3_XATTR_MAGIC;
      h->h_refcount = b->refcount;
      h->h_blocks = 1;

      for (i = 0; i < b->n; ++i)
        {
          struct ext2_ext_attr_entry *e =
            (struct ext2_ext_attr_entry *) &buf[ptr];

          const struct prefix_struct *p;
          char *name;
          int name_len;


          for (p = prefix_list; p->prefix; p++)
            {
              if (!strncmp (p->prefix, b->names[i], p->prefix_len))
                break;
            }

          if (!p->prefix)
            {
              fprintf (stderr,
                       "Can't find an xattr prefix to match %s for path %s\n",
                       b->names[i], path);
              errs++;
              free (buf);
              xattr_free (b);
              return;
            }

          name = b->names[i] + p->prefix_len;
          name_len = strlen (name);
          value_ptr -= EXT2_EXT_ATTR_SIZE (b->lens[i]);

          if ((ptr + EXT2_EXT_ATTR_LEN (name_len) + 4) > value_ptr)
            {
              fprintf (stderr,
                       "too much extended attribute data for path %s\n",
                       path);
              errs++;
              free (buf);
              xattr_free (b);
              return;
            }

          memcpy (&buf[value_ptr], b->values[i], b->lens[i]);
          memcpy (&buf[ptr + sizeof (struct ext2_ext_attr_entry)], name,
                  name_len);

          e->e_name_len = name_len;
          e->e_name_index = p->index;
          e->e_value_offs = value_ptr;
          e->e_value_block = 0;
          e->e_value_size = b->lens[i];
          //e->e_hash = 0;
          e->e_hash = ext2fs_ext_attr_hash_entry (e, &buf[value_ptr]);

          ptr += EXT2_EXT_ATTR_LEN (name_len);

        }

/*We know there are at least 4 zero bytes to terminate the list */

/*next we have to find a block to stash this*/

      if ((ret = ext2fs_alloc_block (fs, 0, NULL, &b->blk)))
        {
          fprintf (stderr, "ext2fs_alloc_block failed %d\n", ret);
          errs++;
          free (buf);
          xattr_free (b);
          return;
        }

      ext2fs_mark_block_bitmap (fs->block_map, b->blk);



      if ((ret = ext2fs_write_ext_attr (fs, b->blk, buf)))
        {
          fprintf (stderr, "ext2fs_write_ext_attr failed %d\n", ret);
          free (buf);
          xattr_free (b);
          return;
        }

      b->next = xattr_list;
      xattr_list = b;

      inode->i_file_acl = b->blk;
      inode->i_blocks += fs->blocksize / 512;
    }
  free (buf);


  if ((ret = ext2fs_write_inode (fs, ino, inode)))
    {
      fprintf (stderr, "ext2fs_write_inode(%s) gave %d\n", path, ret);
      errs++;
    }


  return;
}
