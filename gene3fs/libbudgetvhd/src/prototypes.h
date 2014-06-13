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

/* libbudgetvhd.c */
uint32_t bvhd_checksum(void *_buf, int len);
uint32_t bvhd_now(void);
bvhd_uuid bvhd_new_uid(void);
uint64_t bvhd_round_to_block(BVHD *v, uint64_t ret);
uint64_t bvhd_round_to_sector(BVHD *v, uint64_t ret);
bvhd_geometry bvhd_make_geometry(uint64_t sectors);
int bvhd_check_footer(bvhd_footer *footer);
int bvhd_check_header(bvhd_header *header);
void bvhd_close(BVHD *v);
BVHD *bvhd_open(char *name, int ro);
void bvhd_create_footer(BVHD *v, bvhd_footer *footer);
void bvhd_create_header(BVHD *v, bvhd_header *header);
BVHD *bvhd_create(char *name, uint64_t size, uint64_t blocksize);
uint32_t bvhd_new_block(BVHD *v);
int bvhd_update_bat(BVHD *v, uint32_t block, uint32_t offset);
int bvhd_read_sector(BVHD *v, void *buf, uint64_t sector);
uint32_t bvhd_read(BVHD *v, void *_buf, uint64_t sector, uint32_t nsectors);
int bvhd_write_sector(BVHD *v, const void *buf, uint64_t sector);
uint32_t bvhd_write(BVHD *v, const void *_buf, uint64_t sector, uint32_t nsectors);
uint64_t bvhd_size(BVHD *v);
void bvhd_flush(BVHD *v);
uint64_t bvhd_block_size(BVHD *v);
int bvhd_bat_filled(BVHD *v, uint64_t sector);
/* version.c */
char *libbudgetvhd_get_version(void);
/* swab.c */
int bvhd_swab16(uint16_t *b, size_t n);
int bvhd_swab32(uint32_t *b, size_t n);
int bvhd_swab64(uint64_t *b, size_t n);
int bvhd_swab_header(bvhd_header *h);
int bvhd_swab_geometry(bvhd_geometry *g);
int bvhd_swab_footer(bvhd_footer *f);
