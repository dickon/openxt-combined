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

static Display *dpy;
static Window root;
static XRRScreenResources *res;
static RRCrtc crtc;

static int
happy_mode (XRRCrtcInfo * crtc_info)
{
  if (crtc_info->mode == None)
    return 0;


  if (crtc_info->x != 0)
    return 0;
  if (crtc_info->y != 0)
    return 0;
  if (crtc_info->width <= 0)
    return 0;
  if (crtc_info->height <= 0)
    return 0;

  return 1;
}

static RRCrtc
find_crtc (void)
{
  XRRCrtcInfo *crtc_info = NULL;
  RRCrtc crtc;

  int i;

  if (res) {
      XRRFreeScreenResources(res);
  }
  res = XRRGetScreenResources (dpy, root);

  if (!res)
    fatal ("XRRGetScreenResources failed");


  for (i = 0; i < res->ncrtc; i++)
    {

      crtc = res->crtcs[i];

      /* Sanity checks */
      crtc_info = XRRGetCrtcInfo (dpy, res, crtc);

      if (!happy_mode (crtc_info)) {
          if (crtc_info) {
              XRRFreeCrtcInfo(crtc_info);
              crtc_info = NULL;
          }
          continue;
      }

      if (crtc_info) {
          XRRFreeCrtcInfo(crtc_info);
          crtc_info = NULL;
      }

      return crtc;
    }

  fatal ("Failed to find crtc");
}

static int
badness (int dw, int mw, int dh, int mh)
{
  if (mw > dw)
    return -1;
  if (mh > dh)
    return -1;

  return (dw - mw) + (dh - mh);
}



static XRRModeInfo *
pick_mode (int w, int h, int tolerance)
{
  XRRModeInfo *best_mode = NULL;
  int bad, best;
  int i;

  /* Lookup display mode to set */
  for (i = 0; i < res->nmode; i++)
    {
      XRRModeInfo *mode = &res->modes[i];

      bad = badness (w, mode->width, h, mode->height);

      if (bad < 0)
        continue;

      if (!best_mode || (bad < best))
        {
          best_mode = mode;
          best = bad;
        }
    }

  if (best > tolerance)
    return NULL;

  return best_mode;
}

static void
resize_x_screen (int w, int h)
{
  double dpi = (25.4 * DisplayHeight (dpy, DefaultScreen (dpy))) /
    DisplayHeightMM (dpy, DefaultScreen (dpy));
  int w_mm = (25.4 * w) / dpi;
  int h_mm = (25.4 * h) / dpi;

  warning ("Resizing X server screen: %dx%d %dx%dmm", w, h, w_mm, h_mm);

  XRRSetScreenSize (dpy, root, w, h, w_mm, h_mm);
}

static RRMode
push_mode (int w, int h, int strict_timings)
{
  RRMode id;
  char name[12];
  int o;
  XRRModeInfo mode;
  XRRCrtcInfo *crtc_info = NULL;

  warning ("pushing new mode to X server: %dx%d", w, h);

  mode.nameLength = snprintf (name, 12, "%dx%d", w, h);
  mode.name = name;

  mode.width = w;
  mode.height = h;

  mode.hSkew = 0;

  if (strict_timings)
    {
      mode.dotClock = 136 * 1e6;
      mode.hSyncStart = w + 64;
      mode.hSyncEnd = w + 256;
      mode.hTotal = w + 560;
      mode.vSyncStart = h + 1;
      mode.vSyncEnd = h + 4;
      mode.vTotal = h + 60;
      mode.modeFlags = RR_HSyncPositive | RR_VSyncPositive;
    }
  else
    {
      mode.dotClock = 60 * 1e6;
      mode.hSyncStart = 0;
      mode.hSyncEnd = 0;
      mode.hTotal = w;
      mode.vSyncStart = 0;
      mode.vSyncEnd = 0;
      mode.vTotal = h;
      mode.modeFlags = 0;
    }

  id = XRRCreateMode (dpy, root, &mode);
  if (id == None)
    {
      warning ("Failed to create mode: %dx%d", w, h);
      return None;
    }

  crtc_info = XRRGetCrtcInfo (dpy, res, crtc);
  if (crtc_info) {
      for (o = 0; o < crtc_info->noutput; o++)
      {
          XRRAddOutputMode (dpy, crtc_info->outputs[o], id);
      }
      
      XRRFreeCrtcInfo(crtc_info);
  }

  /* Reload mode list */
  crtc = find_crtc ();

  return id;
}

int
x_set_mode (int w, int h)
{
  Status s;
  XRRCrtcInfo *crtc_info = NULL;
  XRRModeInfo *mode = NULL;
  RRMode mode_id;
  int err = 0;

  if (!w || !h) {
      goto done;
  }

  crtc_info = XRRGetCrtcInfo (dpy, res, crtc);

  if (w != crtc_info->width || h != crtc_info->height)
      warning ("looking for a mode at %dx%d currently have %dx%d",
           w, h, crtc_info->width, crtc_info->height);

  if (w == crtc_info->width && h == crtc_info->height) {
      goto done; /*Nothing to do */
  }

  mode = pick_mode (w, h, 0);

  if (mode)
    {
      mode_id = mode->id;
      warning ("best mode for %dx%d is %dx%d", w, h, mode->width,
               mode->height);

      if (mode->width == crtc_info->width
          && mode->height == crtc_info->height)
          goto done;               /*Nothing to do */

      w = mode->width;
      h = mode->height;
    }
  else
    {
      warning ("Didn't find any useful mode for %dx%d", w, h);

      mode_id = push_mode (w, h, 0);
      if (mode_id == None) {
          goto error;
      }
    }

  /*
   * If crtc resolution is larger than the target size,
   * disconnect it from the output before resizing the screen
   */
  if (w < crtc_info->width || h < crtc_info->height)
    {
      XRRSetCrtcConfig (dpy, res, crtc, CurrentTime,
                        0, 0, None, RR_Rotate_0, NULL, 0);
    }

  resize_x_screen (w, h);

  s = XRRSetCrtcConfig (dpy, res, crtc, CurrentTime,
                        crtc_info->x, crtc_info->y, mode_id,
                        crtc_info->rotation, crtc_info->outputs,
                        crtc_info->noutput);

  warning ("Setting mode to %d %d, %s", w, h,
           s == RRSetConfigSuccess ? "succeeded" : "failed");

  if (s != RRSetConfigSuccess)
    {
      warning ("Revert X screen size to %dx%x", crtc_info->width,
               crtc_info->height);
      resize_x_screen (crtc_info->width, crtc_info->height);
      goto error;
    }

  goto done;

error:
  err = -1;

done:
  if (crtc_info) {
      XRRFreeCrtcInfo(crtc_info);
  }

  return err;

}

void
x_init (void)
{

  dpy = XOpenDisplay (":0");

  if (!dpy)
    fatal ("unable to open xorg display");

  root = DefaultRootWindow (dpy);

  crtc = find_crtc ();

}
