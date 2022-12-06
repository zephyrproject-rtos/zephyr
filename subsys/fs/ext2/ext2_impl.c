/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "ext2.h"
#include "ext2_mem.h"
#include "ext2_impl.h"
#include "ext2_struct.h"
#include "ext2_blocks.h"
#include "ext2_flash.h"

LOG_MODULE_REGISTER(ext2, CONFIG_EXT2_LOG_LEVEL);

/* FS operations ------------------------------------------------------------ */

static int init_fs_backend(struct ext2_data *fs, const void *storage_dev, int flags)
{
	return flash_init_backend(fs, storage_dev, flags);
}

static void fs_timer_expired_h(struct k_timer *tm)
{
	struct ext2_data *fs = k_timer_user_data_get(tm);

	sync_blocks(fs);
}

static void fs_timer_stopped_h(struct k_timer *tm)
{
	LOG_WRN("Periodical sync has been stopped.");
}

static struct k_timer fs_timer;
static struct ext2_data __fs;
static bool initialized;

int ext2_init_struct(struct ext2_data **fsp, const void *storage_dev, int flags)
{
	if (initialized) {
		return -EBUSY;
	}

	int ret = 0;
	struct ext2_data *fs = &__fs;
	int64_t dev_size, write_size;

	*fsp = fs;
	fs->flags = 0;

	ret = init_fs_backend(fs, storage_dev, flags);
	if (ret < 0) {
		return ret;
	}

	dev_size = fs->get_device_size(fs);
	if (dev_size < 0) {
		ret = dev_size;
		goto err;
	}

	write_size = fs->get_write_size(fs);
	if (write_size < 0) {
		ret = write_size;
		goto err;
	}

	fs->device_size = dev_size;
	fs->page_size = write_size;

	if (flags & FS_MOUNT_FLAG_READ_ONLY) {
		fs->flags |= EXT2_DATA_FLAGS_RO;
	}

	initialized = true;
	return 0;
err:
	fs->close_backend(fs);
	return ret;
}

int ext2_verify_superblock(struct disk_superblock *sb)
{
	/* Check if it is a valid Ext2 file system. */
	if (sb->s_magic != EXT2_MAGIC_NUMBER) {
		LOG_ERR("Wrong file system magic number (%x)", sb->s_magic);
		return -EINVAL;
	}

	/* For now we don't support file systems with frag size different from block size */
	if (sb->s_log_block_size != sb->s_log_frag_size) {
		return -ENOTSUP;
	}

	/* Support only second revision */
	if (sb->s_rev_level != EXT2_DYNAMIC_REV) {
		return -ENOTSUP;
	}

	/* Check if file system may contain errors. */
	if (sb->s_state == EXT2_ERROR_FS) {

		LOG_WRN("File system may contain errors.");
		switch (sb->s_errors) {
		case EXT2_ERRORS_CONTINUE:
			break;

		case EXT2_ERRORS_RO:
			LOG_WRN("File system can be mounted read only");
			return -EROFS;

		case EXT2_ERRORS_PANIC:
			LOG_ERR("File system can't be mounted");
			/* panic or return that fs is invalid */
			__ASSERT(sb->s_state == EXT2_VALID_FS, "Error detected in superblock");
			return -EINVAL;
		default:
			LOG_WRN("Unknown option for superblock s_errors field.");
		}
	}

	if ((sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE) == 0) {
		LOG_ERR("File system without file type stored in de is not supported");
		return -ENOTSUP;
	}

	if ((sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED) > 0) {
		LOG_ERR("File system can't be mounted. Incompat features 0x%x not supported",
				(sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED));
		return -ENOTSUP;
	}

	if ((sb->s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED) > 0) {
		LOG_WRN("File system can be mounted read only. RO features 0x%x detected.",
				(sb->s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED));
		return -EROFS;
	}

	LOG_DBG("ino_cnt:%d blk_cnt:%d blk_per_grp:%d ino_per_grp:%d free_ino:%d free_blk:%d "
			"blk_size:%d ino_size:%d mntc:%d",
			sb->s_inodes_count, sb->s_blocks_count, sb->s_blocks_per_group,
			sb->s_inodes_per_group, sb->s_free_inodes_count, sb->s_free_blocks_count,
			1024 << sb->s_log_block_size, sb->s_inode_size, sb->s_mnt_count);
	return 0;
}

int ext2_init_fs(struct ext2_data *fs)
{
	int ret = 0;

	/* Fetch superblock */
	if (fs->block_size == 1024) {
		fs->sblock_offset = 0;
		fs->sblock = get_block(fs, 1);
	} else {
		fs->sblock_offset = 1024;
		fs->sblock = get_block(fs, 0);
	}

	if (fs->sblock == NULL) {
		ret = ENOENT;
		goto out;
	}

	if (!(fs->flags & EXT2_DATA_FLAGS_RO)) {
		/* Update sblock fields set during the successful mount. */
		EXT2_DATA_SBLOCK(fs)->s_state = EXT2_ERROR_FS;
		EXT2_DATA_SBLOCK(fs)->s_mnt_count += 1;

		block_set_dirty(fs->sblock);

		k_timer_init(&fs_timer, fs_timer_expired_h, fs_timer_stopped_h);
		k_timer_user_data_set(&fs_timer, fs);
		k_timer_start(&fs_timer,
				K_SECONDS(CONFIG_EXT2_SYNC_SECONDS),
				K_SECONDS(CONFIG_EXT2_SYNC_SECONDS));
	}
	return 0;
out:
	drop_block(fs->sblock);
	fs->sblock = NULL;
	return ret;
}

int ext2_close_fs(struct ext2_data *fs)
{
	if (!(fs->flags & EXT2_DATA_FLAGS_RO)) {
		k_timer_stop(&fs_timer);

		EXT2_DATA_SBLOCK(fs)->s_state = EXT2_VALID_FS;
		block_set_dirty(fs->sblock);

		sync_blocks(fs);
	}

	drop_block(fs->sblock);
	fs->sblock = NULL;
	return 0;
}

int ext2_close_struct(struct ext2_data *fs)
{
	if (fs->close_backend) {
		fs->close_backend(fs);
	}
	memset(fs, 0, sizeof(struct ext2_data));
	initialized = false;
	return 0;
}
