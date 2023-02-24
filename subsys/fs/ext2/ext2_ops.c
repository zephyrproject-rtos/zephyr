/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/fs/ext2.h>
#include <zephyr/logging/log.h>

#include "../fs_impl.h"
#include "ext2.h"
#include "ext2_impl.h"
#include "ext2_struct.h"

LOG_MODULE_DECLARE(ext2);

/* File system level operations */

#ifdef CONFIG_FILE_SYSTEM_MKFS
FS_EXT2_DECLARE_DEFAULT_CONFIG(ext2_default_cfg);
#endif

static int ext2_mount(struct fs_mount_t *mountp)
{
	int ret = 0;
	struct ext2_data *fs = NULL;
	struct ext2_disk_superblock *sb;
#ifdef CONFIG_FILE_SYSTEM_MKFS
	bool do_format = false;
	bool possible_format = (mountp->flags & FS_MOUNT_FLAG_NO_FORMAT) == 0 &&
				(mountp->flags & FS_MOUNT_FLAG_READ_ONLY) == 0;
#endif

	/* Allocate superblock structure for temporary use */
	sb = ext2_heap_alloc(sizeof(struct ext2_disk_superblock));
	if (sb == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ret = ext2_init_storage(&fs, mountp->storage_dev, mountp->flags);
	if (ret < 0) {
		goto err;
	}

	fs->flags = 0;
	if (mountp->flags & FS_MOUNT_FLAG_READ_ONLY) {
		fs->flags |= EXT2_DATA_FLAGS_RO;
	}

	ret = fs->backend_ops->read_superblock(fs, sb);
	if (ret < 0) {
		goto err;
	}

	ret = ext2_verify_superblock(sb);
	if (ret == 0) {
		fs->block_size = 1024 << sb->s_log_block_size;

	} else if (ret == -EROFS) {
		fs->block_size = 1024 << sb->s_log_block_size;
		fs->flags |= EXT2_DATA_FLAGS_RO;

#ifdef CONFIG_FILE_SYSTEM_MKFS
	} else if (ret == -EINVAL && possible_format) {
		do_format = true;
		fs->block_size = ext2_default_cfg.block_size;
#endif

	} else {
		goto err;
	}

	if (fs->block_size % fs->write_size != 0) {
		LOG_ERR("Blocks size isn't multiple of sector size. (bsz: %d, ssz: %d)",
				fs->block_size, fs->write_size);
		ret = -ENOTSUP;
		goto err;
	}

	/* Temporary superblock won't be used anymore */
	ext2_heap_free(sb);

	ext2_init_blocks_slab(fs);

#ifdef CONFIG_FILE_SYSTEM_MKFS
	if (do_format) {
		LOG_INF("Formatting the storage device");

		ret = ext2_format(fs, &ext2_default_cfg);
		if (ret < 0) {
			goto err;
		}
		/* We don't need to verify superblock here again. Format has succeeded hence
		 * superblock must be valid.
		 */
	}
#endif

	ret = ext2_init_fs(fs);
	if (ret < 0) {
		goto err;
	}

	mountp->fs_data = fs;
	return 0;

err:
	ext2_heap_free(sb);
	ext2_close_struct(fs);
	return ret;
}

#if defined(CONFIG_FILE_SYSTEM_MKFS)

static int ext2_mkfs(uintptr_t dev_id, void *vcfg, int flags)
{
	int ret = 0;
	struct ext2_data *fs;
	struct ext2_cfg *cfg = vcfg;

	if (cfg == NULL) {
		cfg = &ext2_default_cfg;
	}

	ret = ext2_init_storage(&fs, (const void *)dev_id, flags);
	if (ret < 0) {
		LOG_ERR("Initialization of %ld device failed (%d)", dev_id, ret);
		goto out;
	}

	fs->block_size = cfg->block_size;

	ext2_init_blocks_slab(fs);

	LOG_INF("Formatting the storage device");
	ret = ext2_format(fs, cfg);
	if (ret < 0) {
		LOG_ERR("Format of %ld device failed (%d)", dev_id, ret);
	}

out:
	ext2_close_struct(fs);
	return ret;
}

#endif /* CONFIG_FILE_SYSTEM_MKFS */

static int ext2_unmount(struct fs_mount_t *mountp)
{
	int ret;
	struct ext2_data *fs = mountp->fs_data;

	ret = ext2_close_fs(fs);
	if (ret < 0) {
		return ret;
	}

	ret = ext2_close_struct(fs);
	if (ret < 0) {
		return ret;
	}
	mountp->fs_data = NULL;
	return 0;
}

static int ext2_statvfs(struct fs_mount_t *mountp, const char *path, struct fs_statvfs *stat)
{
	ARG_UNUSED(path);
	struct ext2_data *fs = mountp->fs_data;

	stat->f_bsize = fs->block_size;
	stat->f_frsize = fs->block_size;
	stat->f_blocks = EXT2_DATA_SBLOCK(fs)->s_blocks_count;
	stat->f_bfree = EXT2_DATA_SBLOCK(fs)->s_free_blocks_count;

	return 0;
}

/* File system interface */

static const struct fs_file_system_t ext2_fs = {
	.mount = ext2_mount,
	.unmount = ext2_unmount,
	.statvfs = ext2_statvfs,
#if defined(CONFIG_FILE_SYSTEM_MKFS)
	.mkfs = ext2_mkfs,
#endif
};

static int ext2_init(void)
{
	int rc = fs_register(FS_EXT2, &ext2_fs);

	if (rc < 0) {
		LOG_WRN("Ext2 register error (%d)\n", rc);
	} else {
		LOG_DBG("Ext2 fs registered\n");
	}

	return rc;
}

SYS_INIT(ext2_init, POST_KERNEL, 99);
