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
#include <zephyr/sys/byteorder.h>

#include "ext2.h"
#include "ext2_impl.h"
#include "ext2_struct.h"
#include "ext2_diskops.h"
#include "ext2_bitmap.h"

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
K_HEAP_DEFINE(direntry_heap, MAX_DIRENTRY_SIZE);
K_MEM_SLAB_DEFINE(inode_struct_slab, sizeof(struct ext2_inode), MAX_INODES, sizeof(void *));

/* Helper functions --------------------------------------------------------- */

void error_behavior(struct ext2_data *fs, const char *msg)
{
	LOG_ERR("File system corrupted: %s", msg);

	/* If file system is not initialized panic */
	if (!initialized) {
		LOG_ERR("File system data not found. Panic...");
		k_panic();
	}

	switch (fs->sblock.s_errors) {
	case EXT2_ERRORS_CONTINUE:
		/* Do nothing */
		break;
	case EXT2_ERRORS_RO:
		LOG_WRN("Marking file system as read only");
		fs->flags |= EXT2_DATA_FLAGS_RO;
		break;
	case EXT2_ERRORS_PANIC:
		LOG_ERR("Panic...");
		k_panic();
		break;
	default:
		LOG_ERR("Unrecognized errors behavior in superblock s_errors field. Panic...");
		k_panic();
	}
}

/* Block operations --------------------------------------------------------- */

static struct ext2_block *get_block_struct(void)
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
		k_mem_slab_free(&ext2_block_struct_slab, (void *)b);
		return NULL;
	}
	return b;
}

struct ext2_block *ext2_get_block(struct ext2_data *fs, uint32_t block)
{
	int ret;
	struct ext2_block *b = get_block_struct();

	if (!b) {
		return NULL;
	}
	b->num = block;
	b->flags = EXT2_BLOCK_ASSIGNED;
	ret = fs->backend_ops->read_block(fs, b->data, block);
	if (ret < 0) {
		LOG_ERR("get block: read block error %d", ret);
		ext2_drop_block(b);
		return NULL;
	}
	return b;
}

struct ext2_block *ext2_get_empty_block(struct ext2_data *fs)
{
	struct ext2_block *b = get_block_struct();

	if (!b) {
		return NULL;
	}
	b->num = 0;
	b->flags = 0;
	memset(b->data, 0, fs->block_size);
	return b;
}

