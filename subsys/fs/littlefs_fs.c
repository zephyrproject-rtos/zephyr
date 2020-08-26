/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <errno.h>
#include <init.h>
#include <fs/fs.h>

#define LFS_LOG_REGISTER
#include <lfs_util.h>

#include <lfs.h>
#include <fs/littlefs.h>
#include <drivers/flash.h>
#include <storage/flash_map.h>

#include "fs_impl.h"

struct lfs_file_data {
	struct lfs_file file;
	struct lfs_file_config config;
	struct k_mem_block cache_block;
};

#define LFS_FILEP(fp) (&((struct lfs_file_data *)(fp->filep))->file)

/* Global memory pool for open files and dirs */
K_MEM_SLAB_DEFINE(file_data_pool, sizeof(struct lfs_file_data),
		  CONFIG_FS_LITTLEFS_NUM_FILES, 4);
K_MEM_SLAB_DEFINE(lfs_dir_pool, sizeof(struct lfs_dir),
		  CONFIG_FS_LITTLEFS_NUM_DIRS, 4);

/* If not explicitly customizing provide a default that's appropriate
 * based on other configuration options.
 */
#ifndef CONFIG_FS_LITTLEFS_FC_MEM_POOL
BUILD_ASSERT(CONFIG_FS_LITTLEFS_CACHE_SIZE >= 4);
#define CONFIG_FS_LITTLEFS_FC_MEM_POOL_MIN_SIZE 4
#define CONFIG_FS_LITTLEFS_FC_MEM_POOL_MAX_SIZE CONFIG_FS_LITTLEFS_CACHE_SIZE
#define CONFIG_FS_LITTLEFS_FC_MEM_POOL_NUM_BLOCKS CONFIG_FS_LITTLEFS_NUM_FILES
#endif

K_MEM_POOL_DEFINE(file_cache_pool,
		  CONFIG_FS_LITTLEFS_FC_MEM_POOL_MIN_SIZE,
		  CONFIG_FS_LITTLEFS_FC_MEM_POOL_MAX_SIZE,
		  CONFIG_FS_LITTLEFS_FC_MEM_POOL_NUM_BLOCKS, 4);

static inline void fs_lock(struct fs_littlefs *fs)
{
	k_mutex_lock(&fs->mutex, K_FOREVER);
}

static inline void fs_unlock(struct fs_littlefs *fs)
{
	k_mutex_unlock(&fs->mutex);
}

static int lfs_to_errno(int error)
{
	if (error >= 0) {
		return error;
	}

	switch (error) {
	default:
	case LFS_ERR_IO:        /* Error during device operation */
		return -EIO;
	case LFS_ERR_CORRUPT:   /* Corrupted */
		return -EFAULT;
	case LFS_ERR_NOENT:     /* No directory entry */
		return -ENOENT;
	case LFS_ERR_EXIST:     /* Entry already exists */
		return -EEXIST;
	case LFS_ERR_NOTDIR:    /* Entry is not a dir */
		return -ENOTDIR;
	case LFS_ERR_ISDIR:     /* Entry is a dir */
		return -EISDIR;
	case LFS_ERR_NOTEMPTY:  /* Dir is not empty */
		return -ENOTEMPTY;
	case LFS_ERR_BADF:      /* Bad file number */
		return -EBADF;
	case LFS_ERR_FBIG:      /* File too large */
		return -EFBIG;
	case LFS_ERR_INVAL:     /* Invalid parameter */
		return -EINVAL;
	case LFS_ERR_NOSPC:     /* No space left on device */
		return -ENOSPC;
	case LFS_ERR_NOMEM:     /* No more memory available */
		return -ENOMEM;
	}
}

static int errno_to_lfs(int error)
{
	if (error >= 0) {
		return LFS_ERR_OK;
	}

	switch (error) {
	default:
	case -EIO:              /* Error during device operation */
		return LFS_ERR_IO;
	case -EFAULT:		/* Corrupted */
		return LFS_ERR_CORRUPT;
	case -ENOENT:           /* No directory entry */
		return LFS_ERR_NOENT;
	case -EEXIST:           /* Entry already exists */
		return LFS_ERR_EXIST;
	case -ENOTDIR:          /* Entry is not a dir */
		return LFS_ERR_NOTDIR;
	case -EISDIR:           /* Entry is a dir */
		return LFS_ERR_ISDIR;
	case -ENOTEMPTY:        /* Dir is not empty */
		return LFS_ERR_NOTEMPTY;
	case -EBADF:            /* Bad file number */
		return LFS_ERR_BADF;
	case -EFBIG:            /* File too large */
		return LFS_ERR_FBIG;
	case -EINVAL:           /* Invalid parameter */
		return LFS_ERR_INVAL;
	case -ENOSPC:           /* No space left on device */
		return LFS_ERR_NOSPC;
	case -ENOMEM:           /* No more memory available */
		return LFS_ERR_NOMEM;
	}
}


