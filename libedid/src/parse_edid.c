/*
 * parse_edid.c
 *
 * Copyright (c) 2011 Citrix Systems Inc., 
 * All rights reserved.
 *
 */

/*
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

static EDIDMode *
mode_add (EDIDMode * ret, int *n, int clock, int hactive, int hsync_start,
          int hsync_end, int htotal, int vactive, int vsync_start,
          int vsync_end, int vtotal)
{
  EDIDMode *ptr;

  if (!hactive)
    return ret;
  if (!vactive)
    return ret;

  if (!htotal)
    return ret;
  if (!vtotal)
    return ret;

  if (!*n)
    {
      (*n)++;
      ptr = ret = xmalloc (2 * sizeof (*ret));
    }
  else
    {
      ret = xrealloc (ret, sizeof (*ret) * (*n + 2));
      ptr = &ret[*n];
      (*n)++;
    }
  ptr[1].valid = 0;

  ptr[0].valid = 1;

  ptr[0].clock = clock;

  ptr[0].hactive = hactive;
  ptr[0].hsync_start = hsync_start;
  ptr[0].hsync_end = hsync_end;
  ptr[0].htotal = htotal;


  ptr[0].vactive = vactive;
  ptr[0].vsync_start = vsync_start;
  ptr[0].vsync_end = vsync_end;
  ptr[0].vtotal = vtotal;


  return ret;
}

static int
compare_modes (const void *_b, const void *_a)
{
  EDIDMode *a = (EDIDMode *) _a;
  EDIDMode *b = (EDIDMode *) _b;

  int ra, rb;

  if (a->hactive > b->hactive)
    return 1;
  if (a->hactive < b->hactive)
    return -1;

  if (a->vactive > b->vactive)
    return 1;
  if (a->vactive < b->vactive)
    return -1;

  ra = a->clock / (a->htotal * a->vtotal);
  rb = b->clock / (b->htotal * b->vtotal);

  if (ra > rb)
    return 1;
  if (ra < rb)
    return -1;

  return 0;
}


static void
sort_modes (EDIDMode * m, int n)
{
  if (!m)
    return;
  qsort (m, n, sizeof (EDIDMode), compare_modes);
}

static void
print_mode (EDIDMode * m)
{
  float hs, vs;

  hs = (float) m->clock;
  vs = hs = hs / (float) m->htotal;
  hs = hs / 1000.0;
  vs = vs / (float) m->vtotal;

  printf
    ("Mode: %dx%d   %d %d %d %d  %d %d %d %d   hsync=%.1fkHz  vsync=%.1fHz\n",
     m->hactive, m->vactive, m->hactive, m->hsync_start, m->hsync_end,
     m->htotal, m->vactive, m->vsync_start, m->vsync_end, m->vtotal, hs, vs);


}

static void
print_modes (EDIDMode * m, int n)
{
  int i;
  for (i = 0; i < n; ++i)
    print_mode (&m[i]);
}

/* Standard modes:
  ./gtf    720 400 70
  ./gtf    720 400 88
  ./gtf    640 480 60
  ./gtf    640 480 67
  ./gtf    640 480 72
  ./gtf    640 480 75
  ./gtf    800 600 56
  ./gtf    800 600 60

  ./gtf    800 600 72
  ./gtf    800 600 75
  ./gtf    832 624 75
  ./gtf    1024 768 87i
  ./gtf    1024 768 60
  ./gtf    1024 768 70
  ./gtf    1024 768 75
  ./gtf    1280 1024 75
*/

