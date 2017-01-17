#ifndef BTRFS_VERSION_H
#define BTRFS_VERSION_H

#include <linux/version.h>

#define BTRFS_RHEL_KERNEL_VERSION(a, b, c, d, e, f) \
	(((((a) << 16) + ((b) << 8) + (c)) * 10000000ULL) + ((d) * 10000) + ((e) * 100) + (f))

#ifdef RHEL_VERSION_CODE
#define BTRFS_RHEL_VERSION_CODE \
((LINUX_VERSION_CODE * 10000000ULL) + RHEL_VERSION_CODE)
#else
#define BTRFS_RHEL_VERSION_CODE		0
#endif

#endif