static int lfs_api_read(const struct lfs_config *c, lfs_block_t block,
			lfs_off_t off, void *buffer, lfs_size_t size)
{
	const struct flash_area *fa = c->context;
	size_t offset = block * c->block_size + off;

	int rc = flash_area_read(fa, offset, buffer, size);

	return errno_to_lfs(rc);
}

static int lfs_api_prog(const struct lfs_config *c, lfs_block_t block,
			lfs_off_t off, const void *buffer, lfs_size_t size)
{
	const struct flash_area *fa = c->context;
	size_t offset = block * c->block_size + off;

	int rc = flash_area_write(fa, offset, buffer, size);

	return errno_to_lfs(rc);
}

static int lfs_api_erase(const struct lfs_config *c, lfs_block_t block)
{
	const struct flash_area *fa = c->context;
	size_t offset = block * c->block_size;

	int rc = flash_area_erase(fa, offset, c->block_size);

	return errno_to_lfs(rc);
}

static int lfs_api_sync(const struct lfs_config *c)
{
	return LFS_ERR_OK;
}

static void release_file_data(struct fs_file_t *fp)
{
	struct lfs_file_data *fdp = fp->filep;

	if (fdp->config.buffer) {
		k_mem_pool_free(&fdp->cache_block);
	}

	k_mem_slab_free(&file_data_pool, &fp->filep);
	fp->filep = NULL;
}

static int lfs_flags_from_zephyr(unsigned int zflags)
{
	int flags = (zflags & FS_O_CREATE) ? LFS_O_CREAT : 0;

	/* LFS_O_READONLY and LFS_O_WRONLY can be selected at the same time,
	 * this is not a mistake, together they create RDWR access.
	 */
	flags |= (zflags & FS_O_READ) ? LFS_O_RDONLY : 0;
	flags |= (zflags & FS_O_WRITE) ? LFS_O_WRONLY : 0;

	flags |= (zflags & FS_O_APPEND) ? LFS_O_APPEND : 0;

	return flags;
}

static int littlefs_open(struct fs_file_t *fp, const char *path,
			 fs_mode_t zflags)
{
	struct fs_littlefs *fs = fp->mp->fs_data;
	struct lfs *lfs = &fs->lfs;
	int flags = lfs_flags_from_zephyr(zflags);
	int ret = k_mem_slab_alloc(&file_data_pool, &fp->filep, K_NO_WAIT);

	if (ret != 0) {
		return ret;
	}

	struct lfs_file_data *fdp = fp->filep;

	memset(fdp, 0, sizeof(*fdp));

	ret = k_mem_pool_alloc(&file_cache_pool, &fdp->cache_block,
			       lfs->cfg->cache_size, K_NO_WAIT);
	LOG_DBG("alloc %u file cache: %d", lfs->cfg->cache_size, ret);
	if (ret != 0) {
		goto out;
	}

	fdp->config.buffer = fdp->cache_block.data;
	path = fs_impl_strip_prefix(path, fp->mp);

	fs_lock(fs);

	ret = lfs_file_opencfg(&fs->lfs, &fdp->file,
			       path, flags, &fdp->config);

	fs_unlock(fs);
out:
	if (ret < 0) {
		release_file_data(fp);
	}

	return lfs_to_errno(ret);
}

static int littlefs_close(struct fs_file_t *fp)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	int ret = lfs_file_close(&fs->lfs, LFS_FILEP(fp));

	fs_unlock(fs);

	release_file_data(fp);

	return lfs_to_errno(ret);
}

