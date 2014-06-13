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

/*
 * test.c:
 *
 */

static char rcsid[] = "$Id:$";

#include "project.h"

static int
test_null (ICBINN * icb)
{
  return icbinn_null (icb);
}

static int
test_cleanup (ICBINN * icb)
{
  struct icbinn_stat buf;
  icbinn_rmdir (icb, "icbinntest/dir1");
  icbinn_rmdir (icb, "icbinntest/dir2");
  icbinn_rmdir (icb, "icbinntest/dir3");
  icbinn_rmdir (icb, "icbinntest/dir4");
  icbinn_unlink (icb, "icbinntest/file1");
  icbinn_unlink (icb, "icbinntest/file2");
  icbinn_unlink (icb, "icbinntest/file3");
  icbinn_unlink (icb, "icbinntest/file4");
  icbinn_rmdir (icb, "icbinntest");


  if (!icbinn_stat (icb, "icbinntest", &buf))
    return -1;

  if (icbinn_mkdir (icb, "icbinntest"))
    return -1;

  return 0;
}


static int
test_mkdir (ICBINN * icb)
{
  if (icbinn_mkdir (icb, "icbinntest/dir1"))
    return -1;
  if (!icbinn_mkdir (icb, "icbinntest/dir1"))
    return -1;

  return 0;
}


static int
test_rmdir (ICBINN * icb)
{
  if (icbinn_rmdir (icb, "icbinntest/dir1"))
    return -1;
  if (!icbinn_rmdir (icb, "icbinntest/dir1"))
    return -1;

  return 0;
}

static int
test_renamedir (ICBINN * icb)
{

  if (icbinn_mkdir (icb, "icbinntest/dir1"))
    return -1;
  if (icbinn_rename (icb, "icbinntest/dir1", "icbinntest/dir2"))
    return -1;
  if (!icbinn_rename (icb, "icbinntest/dir1", "icbinntest/dir2"))
    return -1;
  if (icbinn_rmdir (icb, "icbinntest/dir2"))
    return -1;

  return 0;
}

static int
test_fileio (ICBINN * icb)
{
  int fd;
  int ret = 0;
  char buf[4];

  fd =
    icbinn_open (icb, "icbinntest/file1",
                 ICBINN_RDWR | ICBINN_CREAT | ICBINN_TRUNC);
  if (fd < 0)
    return -1;

  if (icbinn_pwrite (icb, fd, "fish", 4, 0) != 4)
    return -1;

  if (icbinn_pwrite (icb, fd, "soup", 4, 1024) != 4)
    return -1;

  if (icbinn_pread (icb, fd, buf, 4, 0) != 4)
    return -1;

  if (memcmp (buf, "fish", 4))
    return -1;

  if (icbinn_pread (icb, fd, buf, 4, 4) != 4)
    return -1;

  if (memcmp (buf, "\0\0\0\0", 4))
    return -1;

  if (icbinn_pread (icb, fd, buf, 4, 1024) != 4)
    return -1;

  if (memcmp (buf, "soup", 4))
    return -1;

  if (icbinn_pread (icb, fd, buf, 4, 1028) != 0)
    return -1;

  icbinn_close (icb, fd);

  return 0;
}

static int
test_renamefile (ICBINN * icb)
{

  if (icbinn_rename (icb, "icbinntest/file1", "icbinntest/file2"))
    return -1;
  if (!icbinn_rename (icb, "icbinntest/file1", "icbinntest/file2"))
    return -1;

  return 0;
}

static int
test_unlink (ICBINN * icb)
{

  if (icbinn_unlink (icb, "icbinntest/file2"))
    return -1;
  if (!icbinn_unlink (icb, "icbinntest/file2"))
    return -1;

  return 0;
}

static int
test_ftruncate (ICBINN * icb)
{
  int fd;

  fd =
    icbinn_open (icb, "icbinntest/file2",
                 ICBINN_RDWR | ICBINN_CREAT | ICBINN_TRUNC);
  if (fd < 0)
    return -1;

  if (icbinn_ftruncate (icb, fd, 1234567890))
    return -1;

  icbinn_close (icb, fd);

  return 0;
}

