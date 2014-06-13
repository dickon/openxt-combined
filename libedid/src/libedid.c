/*
 * libedid.c
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

static void
get_vendor_section (uint8_t * c, struct vendor *r)
{
  r->name[0] = L1;
  r->name[1] = L2;
  r->name[2] = L3;
  r->name[3] = '\0';

  r->prod_id = PROD_ID;
  r->serial = SERIAL_NO;
  r->week = WEEK;
  r->year = YEAR;
}

static void
get_version_section (uint8_t * c, struct edid_version *r)
{
  r->version = EDID_VERSION;
  r->revision = REVISION;
}

static void
get_display_section (uint8_t * c, struct disp_features *r,
                     const struct edid_version *v)
{
  r->input_type = INPUT_TYPE;
  if (!DIGITAL (r->input_type))
    {
      r->input_voltage = INPUT_VOLTAGE;
      r->input_setup = SETUP;
      r->input_sync = SYNC;
    }
  else if (v->revision == 2 || v->revision == 3)
    {
      r->input_dfp = DFP;
    }
  else if (v->revision >= 4)
    {
      r->input_bpc = BPC;
      r->input_interface = DIGITAL_INTERFACE;
    }
  r->hsize = HSIZE_MAX;
  r->vsize = VSIZE_MAX;
  r->gamma = GAMMA;
  r->dpms = DPMS;
  r->display_type = DISPLAY_TYPE;
  r->msc = MSC;
  r->redx = REDX;
  r->redy = REDY;
  r->greenx = GREENX;
  r->greeny = GREENY;
  r->bluex = BLUEX;
  r->bluey = BLUEY;
  r->whitex = WHITEX;
  r->whitey = WHITEY;
}

static void
get_established_timing_section (uint8_t * c, struct established_timings *r)
{
  r->t1 = T1;
  r->t2 = T2;
  r->t_manu = T_MANU;
}

static void
get_std_timing_section (uint8_t * c, struct std_timings *r,
                        const struct edid_version *v)
{
  int i;

  for (i = 0; i < STD_TIMINGS; i++)
    {
      if (VALID_TIMING)
        {
          r[i].hsize = HSIZE1;
          VSIZE1 (r[i].vsize);
          r[i].refresh = REFRESH_R;
        }
      else
        {
          r[i].hsize = r[i].vsize = r[i].refresh = 0;
        }
      NEXT_STD_TIMING;
    }
}

static void
get_dst_timing_section (uint8_t * c, struct std_timings *t,
                        const struct edid_version *v)
{
  int j;
  c = c + 5;
  for (j = 0; j < 5; j++)
    {
      t[j].hsize = HSIZE1;
      VSIZE1 (t[j].vsize);
      t[j].refresh = REFRESH_R;
      NEXT_STD_TIMING;
    }
}

static void
get_detailed_timing_section (uint8_t * c, struct detailed_timings *r)
{
  r->clock = PIXEL_CLOCK;
  r->h_active = H_ACTIVE;
  r->h_blanking = H_BLANK;
  r->v_active = V_ACTIVE;
  r->v_blanking = V_BLANK;
  r->h_sync_off = H_SYNC_OFF;
  r->h_sync_width = H_SYNC_WIDTH;
  r->v_sync_off = V_SYNC_OFF;
  r->v_sync_width = V_SYNC_WIDTH;
  r->h_size = H_SIZE;
  r->v_size = V_SIZE;
  r->h_border = H_BORDER;
  r->v_border = V_BORDER;
  r->interlaced = INTERLACED;
  r->stereo = STEREO;
  r->stereo_1 = STEREO1;
  r->sync = SYNC_T;
  r->misc = MISC;
}

static void
get_monitor_ranges (uint8_t * c, struct monitor_ranges *r)
{
  r->min_v = MIN_V;
  r->max_v = MAX_V;
  r->min_h = MIN_H;
  r->max_h = MAX_H;
  r->max_clock = 0;
  if (MAX_CLOCK != 0xff)        /* is specified? */
    r->max_clock = MAX_CLOCK * 10;
  if (HAVE_2ND_GTF)
    {
      r->gtf_2nd_f = F_2ND_GTF;
      r->gtf_2nd_c = C_2ND_GTF;
      r->gtf_2nd_m = M_2ND_GTF;
      r->gtf_2nd_k = K_2ND_GTF;
      r->gtf_2nd_j = J_2ND_GTF;
    }
  else
    {
      r->gtf_2nd_f = 0;
    }
  if (HAVE_CVT)
    {
      r->max_clock_khz = MAX_CLOCK_KHZ;
      r->max_clock = r->max_clock_khz / 1000;
      r->maxwidth = MAXWIDTH;
      r->supported_aspect = SUPPORTED_ASPECT;
      r->preferred_aspect = PREFERRED_ASPECT;
      r->supported_blanking = SUPPORTED_BLANKING;
      r->supported_scaling = SUPPORTED_SCALING;
      r->preferred_refresh = PREFERRED_REFRESH;
    }
  else
    {
      r->max_clock_khz = 0;
    }
}

static void
fetch_detailed_block (uint8_t * c, struct edid_version *ver,
                      struct detailed_monitor_section *det_mon)
{
  const unsigned char empty_block[18] = { 0 };