static int littlefs_unlink(struct fs_mount_t *mountp, const char *path)
{
	struct fs_littlefs *fs = mountp->fs_data;

	path = fs_impl_strip_prefix(path, mountp);

	fs_lock(fs);

	int ret = lfs_remove(&fs->lfs, path);

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

static int littlefs_rename(struct fs_mount_t *mountp, const char *from,
			   const char *to)
{
	struct fs_littlefs *fs = mountp->fs_data;

	from = fs_impl_strip_prefix(from, mountp);
	to = fs_impl_strip_prefix(to, mountp);

	fs_lock(fs);

	int ret = lfs_rename(&fs->lfs, from, to);

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

static ssize_t littlefs_read(struct fs_file_t *fp, void *ptr, size_t len)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	ssize_t ret = lfs_file_read(&fs->lfs, LFS_FILEP(fp), ptr, len);

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

static ssize_t littlefs_write(struct fs_file_t *fp, const void *ptr, size_t len)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	ssize_t ret = lfs_file_write(&fs->lfs, LFS_FILEP(fp), ptr, len);

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

BUILD_ASSERT((FS_SEEK_SET == LFS_SEEK_SET)
	     && (FS_SEEK_CUR == LFS_SEEK_CUR)
	     && (FS_SEEK_END == LFS_SEEK_END));

static int littlefs_seek(struct fs_file_t *fp, off_t off, int whence)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	off_t ret = lfs_file_seek(&fs->lfs, LFS_FILEP(fp), off, whence);

	fs_unlock(fs);

	if (ret >= 0) {
		ret = 0;
	}

	return lfs_to_errno(ret);
}

static off_t littlefs_tell(struct fs_file_t *fp)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	off_t ret = lfs_file_tell(&fs->lfs, LFS_FILEP(fp));

	fs_unlock(fs);
	return ret;
}

static int littlefs_truncate(struct fs_file_t *fp, off_t length)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	int ret = lfs_file_truncate(&fs->lfs, LFS_FILEP(fp), length);

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

