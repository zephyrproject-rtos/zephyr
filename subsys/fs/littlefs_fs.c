/*
 * Copyright (c) 2019 Bolt Innovation Management, LLC
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>

#define LFS_LOG_REGISTER
#include <lfs_util.h>

#include <lfs.h>
#include <zephyr/fs/littlefs.h>
#ifdef CONFIG_FS_LITTLEFS_FMP_DEV
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#endif
#ifdef CONFIG_FS_LITTLEFS_BLK_DEV
#include <zephyr/storage/disk_access.h>
#endif

#include "fs_impl.h"

/* Used on devices that have no explicit erase */
#define LITTLEFS_DEFAULT_BLOCK_SIZE     4096

/* note: one of the next options have to be enabled, at least */
BUILD_ASSERT(IS_ENABLED(CONFIG_FS_LITTLEFS_BLK_DEV) ||
	     IS_ENABLED(CONFIG_FS_LITTLEFS_FMP_DEV));

struct lfs_file_data {
	struct lfs_file file;
	struct lfs_file_config config;
	void *cache_block;
};

#define LFS_FILEP(fp) (&((struct lfs_file_data *)(fp->filep))->file)

/* Global memory pool for open files and dirs */
K_MEM_SLAB_DEFINE_STATIC(file_data_pool, sizeof(struct lfs_file_data),
			 CONFIG_FS_LITTLEFS_NUM_FILES, 4);
K_MEM_SLAB_DEFINE_STATIC(lfs_dir_pool, sizeof(struct lfs_dir),
			 CONFIG_FS_LITTLEFS_NUM_DIRS, 4);

/* Inferred overhead, in bytes, for each k_heap_aligned allocation for
 * the filecache heap.  This relates to the CHUNK_UNIT parameter in
 * the heap implementation, but that value is not visible outside the
 * kernel.
 * FIXME: value for this macro should be rather taken from the Kernel
 * internals than set by user, but we do not have a way to do so now.
 */
#define FC_HEAP_PER_ALLOC_OVERHEAD CONFIG_FS_LITTLEFS_HEAP_PER_ALLOC_OVERHEAD_SIZE

#if (CONFIG_FS_LITTLEFS_FC_HEAP_SIZE - 0) <= 0
BUILD_ASSERT((CONFIG_FS_LITTLEFS_HEAP_PER_ALLOC_OVERHEAD_SIZE % 8) == 0);
/* Auto-generate heap size from cache size and number of files */
#undef CONFIG_FS_LITTLEFS_FC_HEAP_SIZE
#define CONFIG_FS_LITTLEFS_FC_HEAP_SIZE						\
	((CONFIG_FS_LITTLEFS_CACHE_SIZE + FC_HEAP_PER_ALLOC_OVERHEAD) *		\
	CONFIG_FS_LITTLEFS_NUM_FILES)
#endif /* CONFIG_FS_LITTLEFS_FC_HEAP_SIZE */

static K_HEAP_DEFINE(file_cache_heap, CONFIG_FS_LITTLEFS_FC_HEAP_SIZE);

static inline bool littlefs_on_blkdev(int flags)
{
	return (flags & FS_MOUNT_FLAG_USE_DISK_ACCESS) ? true : false;
}

static inline void *fc_allocate(size_t size)
{
	void *ret = NULL;

	ret = k_heap_alloc(&file_cache_heap, size, K_NO_WAIT);

	return ret;
}

static inline void fc_release(void *buf)
{
	k_heap_free(&file_cache_heap, buf);
}

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


#ifdef CONFIG_FS_LITTLEFS_FMP_DEV

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

	int rc = flash_area_flatten(fa, offset, c->block_size);

	return errno_to_lfs(rc);
}
#endif /* CONFIG_FS_LITTLEFS_FMP_DEV */

#ifdef CONFIG_FS_LITTLEFS_BLK_DEV
static int lfs_api_read_blk(const struct lfs_config *c, lfs_block_t block,
			    lfs_off_t off, void *buffer, lfs_size_t size)
{
	const char *disk = c->context;
	int rc = disk_access_read(disk, buffer, block,
				  size / c->block_size);

	return errno_to_lfs(rc);
}

static int lfs_api_prog_blk(const struct lfs_config *c, lfs_block_t block,
			    lfs_off_t off, const void *buffer, lfs_size_t size)
{
	const char *disk = c->context;
	int rc = disk_access_write(disk, buffer, block, size / c->block_size);

	return errno_to_lfs(rc);
}