  if (ver->version == 1 && ver->revision >= 1 && IS_MONITOR_DESC)
    {
      switch (MONITOR_DESC_TYPE)
        {
        case MONITOR_RANGES:
          det_mon->type = DS_RANGES;
          get_monitor_ranges (c, &det_mon->section.ranges);
          break;
        case ADD_STD_TIMINGS:
          det_mon->type = DS_STD_TIMINGS;
          get_dst_timing_section (c, det_mon->section.std_t, ver);
          break;
        default:
          det_mon->type = DS_UNKOWN;
          break;
        }
      if (c[3] <= 0x0F && memcmp (c, empty_block, sizeof (empty_block)))
        {
          det_mon->type = DS_VENDOR + c[3];
        }
    }
  else
    {
      det_mon->type = DT;
      get_detailed_timing_section (c, &det_mon->section.d_timings);
    }
}

static void
get_dt_md_section (uint8_t * c, struct edid_version *ver,
                   struct detailed_monitor_section *det_mon)
{
  int i;

  for (i = 0; i < DET_TIMINGS; ++i)
    {
      fetch_detailed_block (c, ver, det_mon + i);
      NEXT_DT_MD_SECTION;
    }
}

INTERNAL void
edid_parse_raw (void *edid_raw, edid_info * edid)
{
  memset (edid, 0, sizeof (edid_info));
  get_vendor_section (SECTION (VENDOR_SECTION, edid_raw), &(edid->vendor));
  get_version_section (SECTION (VERSION_SECTION, edid_raw), &(edid->version));
  get_display_section (SECTION (DISPLAY_SECTION, edid_raw), &(edid->features),
                       &(edid->version));
  get_established_timing_section (SECTION
                                  (ESTABLISHED_TIMING_SECTION, edid_raw),
                                  &(edid->timings1));
  get_std_timing_section (SECTION (STD_TIMING_SECTION, edid_raw),
                          edid->timings2, &(edid->version));
  get_dt_md_section (SECTION (DET_TIMING_SECTION, edid_raw), &(edid->version),
                     edid->det_mon);
}


static void
timings_list_add_est_modes (timings_list ** l,
                            const struct established_timings *et)
{
  const struct std_timings established_modes_1[] = {
    {720, 400, 70}, {720, 400, 88}, {640, 480, 60}, {640, 480, 67},
    {640, 480, 72}, {640, 480, 75}, {800, 600, 56}, {800, 600, 60}
  };
  const struct std_timings established_modes_2[] = {
    {800, 600, 72}, {800, 600, 75}, {832, 624, 75}, {1024, 768, 87},
    {1024, 768, 60}, {1024, 768, 70}, {1024, 768, 75}, {1280, 1024, 75}
  };
  uint8_t t;
  int i;

  for (t = et->t1, i = 0; t; t >>= 1, ++i)
    if (t & 0x01)
      {
        timings_list_add (l, established_modes_1 + i);
      }
  for (t = et->t2, i = 0; t; t >>= 1, ++i)
    if (t & 0x01)
      {
        timings_list_add (l, established_modes_2 + i);
      }
}

static void
timings_list_add_std_modes (timings_list ** l, const struct std_timings *st)
{
  const struct std_timings *stend = st + STD_TIMINGS;

  for (; st < stend; ++st)
    {
      if (!st->hsize || !st->vsize || !st->refresh)
        {
          return;
        }
      timings_list_add (l, st);
    }
}

static void
timings_list_add_dt_std_modes (timings_list ** l,
                               const struct detailed_monitor_section *dt)
{
  const struct detailed_monitor_section *dtend = dt + DET_TIMINGS;
  const struct std_timings *st, *stend;

  for (; dt < dtend; ++dt)
    {
      if (dt->type == DS_STD_TIMINGS)
        {
          for (st = dt->section.std_t, stend = st + 5; st < stend; ++st)
            {
              if (!st->hsize || !st->vsize || !st->refresh)
                {
                  return;
                }
              timings_list_add (l, st);
            }
        }
    }
}

static timings_list *
edid_timings_list_init (const edid_info * edid)
{
  timings_list *n = NULL;

  if (!edid)
    {
      return NULL;
    }
  timings_list_add_est_modes (&n, &(edid->timings1));
  timings_list_add_std_modes (&n, edid->timings2);
  timings_list_add_dt_std_modes (&n, edid->det_mon);
  return n;
}

static void
edid_timings_list_add (timings_list ** l, const struct std_timings *m)
{
  timings_list *n;

  if (!l || !(n = calloc (1, sizeof (timings_list))))
    {
      return;
    }
  memcpy (&(n->mode), m, sizeof (struct std_timings));
  if (!*l)
    {
      *l = n;
    }
  else
    {
      n->next = *l;
      *l = n;
    }
}

static void
edid_timings_list_free (timings_list * l)
{
  timings_list *o;

  if (!l)
    {
      return;
    }
  if (!l->next)
    {
      free (l);
      return;
    }
  for (o = l, l = l->next; l; o = l, l = l->next)
    {
      free (o);
    }
  free (o);
}