int ext2_write_block(struct ext2_data *fs, struct ext2_block *b)
{
	int ret;

	if (!(b->flags & EXT2_BLOCK_ASSIGNED)) {
		return -EINVAL;
	}

	ret = fs->backend_ops->write_block(fs, b->data, b->num);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

void ext2_drop_block(struct ext2_block *b)
{
	if (b == NULL) {
		return;
	}

	if (b != NULL && b->data != NULL) {
		k_mem_slab_free(&ext2_block_memory_slab, (void *)b->data);
		k_mem_slab_free(&ext2_block_struct_slab, (void *)b);
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

int ext2_assign_block_num(struct ext2_data *fs, struct ext2_block *b)
{
	int64_t new_block;

	if (b->flags & EXT2_BLOCK_ASSIGNED) {
		return -EINVAL;
	}

	/* Allocate block in the file system. */
	new_block = ext2_alloc_block(fs);
	if (new_block < 0) {
		return new_block;
	}

	b->num = new_block;
	b->flags |= EXT2_BLOCK_ASSIGNED;
	return 0;
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
	fs->open_inodes = 0;
	fs->flags = 0;
	fs->bgroup.num = -1;

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

int ext2_verify_disk_superblock(struct ext2_disk_superblock *sb)
{
	/* Check if it is a valid Ext2 file system. */
	if (sys_le16_to_cpu(sb->s_magic) != EXT2_MAGIC_NUMBER) {
		LOG_ERR("Wrong file system magic number (%x)", sb->s_magic);
		return -EINVAL;
	}

	/* For now we don't support file systems with frag size different from block size */
	if (sys_le32_to_cpu(sb->s_log_block_size) != sb->s_log_frag_size) {
		LOG_ERR("Filesystem with frag_size != block_size is not supported");
		return -ENOTSUP;
	}

	/* Support only second revision */
	if (sys_le32_to_cpu(sb->s_rev_level) != EXT2_DYNAMIC_REV) {
		LOG_ERR("Filesystem with revision %d is not supported", sb->s_rev_level);
		return -ENOTSUP;
	}

	if (sys_le16_to_cpu(sb->s_inode_size) != EXT2_GOOD_OLD_INODE_SIZE) {
		LOG_ERR("Filesystem with inode size %d is not supported", sb->s_inode_size);
		return -ENOTSUP;
	}

	/* Check if file system may contain errors. */
	if (sys_le16_to_cpu(sb->s_state) == EXT2_ERROR_FS) {
		LOG_WRN("File system may contain errors.");
		switch (sys_le16_to_cpu(sb->s_errors)) {
		case EXT2_ERRORS_CONTINUE:
			break;

		case EXT2_ERRORS_RO:
			LOG_WRN("File system can be mounted read only");
			return -EROFS;

		case EXT2_ERRORS_PANIC:
			LOG_ERR("File system can't be mounted. Panic...");
			k_panic();
		default:
			LOG_WRN("Unknown option for superblock s_errors field.");
		}
	}

	if ((sys_le32_to_cpu(sb->s_feature_incompat) & EXT2_FEATURE_INCOMPAT_FILETYPE) == 0) {
		LOG_ERR("File system without file type stored in de is not supported");
		return -ENOTSUP;
	}

	if ((sys_le32_to_cpu(sb->s_feature_incompat) & ~EXT2_FEATURE_INCOMPAT_SUPPORTED) > 0) {
		LOG_ERR("File system can't be mounted. Incompat features %d not supported",
				(sb->s_feature_incompat & ~EXT2_FEATURE_INCOMPAT_SUPPORTED));
		return -ENOTSUP;
	}

	if ((sys_le32_to_cpu(sb->s_feature_ro_compat) & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED) > 0) {
		LOG_WRN("File system can be mounted read only. RO features %d detected.",
				(sb->s_feature_ro_compat & ~EXT2_FEATURE_RO_COMPAT_SUPPORTED));
		return -EROFS;
	}

	LOG_DBG("ino_cnt:%d blk_cnt:%d blk_per_grp:%d ino_per_grp:%d free_ino:%d free_blk:%d "
			"blk_size:%d ino_size:%d mntc:%d",
			sys_le32_to_cpu(sb->s_inodes_count),
			sys_le32_to_cpu(sb->s_blocks_count),
			sys_le32_to_cpu(sb->s_blocks_per_group),
			sys_le32_to_cpu(sb->s_inodes_per_group),
			sys_le32_to_cpu(sb->s_free_inodes_count),
			sys_le32_to_cpu(sb->s_free_blocks_count),
			sys_le32_to_cpu(1024 << sb->s_log_block_size),
			sys_le16_to_cpu(sb->s_inode_size),
			sys_le16_to_cpu(sb->s_mnt_count));
	return 0;
}

int ext2_init_fs(struct ext2_data *fs)
{
	int ret = 0;

	/* Fetch superblock */
	ret = ext2_fetch_superblock(fs);
	if (ret < 0) {
		return ret;
	}

	if (!(fs->flags & EXT2_DATA_FLAGS_RO)) {
		/* Update sblock fields set during the successful mount. */
		fs->sblock.s_state = EXT2_ERROR_FS;
		fs->sblock.s_mnt_count += 1;
		ret = ext2_commit_superblock(fs);
		if (ret < 0) {
			return ret;
		}
	}

	ret = ext2_fetch_block_group(fs, 0);
	if (ret < 0) {
		return ret;
	}
	ret = ext2_fetch_bg_ibitmap(&fs->bgroup);
	if (ret < 0) {
		return ret;
	}
	ret = ext2_fetch_bg_bbitmap(&fs->bgroup);
	if (ret < 0) {
		return ret;
	}

	/* Validate superblock */
	uint32_t set;
	struct ext2_superblock *sb = &fs->sblock;
	uint32_t fs_blocks = sb->s_blocks_count - sb->s_first_data_block;

	set = ext2_bitmap_count_set(BGROUP_BLOCK_BITMAP(&fs->bgroup), fs_blocks);

	if (set != sb->s_blocks_count - sb->s_free_blocks_count - sb->s_first_data_block) {
		error_behavior(fs, "Wrong number of used blocks in superblock and bitmap");
		return -EINVAL;
	}

	set = ext2_bitmap_count_set(BGROUP_INODE_BITMAP(&fs->bgroup), sb->s_inodes_count);

	if (set != sb->s_inodes_count - sb->s_free_inodes_count) {
		error_behavior(fs, "Wrong number of used inodes in superblock and bitmap");
		return -EINVAL;
	}
	return 0;
}

int ext2_close_fs(struct ext2_data *fs)
{
	int ret = 0;

	/* Close all open inodes */
	for (int32_t i = 0; i < fs->open_inodes; ++i) {
		if (fs->inode_pool[i] != NULL) {
			ext2_inode_drop(fs->inode_pool[i]);
		}
	}

	/* To save file system as correct it must be writable and without errors */
	if (!(fs->flags & (EXT2_DATA_FLAGS_RO | EXT2_DATA_FLAGS_ERR))) {
		fs->sblock.s_state = EXT2_VALID_FS;
		ret = ext2_commit_superblock(fs);
		if (ret < 0) {
			return ret;
		}
	}

	/* free block group if it is fetched */
	ext2_drop_block(fs->bgroup.inode_table);
	ext2_drop_block(fs->bgroup.inode_bitmap);
	ext2_drop_block(fs->bgroup.block_bitmap);

	if (fs->backend_ops->sync(fs) < 0) {
		return -EIO;
	}
	return 0;
}

int ext2_close_struct(struct ext2_data *fs)
{
	memset(fs, 0, sizeof(struct ext2_data));
	initialized = false;
	return 0;
}

/* Lookup ------------------------------------------------------------------- */

/* Functions needed by lookup inode */
static const char *skip_slash(const char *str);
static char *strchrnul(const char *str, const char c);
static int64_t find_dir_entry(struct ext2_inode *inode, const char *name, size_t len,
		uint32_t *r_offset);

int ext2_lookup_inode(struct ext2_data *fs, struct ext2_lookup_args *args)
{
	LOG_DBG("Looking for file %s", args->path);

	int rc, ret = 0;
	struct ext2_inode *cur_dir = NULL, *next = NULL;
	static char name_buf[EXT2_MAX_FILE_NAME + 1];

	/* Start looking from root directory of file system */
	rc = ext2_inode_get(fs, EXT2_ROOT_INODE, &cur_dir);
	if (rc < 0) {
		ret = rc;
		goto out;
	}

	/* There may be slash at the beginning of path */
	const char *path = args->path;

	path = skip_slash(path);

	/* If path is empty then return root directory */
	if (path[0] == '\0') {
		args->inode = cur_dir;
		cur_dir = NULL;
		goto out;
	}

	for (;;) {
		/* Get path component */
		char *end = strchrnul(path, '/');
		size_t len = end - path;

		if (len > EXT2_MAX_FILE_NAME) {
			ret = -ENAMETOOLONG;
			goto out;
		}

		strncpy(name_buf, path, len);
		name_buf[len] = '\0';

		/* Search in current directory */
		uint32_t dir_off = 0;
		/* using 64 bit value to don't lose any information on error */
		int64_t ino = find_dir_entry(cur_dir, name_buf, len, &dir_off);

		const char *next_path = skip_slash(end);
		bool last_entry = next_path[0] == '\0';

		if (!last_entry) {
			/*  prepare the next loop iteration */

			if (ino < 0) {
				/* next entry not found */
				ret = -ENOENT;
				goto out;
			}

			rc = ext2_inode_get(fs, ino, &next);
			if (rc < 0) {
				/* error while fetching next entry */
				ret = rc;
				goto out;
			}

			if (!(next->i_mode & EXT2_S_IFDIR)) {
				/* path component should be directory */
				ret = -ENOTDIR;
				goto out;
			}

			/* Go to the next path component */
			path = next_path;

			/* Move to next directory */
			ext2_inode_drop(cur_dir);
			cur_dir = next;

			next = NULL;
			continue;
		}

		/* Last entry */

		if (ino < 0 && !(args->flags & LOOKUP_ARG_CREATE)) {
			/* entry not found but we need it */
			ret = -ENOENT;
			goto out;
		}

		if (ino > 0) {
			rc = ext2_inode_get(fs, ino, &next);
			if (rc < 0) {
				ret = rc;
				goto out;
			}
		}

		/* Store parent directory and offset in parent directory */
		if (args->flags & (LOOKUP_ARG_CREATE | LOOKUP_ARG_STAT | LOOKUP_ARG_UNLINK)) {
			/* In create it will be valid only if we have found existing file */
			args->offset = dir_off;
			args->parent = cur_dir;
			cur_dir = NULL;
		}

		/* Store name info */
		if (args->flags & LOOKUP_ARG_CREATE) {
			args->name_pos = path - args->path;
			args->name_len = len;
		}

		/* Store found inode */
		if (ino > 0) {
			args->inode = next;
			next = NULL;
		}
		goto out;
	}

out:
	/* Always free that inodes.
	 * If some of them is returned from function then proper pointer is set to NULL.
	 */
	ext2_inode_drop(cur_dir);
	ext2_inode_drop(next);
	return ret;
}

/* Return position of given char or end of string. */
static char *strchrnul(const char *s, char c)
{
	while ((*s != c) && (*s != '\0')) {
		s++;
	}
	return (char *) s;
}

static const char *skip_slash(const char *s)
{
	while ((*s == '/') && (*s != '\0')) {
		s++;
	}
	return s;
}

/**
 * @brief Find inode
 *
 * @note Inodes are 32 bit. When we return signed 64 bit number then we don't
 *       lose any information.
 *
 * @param r_offset If not NULL then offset in directory of that entry is written here.
 * @return Inode number or negative error code
 */
static int64_t find_dir_entry(struct ext2_inode *inode, const char *name, size_t len,
		uint32_t *r_offset)
{
	int rc;
	uint32_t block, block_off, offset = 0;
	int64_t ino = -1;
	struct ext2_data *fs = inode->i_fs;
	struct ext2_direntry *de;

	while (offset < inode->i_size) {
		block = offset / fs->block_size;
		block_off = offset % fs->block_size;

		rc = ext2_fetch_inode_block(inode, block);
		if (rc < 0) {
			return rc;
		}

		struct ext2_disk_direntry *disk_de =
			EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(inode), block_off);

		de = ext2_fetch_direntry(disk_de);
		if (de == NULL) {
			return -EINVAL;
		}

		if (len == de->de_name_len && strncmp(de->de_name, name, len) == 0) {
			ino = de->de_inode;
			if (r_offset) {
				/* Return offset*/
				*r_offset = offset;
			}
			goto success;
		}
		/* move to the next directory entry */
		offset += de->de_rec_len;
		k_heap_free(&direntry_heap, de);
	}

	return -EINVAL;
success:
	k_heap_free(&direntry_heap, de);
	return (int64_t)ino;
}

/* Inode operations --------------------------------------------------------- */

k_ssize_t ext2_inode_read(struct ext2_inode *inode, void *buf, uint32_t offset, size_t nbytes)
{
	int rc = 0;
	k_ssize_t read = 0;
	uint32_t block_size = inode->i_fs->block_size;

	while (read < nbytes && offset < inode->i_size) {

		uint32_t block = offset / block_size;
		uint32_t block_off = offset % block_size;

		rc = ext2_fetch_inode_block(inode, block);
		if (rc < 0) {
			break;
		}

		uint32_t left_on_blk = block_size - block_off;
		uint32_t left_in_file = inode->i_size - offset;
		size_t to_read = MIN(nbytes, MIN(left_on_blk, left_in_file));

		memcpy((uint8_t *)buf + read, inode_current_block_mem(inode) + block_off, to_read);

		read += to_read;
		offset += to_read;
	}

	if (rc < 0) {
		return rc;
	}
	return read;
}

k_ssize_t ext2_inode_write(struct ext2_inode *inode, const void *buf, uint32_t offset,
			      size_t nbytes)
{
	int rc = 0;
	k_ssize_t written = 0;
	uint32_t block_size = inode->i_fs->block_size;

	while (written < nbytes) {
		uint32_t block = offset / block_size;
		uint32_t block_off = offset % block_size;

		LOG_DBG("inode:%d Write to block %d (offset: %d-%zd/%d)",
				inode->i_id, block, offset, offset + nbytes, inode->i_size);

		rc = ext2_fetch_inode_block(inode, block);
		if (rc < 0) {
			break;
		}

		size_t to_write = MIN(nbytes, block_size - block_off);

		memcpy(inode_current_block_mem(inode) + block_off, (uint8_t *)buf + written,
				to_write);
		LOG_DBG("Written %zd bytes at offset %d in block i%d", to_write, block_off, block);

		rc = ext2_commit_inode_block(inode);
		if (rc < 0) {
			break;
		}

		written += to_write;
	}

	if (rc < 0) {
		return rc;
	}

	if (offset + written > inode->i_size) {
		LOG_DBG("New inode size: %d -> %zd", inode->i_size, offset + written);
		inode->i_size = offset + written;
		rc = ext2_commit_inode(inode);
		if (rc < 0) {
			return rc;
		}
	}

	return written;
}

int ext2_inode_trunc(struct ext2_inode *inode, k_off_t length)
{
	if (length > UINT32_MAX) {
		return -ENOTSUP;
	}

	int rc = 0;
	uint32_t new_size = (uint32_t)length;
	uint32_t old_size = inode->i_size;
	const uint32_t block_size = inode->i_fs->block_size;

	LOG_DBG("Resizing inode from %d to %d", old_size, new_size);

	if (old_size == new_size) {
		return 0;
	}

	uint32_t used_blocks = new_size / block_size + (new_size % block_size != 0);

	if (new_size > old_size) {
		if (old_size % block_size != 0) {
			/* file ends inside some block */

			LOG_DBG("Has to insert zeros to the end of block");

			/* insert zeros to the end of last block */
			uint32_t old_block = old_size / block_size;
			uint32_t start_off = old_size % block_size;
			uint32_t to_write = MIN(new_size - old_size, block_size - start_off);

			rc = ext2_fetch_inode_block(inode, old_block);
			if (rc < 0) {
				return rc;
			}

			memset(inode_current_block_mem(inode) + start_off, 0, to_write);
			rc = ext2_commit_inode_block(inode);
			if (rc < 0) {
				return rc;
			}
		}

		/* There is no need to zero rest of blocks because they will be automatically
		 * treated as zero filled.
		 */

	} else {
		/* First removed block is just the number of used blocks.
		 * (We count blocks from zero hence its number is just number of used blocks.)
		 */
		uint32_t start_blk = used_blocks;
		int64_t removed_blocks;

		LOG_DBG("Inode trunc from blk: %d", start_blk);

		/* Remove blocks starting with start_blk. */
		removed_blocks = ext2_inode_remove_blocks(inode, start_blk);
		if (removed_blocks < 0) {
			return removed_blocks;
		}

		LOG_DBG("Removed blocks: %lld (%lld)",
				removed_blocks, removed_blocks * (block_size / 512));
		inode->i_blocks -= removed_blocks * (block_size / 512);
	}

	inode->i_size = new_size;

	LOG_DBG("New inode size: %d (blocks: %d)", inode->i_size, inode->i_blocks);

	rc = ext2_commit_inode(inode);
	return rc;
}

static int write_one_block(struct ext2_data *fs, struct ext2_block *b)
{
	int ret = 0;

	if (!(b->flags & EXT2_BLOCK_ASSIGNED)) {
		ret = ext2_assign_block_num(fs, b);
		if (ret < 0) {
			return ret;
		}
	}

	ret = ext2_write_block(fs, b);
	return ret;
}

int ext2_inode_sync(struct ext2_inode *inode)
{
	int ret;
	struct ext2_data *fs = inode->i_fs;

	for (int i = 0; i < 4; ++i) {
		if (inode->blocks[i] == NULL) {
			break;
		}
		ret = write_one_block(fs, inode->blocks[i]);
		if (ret < 0) {
			return ret;
		}
		ret = fs->backend_ops->sync(fs);
		if (ret < 0) {
			return ret;
		}
	}
	return 0;
}

int ext2_get_direntry(struct ext2_file *dir, struct fs_dirent *ent)
{
	if (dir->f_off >= dir->f_inode->i_size) {
		/* end of directory */
		ent->name[0] = 0;
		return 0;
	}

	struct ext2_data *fs = dir->f_inode->i_fs;

	int rc, ret = 0;
	uint32_t block = dir->f_off / fs->block_size;
	uint32_t block_off = dir->f_off % fs->block_size;
	uint32_t len;

	LOG_DBG("Reading dir entry from block %d at offset %d", block, block_off);

	rc = ext2_fetch_inode_block(dir->f_inode, block);
	if (rc < 0) {
		return rc;
	}

	struct ext2_inode *inode = NULL;
	struct ext2_disk_direntry *disk_de =
		EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(dir->f_inode), block_off);
	struct ext2_direntry *de = ext2_fetch_direntry(disk_de);

	if (de == NULL) {
		LOG_ERR("Read directory entry name too long");
		return -EINVAL;
	}

	LOG_DBG("inode=%d name_len=%d rec_len=%d", de->de_inode, de->de_name_len, de->de_rec_len);

	len = de->de_name_len;
	if (de->de_name_len > MAX_FILE_NAME) {
		LOG_WRN("Directory name won't fit in direntry");
		len = MAX_FILE_NAME;
	}
	memcpy(ent->name, de->de_name, len);
	ent->name[len] = '\0';

	LOG_DBG("name_len=%d name=%s %d", de->de_name_len, ent->name, EXT2_MAX_FILE_NAME);

	/* Get type of directory entry */
	ent->type = de->de_file_type & EXT2_FT_DIR ? FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE;

	/* Get size only for files. Directories have size 0. */
	size_t size = 0;

	if (ent->type == FS_DIR_ENTRY_FILE) {
		rc = ext2_inode_get(fs, de->de_inode, &inode);
		if (rc < 0) {
			ret = rc;
			goto out;
		}
		size = inode->i_size;
	}

	ent->size = size;

	/* Update offset to point to next directory entry */
	dir->f_off += de->de_rec_len;

out:
	k_heap_free(&direntry_heap, de);
	ext2_inode_drop(inode);
	return ret;
}

/* Create files and directories */

/* Allocate inode number and fill inode table with default values. */
static int ext2_create_inode(struct ext2_data *fs, struct ext2_inode *parent,
		struct ext2_inode *inode, int type)
{
	int rc;
	int32_t ino = ext2_alloc_inode(fs);

	if (ino < 0) {
		return ino;
	}

	/* fill inode with correct data */
	inode->i_fs = fs;
	inode->flags = 0;
	inode->i_id = ino;
	inode->i_size = 0;
	inode->i_mode = type == FS_DIR_ENTRY_FILE ? EXT2_DEF_FILE_MODE : EXT2_DEF_DIR_MODE;
	inode->i_links_count = 0;
	memset(inode->i_block, 0, 15 * 4);

	if (type == FS_DIR_ENTRY_DIR) {
		/* Block group current block is already fetched. We don't have to do it again.
		 * (It was done above in ext2_alloc_inode function.)
		 */
		fs->bgroup.bg_used_dirs_count += 1;
		rc = ext2_commit_bg(fs);
		if (rc < 0) {
			return rc;
		}
	}

	rc = ext2_commit_inode(inode);
	return rc;
}

struct ext2_direntry *ext2_create_direntry(const char *name, uint8_t namelen, uint32_t ino,
		uint8_t filetype)
{
	__ASSERT(namelen <= EXT2_MAX_FILE_NAME, "Name length to long");

	uint32_t prog_rec_len = sizeof(struct ext2_direntry) + namelen;
	struct ext2_direntry *de = k_heap_alloc(&direntry_heap, prog_rec_len, K_FOREVER);

	/* Size of future disk structure. */
	uint32_t reclen = sizeof(struct ext2_disk_direntry) + namelen;

	/* Align reclen to 4 bytes. */
	reclen = ROUND_UP(reclen, 4);

	de->de_inode = ino;
	de->de_rec_len = reclen;
	de->de_name_len = (uint8_t)namelen;
	de->de_file_type = filetype;
	memcpy(de->de_name, name, namelen);

	LOG_DBG("Initialized directory entry %p{%s(%d) %d %d %c}",
			de, de->de_name, de->de_name_len, de->de_inode, de->de_rec_len,
			de->de_file_type == EXT2_FT_DIR ? 'd' : 'f');
	return de;
}

static int ext2_add_direntry(struct ext2_inode *dir, struct ext2_direntry *entry)
{
	LOG_DBG("Adding entry: {in=%d type=%d name_len=%d} to directory (in=%d)",
			entry->de_inode, entry->de_file_type, entry->de_name_len, dir->i_id);

	int rc = 0;
	uint32_t block_size = dir->i_fs->block_size;
	uint32_t entry_size = sizeof(struct ext2_disk_direntry) + entry->de_name_len;

	if (entry_size > block_size) {
		return -EINVAL;
	}

	/* Find last entry */
	/* get last block and start from first entry on that block */
	int last_blk = (dir->i_size / block_size) - 1;

	rc = ext2_fetch_inode_block(dir, last_blk);
	if (rc < 0) {
		return rc;
	}

	uint32_t offset = 0;
	uint16_t reclen;

	struct ext2_disk_direntry *de = 0;

	/* loop must be executed at least once, because block_size > 0 */
	while (offset < block_size) {
		de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(dir), offset);
		reclen = ext2_get_disk_direntry_reclen(de);
		if (offset + reclen == block_size) {
			break;
		}
		offset += reclen;
	}


	uint32_t occupied = sizeof(struct ext2_disk_direntry) + ext2_get_disk_direntry_namelen(de);

	/* Align to 4 bytes */
	occupied = ROUND_UP(occupied, 4);

	LOG_DBG("Occupied: %d total: %d needed: %d", occupied, reclen, entry_size);

	if (reclen - occupied >= entry_size) {
		/* Entry fits into current block */
		offset += occupied;
		entry->de_rec_len = block_size - offset;
		ext2_set_disk_direntry_reclen(de, occupied);
	} else {
		LOG_DBG("Allocating new block for directory");

		/* Have to allocate new block */
		rc = ext2_fetch_inode_block(dir, last_blk + 1);
		if (rc < 0) {
			return rc;
		}

		/* Increase size of directory */
		dir->i_size += block_size;
		rc = ext2_commit_inode(dir);
		if (rc < 0) {
			return rc;
		}
		rc = ext2_commit_inode_block(dir);
		if (rc < 0) {
			return rc;
		}

		/* New entry will start at offset 0 */
		offset = 0;
		entry->de_rec_len = block_size;
	}

	LOG_DBG("Writing entry {in=%d type=%d rec_len=%d name_len=%d} to block %d of inode %d",
			entry->de_inode, entry->de_file_type, entry->de_rec_len, entry->de_name_len,
			inode_current_block(dir)->num, dir->i_id);


	de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(dir), offset);
	ext2_write_direntry(de, entry);

	rc = ext2_commit_inode_block(dir);
	return rc;
}

