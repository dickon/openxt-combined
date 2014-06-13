/*
 * test.c
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


#define EDID1_LEN 128

#include "edid.h"

#if 0

void print_vendor(const struct vendor *vendor)
{
    printf("\tIdentifier \"%s\"\n", vendor->name);
    printf("\tProduct ID \"%x\"\n", vendor->prod_id);
}

void print_version(const struct edid_version *ver)
{
    printf("\t# EDID version %d revision %d\n", ver->version, ver->revision);
}

void print_display(const struct disp_features *feat)
{
    (void) feat;
}

void print_established_timing(const struct established_timings *et)
{
    uint8_t t;
    int i;
    const char *established_timings_1[] = {
        "720×400 70Hz", "720×400 88Hz", "640×480 60Hz", "640×480 67Hz",
        "640×480 72Hz", "640×480 75Hz", "800×600 56Hz", "800×600 60Hz"
    };
    const char *established_timings_2[] = {
        "800×600 72Hz", "800×600 75Hz", "832×624 75Hz", "1024×768 87Hz (Interlaced)",
        "1024×768 60Hz", "1024×768 70Hz", "1024×768 75Hz", "1280×1024 75Hz"
    };

    printf("\nEstablished timings section:\n");
    for (t = et->t1, i = 0; t; t >>= 1, ++i)
        if (t & 0x01) {
            printf("\t%s\n", established_timings_1[i]);
        }
    for (t = et->t2, i = 0; t; t >>= 1, ++i)
        if (t & 0x01) {
            printf("\t%s\n", established_timings_2[i]);
        }
    printf("End section\n");
}

void print_std_timing(const struct std_timings st[8])
{
    printf("\t%dx%d %d Hz\n", st->hsize, st->vsize, st->refresh);
}

void print_dt_timing_section(const struct detailed_timings *d)
{
    int htotal = d->h_active + d->h_blanking;
    int vtotal = d->v_active + d->v_blanking;

    printf("\nDetailed timing section:\n");
    printf("\tMode\t\"%dx%d\"\t# vfreq %3.3fHz, hfreq %6.3fkHz\n",
           d->h_active, d->v_active,
           (double)d->clock / ((double)vtotal * (double)htotal),
           (double)d->clock / (double)(htotal * 1000));
    printf("\t\tDotClock\t%f\n", d->clock / 1000000.0);
    printf("\tEndMode\n");
    printf("End section.\n");
}

void print_monitor_ranges(const struct monitor_ranges *ranges)
{
    printf("\nSection monitor ranges:\n");
    printf("\tVSync: %d-%d\n", ranges->min_v, ranges->max_v);
    printf("\tHSync: %d-%d\n", ranges->min_h, ranges->max_h);
    printf("\tMaxClock: %dMHz\n", ranges->max_clock);
    if (ranges->preferred_refresh) {
        printf("\tPrefered refresh: %dHz\n", ranges->preferred_refresh);
    }
    printf("End section\n");
}

void print_detailed_block(const struct detailed_monitor_section *det_mon)
{
    int i;

    switch (det_mon->type) {
        case DS_RANGES:
            print_monitor_ranges(&(det_mon->section.ranges));
            break;
        case DS_STD_TIMINGS:
            for (i = 0; i < 5; ++i) {
                if (!(det_mon->section.std_t[i].hsize) ||
                    !(det_mon->section.std_t[i].vsize) ||
                    !(det_mon->section.std_t[i].refresh)) {
                    break;
                }
                print_std_timing(det_mon->section.std_t + i);
            }
            break;
        case DT:
            print_dt_timing_section(&(det_mon->section.d_timings));
        case DS_VENDOR:
            printf("\nVendor defined block.\n");
        case DS_UNKOWN:
            printf("\nUnknown detailed block.\n");
        default:
            printf("\nUnmatched detailed block.\n");
    }
}

void edid_print(const edid_info *edid)
{
    int i;

    print_vendor(&(edid->vendor));
    print_version(&(edid->version));
    print_display(&(edid->features));
    print_established_timing(&(edid->timings1));

    /* Print std timings */
    printf("\nStandard timings section:\n");
    for (i = 0; i < 8; ++i) {
        if (!edid->timings2[i].hsize || !edid->timings2[i].vsize ||
            !edid->timings2[i].refresh) {
            break;
        }
        print_std_timing(edid->timings2 + i);
    }
    printf("End section:\n");

    for (i = 0; i < 4; ++i) {
        print_detailed_block(edid->det_mon + i);
    }
}

void timings_list_print(const timings_list *l)
{
    const timings_list *s;

    for (s = l; s; s = s->next) {
        printf("%dx%d %dHz\n", s->mode.hsize, s->mode.vsize, s->mode.refresh);
    }
}

#endif
static const char *usage = "<edid file>\n\nParse EDID from file and display a summary.\n";


int main(int argc,char *argv[])
{
    FILE *edid_file;
    uint8_t edid_raw[EDID_SIZE] = { 0 };

    if (argc != 2) {
        fprintf(stderr, "%s: %s\n", argv[0], usage);
        return 1;
    }
    if (!(edid_file = fopen(argv[1], "rb"))) {
        perror("fopen");
        return 2;
    }
    if (fread(edid_raw, sizeof (uint8_t), EDID_SIZE, edid_file) <=0) {
        perror("fread");
        return 2;
    }
    if (fclose(edid_file)) {
        perror("fclose");
        return 2;
    }

    (void) edid_parse(edid_raw);
}
