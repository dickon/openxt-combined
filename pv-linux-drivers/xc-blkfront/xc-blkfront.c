/*
 * blkfront.c
 *
 * XenLinux virtual block device driver.
 *
 * Modifications by Paulian Marinca are (c) 2012 Citrix Systems, Inc.
 * Copyright (c) 2003-2004, Keir Fraser & Steve Hand
 * Modifications by Mark A. Williamson are (c) Intel Research Cambridge
 * Copyright (c) 2004, Christian Limpach
 * Copyright (c) 2004, Andrew Warfield
 * Copyright (c) 2005, Christopher Clark
 * Copyright (c) 2005, XenSource Ltd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*
 * Copyright (c) 2014 Citrix Systems, Inc.
 */



#include "xen-dkms.h"
#define DPRINTK(fmt, args...)				\
	pr_debug("xen-blkfront (%s:%d) " fmt ".\n",	\
		 __func__, __LINE__, ##args)

#include <linux/interrupt.h>
#include <linux/blkdev.h>
#include <linux/hdreg.h>
#include <linux/cdrom.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/scatterlist.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/wait.h>

#include <xen/xen.h>
#include <xen/xenbus.h>
#include <xen/grant_table.h>
#include <xen/events.h>
#include <xen/page.h>
#include <xen/platform_pci.h>

#include <xen/interface/grant_table.h>
#include <xen/interface/io/blkif.h>
#include <xen/interface/io/protocols.h>

#include <asm/xen/hypervisor.h>

#if ( LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,32) )
#if (!defined RHEL_RELEASE_CODE)
#define blk_queue_max_segments blk_queue_max_phys_segments
#endif
#define flush_work_sync(a) flush_scheduled_work()
#endif

#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0) )
# define BLKFRONT_IRQF 0
#else
# define BLKFRONT_IRQF IRQF_SAMPLE_RANDOM
#endif

enum blkif_state {
	BLKIF_STATE_DISCONNECTED,
	BLKIF_STATE_CONNECTED,
	BLKIF_STATE_SUSPENDED,
	BLKIF_STATE_RINGFLUSH,
};

struct blk_shadow {
	struct blkif_request req;
	struct request *request;
	unsigned long frame[BLKIF_MAX_SEGMENTS_PER_REQUEST];
};

static DEFINE_MUTEX(blkfront_mutex);
static const struct block_device_operations xlvbd_block_fops;

#define BLK_RING_SIZE __CONST_RING_SIZE(blkif, PAGE_SIZE)

/*
 * We have one of these per vbd, whether ide, scsi or 'other'.  They
 * hang in private_data off the gendisk structure. We may end up
 * putting all kinds of interesting stuff here :-)
 */
struct blkfront_info
{
	struct mutex mutex;
	wait_queue_head_t rflushq;
	struct xenbus_device *xbdev;
	struct gendisk *gd;
	int vdevice;
	blkif_vdev_t handle;
	enum blkif_state connected;
	int ring_ref;
	struct blkif_front_ring ring;
	struct scatterlist sg[BLKIF_MAX_SEGMENTS_PER_REQUEST];
	unsigned int evtchn, irq;
	struct request_queue *rq;
	struct work_struct work;
	struct gnttab_free_callback callback;
	struct blk_shadow shadow[BLK_RING_SIZE];
	unsigned long shadow_free;
	unsigned int feature_flush;
	unsigned int flush_op;
	int is_ready;
};

static DEFINE_SPINLOCK(blkif_io_lock);


static unsigned int nr_minors;
static unsigned long *minors;
static DEFINE_SPINLOCK(minor_lock);

#define MAXIMUM_OUTSTANDING_BLOCK_REQS \
	(BLKIF_MAX_SEGMENTS_PER_REQUEST * BLK_RING_SIZE)
#define GRANT_INVALID_REF	0

#define PARTS_PER_DISK		16
#define PARTS_PER_EXT_DISK      256

#define BLKIF_MAJOR(dev) ((dev)>>8)
#define BLKIF_MINOR(dev) ((dev) & 0xff)

#define EXT_SHIFT 28
#define EXTENDED (1<<EXT_SHIFT)
#define VDEV_IS_EXTENDED(dev) ((dev)&(EXTENDED))
#define BLKIF_MINOR_EXT(dev) ((dev)&(~EXTENDED))
#define EMULATED_HD_DISK_MINOR_OFFSET (0)
#define EMULATED_HD_DISK_NAME_OFFSET (EMULATED_HD_DISK_MINOR_OFFSET / 256)
#define EMULATED_SD_DISK_MINOR_OFFSET (0)
#define EMULATED_SD_DISK_NAME_OFFSET (EMULATED_SD_DISK_MINOR_OFFSET / 256)

#define DEV_NAME	"xvd"	/* name in /dev */

extern int xc_suppress_hibernate;

static int get_id_from_freelist(struct blkfront_info *info)
{
	unsigned long free = info->shadow_free;

	BUG_ON(free >= BLK_RING_SIZE);
	info->shadow_free = info->shadow[free].req.id;
	info->shadow[free].req.id = 0x0fffffee; /* debug */
	return free;
}

static void add_id_to_freelist(struct blkfront_info *info,
			       unsigned long id)
{
	info->shadow[id].req.id  = info->shadow_free;
	info->shadow[id].request = NULL;
	info->shadow_free = id;
}

static int xlbd_reserve_minors(unsigned int minor, unsigned int nr)
{
	unsigned int end = minor + nr;
	int rc;

	if (end > nr_minors) {
		unsigned long *bitmap, *old;

		bitmap = kzalloc(BITS_TO_LONGS(end) * sizeof(*bitmap),
				 GFP_KERNEL);
		if (bitmap == NULL)
			return -ENOMEM;

		spin_lock(&minor_lock);
		if (end > nr_minors) {
			old = minors;
			memcpy(bitmap, minors,
			       BITS_TO_LONGS(nr_minors) * sizeof(*bitmap));
			minors = bitmap;
			nr_minors = BITS_TO_LONGS(end) * BITS_PER_LONG;
		} else
			old = bitmap;
		spin_unlock(&minor_lock);
		kfree(old);
	}

	spin_lock(&minor_lock);
	if (find_next_bit(minors, end, minor) >= end) {
		for (; minor < end; ++minor)
			__set_bit(minor, minors);
		rc = 0;
	} else
		rc = -EBUSY;
	spin_unlock(&minor_lock);

	return rc;
}

static void xlbd_release_minors(unsigned int minor, unsigned int nr)
{
	unsigned int end = minor + nr;

	BUG_ON(end > nr_minors);
	spin_lock(&minor_lock);
	for (; minor < end; ++minor)
		__clear_bit(minor, minors);
	spin_unlock(&minor_lock);
}

static void blkif_restart_queue_callback(void *arg)
{
	struct blkfront_info *info = (struct blkfront_info *)arg;
	schedule_work(&info->work);
}

static int blkif_getgeo(struct block_device *bd, struct hd_geometry *hg)
{
	/* We don't have real geometry info, but let's at least return
	   values consistent with the size of the device */
	sector_t nsect = get_capacity(bd->bd_disk);
	sector_t cylinders = nsect;

	hg->heads = 0xff;
	hg->sectors = 0x3f;
	sector_div(cylinders, hg->heads * hg->sectors);
	hg->cylinders = cylinders;
	if ((sector_t)(hg->cylinders + 1) * hg->heads * hg->sectors < nsect)
		hg->cylinders = 0xffff;
	return 0;
}

static int blkif_ioctl(struct block_device *bdev, fmode_t mode,
		       unsigned command, unsigned long argument)
{
	struct blkfront_info *info = bdev->bd_disk->private_data;
	int i;

	dev_dbg(&info->xbdev->dev, "command: 0x%x, argument: 0x%lx\n",
		command, (long)argument);

	switch (command) {
	case CDROMMULTISESSION:
		dev_dbg(&info->xbdev->dev, "FIXME: support multisession CDs later\n");
		for (i = 0; i < sizeof(struct cdrom_multisession); i++)
			if (put_user(0, (char __user *)(argument + i)))
				return -EFAULT;
		return 0;

	case CDROM_GET_CAPABILITY: {
		struct gendisk *gd = info->gd;
		if (gd->flags & GENHD_FL_CD)
			return 0;
		return -EINVAL;
	}

	default:
		/*printk(KERN_ALERT "ioctl %08x not supported by Xen blkdev\n",
		  command);*/
		return -EINVAL; /* same return as native Linux */
	}

	return 0;
}

/*
 * Generate a Xen blkfront IO request from a blk layer request.  Reads
 * and writes are handled as expected.
 *
 * @req: a request struct
 */
static int blkif_queue_request(struct request *req)
{
	struct blkfront_info *info = req->rq_disk->private_data;
	unsigned long buffer_mfn;
	struct blkif_request *ring_req;
	unsigned long id;
	unsigned int fsect, lsect;
	int i, ref;
	grant_ref_t gref_head;
	struct scatterlist *sg;

	if (unlikely(info->connected != BLKIF_STATE_CONNECTED))
		return 1;

	if (xc_gnttab_alloc_grant_references(
		BLKIF_MAX_SEGMENTS_PER_REQUEST, &gref_head) < 0) {
		xc_gnttab_request_free_callback(
			&info->callback,
			blkif_restart_queue_callback,
			info,
			BLKIF_MAX_SEGMENTS_PER_REQUEST);
		return 1;
	}

	/* Fill out a communications ring structure. */
	ring_req = RING_GET_REQUEST(&info->ring, info->ring.req_prod_pvt);
	id = get_id_from_freelist(info);
	info->shadow[id].request = req;

	ring_req->id = id;
	ring_req->u.rw.sector_number = (blkif_sector_t)blk_rq_pos(req);
	ring_req->handle = info->handle;

	ring_req->operation = rq_data_dir(req) ?
		BLKIF_OP_WRITE : BLKIF_OP_READ;

#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36) )

	if (req->cmd_flags & (REQ_FLUSH | REQ_FUA)) {
		/*
		 * Ideally we can do an unordered flush-to-disk. In case the
		 * backend onlysupports barriers, use that. A barrier request
		 * a superset of FUA, so we can implement it the same
		 * way.  (It's also a FLUSH+FUA, since it is
		 * guaranteed ordered WRT previous writes.)
		 */
		ring_req->operation = info->flush_op;
	}
#endif
	ring_req->nr_segments = blk_rq_map_sg(req->q, req, info->sg);
	BUG_ON(ring_req->nr_segments > BLKIF_MAX_SEGMENTS_PER_REQUEST);

	for_each_sg(info->sg, sg, ring_req->nr_segments, i) {
		buffer_mfn = pfn_to_mfn(page_to_pfn(sg_page(sg)));
		fsect = sg->offset >> 9;
		lsect = fsect + (sg->length >> 9) - 1;
		/* install a grant reference. */
		ref = xc_gnttab_claim_grant_reference(&gref_head);
		BUG_ON(ref == -ENOSPC);

		xc_gnttab_grant_foreign_access_ref(
				ref,
				info->xbdev->otherend_id,
				buffer_mfn,
				rq_data_dir(req) );

		info->shadow[id].frame[i] = mfn_to_pfn(buffer_mfn);
		ring_req->u.rw.seg[i] =
				(struct blkif_request_segment) {
					.gref       = ref,
					.first_sect = fsect,
					.last_sect  = lsect };
	}

	info->ring.req_prod_pvt++;

	/* Keep a private copy so we can reissue requests when recovering. */
	info->shadow[id].req = *ring_req;

	xc_gnttab_free_grant_references(gref_head);

	return 0;
}


static inline void flush_requests(struct blkfront_info *info)
{
	int notify;

	RING_PUSH_REQUESTS_AND_CHECK_NOTIFY(&info->ring, notify);

	if (notify)
		xc_notify_remote_via_irq(info->irq);
}

/*
 * do_blkif_request
 *  read a block; request is in a request queue
 */
static void do_blkif_request(struct request_queue *rq)
{
	struct blkfront_info *info = NULL;
	struct request *req;
	int queued;


	queued = 0;

	while ((req = blk_peek_request(rq)) != NULL) {
		info = req->rq_disk->private_data;

		if (RING_FULL(&info->ring))
			goto wait;

		blk_start_request(req);

		if (req->cmd_type != REQ_TYPE_FS) {
			__blk_end_request_all(req, -EIO);
			continue;
		}

		pr_debug("do_blk_req %p: cmd %p, sec %lx, "
			 "(%u/%u) buffer:%p [%s]\n",
			 req, req->cmd, (unsigned long)blk_rq_pos(req),
			 blk_rq_cur_sectors(req), blk_rq_sectors(req),
			 req->buffer, rq_data_dir(req) ? "write" : "read");

		if (blkif_queue_request(req)) {
			blk_requeue_request(rq, req);
wait:
			/* Avoid pointless unplugs. */
			blk_stop_queue(rq);
			break;
		}

		queued++;
	}

	if (queued != 0)
		flush_requests(info);
}

static int xlvbd_init_blk_queue(struct gendisk *gd, u16 sector_size)
{
	struct request_queue *rq;

	rq = blk_init_queue(do_blkif_request, &blkif_io_lock);
	if (rq == NULL)
		return -1;

	queue_flag_set_unlocked(QUEUE_FLAG_VIRT, rq);

	/* Hard sector size and max sectors impersonate the equiv. hardware. */
	blk_queue_logical_block_size(rq, sector_size);
	blk_queue_max_hw_sectors(rq, 512);

	/* Each segment in a request is up to an aligned page in size. */
	blk_queue_segment_boundary(rq, PAGE_SIZE - 1);
	blk_queue_max_segment_size(rq, PAGE_SIZE);

	/* Ensure a merged request will fit in a single I/O ring slot. */
	blk_queue_max_segments(rq, BLKIF_MAX_SEGMENTS_PER_REQUEST);

	/* Make sure buffer addresses are sector-aligned. */
	blk_queue_dma_alignment(rq, 511);

	/* Make sure we don't use bounce buffers. */
	blk_queue_bounce_limit(rq, BLK_BOUNCE_ANY);

	gd->queue = rq;

	return 0;
}


#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36) )
static void xlvbd_flush(struct blkfront_info *info)
{
	blk_queue_flush(info->rq, info->feature_flush);
	printk(KERN_INFO "xen_blk: %s: %s: %s\n",
	       info->gd->disk_name,
	       info->flush_op == BLKIF_OP_WRITE_BARRIER ?
		"barrier" : (info->flush_op == BLKIF_OP_FLUSH_DISKCACHE ?
		"flush diskcache" : "barrier or flush"),
	       info->feature_flush ? "enabled" : "disabled");
}
#else
#define xlvbd_flush(a)
#endif

static int xen_translate_vdev(int vdevice, int *minor, unsigned int *offset)
{
	int major;
	major = BLKIF_MAJOR(vdevice);
	*minor = BLKIF_MINOR(vdevice);
	switch (major) {
		case XEN_IDE0_MAJOR:
			*offset = (*minor / 64) + EMULATED_HD_DISK_NAME_OFFSET;
			*minor = ((*minor / 64) * PARTS_PER_DISK) +
				EMULATED_HD_DISK_MINOR_OFFSET;
			break;
		case XEN_IDE1_MAJOR:
			*offset = (*minor / 64) + 2 + EMULATED_HD_DISK_NAME_OFFSET;
			*minor = (((*minor / 64) + 2) * PARTS_PER_DISK) +
				EMULATED_HD_DISK_MINOR_OFFSET;
			break;
		case XEN_SCSI_DISK0_MAJOR:
			*offset = (*minor / PARTS_PER_DISK) + EMULATED_SD_DISK_NAME_OFFSET;
			*minor = *minor + EMULATED_SD_DISK_MINOR_OFFSET;
			break;
		case XEN_SCSI_DISK1_MAJOR:
		case XEN_SCSI_DISK2_MAJOR:
		case XEN_SCSI_DISK3_MAJOR:
		case XEN_SCSI_DISK4_MAJOR:
		case XEN_SCSI_DISK5_MAJOR:
		case XEN_SCSI_DISK6_MAJOR:
		case XEN_SCSI_DISK7_MAJOR:
			*offset = (*minor / PARTS_PER_DISK) + 
				((major - XEN_SCSI_DISK1_MAJOR + 1) * 16) +
				EMULATED_SD_DISK_NAME_OFFSET;
			*minor = *minor +
				((major - XEN_SCSI_DISK1_MAJOR + 1) * 16 * PARTS_PER_DISK) +
				EMULATED_SD_DISK_MINOR_OFFSET;
			break;
		case XEN_SCSI_DISK8_MAJOR:
		case XEN_SCSI_DISK9_MAJOR:
		case XEN_SCSI_DISK10_MAJOR:
		case XEN_SCSI_DISK11_MAJOR:
		case XEN_SCSI_DISK12_MAJOR:
		case XEN_SCSI_DISK13_MAJOR:
		case XEN_SCSI_DISK14_MAJOR:
		case XEN_SCSI_DISK15_MAJOR:
			*offset = (*minor / PARTS_PER_DISK) + 
				((major - XEN_SCSI_DISK8_MAJOR + 8) * 16) +
				EMULATED_SD_DISK_NAME_OFFSET;
			*minor = *minor +
				((major - XEN_SCSI_DISK8_MAJOR + 8) * 16 * PARTS_PER_DISK) +
				EMULATED_SD_DISK_MINOR_OFFSET;
			break;
		case XENVBD_MAJOR:
			*offset = *minor / PARTS_PER_DISK;
			break;
		default:
			printk(KERN_WARNING "xen_blk: your disk configuration is "
					"incorrect, please use an xvd device instead\n");
			return -ENODEV;
	}
	return 0;
}

static int xlvbd_alloc_gendisk(blkif_sector_t capacity,
			       struct blkfront_info *info,
			       u16 vdisk_info, u16 sector_size)
{
	struct gendisk *gd;
	int nr_minors = 1;
	int err;
	unsigned int offset;
	int minor;
	int nr_parts;

	BUG_ON(info->gd != NULL);
	BUG_ON(info->rq != NULL);

	if ((info->vdevice>>EXT_SHIFT) > 1) {
		/* this is above the extended range; something is wrong */
		printk(KERN_WARNING "xen_blk: vdevice 0x%x is above the extended range; ignoring\n", info->vdevice);
		return -ENODEV;
	}

	if (!VDEV_IS_EXTENDED(info->vdevice)) {
		err = xen_translate_vdev(info->vdevice, &minor, &offset);
		if (err)
			return err;		
 		nr_parts = PARTS_PER_DISK;
	} else {
		minor = BLKIF_MINOR_EXT(info->vdevice);
		nr_parts = PARTS_PER_EXT_DISK;
		offset = minor / nr_parts;
		if (xen_hvm_domain() && offset < EMULATED_HD_DISK_NAME_OFFSET + 4)
			printk(KERN_WARNING "xen_blk: vdevice 0x%x might conflict with "
					"emulated IDE disks,\n\t choose an xvd device name"
					"from xvde on\n", info->vdevice);
	}
	err = -ENODEV;

	if ((minor % nr_parts) == 0)
		nr_minors = nr_parts;

	err = xlbd_reserve_minors(minor, nr_minors);
	if (err)
		goto out;
	err = -ENODEV;

	gd = alloc_disk(nr_minors);
	if (gd == NULL)
		goto release;

	if (nr_minors > 1) {
		if (offset < 26)
			sprintf(gd->disk_name, "%s%c", DEV_NAME, 'a' + offset);
		else
			sprintf(gd->disk_name, "%s%c%c", DEV_NAME,
				'a' + ((offset / 26)-1), 'a' + (offset % 26));
	} else {
		if (offset < 26)
			sprintf(gd->disk_name, "%s%c%d", DEV_NAME,
				'a' + offset,
				minor & (nr_parts - 1));
		else
			sprintf(gd->disk_name, "%s%c%c%d", DEV_NAME,
				'a' + ((offset / 26) - 1),
				'a' + (offset % 26),
				minor & (nr_parts - 1));
	}

	gd->major = XENVBD_MAJOR;
	gd->first_minor = minor;
	gd->fops = &xlvbd_block_fops;
	gd->private_data = info;
	gd->driverfs_dev = &(info->xbdev->dev);
	set_capacity(gd, capacity);

	if (xlvbd_init_blk_queue(gd, sector_size)) {
		del_gendisk(gd);
		goto release;
	}

	info->rq = gd->queue;
	info->gd = gd;

	xlvbd_flush(info);

	if (vdisk_info & VDISK_READONLY)
		set_disk_ro(gd, 1);

	if (vdisk_info & VDISK_REMOVABLE)
		gd->flags |= GENHD_FL_REMOVABLE;

	if (vdisk_info & VDISK_CDROM)
		gd->flags |= GENHD_FL_CD;

	return 0;

 release:
	xlbd_release_minors(minor, nr_minors);
 out:
	return err;
}

static void xlvbd_release_gendisk(struct blkfront_info *info)
{
	unsigned int minor, nr_minors;
	unsigned long flags;

	if (info->rq == NULL)
		return;

	spin_lock_irqsave(&blkif_io_lock, flags);

	/* No more blkif_request(). */
	blk_stop_queue(info->rq);

	/* No more gnttab callback work. */
	xc_gnttab_cancel_free_callback(&info->callback);
	spin_unlock_irqrestore(&blkif_io_lock, flags);

	/* Flush gnttab callback work. Must be done with no locks held. */
	flush_work_sync(&info->work);

	del_gendisk(info->gd);

	minor = info->gd->first_minor;
	nr_minors = info->gd->minors;
	xlbd_release_minors(minor, nr_minors);

	blk_cleanup_queue(info->rq);
	info->rq = NULL;

	put_disk(info->gd);
	info->gd = NULL;
}

static void kick_pending_request_queues(struct blkfront_info *info)
{
	if (!RING_FULL(&info->ring)) {
		/* Re-enable calldowns. */
		blk_start_queue(info->rq);
		/* Kick things off immediately. */
		do_blkif_request(info->rq);
	}
}

static void blkif_restart_queue(struct work_struct *work)
{
	struct blkfront_info *info = container_of(work, struct blkfront_info, work);

	spin_lock_irq(&blkif_io_lock);
	if (info->connected == BLKIF_STATE_CONNECTED)
		kick_pending_request_queues(info);
	spin_unlock_irq(&blkif_io_lock);
}

static void blkif_free(struct blkfront_info *info, int suspend)
{
	/* Prevent new requests being issued until we fix things up. */
	spin_lock_irq(&blkif_io_lock);
	info->connected = BLKIF_STATE_RINGFLUSH;
	/* No more blkif_request(). */
	if (info->rq)
		blk_stop_queue(info->rq);
	/* No more gnttab callback work. */
	xc_gnttab_cancel_free_callback(&info->callback);
	spin_unlock_irq(&blkif_io_lock);

	/* Flush gnttab callback work. Must be done with no locks held. */
	flush_work_sync(&info->work);

	/* give blkback the chance to consume the ring */
	printk(KERN_INFO "xen_blk: pm suspend, waiting until all requests consumed\n");
	flush_requests(info);
	while (wait_event_interruptible(info->rflushq,
		info->ring.req_prod_pvt == info->ring.rsp_cons
		&& !RING_HAS_UNCONSUMED_RESPONSES(&info->ring))
			== -ERESTARTSYS)
		;

	info->connected = suspend ?
		BLKIF_STATE_SUSPENDED : BLKIF_STATE_DISCONNECTED;

	/* Free resources associated with old device channel. */
	if (info->ring_ref != GRANT_INVALID_REF) {
		xc_gnttab_end_foreign_access(info->ring_ref, 0,
					     (unsigned long)info->ring.sring);
		info->ring_ref = GRANT_INVALID_REF;
		info->ring.sring = NULL;
	}

	if (info->irq)
		xc_unbind_from_irqhandler(info->irq, info);
	info->evtchn = info->irq = 0;

}

static void blkif_completion(struct blk_shadow *s)
{
	int i;
	for (i = 0; i < s->req.nr_segments; i++)
		xc_gnttab_end_foreign_access(s->req.u.rw.seg[i].gref, 0, 0UL);
}

static irqreturn_t blkif_interrupt(int irq, void *dev_id)
{
	struct request *req;
	struct blkif_response *bret;
	RING_IDX i, rp;
	unsigned long flags;
	struct blkfront_info *info = (struct blkfront_info *)dev_id;
	int error;

	spin_lock_irqsave(&blkif_io_lock, flags);

	if (unlikely(info->connected != BLKIF_STATE_CONNECTED
				&& info->connected != BLKIF_STATE_RINGFLUSH)) {
		spin_unlock_irqrestore(&blkif_io_lock, flags);
		return IRQ_HANDLED;
	}

 again:
	rp = info->ring.sring->rsp_prod;
	rmb(); /* Ensure we see queued responses up to 'rp'. */

	for (i = info->ring.rsp_cons; i != rp; i++) {
		unsigned long id;

		bret = RING_GET_RESPONSE(&info->ring, i);
		id   = bret->id;
		req  = info->shadow[id].request;

		blkif_completion(&info->shadow[id]);

		add_id_to_freelist(info, id);

		error = (bret->status == BLKIF_RSP_OKAY) ? 0 : -EIO;
		switch (bret->operation) {
		case BLKIF_OP_FLUSH_DISKCACHE:
		case BLKIF_OP_WRITE_BARRIER:
			if (unlikely(bret->status == BLKIF_RSP_EOPNOTSUPP)) {
				printk(KERN_WARNING "xen_blk: %s: write %s op failed\n",
				       info->flush_op == BLKIF_OP_WRITE_BARRIER ?
				       "barrier" :  "flush disk cache",
				       info->gd->disk_name);
				error = -EOPNOTSUPP;
			}
			if (unlikely(bret->status == BLKIF_RSP_ERROR &&
				     info->shadow[id].req.nr_segments == 0)) {
				printk(KERN_WARNING "xen_blk: %s: empty write %s op failed\n",
				       info->flush_op == BLKIF_OP_WRITE_BARRIER ?
				       "barrier" :  "flush disk cache",
				       info->gd->disk_name);
				error = -EOPNOTSUPP;
			}
			if (unlikely(error)) {
				if (error == -EOPNOTSUPP)
					error = 0;
				info->feature_flush = 0;
				info->flush_op = 0;
				xlvbd_flush(info);
			}
			/* fall through */
		case BLKIF_OP_READ:
		case BLKIF_OP_WRITE:
			if (unlikely(bret->status != BLKIF_RSP_OKAY))
				dev_dbg(&info->xbdev->dev, "Bad return from blkdev data "
					"request: %x\n", bret->status);

			__blk_end_request_all(req, error);
			break;
		default:
			BUG();
		}
	}

	info->ring.rsp_cons = i;

	if (i != info->ring.req_prod_pvt) {
		int more_to_do;
		RING_FINAL_CHECK_FOR_RESPONSES(&info->ring, more_to_do);
		if (more_to_do)
			goto again;
	} else
		info->ring.sring->rsp_event = i + 1;

	if (unlikely(info->connected == BLKIF_STATE_RINGFLUSH))
		wake_up_interruptible(&info->rflushq);
	else
		kick_pending_request_queues(info);

	spin_unlock_irqrestore(&blkif_io_lock, flags);

	return IRQ_HANDLED;
}


static int setup_blkring(struct xenbus_device *dev,
			 struct blkfront_info *info)
{
	struct blkif_sring *sring;
	int err;

	info->ring_ref = GRANT_INVALID_REF;

	sring = (struct blkif_sring *)__get_free_page(GFP_NOIO | __GFP_HIGH);
	if (!sring) {
		xc_xenbus_dev_fatal(dev, -ENOMEM, "allocating shared ring");
		return -ENOMEM;
	}
	SHARED_RING_INIT(sring);
	FRONT_RING_INIT(&info->ring, sring, PAGE_SIZE);

	err = xc_xenbus_grant_ring(dev, virt_to_mfn(info->ring.sring));
	if (err < 0) {
		free_page((unsigned long)sring);
		info->ring.sring = NULL;
		goto fail;
	}
	info->ring_ref = err;

	err = xc_xenbus_alloc_evtchn(dev, &info->evtchn);
	if (err)
		goto fail;

	err = xc_bind_evtchn_to_irqhandler(info->evtchn,
					blkif_interrupt,
					BLKFRONT_IRQF, "blkif", info);
	if (err <= 0) {
		xc_xenbus_dev_fatal(dev, err,
				 "bind_evtchn_to_irqhandler failed");
		goto fail;
	}
	info->irq = err;

	return 0;
fail:
	blkif_free(info, 0);
	return err;
}


/* Common code used when first setting up, and when resuming. */
static int talk_to_blkback(struct xenbus_device *dev,
			   struct blkfront_info *info)
{
	const char *message = NULL;
	struct xenbus_transaction xbt;
	int err;

	/* Create shared ring, alloc event channel. */
	err = setup_blkring(dev, info);
	if (err)
		goto out;

again:
	err = xc_xenbus_transaction_start(&xbt);
	if (err) {
		xc_xenbus_dev_fatal(dev, err, "starting transaction");
		goto destroy_blkring;
	}

	err = xc_xenbus_printf(xbt, dev->nodename,
	 		    "ring-ref", "%u", info->ring_ref);
	if (err) {
		message = "writing ring-ref";
		goto abort_transaction;
	}

	err = xc_xenbus_printf(xbt, dev->nodename,
			    "event-channel", "%u", info->evtchn);
	if (err) {
		message = "writing event-channel";
		goto abort_transaction;
	}
	err = xc_xenbus_printf(xbt, dev->nodename, "protocol", "%s",
			    XEN_IO_PROTO_ABI_NATIVE);
	if (err) {
		message = "writing protocol";
		goto abort_transaction;
	}

	err = xc_xenbus_transaction_end(xbt, 0);
	if (err) {
		if (err == -EAGAIN)
			goto again;
		xc_xenbus_dev_fatal(dev, err, "completing transaction");
		goto destroy_blkring;
	}

	xc_xenbus_switch_state(dev, XenbusStateInitialised);

	return 0;

 abort_transaction:
	xc_xenbus_transaction_end(xbt, 1);
	if (message)
		xc_xenbus_dev_fatal(dev, err, "%s", message);
 destroy_blkring:
	blkif_free(info, 0);
 out:
	return err;
}

/**
 * Entry point to this code when a new device is created.  Allocate the basic
 * structures and the ring buffer for communication with the backend, and
 * inform the backend of the appropriate details for those.  Switch to
 * Initialised state.
 */
static int blkfront_probe(struct xenbus_device *dev,
			  const struct xenbus_device_id *id)
{
	int err, vdevice, i;
	struct blkfront_info *info;

	/* FIXME: Use dynamic device id if this is not set. */
	err = xc_xenbus_scanf(XBT_NIL, dev->nodename,
			   "virtual-device", "%i", &vdevice);
	if (err != 1) {
		/* go looking in the extended area instead */
		err = xc_xenbus_scanf(XBT_NIL, dev->nodename, "virtual-device-ext",
				   "%i", &vdevice);
		if (err != 1) {
			xc_xenbus_dev_fatal(dev, err, "reading virtual-device");
			return err;
		}
	}

	if (xen_hvm_domain()) {
		char *type;
		int len;
		/* do not create a PV cdrom device if we are an HVM guest */
		type = xc_xenbus_read(XBT_NIL, dev->nodename, "device-type", &len);
		if (IS_ERR(type))
			return -ENODEV;
		if (strncmp(type, "cdrom", 5) == 0) {
			kfree(type);
			return -ENODEV;
		}
		kfree(type);
	}
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		xc_xenbus_dev_fatal(dev, -ENOMEM, "allocating info structure");
		return -ENOMEM;
	}

	mutex_init(&info->mutex);
	init_waitqueue_head(&info->rflushq);
	info->xbdev = dev;
	info->vdevice = vdevice;
	info->connected = BLKIF_STATE_DISCONNECTED;
	INIT_WORK(&info->work, blkif_restart_queue);

	for (i = 0; i < BLK_RING_SIZE; i++)
		info->shadow[i].req.id = i+1;
	info->shadow[BLK_RING_SIZE-1].req.id = 0x0fffffff;

	/* Front end dir is a number, which is used as the id. */
	info->handle = simple_strtoul(strrchr(dev->nodename, '/')+1, NULL, 0);
	dev_set_drvdata(&dev->dev, info);

	err = talk_to_blkback(dev, info);
	if (err) {
		kfree(info);
		dev_set_drvdata(&dev->dev, NULL);
		return err;
	}

	return 0;
}