int ext2_create_file(struct ext2_inode *parent, struct ext2_inode *new_inode,
		struct ext2_lookup_args *args)
{
	int rc, ret = 0;
	struct ext2_direntry *entry;
	struct ext2_data *fs = parent->i_fs;

	rc = ext2_create_inode(fs, args->inode, new_inode, FS_DIR_ENTRY_FILE);
	if (rc < 0) {
		return rc;
	}

	entry = ext2_create_direntry(args->path + args->name_pos, args->name_len, new_inode->i_id,
			EXT2_FT_REG_FILE);

	rc = ext2_add_direntry(parent, entry);
	if (rc < 0) {
		ret = rc;
		goto out;
	}

	/* Successfully added to directory */
	new_inode->i_links_count += 1;

	rc = ext2_commit_inode(new_inode);
	if (rc < 0) {
		ret = rc;
	}
out:
	k_heap_free(&direntry_heap, entry);
	return ret;
}

int ext2_create_dir(struct ext2_inode *parent, struct ext2_inode *new_inode,
		struct ext2_lookup_args *args)
{
	int rc, ret = 0;
	struct ext2_direntry *entry;
	struct ext2_disk_direntry *disk_de;
	struct ext2_data *fs = parent->i_fs;
	uint32_t block_size = parent->i_fs->block_size;

