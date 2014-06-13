/*
 * gen_edid.c
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

struct edid_vendor_info
{
    uint16_t vendor_id;
    uint16_t product_id;
    uint32_t serial;
    uint8_t week;
    uint8_t year;
} __attribute__ ((packed));

EXTERNAL uint8_t *
edid_set_header(uint8_t *e)
{
    uint8_t hdr[] = HEADER_PATTERN;

    memcpy(e, hdr, 8);

    return e;
}

EXTERNAL uint8_t *
edid_set_vendor(uint8_t *e,
                const char *vendor,
                uint16_t product_id,
                uint32_t serial,
                uint8_t week, uint8_t year)
{
    struct edid_vendor_info *info = (void *)(e + VENDOR_SECTION);
    int i = 0;

    info->vendor_id = 0;
    for (; i < 3; i++) {
        uint8_t c;

        if (vendor[i] < '@' || vendor[i] > 'Z')
            return NULL;

        c = vendor[i] - '@';

        info->vendor_id |= ((c & 0x1f) << ((2 - i) * 5));
    }


    info->product_id = product_id;
    info->serial = serial;
    info->week = week;
    info->year = year - 1990;

    return e;
}

EXTERNAL uint8_t *
edid_set_version(uint8_t *e,
                 uint8_t version,
                 uint8_t revision)
{
    uint8_t *v = e + VERSION_SECTION;

    v[0] = version;
    v[1] = revision;

    return e;
}

EXTERNAL uint8_t *
edid_set_display_features(uint8_t *e,
                          uint8_t input,
                          uint16_t size_or_aspect_ratio,
                          float gamma,
                          uint8_t support)
{
    uint8_t *f = e + DISPLAY_SECTION;

    f[0] = input;
    f[1] = size_or_aspect_ratio & 0xff;
    f[2] = size_or_aspect_ratio >> 8;
    f[3] = (int)((gamma * 100) - 100) & 0xff;
    f[4] = support;

    return e;
}

EXTERNAL uint8_t *
edid_set_color_attr(uint8_t *e,
                    uint16_t redx, uint16_t redy,
                    uint16_t greenx, uint16_t greeny,
                    uint16_t bluex, uint16_t bluey,
                    uint16_t whitex, uint16_t whitey)
{
    uint8_t *c = e + 0x19;

    if (redx > 0x3ff || redy > 0x3ff ||
        greenx > 0x3ff || greeny > 0x3ff ||
        bluex > 0x3ff || bluey > 0x3ff ||
        whitex > 0x3ff || whitey > 0x3ff)
        return NULL;

    c[0] = ((redx & 0x3) << 6) | ((redy & 0x3) << 4) |
           ((greenx & 0x3) << 2) | (greeny & 0x3);
    c[1] = ((bluex & 0x3) << 6) | ((bluey & 0x3) << 4) |
           ((whitex & 0x3) << 2) | (whitey & 0x3);

    c[2] = redx >> 2;
    c[3] = redy >> 2;
    c[4] = greenx >> 2;
    c[5] = greeny >> 2;
    c[6] = bluex >> 2;
    c[7] = bluey >> 2;
    c[8] = whitex >> 2;
    c[9] = whitey >> 2;

    return e;
}

EXTERNAL uint8_t *
edid_set_established_timings(uint8_t *e, uint32_t timings)
{
    uint8_t *t = e + ESTABLISHED_TIMING_SECTION;

    t[0] = timings & 0xff;
    t[1] = (timings >> 8) & 0xff;
    t[2] = (timings >> 16) & 0xff;

    return e;
}

EXTERNAL uint8_t *
edid_set_standard_timing(uint8_t *e, int id, uint16_t std_timing)
{
    uint16_t *t = (uint16_t *)(e + STD_TIMING_SECTION);

    if (id < 0 || id >= 8)
        return NULL;

    t[id] = std_timing;

    return e;
}

struct edid_detailed_timing
{
    uint16_t pixclock;
    uint8_t hactive_lo;
    uint8_t hblank_lo;
    uint8_t hblank_hi:4;
    uint8_t hactive_hi:4;
    uint8_t vactive_lo;
    uint8_t vblank_lo;
    uint8_t vblank_hi:4;
    uint8_t vactive_hi:4;
    uint8_t hsync_off_lo;
    uint8_t hsync_width_lo;
    uint8_t vsync_width_lo:4;
    uint8_t vsync_off_lo:4;
    uint8_t vsync_width_hi:2;
    uint8_t vsync_off_hi:2;
    uint8_t hsync_width_hi:2;
    uint8_t hsync_off_hi:2;
    uint8_t hsize_lo;
    uint8_t vsize_lo;
    uint8_t vsize_hi:4;
    uint8_t hsize_hi:4;
    uint8_t hborder;
    uint8_t vborder;
    uint8_t signal;
} __attribute__ ((packed));

EXTERNAL uint8_t *
edid_set_detailed_timing(uint8_t *e, int id,
                         int pixclock,
                         int hactive, int hblank, int hsync_off, int hsync_width,
                         int vactive, int vblank, int vsync_off, int vsync_width,
                         int hsize, int vsize, int hborder, int vborder,
                         uint8_t signal)
{
    struct edid_detailed_timing *t = (void *)(e + DET_TIMING_SECTION +
                                              id * DET_TIMING_INFO_LEN);

    t->pixclock = pixclock;

    t->hactive_lo = hactive & 0xff;
    t->hactive_hi = (hactive >> 8) & 0xf;
    t->hblank_lo = hblank & 0xff;
    t->hblank_hi = (hblank >> 8) & 0xf;
    t->hsync_off_lo = hsync_off & 0xff;
    t->hsync_off_hi = (hsync_off >> 8) & 0x3;
    t->hsync_width_lo = hsync_width & 0xff;
    t->hsync_width_hi = (hsync_width >> 8) & 0x3;
    t->hsize_lo = hsize & 0xff;
    t->hsize_hi = (hsize >> 8) & 0xf;
    t->hborder = hborder;


    t->vactive_lo = vactive & 0xff;
    t->vactive_hi = (vactive >> 8) & 0xf;
    t->vblank_lo = vblank & 0xff;
    t->vblank_hi = (vblank >> 8) & 0xf;
    t->vsync_width_lo = vsync_width & 0xf;
    t->vsync_width_hi = (vsync_width >> 4) & 0x3;
    t->vsync_off_lo = vsync_off & 0xf;
    t->vsync_off_hi = (vsync_off >> 4) & 0x3;
    t->vsize_lo = vsize & 0xff;
    t->vsize_hi = (vsize >> 8) & 0xf;
    t->vborder = vborder;

    t->signal = signal;

    return e;
}

struct edid_display_descriptor
{
    uint16_t signature;
    uint8_t reserved1;
    uint8_t tag;
    uint8_t reserved2;
    uint8_t data[13];
} __attribute__ ((packed));

EXTERNAL uint8_t *
edid_set_display_descriptor(uint8_t *e, int id,
                            uint8_t tag,
                            void *data, size_t len)
{
    struct edid_display_descriptor *d = (void *)(e + DET_TIMING_SECTION +
                                                 id * DET_TIMING_INFO_LEN);

    d->signature = EDID_DISPLAY_DESCR;
    d->reserved1 = 0x00;
    d->tag = tag;
    d->reserved2 = 0x00;
    memcpy(d->data, data, len);
    memset(d->data + len, 0, sizeof (d->data) - len);

    return e;
}

EXTERNAL uint8_t *
edid_finalize(uint8_t *e, int block_count)
{
    uint8_t sum;

    e[126] = block_count;
    e[127] = 0;
    sum = checksum(e, 128);
    e[127] = -sum;
}

EXTERNAL uint8_t *
edid_init_common(uint8_t *e, int hres, int vres)
{
    int pixclock;

    edid_set_header(e);
    edid_set_vendor(e, "AAA", 0xABCD, 0x12345678ul, 12, 2012);
    edid_set_version(e, 1, 4);
    edid_set_display_features(e, EDID_VIDEO_INPUT_SIGNAL_DIGITAL |
                                 EDID_VIDEO_INPUT_COLOR_DEPTH_8BITS |
                                 EDID_VIDEO_INPUT_INTERFACE_DVI,
                                 EDID_DISPLAY_SIZE(33, 25),
                                 2.2f,
                                 EDID_DISPLAY_SUPPORT_DPM_STANDBY |
                                 EDID_DISPLAY_SUPPORT_DPM_SUSPEND |
                                 EDID_DISPLAY_SUPPORT_SRGB_DEFAULT |
                                 EDID_DISPLAY_SUPPORT_COLOR_RGB444 |
                                 EDID_DISPLAY_SUPPORT_FREQ_CONTINUOUS);
    edid_set_color_attr(e, 665, 343,
                           290, 620,
                           155, 75,
                           321, 337);
    edid_set_established_timings(e, EDID_EST_TIMING_640x480_75HZ |
                                    EDID_EST_TIMING_720x400_88HZ |
                                    EDID_EST_TIMING_800x600_75HZ |
                                    EDID_EST_TIMING_1024x768_75HZ |
                                    EDID_EST_TIMING_1280x1024_75HZ);

    edid_set_standard_timing(e, 0, EDID_STD_TIMING(640, EDID_STD_TIMING_AR_4_3, 85));
    edid_set_standard_timing(e, 1, EDID_STD_TIMING(800, EDID_STD_TIMING_AR_4_3, 85));
    edid_set_standard_timing(e, 2, EDID_STD_TIMING(1024, EDID_STD_TIMING_AR_4_3, 85));
    edid_set_standard_timing(e, 3, EDID_STD_TIMING(640, EDID_STD_TIMING_AR_4_3, 70));
    edid_set_standard_timing(e, 4, EDID_STD_TIMING(1280, EDID_STD_TIMING_AR_4_3, 70));
    edid_set_standard_timing(e, 5, EDID_STD_TIMING(1600, EDID_STD_TIMING_AR_4_3, 60));
    edid_set_standard_timing(e, 6, EDID_STD_TIMING_UNUSED);
    edid_set_standard_timing(e, 7, EDID_STD_TIMING_UNUSED);

#define HBLANK_COMMON 184
#define HSYNC_OFF_COMMON 48
#define HSYNC_WIDTH_COMMON 112
#define HSIZE_COMMON 300

#define VBLANK_COMMON 7
#define VSYNC_OFF_COMMON 1
#define VSYNC_WIDTH_COMMON 3
#define VSIZE_COMMON 225

#define REFRESH_DEFAULT 75

    /* Compute clock for 75Hz display refresh */
    pixclock = (hres + HBLANK_COMMON) * (vres + VBLANK_COMMON) * REFRESH_DEFAULT;
    pixclock /= 10000;

    edid_set_detailed_timing(e, 0, pixclock,
                             hres, HBLANK_COMMON, HSYNC_OFF_COMMON, HSYNC_WIDTH_COMMON,
                             vres, VBLANK_COMMON, VSYNC_OFF_COMMON, VSYNC_WIDTH_COMMON,
                             HSIZE_COMMON, VSIZE_COMMON, 0, 0,
                             EDID_DET_TIMING_SIGNAL_DIGITAL);
    edid_set_display_descriptor(e, 1, EDID_DISPLAY_DESCR_TAG_DUMMY,
                                NULL, 0);
    edid_set_display_descriptor(e, 2, EDID_DISPLAY_DESCR_TAG_SERIAL,
                                "0123456789\n  ", 13);
    edid_set_display_descriptor(e, 3, EDID_DISPLAY_DESCR_TAG_NAME,
                                "libedid disp\n", 13);

    return edid_finalize(e, 0);
}