static int blkif_recover(struct blkfront_info *info)
{
	int i;
	struct blkif_request *req;
	struct blk_shadow *copy;
	int j;

	/* Stage 1: Make a safe copy of the shadow state. */
	copy = kmalloc(sizeof(info->shadow),
		       GFP_NOIO | __GFP_REPEAT | __GFP_HIGH);
	if (!copy)
		return -ENOMEM;
	memcpy(copy, info->shadow, sizeof(info->shadow));

	/* Stage 2: Set up free list. */
	memset(&info->shadow, 0, sizeof(info->shadow));
	for (i = 0; i < BLK_RING_SIZE; i++)
		info->shadow[i].req.id = i+1;
	info->shadow_free = info->ring.req_prod_pvt;
	info->shadow[BLK_RING_SIZE-1].req.id = 0x0fffffff;

	/* Stage 3: Find pending requests and requeue them. */
	for (i = 0; i < BLK_RING_SIZE; i++) {
		/* Not in use? */
		if (!copy[i].request)
			continue;

		/* Grab a request slot and copy shadow state into it. */
		req = RING_GET_REQUEST(&info->ring, info->ring.req_prod_pvt);
		*req = copy[i].req;

		/* We get a new request id, and must reset the shadow state. */
		req->id = get_id_from_freelist(info);
		memcpy(&info->shadow[req->id], &copy[i], sizeof(copy[i]));

		/* Rewrite any grant references invalidated by susp/resume. */
		for (j = 0; j < req->nr_segments; j++)
			xc_gnttab_grant_foreign_access_ref(
				req->u.rw.seg[j].gref,
				info->xbdev->otherend_id,
				pfn_to_mfn(info->shadow[req->id].frame[j]),
				rq_data_dir(info->shadow[req->id].request));
		info->shadow[req->id].req = *req;

		info->ring.req_prod_pvt++;
	}

	kfree(copy);

	xc_xenbus_switch_state(info->xbdev, XenbusStateConnected);

	spin_lock_irq(&blkif_io_lock);

	/* Now safe for us to use the shared ring */
	info->connected = BLKIF_STATE_CONNECTED;

	/* Send off requeued requests */
	flush_requests(info);

	/* Kick any other new requests queued since we resumed */
	kick_pending_request_queues(info);

	spin_unlock_irq(&blkif_io_lock);

	return 0;
}

