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

/* libedid.c */
void edid_parse_raw(void *edid_raw, edid_info *edid);
/* version.c */
char *libedid_get_version(void);
/* parse_edid.c */
int edid_valid(uint8_t *edid_raw, int ignore_csum);
EDIDMode *edid_parse(uint8_t *edid);
/* util.c */
void message(int flags, const char *file, const char *function, int line, const char *fmt, ...);
void *xmalloc(size_t s);
void *xrealloc(void *p, size_t s);
char *xstrdup(const char *s);
/* gen_edid.c */
uint8_t *edid_set_header(uint8_t *e);
uint8_t *edid_set_vendor(uint8_t *e, const char *vendor, uint16_t product_id, uint32_t serial, uint8_t week, uint8_t year);
uint8_t *edid_set_version(uint8_t *e, uint8_t version, uint8_t revision);
uint8_t *edid_set_display_features(uint8_t *e, uint8_t input, uint16_t size_or_aspect_ratio, float gamma, uint8_t support);
uint8_t *edid_set_color_attr(uint8_t *e, uint16_t redx, uint16_t redy, uint16_t greenx, uint16_t greeny, uint16_t bluex, uint16_t bluey, uint16_t whitex, uint16_t whitey);
uint8_t *edid_set_established_timings(uint8_t *e, uint32_t timings);
uint8_t *edid_set_standard_timing(uint8_t *e, int id, uint16_t std_timing);
uint8_t *edid_set_detailed_timing(uint8_t *e, int id, int pixclock, int hactive, int hblank, int hsync_off, int hsync_width, int vactive, int vblank, int vsync_off, int vsync_width, int hsize, int vsize, int hborder, int vborder, uint8_t signal);
uint8_t *edid_set_display_descriptor(uint8_t *e, int id, uint8_t tag, void *data, size_t len);
uint8_t *edid_finalize(uint8_t *e, int block_count);
uint8_t *edid_init_common(uint8_t *e, int hres, int vres);
