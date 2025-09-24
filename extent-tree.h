/* SPDX-License-Identifier: GPL-2.0 */

#ifndef BTRFS_EXTENT_TREE_H
#define BTRFS_EXTENT_TREE_H

#include "volumes.h"

struct btrfs_free_cluster;

/* Size class for block groups - kernel 6.12 compatibility */
enum btrfs_block_group_size_class {
	BTRFS_BG_SZ_NONE,
	BTRFS_BG_SZ_SMALL,
	BTRFS_BG_SZ_MEDIUM,
	BTRFS_BG_SZ_LARGE,
};

/* Allocation policy for extent allocation - kernel 6.12 compatibility */
enum btrfs_extent_allocation_policy {
	BTRFS_EXTENT_ALLOC_CLUSTERED,
	BTRFS_EXTENT_ALLOC_ZONED,
};

/*
 * Structure used internally for find_free_extent() function.  Wraps needed
 * parameters.
 */
struct find_free_extent_ctl {
	/* Basic allocation info */
	u64 ram_bytes;
	u64 num_bytes;
	u64 min_alloc_size;
	u64 empty_size;
	u64 flags;
	int delalloc;

	/* Where to start the search inside the bg */
	u64 search_start;

	/* For clustered allocation */
	u64 empty_cluster;
	struct btrfs_free_cluster *last_ptr;
	bool use_cluster;

	bool have_caching_bg;
	bool orig_have_caching_bg;

	/* Allocation is called for tree-log */
	bool for_treelog;

	/* Allocation is called for data relocation */
	bool for_data_reloc;

	/* RAID index, converted from flags */
	int index;

	/*
	 * Current loop number, check find_free_extent_update_loop() for details
	 */
	int loop;

	/*
	 * Whether we're refilling a cluster, if true we need to re-search
	 * current block group but don't try to refill the cluster again.
	 */
	bool retry_clustered;

	/*
	 * Whether we're updating free space cache, if true we need to re-search
	 * current block group but don't try updating free space cache again.
	 */
	bool retry_unclustered;

	/*
	 * Set to true if we're retrying the allocation on this block group
	 * after waiting for caching progress, this is so that we retry only
	 * once before moving on to another block group.
	 */
	bool retry_uncached;

	/* If current block group is cached */
	int cached;

	/* Max contiguous hole found */
	u64 max_extent_size;

	/* Total free space from free space cache, not always contiguous */
	u64 total_free_space;

	/* Found result */
	u64 found_offset;

	/* Hint where to start looking for an empty space */
	u64 hint_byte;

	/* Allocation policy */
	enum btrfs_extent_allocation_policy policy;

	/* Whether or not the allocator is currently following a hint */
	bool hinted;

	/* Size class of block groups to prefer in early loops */
	enum btrfs_block_group_size_class size_class;
};

#endif