static int blkfront_suspend(struct xenbus_device *dev)
{
	struct blkfront_info *info = dev_get_drvdata(&dev->dev);
	blkif_free(info, 1);
	return 0;
}

/**
 * We are reconnecting to the backend, due to a suspend/resume, or a backend
 * driver restart.  We tear down our blkif structure and recreate it, but
 * leave the device-layer structures intact so that this is transparent to the
 * rest of the kernel.
 */
static int blkfront_resume(struct xenbus_device *dev)
{
	struct blkfront_info *info = dev_get_drvdata(&dev->dev);
	int err;

	dev_dbg(&dev->dev, "blkfront_resume: %s\n", dev->nodename);

	err = talk_to_blkback(dev, info);
	if (info->connected == BLKIF_STATE_SUSPENDED && !err)
		err = blkif_recover(info);

	return err;
}

static void
blkfront_closing(struct blkfront_info *info)
{
	struct xenbus_device *xbdev = info->xbdev;
	struct block_device *bdev = NULL;

	mutex_lock(&info->mutex);

	if (xbdev->state == XenbusStateClosing) {
		mutex_unlock(&info->mutex);
		return;
	}

	if (info->gd)
		bdev = bdget_disk(info->gd, 0);

	mutex_unlock(&info->mutex);

	if (!bdev) {
		xc_xenbus_frontend_closed(xbdev);
		return;
	}

	mutex_lock(&bdev->bd_mutex);

	if (bdev->bd_openers) {
		xc_xenbus_dev_error(xbdev, -EBUSY,
				 "Device in use; refusing to close");
		xc_xenbus_switch_state(xbdev, XenbusStateClosing);
	} else {
		xlvbd_release_gendisk(info);
		xc_xenbus_frontend_closed(xbdev);
	}

	mutex_unlock(&bdev->bd_mutex);
	bdput(bdev);
}