static int littlefs_sync(struct fs_file_t *fp)
{
	struct fs_littlefs *fs = fp->mp->fs_data;

	fs_lock(fs);

	int ret = lfs_file_sync(&fs->lfs, LFS_FILEP(fp));

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

static int littlefs_mkdir(struct fs_mount_t *mountp, const char *path)
{
	struct fs_littlefs *fs = mountp->fs_data;

	path = fs_impl_strip_prefix(path, mountp);
	fs_lock(fs);

	int ret = lfs_mkdir(&fs->lfs, path);

	fs_unlock(fs);
	return lfs_to_errno(ret);
}

static int littlefs_opendir(struct fs_dir_t *dp, const char *path)
{
	struct fs_littlefs *fs = dp->mp->fs_data;

	if (k_mem_slab_alloc(&lfs_dir_pool, &dp->dirp, K_NO_WAIT) != 0) {
		return -ENOMEM;
	}

	memset(dp->dirp, 0, sizeof(struct lfs_dir));

	path = fs_impl_strip_prefix(path, dp->mp);

	fs_lock(fs);

	int ret = lfs_dir_open(&fs->lfs, dp->dirp, path);

	fs_unlock(fs);

	if (ret < 0) {
		k_mem_slab_free(&lfs_dir_pool, &dp->dirp);
	}

	return lfs_to_errno(ret);
}

static void info_to_dirent(const struct lfs_info *info, struct fs_dirent *entry)
{
	entry->type = ((info->type == LFS_TYPE_DIR) ?
		       FS_DIR_ENTRY_DIR : FS_DIR_ENTRY_FILE);
	entry->size = info->size;
	strncpy(entry->name, info->name, sizeof(entry->name));
	entry->name[sizeof(entry->name) - 1] = '\0';
}

static int littlefs_readdir(struct fs_dir_t *dp, struct fs_dirent *entry)
{
	struct fs_littlefs *fs = dp->mp->fs_data;

	fs_lock(fs);

	struct lfs_info info;
	int ret = lfs_dir_read(&fs->lfs, dp->dirp, &info);

	fs_unlock(fs);

	if (ret > 0) {
		info_to_dirent(&info, entry);
		ret = 0;
	} else if (ret == 0) {
		entry->name[0] = 0;
	}

	return lfs_to_errno(ret);
}

static int littlefs_closedir(struct fs_dir_t *dp)
{
	struct fs_littlefs *fs = dp->mp->fs_data;

	fs_lock(fs);

	int ret = lfs_dir_close(&fs->lfs, dp->dirp);

	fs_unlock(fs);

	k_mem_slab_free(&lfs_dir_pool, &dp->dirp);

	return lfs_to_errno(ret);
}

static int littlefs_stat(struct fs_mount_t *mountp,
			 const char *path, struct fs_dirent *entry)
{
	struct fs_littlefs *fs = mountp->fs_data;

	path = fs_impl_strip_prefix(path, mountp);

	fs_lock(fs);

	struct lfs_info info;
	int ret = lfs_stat(&fs->lfs, path, &info);

	fs_unlock(fs);

	if (ret >= 0) {
		info_to_dirent(&info, entry);
		ret = 0;
	}

	return lfs_to_errno(ret);
}

static int littlefs_statvfs(struct fs_mount_t *mountp,
			    const char *path, struct fs_statvfs *stat)
{
	struct fs_littlefs *fs = mountp->fs_data;
	struct lfs *lfs = &fs->lfs;

	stat->f_bsize = lfs->cfg->prog_size;
	stat->f_frsize = lfs->cfg->block_size;
	stat->f_blocks = lfs->cfg->block_count;

	path = fs_impl_strip_prefix(path, mountp);

	fs_lock(fs);

	ssize_t ret = lfs_fs_size(lfs);

	fs_unlock(fs);

	if (ret >= 0) {
		stat->f_bfree = stat->f_blocks - ret;
		ret = 0;
	}

	return lfs_to_errno(ret);
}

/* Return maximum page size in a flash area.  There's no flash_area
 * API to implement this, so we have to make one here.
 */
struct get_page_ctx {
	const struct flash_area *area;
	lfs_size_t max_size;
};

static bool get_page_cb(const struct flash_pages_info *info, void *ctxp)
{
	struct get_page_ctx *ctx = ctxp;

	size_t info_start = info->start_offset;
	size_t info_end = info_start + info->size - 1U;
	size_t area_start = ctx->area->fa_off;
	size_t area_end = area_start + ctx->area->fa_size - 1U;

	/* Ignore pages outside the area */
	if (info_end < area_start) {
		return true;
	}
	if (info_start > area_end) {
		return false;
	}

	if (info->size > ctx->max_size) {
		ctx->max_size = info->size;
	}

	return true;
}

/* Iterate over all page groups in the flash area and return the
 * largest page size we see.  This works as long as the partition is
 * aligned so that erasing with this size is supported throughout the
 * partition.
 */
static lfs_size_t get_block_size(const struct flash_area *fa)
{
	struct get_page_ctx ctx = {
		.area = fa,
		.max_size = 0,
	};
	struct device *dev = flash_area_get_device(fa);

	flash_page_foreach(dev, get_page_cb, &ctx);

	return ctx.max_size;
}

static int littlefs_mount(struct fs_mount_t *mountp)
{
	int ret;
	struct fs_littlefs *fs = mountp->fs_data;
	unsigned int area_id = (uintptr_t)mountp->storage_dev;
	struct device *dev;

	LOG_INF("LittleFS version %u.%u, disk version %u.%u",
		LFS_VERSION_MAJOR, LFS_VERSION_MINOR,
		LFS_DISK_VERSION_MAJOR, LFS_DISK_VERSION_MINOR);

	if (fs->area) {
		return -EBUSY;
	}

	/* Create and take mutex. */
	k_mutex_init(&fs->mutex);
	fs_lock(fs);

	/* Open flash area */
	ret = flash_area_open(area_id, &fs->area);
	if ((ret < 0) || (fs->area == NULL)) {
		LOG_ERR("can't open flash area %d", area_id);
		ret = -ENODEV;
		goto out;
	}
	LOG_DBG("FS area %u at 0x%x for %u bytes",
		area_id, (uint32_t)fs->area->fa_off,
		(uint32_t)fs->area->fa_size);

	dev = flash_area_get_device(fs->area);
	if (dev == NULL) {
		LOG_ERR("can't get flash device: %s", fs->area->fa_dev_name);
		ret = -ENODEV;
		goto out;
	}

	BUILD_ASSERT(CONFIG_FS_LITTLEFS_READ_SIZE > 0);
	BUILD_ASSERT(CONFIG_FS_LITTLEFS_PROG_SIZE > 0);
	BUILD_ASSERT(CONFIG_FS_LITTLEFS_CACHE_SIZE > 0);
	BUILD_ASSERT(CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE > 0);
	BUILD_ASSERT((CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE % 8) == 0);
	BUILD_ASSERT((CONFIG_FS_LITTLEFS_CACHE_SIZE
		      % CONFIG_FS_LITTLEFS_READ_SIZE) == 0);
	BUILD_ASSERT((CONFIG_FS_LITTLEFS_CACHE_SIZE
		      % CONFIG_FS_LITTLEFS_PROG_SIZE) == 0);

	struct lfs_config *lcp = &fs->cfg;

	lfs_size_t read_size = lcp->read_size;

	if (read_size == 0) {
		read_size = CONFIG_FS_LITTLEFS_READ_SIZE;
	}

	lfs_size_t prog_size = lcp->prog_size;

	if (prog_size == 0) {
		prog_size = CONFIG_FS_LITTLEFS_PROG_SIZE;
	}

	/* Yes, you can override block size. */
	lfs_size_t block_size = lcp->block_size;

	if (block_size == 0) {
		block_size = get_block_size(fs->area);
	}
	if (block_size == 0) {
		__ASSERT_NO_MSG(block_size != 0);
		ret = -EINVAL;
		goto out;
	}

	int32_t block_cycles = lcp->block_cycles;

	if (block_cycles == 0) {
		block_cycles = CONFIG_FS_LITTLEFS_BLOCK_CYCLES;
	}
	if (block_cycles <= 0) {
		/* Disable leveling (littlefs v2.1+ semantics) */
		block_cycles = -1;
	}

	lfs_size_t cache_size = lcp->cache_size;

	if (cache_size == 0) {
		cache_size = CONFIG_FS_LITTLEFS_CACHE_SIZE;
	}

	lfs_size_t lookahead_size = lcp->lookahead_size;

	if (lookahead_size == 0) {
		lookahead_size = CONFIG_FS_LITTLEFS_LOOKAHEAD_SIZE;
	}


	/* No, you don't get to override this. */
	lfs_size_t block_count = fs->area->fa_size / block_size;

	LOG_INF("FS at %s:0x%x is %u 0x%x-byte blocks with %u cycle",
		dev->name, (uint32_t)fs->area->fa_off,
		block_count, block_size, block_cycles);
	LOG_INF("sizes: rd %u ; pr %u ; ca %u ; la %u",
		read_size, prog_size, cache_size, lookahead_size);

	__ASSERT_NO_MSG(prog_size != 0);
	__ASSERT_NO_MSG(read_size != 0);
	__ASSERT_NO_MSG(cache_size != 0);
	__ASSERT_NO_MSG(block_size != 0);

	__ASSERT((fs->area->fa_size % block_size) == 0,
		 "partition size must be multiple of block size");
	__ASSERT((block_size % prog_size) == 0,
		 "erase size must be multiple of write size");
	__ASSERT((block_size % cache_size) == 0,
		 "cache size incompatible with block size");

	/* Set the validated/defaulted values. */
	lcp->context = (void *)fs->area;
	lcp->read = lfs_api_read;
	lcp->prog = lfs_api_prog;
	lcp->erase = lfs_api_erase;
	lcp->sync = lfs_api_sync;
	lcp->read_size = read_size;
	lcp->prog_size = prog_size;
	lcp->block_size = block_size;
	lcp->block_count = block_count;
	lcp->block_cycles = block_cycles;
	lcp->cache_size = cache_size;
	lcp->lookahead_size = lookahead_size;

	/* Mount it, formatting if needed. */
	ret = lfs_mount(&fs->lfs, &fs->cfg);
	if (ret < 0) {
		LOG_WRN("can't mount (LFS %d); formatting", ret);
		ret = lfs_format(&fs->lfs, &fs->cfg);
		if (ret < 0) {
			LOG_ERR("format failed (LFS %d)", ret);
			ret = lfs_to_errno(ret);
			goto out;
		}
		ret = lfs_mount(&fs->lfs, &fs->cfg);
		if (ret < 0) {
			LOG_ERR("remount after format failed (LFS %d)", ret);
			ret = lfs_to_errno(ret);
			goto out;
		}
	}

	LOG_INF("%s mounted", log_strdup(mountp->mnt_point));

out:
	if (ret < 0) {
		fs->area = NULL;
	}

	fs_unlock(fs);

	return ret;
}

static int littlefs_unmount(struct fs_mount_t *mountp)
{
	struct fs_littlefs *fs = mountp->fs_data;

	fs_lock(fs);

	lfs_unmount(&fs->lfs);
	flash_area_close(fs->area);
	fs->area = NULL;

	fs_unlock(fs);

	LOG_INF("%s unmounted", log_strdup(mountp->mnt_point));

	return 0;
}

/* File system interface */
static struct fs_file_system_t littlefs_fs = {
	.open = littlefs_open,
	.close = littlefs_close,
	.read = littlefs_read,
	.write = littlefs_write,
	.lseek = littlefs_seek,
	.tell = littlefs_tell,
	.truncate = littlefs_truncate,
	.sync = littlefs_sync,
	.opendir = littlefs_opendir,
	.readdir = littlefs_readdir,
	.closedir = littlefs_closedir,
	.mount = littlefs_mount,
	.unmount = littlefs_unmount,
	.unlink = littlefs_unlink,
	.rename = littlefs_rename,
	.mkdir = littlefs_mkdir,
	.stat = littlefs_stat,
	.statvfs = littlefs_statvfs,
};

static int littlefs_init(struct device *dev)
{
	ARG_UNUSED(dev);
	return fs_register(FS_LITTLEFS, &littlefs_fs);
}

SYS_INIT(littlefs_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