static const EDIDMode std_modes[16] = {
#if 0                           /* Eric's */
  /* 0x0001 800x600@60.00 VESA */ {1, 37900000, 800, 840, 968, 1056, 600, 601,
                                   605, 628},
  /* 0x0002 800x600@56.00 VESA */ {1, 35100000, 800, 824, 896, 1024, 600, 601,
                                   603, 625},
  /* 0x0004 640x480@75.00 VESA */ {1, 37500000, 640, 656, 720, 840, 480, 481,
                                   484, 500},
  /* 0x0008 640x480@72.00 VESA */ {1, 37900000, 640, 664, 704, 832, 480, 489,
                                   492, 520},
  /* 0x0010 640x480@67.00 MACII */ {1, 27000000, 640, 664, 728, 816, 480, 481,
                                    484, 499},
  /* 0x0020 640x480@60.00 VGA */ {1, 31500000, 640, 664, 760, 800, 480, 491,
                                  493, 525},
  /* 0x0040 720x400@88.00 XGA */ {1, 33750000, 720, 752, 816, 912, 400, 403,
                                  413, 424},
  /* 0x0080 720x400@70.00 VGA */ {1, 26250000, 720, 744, 808, 896, 400, 403,
                                  413, 420},
  /* 0x0100 1280x1024@75.00 VESA */ {1, 135000000, 1280, 1296, 1440, 1688,
                                     1024, 1025, 1028, 1066},
  /* 0x0200 1024x768@75.00 VESA */ {1, 82000000, 1024, 1088, 1192, 1360, 768,
                                    771, 775, 805},
  /* 0x0400 1024x768@70.00 VESA */ {1, 56400000, 1024, 1048, 1184, 1328, 768,
                                    771, 777, 806},
  /* 0x0800 1024x768@60.00 VESA */ {1, 48400000, 1024, 1032, 1176, 1344, 768,
                                    771, 777, 806},
  /* 0x1000 1024x768i@87.00 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  /* Don't support interlace */
  /* 0x2000 832x624@75.00 MACII */ {1, 53250000, 832, 880, 960, 1088, 624,
                                    627, 631, 654},
  /* 0x4000 800x600@75.00 VESA */ {1, 46900000, 800, 816, 896, 1056, 600, 601,
                                   604, 625},
  /* 0x8000 800x600@72.00 VESA */ {1, 48000000, 800, 856, 976, 1040, 600, 637,
                                   643, 666},
#endif


#if 0                           /* TH's */
  /* 0x0001 800x600@60.00 */ {1, 38000000, 800, 832, 912, 1024, 600, 601, 604,
                              622},
  /* 0x0002 800x600@56.00 */ {1, 35000000, 800, 832, 912, 1024, 600, 601, 604,
                              620},
  /* 0x0004 640x480@75.00 */ {1, 30000000, 640, 664, 728, 816, 480, 481, 484,
                              502},
  /* 0x0008 640x480@72.00 */ {1, 29000000, 640, 664, 728, 816, 480, 481, 484,
                              501},
  /* 0x0020 640x480@60.00 */ {1, 23000000, 640, 656, 720, 800, 480, 481, 484,
                              497},
  /* 0x0010 640x480@67.00 MACII */ {1, 27000000, 640, 664, 728, 816, 480, 481,
                                    484, 499},
  /* 0x0040 720x400@88.00 */ {1, 34000000, 720, 752, 824, 928, 400, 401, 404,
                              421},
  /* 0x0080 720x400@70.00 */ {1, 26000000, 720, 736, 808, 896, 400, 401, 404,
                              417},
  /* 0x0100 1280x1024@75.00 */ {1, 138000000, 1280, 1368, 1504, 1728, 1024,
                                1025, 1028, 1069},
  /* 0x0200 1024x768@75.00 */ {1, 81000000, 1024, 1080, 1192, 1360, 768, 769,
                               772, 802},
  /* 0x0400 1024x768@70.00 */ {1, 76000000, 1024, 1080, 1192, 1360, 768, 769,
                               772, 800},
  /* 0x0800 1024x768@60.00 */ {1, 64000000, 1024, 1080, 1184, 1344, 768, 769,
                               772, 795}, 2
    /* 0x1000 1024x768i@87.00 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  /* 0x2000 832x624@75.00 */ {1, 53000000, 832, 872, 960, 1088, 624, 625, 628,
                              652},
  /* 0x4000 800x600@75.00 */ {1, 48000000, 800, 840, 920, 1040, 600, 601, 604,
                              627},
  /* 0x8000 800x600@72.00 */ {1, 46000000, 800, 840, 920, 1040, 600, 601, 604,
                              626},
#endif

#if 1                           /* JMM's see http://tinyvga.com/vga-timing and others */
  /* 0x0001 800x600@60.00 */ {1, 40000000, 800, 840, 968, 1056, 600, 601, 605,
                              628},
  /* 0x0002 800x600@56.00 */ {1, 36000000, 800, 824, 896, 1024, 600, 601, 603,
                              625},
  /* 0x0004 640x480@75.00 */ {1, 31500000, 640, 656, 720, 840, 480, 481, 484,
                              500},
  /* 0x0008 640x480@72.00 */ {1, 31500000, 640, 664, 704, 832, 480, 489, 492,
                              520},
  /* 0x0010 640x480@67.00 */ {1, 27106000, 640, 661, 725, 810, 480, 481, 484,
                              499},
  /* 0x0020 640x480@60.00 */ {1, 25175000, 640, 656, 752, 800, 480, 490, 492,
                              525},
                                                              /* 0x0040 720x400@88.00 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
                                                              /*This should be in the XGA-2 TRM but isn't that I can see */
                                                              /* 0x0080 720x400@70.00 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
                                                              /*{1, 28322000, 720, 738, 846, 900, 400, 413, 415, 449}, */
  /* 0x0100 1280x1024@75.00 */ {1, 135000000, 1280, 1296, 1440, 1688, 1024,
                                1026, 1028, 1066},
  /* 0x0200 1024x768@75.00 */ {1, 78800000, 1024, 1040, 1136, 1312, 768, 769,
                               772, 800},
  /* 0x0400 1024x768@70.00 */ {1, 75000000, 1024, 1048, 1184, 1328, 768, 771,
                               777, 806},
  /* 0x0800 1024x768@60.00 */ {1, 65000000, 1024, 1032, 1176, 1344, 768, 771,
                               777, 806},
  /* 0x1000 1024x768i@87.00 */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  /* 0x2000 832x624@75.00 */ {1, 57000000, 832, 876, 940, 1152, 624, 625, 628,
                              667},
  /* 0x4000 800x600@75.00 */ {1, 49500000, 800, 816, 896, 1056, 600, 601, 604,
                              625},
  /* 0x8000 800x600@72.00 */ {1, 50000000, 800, 856, 976, 1040, 600, 637, 643,
                              666},
#endif
};