/*
 * Invoked when the backend is finally 'ready' (and has told produced
 * the details about the physical device - #sectors, size, etc).
 */
static void blkfront_connect(struct blkfront_info *info)
{
	unsigned long long sectors;
	unsigned long sector_size;
	unsigned int binfo;
	int err;
	int barrier, flush;

	switch (info->connected) {
	case BLKIF_STATE_CONNECTED:
		/*
		 * Potentially, the back-end may be signalling
		 * a capacity change; update the capacity.
		 */
		err = xc_xenbus_scanf(XBT_NIL, info->xbdev->otherend,
				   "sectors", "%Lu", &sectors);
		if (XENBUS_EXIST_ERR(err))
			return;
		printk(KERN_INFO "Setting capacity to %Lu\n",
		       sectors);
		set_capacity(info->gd, sectors);
		revalidate_disk(info->gd);

		/* fall through */
	case BLKIF_STATE_SUSPENDED:
		return;

	default:
		break;
	}

	dev_dbg(&info->xbdev->dev, "%s:%s.\n",
		__func__, info->xbdev->otherend);

	err = xc_xenbus_gather(XBT_NIL, info->xbdev->otherend,
			    "sectors", "%llu", &sectors,
			    "info", "%u", &binfo,
			    "sector-size", "%lu", &sector_size,
			    NULL);
	if (err) {
		xc_xenbus_dev_fatal(info->xbdev, err,
				 "reading backend fields at %s",
				 info->xbdev->otherend);
		return;
	}

	info->feature_flush = 0;
	info->flush_op = 0;


#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,36) || ( (defined(RHEL_RELEASE_CODE) && RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(6,5) ) ) )
	err = xc_xenbus_gather(XBT_NIL, info->xbdev->otherend,
			    "feature-barrier", "%d", &barrier,
			    NULL);
	/*
	 * If there's no "feature-barrier" defined, then it means
	 * we're dealing with a very old backend which writes
	 * synchronously; nothing to do.
	 *
	 * If there are barriers, then we use flush.
	 */
	if (!err && barrier) {
		info->feature_flush = REQ_FLUSH | REQ_FUA;
		info->flush_op = BLKIF_OP_WRITE_BARRIER;
	}
	/*
	 * And if there is "feature-flush-cache" use that above
	 * barriers.
	 */
	err = xc_xenbus_gather(XBT_NIL, info->xbdev->otherend,
			    "feature-flush-cache", "%d", &flush,
			    NULL);

	if (!err && flush) {
		info->feature_flush = REQ_FLUSH;
		info->flush_op = BLKIF_OP_FLUSH_DISKCACHE;
	}
#endif		
	err = xlvbd_alloc_gendisk(sectors, info, binfo, sector_size);
	if (err) {
		xc_xenbus_dev_fatal(info->xbdev, err, "xlvbd_add at %s",
				 info->xbdev->otherend);
		return;
	}

	xc_xenbus_switch_state(info->xbdev, XenbusStateConnected);

	/* Kick pending requests. */
	spin_lock_irq(&blkif_io_lock);
	info->connected = BLKIF_STATE_CONNECTED;
	kick_pending_request_queues(info);
	spin_unlock_irq(&blkif_io_lock);

	add_disk(info->gd);

	info->is_ready = 1;
}

