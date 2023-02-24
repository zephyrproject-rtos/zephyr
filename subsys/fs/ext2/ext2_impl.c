/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "ext2.h"
#include "ext2_impl.h"
#include "ext2_struct.h"

LOG_MODULE_REGISTER(ext2, CONFIG_EXT2_LOG_LEVEL);

static struct ext2_data __fs;
static bool initialized;

#define BLOCK_MEMORY_BUFFER_SIZE (CONFIG_EXT2_MAX_BLOCK_COUNT * CONFIG_EXT2_MAX_BLOCK_SIZE)
#define BLOCK_STRUCT_BUFFER_SIZE (CONFIG_EXT2_MAX_BLOCK_COUNT * sizeof(struct ext2_block))

/* Structures for blocks slab alocator */
struct k_mem_slab ext2_block_memory_slab, ext2_block_struct_slab;
char __aligned(sizeof(void *)) __ext2_block_memory_buffer[BLOCK_MEMORY_BUFFER_SIZE];
char __aligned(sizeof(void *)) __ext2_block_struct_buffer[BLOCK_STRUCT_BUFFER_SIZE];

/* Initialize heap memory allocator */
K_HEAP_DEFINE(ext2_heap, CONFIG_EXT2_HEAP_SIZE);

/* Helper functions --------------------------------------------------------- */

void *ext2_heap_alloc(size_t size)
{
	return k_heap_alloc(&ext2_heap, size, K_NO_WAIT);
}

void  ext2_heap_free(void *ptr)
{
	k_heap_free(&ext2_heap, ptr);
}

/* Block operations --------------------------------------------------------- */

struct ext2_block *ext2_get_block(struct ext2_data *fs, uint32_t block)
{
	int ret;
	struct ext2_block *b;

	ret = k_mem_slab_alloc(&ext2_block_struct_slab, (void **)&b, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("get block: alloc block struct error %d", ret);
		return NULL;
	}

	ret = k_mem_slab_alloc(&ext2_block_memory_slab, (void **)&b->data, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("get block: alloc block memory error %d", ret);
		goto fail_alloc;
	}

	b->num = block;
	ret = fs->backend_ops->read_block(fs, b->data, block);
	if (ret < 0) {
		LOG_ERR("get block: read block error %d", ret);
		goto fail_read;
	}

	return b;

fail_read:
	k_mem_slab_free(&ext2_block_memory_slab, (void **)&b->data);
fail_alloc:
	k_mem_slab_free(&ext2_block_struct_slab, (void **)&b);
	return NULL;
}

int ext2_sync_block(struct ext2_data *fs, struct ext2_block *b)
{
	int ret;

	ret = fs->backend_ops->write_block(fs, b->data, b->num);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

void ext2_drop_block(struct ext2_block *b)
{
	if (b->data != NULL) {
		k_mem_slab_free(&ext2_block_memory_slab, (void **)&b->data);
		k_mem_slab_free(&ext2_block_struct_slab, (void **)&b);
	}
}

void ext2_init_blocks_slab(struct ext2_data *fs)
{
	memset(__ext2_block_memory_buffer, 0, BLOCK_MEMORY_BUFFER_SIZE);
	memset(__ext2_block_struct_buffer, 0, BLOCK_STRUCT_BUFFER_SIZE);

	/* These calls will always succeed because sizes and memory buffers are properly aligned. */

	k_mem_slab_init(&ext2_block_struct_slab, __ext2_block_struct_buffer,
			sizeof(struct ext2_block), CONFIG_EXT2_MAX_BLOCK_COUNT);

	k_mem_slab_init(&ext2_block_memory_slab, __ext2_block_memory_buffer, fs->block_size,
			CONFIG_EXT2_MAX_BLOCK_COUNT);
}

/* FS operations ------------------------------------------------------------ */

int ext2_init_storage(struct ext2_data **fsp, const void *storage_dev, int flags)
{
	if (initialized) {
		return -EBUSY;
	}

	int ret = 0;
	struct ext2_data *fs = &__fs;
	int64_t dev_size, write_size;

	*fsp = fs;

	ret = ext2_init_disk_access_backend(fs, storage_dev, flags);
	if (ret < 0) {
		return ret;
	}

	dev_size = fs->backend_ops->get_device_size(fs);
	if (dev_size < 0) {
		ret = dev_size;
		goto err;
	}

	write_size = fs->backend_ops->get_write_size(fs);
	if (write_size < 0) {
		ret = write_size;
		goto err;
	}

	if (write_size < 1024 && 1024 % write_size != 0) {
		ret = -EINVAL;
		LOG_ERR("expecting sector size that divides 1024 (got: %lld)", write_size);
		goto err;
	}

	LOG_DBG("Device size: %lld", dev_size);
	LOG_DBG("Write size: %lld", write_size);

	fs->device_size = dev_size;
	fs->write_size = write_size;

	initialized = true;
err:
	return ret;
}

int ext2_verify_superblock(struct ext2_disk_superblock *sb)
{
	/* Check if it is a valid Ext2 file system. */
	if (sb->s_magic != EXT2_MAGIC_NUMBER) {
		LOG_ERR("Wrong file system magic number (%x)", sb->s_magic);
		return -EINVAL;
	}

	/* For now we don't support file systems with frag size different from block size */
	if (sb->s_log_block_size != sb->s_log_frag_size) {
		LOG_ERR("Filesystem with frag_size != block_size is not supported");
		return -ENOTSUP;
	}

	/* Support only second revision */
	if (sb->s_rev_level != EXT2_DYNAMIC_REV) {
		LOG_ERR("Filesystem with revision %d is not supported", sb->s_rev_level);
		return -ENOTSUP;
	}

	if (sb->s_inode_size != EXT2_GOOD_OLD_INODE_SIZE) {
		LOG_ERR("Filesystem with inode size %d is not supported", sb->s_inode_size);
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
		LOG_ERR("File system can't be mounted. Incompat features %d not supported",
				(sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED));
		return -ENOTSUP;
	}

	if ((sb->s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED) > 0) {
		LOG_WRN("File system can be mounted read only. RO features %d detected.",
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
		fs->sblock = ext2_get_block(fs, 1);
	} else {
		fs->sblock_offset = 1024;
		fs->sblock = ext2_get_block(fs, 0);
	}

	if (fs->sblock == NULL) {
		ret = ENOENT;
		goto out;
	}

	if (!(fs->flags & EXT2_DATA_FLAGS_RO)) {
		/* Update sblock fields set during the successful mount. */
		EXT2_DATA_SBLOCK(fs)->s_state = EXT2_ERROR_FS;
		EXT2_DATA_SBLOCK(fs)->s_mnt_count += 1;

		ret = ext2_sync_block(fs, fs->sblock);
		if (ret < 0) {
			goto out;
		}
	}
	return 0;
out:
	ext2_drop_block(fs->sblock);
	fs->sblock = NULL;
	return ret;
}

int ext2_close_fs(struct ext2_data *fs)
{
	int ret = 0;

	if (!(fs->flags & EXT2_DATA_FLAGS_RO)) {
		EXT2_DATA_SBLOCK(fs)->s_state = EXT2_VALID_FS;

		ret = ext2_sync_block(fs, fs->sblock);
		if (ret < 0) {
			goto out;
		}
	}

	ext2_drop_block(fs->sblock);
	fs->sblock = NULL;
out:
	return ret;
}

int ext2_close_struct(struct ext2_data *fs)
{
	memset(fs, 0, sizeof(struct ext2_data));
	initialized = false;
	return 0;
}