	rc = ext2_create_inode(fs, args->inode, new_inode, FS_DIR_ENTRY_DIR);
	if (rc < 0) {
		return rc;
	}

	/* Directory must have at least one block */
	new_inode->i_size = block_size;

	entry = ext2_create_direntry(args->path + args->name_pos, args->name_len, new_inode->i_id,
			EXT2_FT_DIR);

	rc = ext2_add_direntry(parent, entry);
	if (rc < 0) {
		ret = rc;
		goto out;
	}

	/* Successfully added to directory */
	new_inode->i_links_count += 1;

	k_heap_free(&direntry_heap, entry);

	/* Create "." directory entry */
	entry = ext2_create_direntry(".", 1, new_inode->i_id, EXT2_FT_DIR);
	entry->de_rec_len = block_size;

	/* It has to be inserted manually */
	rc = ext2_fetch_inode_block(new_inode, 0);
	if (rc < 0) {
		ret = rc;
		goto out;
	}

	disk_de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(new_inode), 0);
	ext2_write_direntry(disk_de, entry);

	new_inode->i_links_count += 1;

	k_heap_free(&direntry_heap, entry);

	/* Add ".." directory entry */
	entry = ext2_create_direntry("..", 2, parent->i_id, EXT2_FT_DIR);

	rc = ext2_add_direntry(new_inode, entry);
	if (rc < 0) {
		ret = rc;
		goto out;
	}

	/* Successfully added to directory */
	parent->i_links_count += 1;

	rc = ext2_commit_inode_block(new_inode);
	if (rc < 0) {
		ret = rc;
	}

	rc = ext2_commit_inode_block(parent);
	if (rc < 0) {
		ret = rc;
	}

	/* Commit inodes after increasing link counts */
	rc = ext2_commit_inode(new_inode);
	if (rc < 0) {
		ret = rc;
	}

	rc = ext2_commit_inode(parent);
	if (rc < 0) {
		ret = rc;
	}
