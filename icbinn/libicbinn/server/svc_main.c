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

#ifdef HAVE_LIBV4V
# include <libv4v.h>
#endif

#define ICBINN_TIMEOUT_SECS 60
#define MAX_ICBINN_SOCKETS	16


static void
usage (char *name)
{
  fprintf (stderr, "Usage:\n");
  fprintf (stderr, "%s [-t] [-p port] [-p port]\n", name);
  fprintf (stderr,
           "    -t         Use TCP rather than V4V as the transport\n");
  fprintf (stderr, "    -p port    port number to use rather than %d\n",
           ICBINN_PROT_PORT);

  exit (1);
}

static void
check_client_alive (int svfd)
{
  int err;

  if (svfd == -1)
    fatal ("invalid server fd, exiting");

  /* libtirpc closes the fd if it fails to read from it e.g. because the client closed the connection. */
  if ((fcntl (svfd, F_GETFL) == -1) && (errno == EBADF))
    fatal ("client seems to have closed the connection, exiting");

  if ((err = write (svfd, "", 0)) >= 0)
    return;
  if (err == -EAGAIN)
    return;

  fatal ("client does not seem alive, exiting");

}

static void
svc_input_loop (int svfd)
{
  fd_set readfds;
  struct timeval timeout;

  for (;;)
    {
      /* Do this every time, as select modifies the sets and timeval in place when it returns.
         svc_fdset and svc_maxfd are exported from libtirpc */
      readfds = svc_fdset;
      timeout.tv_sec = ICBINN_TIMEOUT_SECS;
      timeout.tv_usec = 0;

      /* short ckt the test if libtirpc has already said good bye */
      if (!FD_ISSET (svfd, &readfds))
        check_client_alive (svfd);

      switch (select (svc_maxfd + 1, &readfds, NULL, NULL, &timeout))
        {
        case -1:
          warning("select failed with errno %d", errno);
          break;
        case 0:
          check_client_alive (svfd);
          break;
        default:
          svc_getreqset (&readfds);
          break;
        }
    }
}

static int
svc_create_socket (struct sockaddr_in *saddr, int port, int use_tcp)
{
  int sockfd = -1;

  if (!use_tcp)
    {
#ifdef HAVE_LIBV4V
      /* this forces a v4v socket without the need of INET_IS_V4V=1 env var */
      sockfd = socket (AF_INETV4V, SOCK_STREAM, IPPROTO_TCP);
#else
      fatal ("no v4v support");
#endif
    }
  else
    {
      int one = 1;
      sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

      setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (one));
    }
  if (sockfd < 0)
    fatal ("failed to create socket: errno %d", errno);

  memset (saddr, 0, sizeof (struct sockaddr_in));
  saddr->sin_family = AF_INET;
  saddr->sin_addr.s_addr = INADDR_ANY;
  saddr->sin_port = htons (port);

  if (bind (sockfd, (struct sockaddr *) saddr, sizeof (struct sockaddr_in)))
    fatal ("bind failed: errno %d", errno);

  if (listen (sockfd, 5))
    fatal ("listen failed: errno %d", errno);

  /* Set O_NONBLOCK flag on sockets */
  {
    long arg = 0;
    arg = fcntl (sockfd, F_GETFL, arg);
    arg |= O_NONBLOCK;
    if (fcntl (sockfd, F_SETFL, arg) < 0)
      fatal ("failed to switch to non-blocking mode, errno is %d", errno);
  }


  return sockfd;
}

static int
svc_accept_one_connexion (int *sockfds, int n, int *socket_index,
                          struct sockaddr_in *cl_addr)
{
  fd_set read_set;
  socklen_t slen;
  int ret = -1;
  int max_sockfd = -1;
  int i;

  /* Add a select loop for accept. */
  /* Do this every time, as select modifies the sets and timeval in place when it returns. */
  FD_ZERO (&read_set);
  for (i = 0; i < n; i++)
    {
      FD_SET (sockfds[i], &read_set);

      if (sockfds[i] > max_sockfd)
        max_sockfd = sockfds[i];
    }

  switch (select (max_sockfd + 1, &read_set, NULL, NULL, NULL))
    {
    case 0:
      return -1;
    case -1:
      warning ("select on accept fds failed with errno %d", errno);
      return -1;
    }


  memset (cl_addr, 0, sizeof (struct sockaddr_in));