/**
 * Callback received when the backend's state changes.
 */
static void blkback_changed(struct xenbus_device *dev,
			    enum xenbus_state backend_state)
{
	struct blkfront_info *info = dev_get_drvdata(&dev->dev);

	dev_dbg(&dev->dev, "blkfront:blkback_changed to state %d.\n", backend_state);

	switch (backend_state) {
	case XenbusStateInitialising:
	case XenbusStateInitWait:
	case XenbusStateInitialised:
	case XenbusStateReconfiguring:
	case XenbusStateReconfigured:
	case XenbusStateUnknown:
	case XenbusStateClosed:
		break;

	case XenbusStateConnected:
		blkfront_connect(info);
		break;

	case XenbusStateClosing:
		blkfront_closing(info);
		break;
	}
}

static int blkfront_remove(struct xenbus_device *xbdev)
{
	struct blkfront_info *info = dev_get_drvdata(&xbdev->dev);
	struct block_device *bdev = NULL;
	struct gendisk *disk;

	dev_dbg(&xbdev->dev, "%s removed", xbdev->nodename);

	blkif_free(info, 0);

	mutex_lock(&info->mutex);

	disk = info->gd;
	if (disk)
		bdev = bdget_disk(disk, 0);

	info->xbdev = NULL;
	mutex_unlock(&info->mutex);

	if (!bdev) {
		kfree(info);
		return 0;
	}

	/*
	 * The xbdev was removed before we reached the Closed
	 * state. See if it's safe to remove the disk. If the bdev
	 * isn't closed yet, we let release take care of it.
	 */

	mutex_lock(&bdev->bd_mutex);
	info = disk->private_data;

	dev_warn(disk_to_dev(disk),
		 "%s was hot-unplugged, %d stale handles\n",
		 xbdev->nodename, bdev->bd_openers);

	if (info && !bdev->bd_openers) {
		xlvbd_release_gendisk(info);
		disk->private_data = NULL;
		kfree(info);
	}

	mutex_unlock(&bdev->bd_mutex);
	bdput(bdev);

	return 0;
}