out:
	k_heap_free(&direntry_heap, entry);
	return ret;
}

static int ext2_del_direntry(struct ext2_inode *parent, uint32_t offset)
{
	int rc = 0;
	uint32_t block_size = parent->i_fs->block_size;

	uint32_t blk = offset / block_size;
	uint32_t blk_off = offset % block_size;

	rc = ext2_fetch_inode_block(parent, blk);
	if (rc < 0) {
		return rc;
	}

	if (blk_off == 0) {
		struct ext2_disk_direntry *de =
			EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(parent), 0);
		uint16_t reclen = ext2_get_disk_direntry_reclen(de);

		if (reclen == block_size) {
			/* Remove whole block */

			uint32_t last_blk = parent->i_size / block_size - 1;
			uint32_t old_blk = parent->i_block[blk];

			/* move last block in place of removed one. Entries start only at beginning
			 * of the block, hence we don't have to care to move any entry.
			 */
			parent->i_block[blk] = parent->i_block[last_blk];
			parent->i_block[last_blk] = 0;

			/* Free removed block */
			rc = ext2_free_block(parent->i_fs, old_blk);
			if (rc < 0) {
				return rc;
			}

			rc = ext2_commit_inode(parent);
			if (rc < 0) {
				return rc;
			}
		} else {
			/* Move next entry to beginning of block */
			struct ext2_disk_direntry *next =
			      EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(parent), reclen);
			uint16_t next_reclen = ext2_get_disk_direntry_reclen(next);

			memmove(de, next, next_reclen);
			ext2_set_disk_direntry_reclen(de, reclen + next_reclen);

			rc = ext2_commit_inode_block(parent);
			if (rc < 0) {
				return rc;
			}
		}

	} else {
		/* Entry inside the block */
		uint32_t cur = 0;
		uint16_t reclen;

		struct ext2_disk_direntry *de =
			EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(parent), 0);

		reclen = ext2_get_disk_direntry_reclen(de);
		/* find previous entry */
		while (cur + reclen < blk_off) {
			cur += reclen;
			de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(parent), cur);
			reclen = ext2_get_disk_direntry_reclen(de);
		}

		struct ext2_disk_direntry *del_entry =
			EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(parent), blk_off);
		uint16_t del_reclen = ext2_get_disk_direntry_reclen(del_entry);

		ext2_set_disk_direntry_reclen(de, reclen + del_reclen);
		rc = ext2_commit_inode_block(parent);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static int remove_inode(struct ext2_inode *inode)
{
	int ret = 0;

	LOG_DBG("inode: %d", inode->i_id);

	/* Free blocks of inode */
	ret = ext2_inode_remove_blocks(inode, 0);
	if (ret < 0) {
		return ret;
	}

	/* Free inode */
	ret = ext2_free_inode(inode->i_fs, inode->i_id, IS_DIR(inode->i_mode));
	return ret;
}