static int lfs_api_sync_blk(const struct lfs_config *c)
{
	const char *disk = c->context;
	int rc = disk_access_ioctl(disk, DISK_IOCTL_CTRL_SYNC, NULL);

	return errno_to_lfs(rc);
}
#else
static int lfs_api_read_blk(const struct lfs_config *c, lfs_block_t block,
			    lfs_off_t off, void *buffer, lfs_size_t size)
{
	return 0;
}

static int lfs_api_prog_blk(const struct lfs_config *c, lfs_block_t block,
			    lfs_off_t off, const void *buffer, lfs_size_t size)
{
	return 0;
}

static int lfs_api_sync_blk(const struct lfs_config *c)
{
	return 0;
}
#endif /* CONFIG_FS_LITTLEFS_BLK_DEV */
static int lfs_api_erase_blk(const struct lfs_config *c, lfs_block_t block)
{
	return 0;
}

static int lfs_api_sync(const struct lfs_config *c)
{
	return LFS_ERR_OK;
}

static void release_file_data(struct fs_file_t *fp)
{
	struct lfs_file_data *fdp = fp->filep;

	if (fdp->config.buffer) {
		fc_release(fdp->cache_block);
	}

	k_mem_slab_free(&file_data_pool, fp->filep);
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

	fdp->cache_block = fc_allocate(lfs->cfg->cache_size);
	if (fdp->cache_block == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	fdp->config.buffer = fdp->cache_block;
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
		k_mem_slab_free(&lfs_dir_pool, dp->dirp);
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

	k_mem_slab_free(&lfs_dir_pool, dp->dirp);

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

#ifdef CONFIG_FS_LITTLEFS_FMP_DEV

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
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
#endif

/* Iterate over all page groups in the flash area and return the
 * largest page size we see.  This works as long as the partition is
 * aligned so that erasing with this size is supported throughout the
 * partition.
 */
static lfs_size_t get_block_size(const struct flash_area *fa)
{
#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
	struct get_page_ctx ctx = {
		.area = fa,
		.max_size = 0,
	};
	const struct device *dev = flash_area_get_device(fa);
#if defined(CONFIG_FLASH_HAS_NO_EXPLICIT_ERASE)
	const struct flash_parameters *fparams = flash_get_parameters(dev);

	if (!(flash_params_get_erase_cap(fparams) & FLASH_ERASE_C_EXPLICIT)) {
		return LITTLEFS_DEFAULT_BLOCK_SIZE;
	}
#endif

	flash_page_foreach(dev, get_page_cb, &ctx);

	return ctx.max_size;
#else
	return LITTLEFS_DEFAULT_BLOCK_SIZE;
#endif
}

static int littlefs_flash_init(struct fs_littlefs *fs, void *dev_id)
{
	unsigned int area_id = POINTER_TO_UINT(dev_id);
	const struct flash_area **fap = (const struct flash_area **)&fs->backend;
	const struct device *dev;
	int ret;

	/* Open flash area */
	ret = flash_area_open(area_id, fap);
	if ((ret < 0) || (*fap == NULL)) {
		LOG_ERR("can't open flash area %d", area_id);
		return -ENODEV;
	}

	LOG_DBG("FS area %u at 0x%x for %u bytes", area_id,
		(uint32_t)(*fap)->fa_off, (uint32_t)(*fap)->fa_size);

	dev = flash_area_get_device(*fap);
	if (dev == NULL) {
		LOG_ERR("can't get flash device: %s",
			(*fap)->fa_dev->name);
		return -ENODEV;
	}

	fs->backend = (void *) *fap;
	return 0;
}
#endif /* CONFIG_FS_LITTLEFS_FMP_DEV */

static int littlefs_init_backend(struct fs_littlefs *fs, void *dev_id, int flags)
{
	int ret = 0;

	if (!(IS_ENABLED(CONFIG_FS_LITTLEFS_FMP_DEV) && !littlefs_on_blkdev(flags)) &&
	    !(IS_ENABLED(CONFIG_FS_LITTLEFS_BLK_DEV) && littlefs_on_blkdev(flags))) {
		LOG_ERR("Can't init littlefs backend, review configs and flags 0x%08x", flags);
		return -ENOTSUP;
	}

#ifdef CONFIG_FS_LITTLEFS_BLK_DEV
	if (littlefs_on_blkdev(flags)) {
		fs->backend = dev_id;
		ret = disk_access_init((char *) fs->backend);
		if (ret < 0) {
			LOG_ERR("Storage init ERROR!");
			return ret;
		}
	}
#endif /* CONFIG_FS_LITTLEFS_BLK_DEV */
#ifdef CONFIG_FS_LITTLEFS_FMP_DEV
	if (!littlefs_on_blkdev(flags)) {
		ret = littlefs_flash_init(fs, dev_id);
		if (ret < 0) {
			return ret;
		}
	}
#endif /* CONFIG_FS_LITTLEFS_FMP_DEV */
	return 0;
}

static int littlefs_init_cfg(struct fs_littlefs *fs, int flags)
{
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

	if (!(IS_ENABLED(CONFIG_FS_LITTLEFS_FMP_DEV) && !littlefs_on_blkdev(flags)) &&
	    !(IS_ENABLED(CONFIG_FS_LITTLEFS_BLK_DEV) && littlefs_on_blkdev(flags))) {
		LOG_ERR("Can't init littlefs config, review configs and flags 0x%08x", flags);
		return -ENOTSUP;
	}

	if (block_size == 0) {
#ifdef CONFIG_FS_LITTLEFS_BLK_DEV
		if (littlefs_on_blkdev(flags)) {
			int ret = disk_access_ioctl((char *) fs->backend,
						DISK_IOCTL_GET_SECTOR_SIZE,
						&block_size);
			if (ret < 0) {
				LOG_ERR("Unable to get sector size");
				return ret;
			}
		}
#endif /* CONFIG_FS_LITTLEFS_BLK_DEV */

#ifdef CONFIG_FS_LITTLEFS_FMP_DEV
		if (!littlefs_on_blkdev(flags)) {
			block_size = get_block_size((struct flash_area *)fs->backend);
		}
#endif /* CONFIG_FS_LITTLEFS_FMP_DEV */
	}

	if (block_size == 0) {
		__ASSERT_NO_MSG(block_size != 0);
		return -EINVAL;
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
	lfs_size_t block_count = 0;

#ifdef CONFIG_FS_LITTLEFS_BLK_DEV
	if (littlefs_on_blkdev(flags)) {
		int ret = disk_access_ioctl((char *) fs->backend,
					DISK_IOCTL_GET_SECTOR_COUNT,
					&block_count);
		if (ret < 0) {
			LOG_ERR("Unable to get sector count!");
			return -EINVAL;
		}
		LOG_INF("FS at %s: is %u 0x%x-byte blocks with %u cycle",
			(char *) fs->backend, block_count, block_size,
			block_cycles);
	}
#endif /* CONFIG_FS_LITTLEFS_BLK_DEV */

#ifdef CONFIG_FS_LITTLEFS_FMP_DEV
	if (!littlefs_on_blkdev(flags)) {
		block_count = ((struct flash_area *)fs->backend)->fa_size
			/ block_size;
		const struct device *dev =
			flash_area_get_device((struct flash_area *)fs->backend);
		LOG_INF("FS at %s:0x%x is %u 0x%x-byte blocks with %u cycle",
			dev->name,
			(uint32_t)((struct flash_area *)fs->backend)->fa_off,
			block_count, block_size, block_cycles);
		LOG_INF("sizes: rd %u ; pr %u ; ca %u ; la %u",
			read_size, prog_size, cache_size, lookahead_size);
	}
#endif /* CONFIG_FS_LITTLEFS_FMP_DEV */

	__ASSERT_NO_MSG(prog_size != 0);
	__ASSERT_NO_MSG(read_size != 0);
	__ASSERT_NO_MSG(cache_size != 0);
	__ASSERT_NO_MSG(block_size != 0);
	__ASSERT_NO_MSG(block_count != 0);

	__ASSERT((block_size % prog_size) == 0,
		 "erase size must be multiple of write size");
	__ASSERT((block_size % cache_size) == 0,
		 "cache size incompatible with block size");

	lcp->context = fs->backend;
	/* Set the validated/defaulted values. */
	if (littlefs_on_blkdev(flags)) {
		lfs_size_t new_cache_size = block_size;
		lfs_size_t new_lookahead_size = block_size * 4;

		lcp->read = lfs_api_read_blk;
		lcp->prog = lfs_api_prog_blk;
		lcp->erase = lfs_api_erase_blk;

		lcp->read_size = block_size;
		lcp->prog_size = block_size;

		if (lcp->cache_size < new_cache_size) {
			LOG_ERR("Configured cache size is too small: %d < %d", lcp->cache_size,
				new_cache_size);
			return -ENOMEM;
		}
		lcp->cache_size = new_cache_size;

		if (lcp->lookahead_size < new_lookahead_size) {
			LOG_ERR("Configured lookahead size is too small: %d < %d",
				lcp->lookahead_size, new_lookahead_size);
			return -ENOMEM;
		}
		lcp->lookahead_size = new_lookahead_size;

		lcp->sync = lfs_api_sync_blk;

		LOG_INF("sizes: rd %u ; pr %u ; ca %u ; la %u",
			lcp->read_size, lcp->prog_size, lcp->cache_size,
			lcp->lookahead_size);
	} else {
		__ASSERT((((struct flash_area *)fs->backend)->fa_size %
			  block_size) == 0,
			 "partition size must be multiple of block size");
#ifdef CONFIG_FS_LITTLEFS_FMP_DEV
		lcp->read = lfs_api_read;
		lcp->prog = lfs_api_prog;
		lcp->erase = lfs_api_erase;
#endif

		lcp->read_size = read_size;
		lcp->prog_size = prog_size;
		lcp->cache_size = cache_size;
		lcp->lookahead_size = lookahead_size;
		lcp->sync = lfs_api_sync;
	}

	lcp->block_size = block_size;
	lcp->block_count = block_count;
	lcp->block_cycles = block_cycles;
	return 0;
}

static int littlefs_init_fs(struct fs_littlefs *fs, void *dev_id, int flags)
{
	int ret = 0;

	LOG_INF("LittleFS version %u.%u, disk version %u.%u",
		LFS_VERSION_MAJOR, LFS_VERSION_MINOR,
		LFS_DISK_VERSION_MAJOR, LFS_DISK_VERSION_MINOR);

	if (fs->backend) {
		return -EBUSY;
	}

	ret = littlefs_init_backend(fs, dev_id, flags);
	if (ret < 0) {
		return ret;
	}

	ret = littlefs_init_cfg(fs, flags);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int littlefs_mount(struct fs_mount_t *mountp)
{
	int ret = 0;
	struct fs_littlefs *fs = mountp->fs_data;

	/* Create and take mutex. */
	k_mutex_init(&fs->mutex);
	fs_lock(fs);

	ret = littlefs_init_fs(fs, mountp->storage_dev, mountp->flags);
	if (ret < 0) {
		goto out;
	}

	/* Mount it, formatting if needed. */
	ret = lfs_mount(&fs->lfs, &fs->cfg);
	if (ret < 0 &&
	    (mountp->flags & FS_MOUNT_FLAG_NO_FORMAT) == 0) {
		if ((mountp->flags & FS_MOUNT_FLAG_READ_ONLY) == 0) {
			LOG_WRN("can't mount (LFS %d); formatting", ret);
			ret = lfs_format(&fs->lfs, &fs->cfg);
			if (ret < 0) {
				LOG_ERR("format failed (LFS %d)", ret);
				ret = lfs_to_errno(ret);
				goto out;
			}
		} else {
			LOG_ERR("can not format read-only system");
			ret = -EROFS;
			goto out;
		}

		ret = lfs_mount(&fs->lfs, &fs->cfg);
		if (ret < 0) {
			LOG_ERR("remount after format failed (LFS %d)", ret);
			ret = lfs_to_errno(ret);
			goto out;
		}
	} else {
		ret = lfs_to_errno(ret);
		goto out;
	}

	LOG_INF("%s mounted", mountp->mnt_point);

out:
	if (ret < 0) {
		fs->backend = NULL;
	}

	fs_unlock(fs);

	return ret;
}

#if defined(CONFIG_FILE_SYSTEM_MKFS)

FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(fs_cfg);

static int littlefs_mkfs(uintptr_t dev_id, void *cfg, int flags)
{
	int ret = 0;
	struct fs_littlefs *fs = &fs_cfg;

	if (cfg != NULL) {
		fs = (struct fs_littlefs *)cfg;
	}

	fs->backend = NULL;

	/* Create and take mutex. */
	k_mutex_init(&fs->mutex);
	fs_lock(fs);

	ret = littlefs_init_fs(fs, UINT_TO_POINTER(dev_id), flags);
	if (ret < 0) {
		goto out;
	}

	ret = lfs_format(&fs->lfs, &fs->cfg);
	if (ret < 0) {
		LOG_ERR("format failed (LFS %d)", ret);
		ret = lfs_to_errno(ret);
		goto out;
	}
out:
	fs->backend = NULL;
	fs_unlock(fs);
	return ret;
}

#endif /* CONFIG_FILE_SYSTEM_MKFS */

static int littlefs_unmount(struct fs_mount_t *mountp)
{
	struct fs_littlefs *fs = mountp->fs_data;

	fs_lock(fs);

	lfs_unmount(&fs->lfs);

#ifdef CONFIG_FS_LITTLEFS_FMP_DEV
	if (!littlefs_on_blkdev(mountp->flags)) {
		flash_area_close(fs->backend);
	}
#endif /* CONFIG_FS_LITTLEFS_FMP_DEV */

	fs->backend = NULL;
	fs_unlock(fs);

	LOG_INF("%s unmounted", mountp->mnt_point);

	return 0;
}

/* File system interface */
static const struct fs_file_system_t littlefs_fs = {
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
#if defined(CONFIG_FILE_SYSTEM_MKFS)
	.mkfs = littlefs_mkfs,
#endif
};

#define DT_DRV_COMPAT zephyr_fstab_littlefs
#define FS_PARTITION(inst) DT_PHANDLE_BY_IDX(DT_DRV_INST(inst), partition, 0)

#define DEFINE_FS(inst) \
static uint8_t __aligned(4) \
	read_buffer_##inst[DT_INST_PROP(inst, cache_size)]; \
static uint8_t __aligned(4) \
	prog_buffer_##inst[DT_INST_PROP(inst, cache_size)]; \
static uint32_t lookahead_buffer_##inst[DT_INST_PROP(inst, lookahead_size) \
					/ sizeof(uint32_t)]; \
BUILD_ASSERT(DT_INST_PROP(inst, read_size) > 0); \
BUILD_ASSERT(DT_INST_PROP(inst, prog_size) > 0); \
BUILD_ASSERT(DT_INST_PROP(inst, cache_size) > 0); \
BUILD_ASSERT(DT_INST_PROP(inst, lookahead_size) > 0); \
BUILD_ASSERT((DT_INST_PROP(inst, lookahead_size) % 8) == 0); \
BUILD_ASSERT((DT_INST_PROP(inst, cache_size) \
	      % DT_INST_PROP(inst, read_size)) == 0); \
BUILD_ASSERT((DT_INST_PROP(inst, cache_size) \
	      % DT_INST_PROP(inst, prog_size)) == 0); \
static struct fs_littlefs fs_data_##inst = { \
	.cfg = { \
		.read_size = DT_INST_PROP(inst, read_size), \
		.prog_size = DT_INST_PROP(inst, prog_size), \
		.cache_size = DT_INST_PROP(inst, cache_size), \
		.lookahead_size = DT_INST_PROP(inst, lookahead_size), \
		.block_cycles = DT_INST_PROP(inst, block_cycles), \
		.read_buffer = read_buffer_##inst, \
		.prog_buffer = prog_buffer_##inst, \
		.lookahead_buffer = lookahead_buffer_##inst, \
	}, \
}; \
struct fs_mount_t FS_FSTAB_ENTRY(DT_DRV_INST(inst)) = { \
	.type = FS_LITTLEFS, \
	.mnt_point = DT_INST_PROP(inst, mount_point), \
	.fs_data = &fs_data_##inst, \
	.storage_dev = (void *)DT_FIXED_PARTITION_ID(FS_PARTITION(inst)), \
	.flags = FSTAB_ENTRY_DT_MOUNT_FLAGS(DT_DRV_INST(inst)), \
};

DT_INST_FOREACH_STATUS_OKAY(DEFINE_FS)

#define REFERENCE_MOUNT(inst) (&FS_FSTAB_ENTRY(DT_DRV_INST(inst))),

static void mount_init(struct fs_mount_t *mp)
{

	LOG_INF("littlefs partition at %s", mp->mnt_point);
	if ((mp->flags & FS_MOUNT_FLAG_AUTOMOUNT) != 0) {
		int rc = fs_mount(mp);

		if (rc < 0) {
			LOG_ERR("Automount %s failed: %d",
				mp->mnt_point, rc);
		} else {
			LOG_INF("Automount %s succeeded",
				mp->mnt_point);
		}
	}
}

static int littlefs_init(void)
{
	static struct fs_mount_t *partitions[] = {
		DT_INST_FOREACH_STATUS_OKAY(REFERENCE_MOUNT)
	};

	int rc = fs_register(FS_LITTLEFS, &littlefs_fs);

	if (rc == 0) {
		struct fs_mount_t **mpi = partitions;

		while (mpi < (partitions + ARRAY_SIZE(partitions))) {
			mount_init(*mpi++);
		}
	}

	return rc;
}

SYS_INIT(littlefs_init, POST_KERNEL, CONFIG_FILE_SYSTEM_INIT_PRIORITY);
