/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __FS_H__
#define __FS_H__

#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>

#define STORAGE_PARTITION_LABEL		storage_partition
#if defined(CONFIG_BMC_PERSISTENT_STORAGE) && FIXED_PARTITION_EXISTS(STORAGE_PARTITION_LABEL)
int fs_init(void);
int fs_exit(void);
bool fs_enabled(void);
ssize_t fs_key_read(uint16_t id, void *buf, size_t size);
ssize_t fs_key_write(uint16_t id, const void *buf, size_t size);
int fs_clear(void);

#else /* defined(CONFIG_BMC_PERSISTENT_STORAGE) && FIXED_PARTITION_EXISTS(STORAGE_PARTITION_LABEL) */
static inline int fs_init(void)
{
	return 0;
}

static inline int fs_exit(void)
{
	return 0;
}

static inline bool fs_enabled(void)
{
	return false;
}

static inline ssize_t fs_key_read(uint16_t id, void *buf, size_t size)
{
	return -ENODEV;
}

static inline ssize_t fs_key_write(uint16_t id, const void *buf, size_t size)
{
	return -ENODEV;
}

static inline int fs_clear(void)
{
	return 0;
}

#endif /* defined(CONFIG_BMC_PERSISTENT_STORAGE) && FIXED_PARTITION_EXISTS(STORAGE_PARTITION_LABEL) */

#endif