static int can_unlink(struct ext2_inode *inode)
{
	if (!IS_DIR(inode->i_mode)) {
		return 0;
	}

	int rc = 0;

	rc = ext2_fetch_inode_block(inode, 0);
	if (rc < 0) {
		return rc;
	}

	/* If directory check if it is empty */

	uint32_t offset = 0;
	struct ext2_disk_direntry *de;

	/* Get first entry */
	de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(inode), 0);
	offset += ext2_get_disk_direntry_reclen(de);

	/* Get second entry */
	de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(inode), offset);
	offset += ext2_get_disk_direntry_reclen(de);

	uint32_t block_size = inode->i_fs->block_size;

	/* If directory has size of one block and second entry ends with block end
	 * then directory is empty.
	 */
	if (offset == block_size && inode->i_size == block_size) {
		return 0;
	}

	return -ENOTEMPTY;
}

int ext2_inode_unlink(struct ext2_inode *parent, struct ext2_inode *inode, uint32_t offset)
{
	int rc;

	rc = can_unlink(inode);
	if (rc < 0) {
		return rc;
	}

	rc = ext2_del_direntry(parent, offset);
	if (rc < 0) {
		return rc;
	}

	if ((IS_REG_FILE(inode->i_mode) && inode->i_links_count == 1) ||
			(IS_DIR(inode->i_mode) && inode->i_links_count == 2)) {

		/* Only set the flag. Inode may still be open. Inode will be
		 * removed after dropping all references to it.
		 */
		inode->flags |= INODE_REMOVE;
	}

	inode->i_links_count -= 1;
	rc = ext2_commit_inode(inode);
	if (rc < 0) {
		return rc;
	}
	return 0;
}