  for (i = 0; i < n; i++)
    {
      if (!FD_ISSET (sockfds[i], &read_set))
        continue;

      slen = sizeof (struct sockaddr_in);
      ret = accept (sockfds[i], (struct sockaddr *) cl_addr, &slen);

      if (ret < 0)
        {
          warning ("accept failed: errno %d", errno);
          continue;
        }

      if (socket_index)
          *socket_index = i;

      return ret;
    }
  return ret;
}

int
main (int argc, char *argv[])
{
  register SVCXPRT *transp;
  int sockfds[MAX_ICBINN_SOCKETS];
  struct sockaddr_in saddrs[MAX_ICBINN_SOCKETS];
  pid_t chld;
  int len, i;
#ifndef HAVE_LIBV4V
  int tflag = 1;
#else
  int tflag = 0;
#endif
  int port[MAX_ICBINN_SOCKETS];

  int num_icbinn_sockets = 0;
  port[0] = ICBINN_PROT_PORT;


  while ((i = getopt (argc, argv, "p:s:th")) != EOF)
    {
      switch (i)
        {
        case 'p':
        case 's':
          if (num_icbinn_sockets == MAX_ICBINN_SOCKETS)
            fatal
              ("attempting to listen to too many sockets - maximum is %d\n",
               MAX_ICBINN_SOCKETS);
          if (!optarg)
            usage (argv[0]);
          port[num_icbinn_sockets++] = atoi (optarg);
          break;
        case 't':
          tflag++;
          break;
        default:
          usage (argv[0]);
        }
    }

  if (!num_icbinn_sockets)
    num_icbinn_sockets = 1;

  if (signal (SIGCHLD, SIG_IGN) == SIG_ERR)
    fatal ("signal failed: errno %d", errno);


  for (i = 0; i < num_icbinn_sockets; ++i)
    {
      sockfds[i] = svc_create_socket (&saddrs[i], port[i], tflag);
      if (sockfds[i] < 0)
        fatal ("Failed to open listening socket tcp=%d port=%d\n", tflag,
               port[i]);
    }


  for (;;)
    {
      struct sockaddr_in cl_addr;
      unsigned long cl_ipaddr;
      int cl_domid;
      int socket_index = 0;
      int svfd;

      svfd =
        svc_accept_one_connexion (sockfds, num_icbinn_sockets, &socket_index,
                                  &cl_addr);

      if (svfd < 0)
        continue;

#ifndef DONT_FORK_AND_MAKE_IT_POSSIBLE_TO_DEBUG

      switch ((chld = fork ()))
        {
        case -1:
          fatal ("fork failed: errno %d", errno);
        case 0:
          break;
        default:
          close (svfd);
          svfd = -1;
          continue;
        }

      info ("in child, index of accepted connection = %d", socket_index);

      for (i = 0; i < num_icbinn_sockets; i++)
        close (sockfds[i]);
#endif

      cl_ipaddr = ntohl (cl_addr.sin_addr.s_addr);
      cl_domid = cl_ipaddr & 0xffff;

      info ("client dom_id = %d", cl_domid);

      init_rand ();

      {
        char icbinn_path[ICBINN_PROT_MAXPATHLEN];
	struct stat st;

        memset (icbinn_path, 0, sizeof (icbinn_path));
        if (get_icbinn_path
            (cl_domid, icbinn_path, ICBINN_PROT_MAXPATHLEN, socket_index) < 0)
          fatal ("failed to get icbinn-path number %d for domid %d\n",
                 socket_index, cl_domid);

        info ("icbinn-path number %d is %s", socket_index, icbinn_path);

	if (stat(icbinn_path, &st) < 0 && errno == ENOENT) {
	  mkdir(icbinn_path, S_IRWXU);
	}

        if (chdir (icbinn_path) < 0)
          fatal ("chdir to icbinn-path %s failed: errno %d", icbinn_path,
                 errno);

        if (chroot (icbinn_path) < 0)
          fatal ("chroot to icbinn-path %s failed: errno %d", icbinn_path,
                 errno);

      }

      transp = svc_fd_create (svfd, 0, 0);
      if (!transp)
        fatal ("cannot create server fd");

      /* the last 0 in arg list guarantees that it won't try to connect to a portmap/rpcbind */
      if (!svc_register
          (transp, ICBINN_PROT_PROGRAM, ICBINN_PROT_VERSION,
           icbinn_prot_program_1, 0))
        {
          fatal
            ("unable to register (ICBINN_PROT_PROGRAM, ICBINN_PROT_VERSION, 0).");
        }

      /* This function never returns */
      svc_input_loop (svfd);

    }
  /* NOTREACHED */
}
