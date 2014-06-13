/*
 * Copyright (c) 2011 Citrix Systems, Inc.
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

#define XENSTORE_NODE "/xc_tools/switcher/current_display_size"

struct xs_handle *xsh;

/* check if connected to xenfb backend */
static int
xenfb_connected()
{
    unsigned int len = 0;
    int conn = 0;
    char *buf = xs_read( xsh, XBT_NULL, "device/vfb/0/state", &len );

    if (buf) {
        conn = strcmp("4", buf) == 0;
        free(buf);
    }
    return conn;
}

static int
get_display_size (int *w, int *h)
{
  unsigned int len = 0;
  char cbuf[128];
  char *buf = xs_read (xsh, XBT_NULL, XENSTORE_NODE, &len);

  if (!buf)
    return -1;
  if (!len) {
      free(buf);
    return -1;
  }


  if (len >= sizeof (cbuf))
    len = sizeof (cbuf) - 1;

  memcpy (cbuf, buf, len);
  cbuf[len] = 0;

  free (buf);

  if (sscanf (cbuf, "%d %d", w, h) != 2)
    return -1;

  return 0;
}

void
xsc_wait_xenfb_connection()
{
    for (;;) {
        if (xenfb_connected()) {
            info("xenfb connection is UP");
            return;
        }
        info("xenfb connectio is DOWN");
        sleep(1);
    }
}


void
xsc_do_thing (void)
{
  int w, h;

  if (get_display_size (&w, &h))
    {
      warning ("failed to get size");
      sleep (2);
      return;
    }

  if (w == 0 || h == 0)
    {
      /* surfman has just restarted, ignore, or X and the UIVM will crash */
      warning ("Invalid display size requested: %dx%d", w, h);
      sleep (2);
      return;
    }

  x_set_mode (w, h);
}



void
xsc_init (void)
{
  while (!(xsh = xs_domain_open ())) {
    warning ("xs_domain_open failed - sleeping 2s");
    sleep(2);
  }

  while (!xs_watch (xsh, XENSTORE_NODE, "")) {
    warning ("Can't make watch on %s - sleeping 2s", XENSTORE_NODE);
    sleep(2);
  }

  xsc_wait_xenfb_connection();
}


void
xsc_mainloop (void)
{

  while (1)
    {
      unsigned int n;
      char **watch = xs_read_watch (xsh, &n);
      free (watch);

      xsc_do_thing ();
    }
}