static int
test_stat (ICBINN * icb)
{
  struct icbinn_stat buf;

  if (icbinn_stat (icb, "icbinntest/file1", &buf))
    return -1;

  if (buf.type != ICBINN_TYPE_FILE)
    return -1;

  if (buf.size != 1028)
    return -1;

  if (icbinn_stat (icb, "icbinntest/file2", &buf))
    return -1;


  if (buf.type != ICBINN_TYPE_FILE)
    return -1;

  if (buf.size != 1234567890)
    return -1;


  if (icbinn_stat (icb, "icbinntest/dir1", &buf))
    return -1;


  if (buf.type != ICBINN_TYPE_DIRECTORY)
    return -1;



  return 0;
}


static int
test_lock (ICBINN * icb)
{
  int fd;

  fd = icbinn_open (icb, "icbinntest/file2", ICBINN_RDWR);
  if (fd < 0)
    return -1;

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_RDLCK))
    return -1;

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_WRLCK))
    return -1;

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_UNLCK))
    return -1;

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_WRLCK))
    return -1;

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_RDLCK))
    return -1;

  if (icbinn_lock (icb, fd, ICBINN_LTYPE_UNLCK))
    return -1;

  icbinn_close (icb, fd);

  return 0;
}

static int
compare (const void *_a, const void *_b)
{
  struct icbinn_dirent *a = (struct icbinn_dirent *) _a;
  struct icbinn_dirent *b = (struct icbinn_dirent *) _b;

  return strcmp (a->name, b->name);
}

static int
test_readdir (ICBINN * icb)
{
  struct icbinn_dirent buf[4];

  if (icbinn_readent (icb, "icbinntest", 0, &buf[0]))
    return -1;
  if (icbinn_readent (icb, "icbinntest", 1, &buf[1]))
    return -1;
  if (icbinn_readent (icb, "icbinntest", 2, &buf[2]))
    return -1;
  if (!icbinn_readent (icb, "icbinntest", 3, &buf[3]))
    return -1;

  qsort (&buf[0], 3, sizeof (struct icbinn_dirent), compare);

  if (strcmp (buf[0].name, "dir1"))
    return -1;
  if (buf[0].type != ICBINN_TYPE_DIRECTORY)
    return -1;

  if (strcmp (buf[1].name, "file1"))
    return -1;
  if (buf[1].type != ICBINN_TYPE_FILE)
    return -1;

  if (strcmp (buf[2].name, "file2"))
    return -1;
  if (buf[2].type != ICBINN_TYPE_FILE)
    return -1;

  return 0;
}

static int
test_rand (ICBINN * icb)
{
  int fd;
  int n;
  char buf[4];

  n = icbinn_rand (icb, ICBINN_RANDOM, buf, 4);
  if (n < 0 || n > 4)
    return -1;

  n = icbinn_rand (icb, ICBINN_URANDOM, buf, 4);
  if (n < 0 || n > 4)
    return -1;

  return 0;
}

#define TEST(a) do { if (a(icb)) { fprintf(stderr,"%s failed\n",#a); return -1; } } while (0)


static int
test (ICBINN * icb)
{
  TEST (test_null);
  TEST (test_cleanup);
  TEST (test_mkdir);
  TEST (test_rmdir);
  TEST (test_renamedir);
  TEST (test_fileio);
  TEST (test_renamefile);
  TEST (test_unlink);
  TEST (test_ftruncate);
  TEST (test_mkdir);
  TEST (test_fileio);
  TEST (test_stat);
  TEST (test_lock);
  TEST (test_readdir);
  TEST (test_rand);
  TEST (test_cleanup);

  printf ("All tests pass\n");
  return 0;
}

static void
usage (char *name)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "%s {-d domid|-h host} [-p port]\n", name);
  exit (1);
}


int
main (int argc, char *argv[])
{
  static char line[1024];
  int c;
  int domid = 0;
  int port = 0;
  char *host = NULL;
  char *cmd = NULL;
  int dflag = 0, hflag = 0;
  ICBINN *icb;

  int ret;



  while ((c = getopt (argc, argv, "d:p:h:")) != EOF)
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
          usage (argv[0]);
        }
    }

  if (!dflag ^ !!hflag)
    usage (argv[0]);

  if (dflag)
    icb = icbinn_clnt_create_v4v (domid, port);
  else
    icb = icbinn_clnt_create_tcp (host, port);

  if (!icb)
    {
      fprintf (stderr, "failed to connect to server\n");
      exit (1);
    }

  ret = test (icb);

  icbinn_clnt_destroy (icb);


  return ret;
}
