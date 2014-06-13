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

#include "project.h"




static ICBINN *
icbinn_clnt_create_by_fd (int sockfd, struct sockaddr *sa, int sa_len)
{
  ICBINN *icb;
  CLIENT *c;

  struct netbuf raddr;


  memset (&raddr, 0, sizeof (raddr));
  raddr.buf = sa;
  raddr.maxlen = raddr.len = sa_len;

  c =
    clnt_vc_create (sockfd, &raddr, ICBINN_PROT_PROGRAM, ICBINN_PROT_VERSION,
                    0, 0);

  if (!c)
    return NULL;

  icb = (ICBINN *) xmalloc (sizeof (ICBINN));
  icb->c = c;

  return icb;
}

static int
open_tcp_socket (const char *host, int port, struct sockaddr_in *sin)
{
  int sockfd;

  if (!host)
    return -1;

  if (!port)
    port = ICBINN_PROT_PORT;

  sockfd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

  if (sockfd < 0)
    return -1;

  memset (sin, 0, sizeof (sin));

  sin->sin_family = AF_INET;

  if (inet_pton (AF_INET, host, (void *) &sin->sin_addr) <= 0)
    {
      struct hostent *hostinfo = gethostbyname (host);
      if (!hostinfo)
        return -1;

      sin->sin_addr = *(struct in_addr *) hostinfo->h_addr; // XXX: ipv6
    }

  sin->sin_port = htons (port);

  if (connect (sockfd, (const struct sockaddr *) sin, sizeof (*sin)))
    return -1;

  return sockfd;
}


EXTERNAL ICBINN *
icbinn_clnt_create_tcp (const char *host, int port)
{
  int sockfd;
  struct sockaddr_in sin;
  ICBINN *ret;

  sockfd = open_tcp_socket (host, port, &sin);
  if (sockfd < 0)
    return NULL;

  ret =
    icbinn_clnt_create_by_fd (sockfd, (struct sockaddr *) &sin, sizeof (sin));

  if (!ret)
    close (sockfd);

  return ret;
}

static int
open_v4v_socket (int domid, int port, struct sockaddr_in *sin)
{
#ifdef HAVE_LIBV4V
  int sockfd;

  if (!port)
    port = ICBINN_PROT_PORT;

  sockfd = socket (AF_INETV4V, SOCK_STREAM, IPPROTO_TCP);

  if (sockfd < 0)
    return -1;

  memset (sin, 0, sizeof (sin));

  sin->sin_family = AF_INET;

  sin->sin_addr.s_addr = htonl (0x1000000UL | domid);

  sin->sin_port = htons (port);

  if (connect (sockfd, (const struct sockaddr *) sin, sizeof (*sin)))
    return -1;

  return sockfd;
#else
  return -1;
#endif
}

EXTERNAL ICBINN *
icbinn_clnt_create_v4v (int domid, int port)
{
  int sockfd;
  struct sockaddr_in sin;
  ICBINN *ret;

  sockfd = open_v4v_socket (domid, port, &sin);
  if (sockfd < 0)
    return NULL;

  ret =
    icbinn_clnt_create_by_fd (sockfd, (struct sockaddr *) &sin, sizeof (sin));

  if (!ret)
    close (sockfd);

  return ret;
}


EXTERNAL void
icbinn_clnt_destroy (ICBINN * icb)
{
  if (!icb)
    return;
  if (icb->c)
    {
      clnt_control (icb->c, CLSET_FD_CLOSE, NULL);
      clnt_destroy (icb->c);
    }
  free (icb);
}
