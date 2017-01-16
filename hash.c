/*
 * Copyright (C) 2014 Filipe David Borba Manana <fdmanana@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <crypto/hash.h>
#include <linux/err.h>
#include "hash.h"
#include "version.h"

static struct crypto_shash *tfm;

#if BTRFS_RHEL_VERSION_CODE <= BTRFS_RHEL_KERNEL_VERSION(3,10,0,123,8,1)
// Copied from linux/err.h
static inline int __must_check PTR_ERR_OR_ZERO(__force const void *ptr)
{
        if (IS_ERR(ptr))
                return PTR_ERR(ptr);
        else
                return 0;
}
#endif

int __init btrfs_hash_init(void)
{
	tfm = crypto_alloc_shash("crc32c", 0, 0);

	return PTR_ERR_OR_ZERO(tfm);
}

void btrfs_hash_exit(void)
{
	crypto_free_shash(tfm);
}

u32 btrfs_crc32c(u32 crc, const void *address, unsigned int length)
{
	struct {
		struct shash_desc shash;
		char ctx[crypto_shash_descsize(tfm)];
	} desc;
	int err;

	desc.shash.tfm = tfm;
	desc.shash.flags = 0;
	*(u32 *)desc.ctx = crc;

	err = crypto_shash_update(&desc.shash, address, length);
	BUG_ON(err);

	return *(u32 *)desc.ctx;
}