EXTERNAL int
edid_valid (uint8_t * edid_raw, int ignore_csum)
{
  int rc;
  uint8_t pattern[] = HEADER_PATTERN;

  rc = memcmp (SECTION (HEADER_SECTION, edid_raw),
               pattern,
               sizeof (pattern));
  if (rc)
      return 0;

  return ignore_csum || (checksum(edid_raw, EDID1_LEN) == 0);
}

EXTERNAL EDIDMode *
edid_parse (uint8_t * edid)
{
  EDIDMode *ret = NULL;

  int n = 0;
  int c;
  const struct detailed_timings *d;
  edid_info ei;

  if (!edid || !edid_valid(edid, 1))
    {
      error ("EDID Invalid");
      return NULL;
    }

  edid_parse_raw (edid, &ei);

  for (c = 0; c < 4; c++)
    {
      if (ei.det_mon[c].type == DT)
        {
          d = &(ei.det_mon[c].section.d_timings);
          info ("Section %d: adding %dx%d: %d %d %d %d %d %d %d %d %d\n",
                c, d->h_active, d->v_active, d->clock, d->h_active,
                d->h_active + d->h_sync_off,
                d->h_active + d->h_sync_off + d->h_sync_width,
                d->h_active + d->h_blanking, d->v_active,
                d->v_active + d->v_sync_off,
                d->v_active + d->v_sync_off + d->v_sync_width,
                d->v_active + d->v_blanking);
          ret =
            mode_add (ret, &n, d->clock, d->h_active,
                      d->h_active + d->h_sync_off,
                      d->h_active + d->h_sync_off + d->h_sync_width,
                      d->h_active + d->h_blanking, d->v_active,
                      d->v_active + d->v_sync_off,
                      d->v_active + d->v_sync_off + d->v_sync_width,
                      d->v_active + d->v_blanking);
        }
    }

  /* Standard modes */
  short stdmodemask = edid[35] | (edid[36] << 8);
  info ("modesflags: %04x\n", stdmodemask);
  for (c = 0; c < 16; c++)
    {
      if (std_modes[c].valid && (stdmodemask & (1 << c)))
        {
          ret = mode_add (ret, &n,
                          std_modes[c].clock,
                          std_modes[c].hactive, std_modes[c].hsync_start,
                          std_modes[c].hsync_end, std_modes[c].htotal,
                          std_modes[c].vactive, std_modes[c].vsync_start,
                          std_modes[c].vsync_end, std_modes[c].vtotal);
        }
    }

  sort_modes (ret, n);
  print_modes (ret, n);

  return ret;
}