int ext2_replace_file(struct ext2_lookup_args *args_from, struct ext2_lookup_args *args_to)
{
	LOG_DBG("Replace existing directory entry in rename");
	LOG_DBG("Inode: %d Inode to replace: %d", args_from->inode->i_id, args_to->inode->i_id);

	int rc = 0;
	struct ext2_disk_direntry *de;

	uint32_t block_size = args_from->parent->i_fs->block_size;
	uint32_t from_offset = args_from->offset;
	uint32_t from_blk = from_offset / block_size;
	uint32_t from_blk_off = from_offset % block_size;

	rc = ext2_fetch_inode_block(args_from->parent, from_blk);
	if (rc < 0) {
		return rc;
	}

	de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(args_from->parent), from_blk_off);

	/* record file type */
	uint8_t file_type = ext2_get_disk_direntry_type(de);

	/* NOTE: Replace the inode number in removed entry with inode of file that will be replaced
	 * with new one. Thanks to that we can use the function that unlinks directory entry to get
	 * rid of old directory entry and link to inode that will no longer be referenced by the
	 * directory entry after it is replaced with moved file.
	 */
	ext2_set_disk_direntry_inode(de, args_to->inode->i_id);
	rc = ext2_inode_unlink(args_from->parent, args_to->inode, args_from->offset);
	if (rc < 0) {
		/* restore the old inode number */
		ext2_set_disk_direntry_inode(de, args_from->inode->i_id);
		return rc;
	}

	uint32_t to_offset = args_to->offset;
	uint32_t to_blk = to_offset / block_size;
	uint32_t to_blk_off = to_offset % block_size;

	rc = ext2_fetch_inode_block(args_to->parent, to_blk);
	if (rc < 0) {
		return rc;
	}

	de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(args_to->parent), to_blk_off);

	/* change inode of new entry */
	ext2_set_disk_direntry_inode(de, args_from->inode->i_id);
	ext2_set_disk_direntry_type(de, file_type);

	rc = ext2_commit_inode_block(args_to->parent);
	if (rc < 0) {
		return rc;
	}
	return 0;
}