static int blkfront_is_ready(struct xenbus_device *dev)
{
	struct blkfront_info *info = dev_get_drvdata(&dev->dev);

	return info->is_ready && info->xbdev;
}

static int blkif_open(struct block_device *bdev, fmode_t mode)
{
	struct gendisk *disk = bdev->bd_disk;
	struct blkfront_info *info;
	int err = 0;

	mutex_lock(&blkfront_mutex);

	info = disk->private_data;
	if (!info) {
		/* xbdev gone */
		err = -ERESTARTSYS;
		goto out;
	}

	mutex_lock(&info->mutex);

	if (!info->gd)
		/* xbdev is closed */
		err = -ERESTARTSYS;

	mutex_unlock(&info->mutex);

out:
	mutex_unlock(&blkfront_mutex);
	return err;
}

static int blkif_release(struct gendisk *disk, fmode_t mode)
{
	struct blkfront_info *info = disk->private_data;
	struct block_device *bdev;
	struct xenbus_device *xbdev;

	mutex_lock(&blkfront_mutex);

	bdev = bdget_disk(disk, 0);
	bdput(bdev);

	if (bdev->bd_openers)
		goto out;

	/*
	 * Check if we have been instructed to close. We will have
	 * deferred this request, because the bdev was still open.
	 */

	mutex_lock(&info->mutex);
	xbdev = info->xbdev;

	if (xbdev && xbdev->state == XenbusStateClosing) {
		/* pending switch to state closed */
		dev_info(disk_to_dev(bdev->bd_disk), "releasing disk\n");
		xlvbd_release_gendisk(info);
		xc_xenbus_frontend_closed(info->xbdev);
 	}

	mutex_unlock(&info->mutex);

	if (!xbdev) {
		/* sudden device removal */
		dev_info(disk_to_dev(bdev->bd_disk), "releasing disk\n");
		xlvbd_release_gendisk(info);
		disk->private_data = NULL;
		kfree(info);
	}

out:
	mutex_unlock(&blkfront_mutex);
	return 0;
}

