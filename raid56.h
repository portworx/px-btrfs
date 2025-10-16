/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2012 Fusion-io  All rights reserved.
 * Copyright (C) 2012 Intel Corp. All rights reserved.
 */

#ifndef BTRFS_RAID56_H
#define BTRFS_RAID56_H

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/bio.h>

static inline int nr_parity_stripes(const struct map_lookup *map)
{
	if (map->type & BTRFS_BLOCK_GROUP_RAID5)
		return 1;
	else if (map->type & BTRFS_BLOCK_GROUP_RAID6)
		return 2;
	else
		return 0;
}

static inline int nr_data_stripes(const struct map_lookup *map)
{
	return map->num_stripes - nr_parity_stripes(map);
}
#define RAID5_P_STRIPE ((u64)-2)
#define RAID6_Q_STRIPE ((u64)-1)

#define is_parity_stripe(x) (((x) == RAID5_P_STRIPE) ||		\
			     ((x) == RAID6_Q_STRIPE))

struct btrfs_device;

/* Forward declarations */
struct btrfs_work;
struct btrfs_io_context;

enum btrfs_rbio_ops {
	BTRFS_RBIO_WRITE,
	BTRFS_RBIO_READ_REBUILD,
	BTRFS_RBIO_PARITY_SCRUB,
	BTRFS_RBIO_REBUILD_MISSING,
};

/* Structure for kernel 6.12 trace events */
struct raid56_bio_trace_info {
	u64 devid;
	u64 offset;
	u32 stripe_nr;
};

struct btrfs_raid_bio {
	struct btrfs_io_context *bioc;

	/* while we're doing rmw on a stripe
	 * we put it into a hash table so we can
	 * lock the stripe and merge more rbios
	 * into it.
	 */
	struct list_head hash_list;

	/*
	 * LRU list for the stripe cache
	 */
	struct list_head stripe_cache;

	/*
	 * for scheduling work in the helper threads
	 */
	struct btrfs_work work;

	/*
	 * bio list and bio_list_lock are used
	 * to add more bios into the stripe
	 * in hopes of avoiding the full rmw
	 */
	struct bio_list bio_list;
	spinlock_t bio_list_lock;

	/* also protected by the bio_list_lock, the
	 * plug list is used by the plugging code
	 * to collect partial bios while plugged.  The
	 * stripe locking code also uses it to hand off
	 * the stripe lock to the next pending IO
	 */
	struct list_head plug_list;

	/*
	 * flags that tell us if it is safe to
	 * merge with this bio
	 */
	unsigned long flags;

	/* size of each individual stripe on disk */
	int stripe_len;

	/* number of data stripes (no p/q) */
	int nr_data;

	int real_stripes;

	int stripe_npages;
	/*
	 * set if we're doing a parity rebuild
	 * for a read from higher up, which is handled
	 * differently from a parity rebuild as part of
	 * rmw
	 */
	enum btrfs_rbio_ops operation;

	/* first bad stripe */
	int faila;

	/* second bad stripe (for raid6 use) */
	int failb;

	int scrubp;
	/*
	 * number of pages needed to represent the full
	 * stripe
	 */
	int nr_pages;

	/*
	 * size of all the bios in the bio_list.  This
	 * helps us decide if the rbio maps to a full
	 * stripe or not
	 */
	int bio_list_bytes;

	int generic_bio_cnt;

	refcount_t refs;

	atomic_t stripes_pending;

	atomic_t error;
	/*
	 * these are two arrays of pointers.  We allocate the
	 * rbio big enough to hold them both and setup their
	 * locations when the rbio is allocated
	 */

	/* pointers to pages that we allocated for
	 * reading/writing stripes directly from the disk (including P/Q)
	 */
	struct page **stripe_pages;

	/*
	 * pointers to the pages in the bio_list.  Stored
	 * here for faster lookup
	 */
	struct page **bio_pages;

	/*
	 * bitmap to record which horizontal stripe has data
	 */
	unsigned long *dbitmap;

	/* allocated with real_stripes-many pointers for finish_*() calls */
	void **finish_pointers;

	/* allocated with stripe_npages-many bits for finish_*() calls */
	unsigned long *finish_pbitmap;
};

int raid56_parity_recover(struct bio *bio, struct btrfs_io_context *bioc,
			  u64 stripe_len, int mirror_num, int generic_io);
int raid56_parity_write(struct bio *bio, struct btrfs_io_context *bioc,
			u64 stripe_len);

void raid56_add_scrub_pages(struct btrfs_raid_bio *rbio, struct page *page,
			    u64 logical);

struct btrfs_raid_bio *raid56_parity_alloc_scrub_rbio(struct bio *bio,
				struct btrfs_io_context *bioc, u64 stripe_len,
				struct btrfs_device *scrub_dev,
				unsigned long *dbitmap, int stripe_nsectors);
void raid56_parity_submit_scrub_rbio(struct btrfs_raid_bio *rbio);

struct btrfs_raid_bio *
raid56_alloc_missing_rbio(struct bio *bio, struct btrfs_io_context *bioc,
			  u64 length);
void raid56_submit_missing_rbio(struct btrfs_raid_bio *rbio);

int btrfs_alloc_stripe_hash_table(struct btrfs_fs_info *info);
void btrfs_free_stripe_hash_table(struct btrfs_fs_info *info);

/* Compatibility function for kernel 6.12 trace events */
struct btrfs_fs_info *btrfs_raid_bio_to_fs_info(const struct btrfs_raid_bio *rbio);

/* Macro to help trace events access rbio->bioc->fs_info */
#define RBIO_BIOC_FS_INFO(rbio) btrfs_raid_bio_to_fs_info(rbio)

#endif