int ext2_move_file(struct ext2_lookup_args *args_from, struct ext2_lookup_args *args_to)
{
	int rc = 0;
	uint32_t block_size = args_from->parent->i_fs->block_size;

	struct ext2_inode *fparent = args_from->parent;
	struct ext2_inode *tparent = args_to->parent;
	uint32_t offset = args_from->offset;
	uint32_t blk = offset / block_size;
	uint32_t blk_off = offset % block_size;

	/* Check if we could just modify existing entry */
	if (fparent->i_id == tparent->i_id) {
		rc = ext2_fetch_inode_block(fparent, blk);
		if (rc < 0) {
			return rc;
		}

		struct ext2_disk_direntry *de;

		de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(fparent), blk_off);

		uint16_t reclen = ext2_get_disk_direntry_reclen(de);

		/* If new name fits in old entry, then just copy it there */
		if (reclen - sizeof(struct ext2_disk_direntry) >= args_to->name_len) {
			LOG_DBG("Old entry is modified to hold new name");
			ext2_set_disk_direntry_namelen(de, args_to->name_len);
			ext2_set_disk_direntry_name(de, args_to->path + args_to->name_pos,
					args_to->name_len);

			rc = ext2_commit_inode_block(fparent);
			return rc;
		}
	}

	LOG_DBG("Create new directory entry in rename");

	int ret = 0;

	rc = ext2_fetch_inode_block(fparent, blk);
	if (rc < 0) {
		return rc;
	}

	struct ext2_disk_direntry *old_de;
	struct ext2_direntry *new_de;

	old_de = EXT2_DISK_DIRENTRY_BY_OFFSET(inode_current_block_mem(fparent), blk_off);

	uint32_t inode = ext2_get_disk_direntry_inode(old_de);
	uint8_t file_type = ext2_get_disk_direntry_type(old_de);

	new_de = ext2_create_direntry(args_to->path + args_to->name_pos, args_to->name_len, inode,
			file_type);

	rc = ext2_add_direntry(tparent, new_de);
	if (rc < 0) {
		ret = rc;
		goto out;
	}

	rc = ext2_del_direntry(fparent, args_from->offset);
	if (rc < 0) {
		return rc;
	}

out:
	k_heap_free(&direntry_heap, new_de);
	return ret;
}

int ext2_inode_get(struct ext2_data *fs, uint32_t ino, struct ext2_inode **ret)
{
	int rc;
	struct ext2_inode *inode;

	for (int i = 0; i < fs->open_inodes; ++i) {
		inode = fs->inode_pool[i];

		if (inode->i_id == ino) {
			*ret = inode;
			inode->i_ref++;
			return 0;
		}
	}

	if (fs->open_inodes >= MAX_INODES) {
		return -ENOMEM;
	}


	rc = k_mem_slab_alloc(&inode_struct_slab, (void **)&inode, K_FOREVER);
	if (rc < 0) {
		return -ENOMEM;
	}
	memset(inode, 0, sizeof(struct ext2_inode));

	if (ino != 0) {
		int rc2 = ext2_fetch_inode(fs, ino, inode);

		if (rc2 < 0) {
			k_mem_slab_free(&inode_struct_slab, (void *)inode);
			return rc2;
		}
	}

	fs->inode_pool[fs->open_inodes] = inode;
	fs->open_inodes++;

	inode->i_fs = fs;
	inode->i_ref = 1;
	*ret = inode;
	return 0;
}

int ext2_inode_drop(struct ext2_inode *inode)
{
	if (inode == NULL) {
		return 0;
	}

	struct ext2_data *fs = inode->i_fs;

	if (fs->open_inodes <= 0) {
		LOG_WRN("All inodes should be already closed");
		return 0;
	}

	inode->i_ref--;

	/* Clean inode if that was last reference */
	if (inode->i_ref == 0) {

		/* find entry */
		uint32_t offset = 0;

		while (offset < MAX_INODES && fs->inode_pool[offset] != inode) {
			offset++;
		}

		if (offset >= MAX_INODES) {
			LOG_ERR("Inode structure at %p not in inode_pool", inode);
			return -EINVAL;
		}

		ext2_inode_drop_blocks(inode);

		if (inode->flags & INODE_REMOVE) {
			/* This is the inode that should be removed because
			 * there was called unlink function on it.
			 */
			int rc = remove_inode(inode);

			if (rc < 0) {
				return rc;
			}
		}

		k_mem_slab_free(&inode_struct_slab, (void *)inode);

		/* copy last open in place of freed inode */
		uint32_t last = fs->open_inodes - 1;

		fs->inode_pool[offset] = fs->inode_pool[last];
		fs->open_inodes--;

	}

	return 0;
}

void ext2_inode_drop_blocks(struct ext2_inode *inode)
{
	for (int i = 0; i < 4; ++i) {
		ext2_drop_block(inode->blocks[i]);
	}
	inode->flags &= ~INODE_FETCHED_BLOCK;
}