static const struct block_device_operations xlvbd_block_fops =
{
	.owner = THIS_MODULE,
	.open = blkif_open,
	.release = blkif_release,
	.getgeo = blkif_getgeo,
	.ioctl = blkif_ioctl,
};


static const struct xenbus_device_id blkfront_ids[] = {
	{ "vbd" },
	{ "" }
};

static struct xenbus_driver blkfront = {
	.name = "xc-vbd",
	.owner = THIS_MODULE,
	.ids = blkfront_ids,
	.probe = blkfront_probe,
	.remove = blkfront_remove,
	.suspend = blkfront_suspend,
	.resume = blkfront_resume,
	.otherend_changed = blkback_changed,
	.is_ready = blkfront_is_ready,
};

int xc_xen_unplug_emul_disks(void);
int xc_hard_unplug_qemu_disks(void);
int xc_is_initramfs_time(void);
static int __init xlblk_init(void)
{
	int err;
	if (!xen_hvm_domain()) {
		printk(KERN_INFO "xen_blk: cannot load on a non Xen HVM platform\n");
		return -ENODEV;
	}

	if (register_blkdev(XENVBD_MAJOR, DEV_NAME)) {
		printk(KERN_WARNING "xen_blk: can't get major %d with name %s\n",
		       XENVBD_MAJOR, DEV_NAME);
		return -ENODEV;
	}

	printk(KERN_INFO "xen_blk: trying to unplug the emulated disks\n");
	err = xc_xen_unplug_emul_disks();
	if (err && err != -ENOENT) {
		xc_suppress_hibernate = 1;
		printk(KERN_WARNING "xen_blk: cannot unplug ide emulated devices, giving up\n");
		goto clean_blkdev;
	}
	xc_hard_unplug_qemu_disks();

	return xenbus_register_frontend(&blkfront);

clean_blkdev:
	unregister_blkdev(XENVBD_MAJOR, DEV_NAME);
	return err;
}
module_init(xlblk_init);


static void __exit xlblk_exit(void)
{
	xc_xenbus_unregister_driver(&blkfront);
	unregister_blkdev(XENVBD_MAJOR, DEV_NAME);
}
module_exit(xlblk_exit);

MODULE_DESCRIPTION("Xen virtual block device frontend");
MODULE_LICENSE("GPL");
MODULE_ALIAS_BLOCKDEV_MAJOR(XENVBD_MAJOR);
MODULE_ALIAS("xen:vbd");
MODULE_ALIAS("xenblk");