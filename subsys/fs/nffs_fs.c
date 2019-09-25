/*
 * Copyright (c) 2017 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <zephyr/types.h>
#include <errno.h>
#include <init.h>
#include <drivers/flash.h>
#include <fs/fs.h>
#include <sys/crc.h>
#include <sys/__assert.h>
#include <sys/printk.h>
#include <nffs/os.h>
#include <nffs/nffs.h>

#include "fs_impl.h"

#define NFFS_MAX_FILE_NAME 256

/*
 * NFFS code keeps fs state in RAM but access to these structures is not
 * thread-safe - we need global lock for each fs operation to guarantee two
 * threads won't modify NFFS at the same time.
 */
static struct k_mutex nffs_lock;

/*
 * TODO: Get rid of global flash_dev which limits
 * system to have multiple instances of NFFS.
 */
static struct device *flash_dev;

/* nffs flash area descriptors */
static struct nffs_area_desc descs[CONFIG_NFFS_FILESYSTEM_MAX_AREAS + 1];

K_MEM_SLAB_DEFINE(nffs_file_pool,		sizeof(struct nffs_file),
		  CONFIG_FS_NFFS_NUM_FILES,		4);
K_MEM_SLAB_DEFINE(nffs_dir_pool,		sizeof(struct nffs_dir),
		  CONFIG_FS_NFFS_NUM_DIRS,		4);
K_MEM_SLAB_DEFINE(nffs_inode_entry_pool,	sizeof(struct nffs_inode_entry),
		  CONFIG_FS_NFFS_NUM_INODES,		4);
K_MEM_SLAB_DEFINE(nffs_block_entry_pool,	sizeof(struct nffs_hash_entry),
		  CONFIG_FS_NFFS_NUM_BLOCKS,		4);
K_MEM_SLAB_DEFINE(nffs_cache_inode_pool,	sizeof(struct nffs_cache_inode),
		  CONFIG_FS_NFFS_NUM_CACHE_INODES,	4);
K_MEM_SLAB_DEFINE(nffs_cache_block_pool,	sizeof(struct nffs_cache_block),
		  CONFIG_FS_NFFS_NUM_CACHE_BLOCKS,	4);

static int translate_error(int error)
{
	switch (error) {
	case FS_EOK:
		return 0;
	case FS_ECORRUPT:
	case FS_EHW:
		return -EIO;
	case FS_EOFFSET:
	case FS_EINVAL:
		return -EINVAL;
	case FS_ENOMEM:
		return -ENOMEM;
	case FS_ENOENT:
		return -ENOENT;
	case FS_EEMPTY:
		return -ENODEV;
	case FS_EFULL:
		return -ENOSPC;
	case FS_EUNEXP:
	case FS_EOS:
		return -EIO;
	case FS_EEXIST:
		return -EEXIST;
	case FS_EACCESS:
		return -EACCES;
	case FS_EUNINIT:
		return -EIO;
	}

	return -EIO;
}

int nffs_os_mempool_init(void)
{
	/*
	 * Just reinitialize slabs here - this is what original implementation
	 * does. We assume all references to previously allocated blocks, if
	 * any, are invalidated in NFFS code already.
	 */

	k_mem_slab_init(&nffs_file_pool, _k_mem_slab_buf_nffs_file_pool,
			sizeof(struct nffs_file),
			CONFIG_FS_NFFS_NUM_FILES);
	k_mem_slab_init(&nffs_dir_pool, _k_mem_slab_buf_nffs_dir_pool,
			sizeof(struct nffs_dir),
			CONFIG_FS_NFFS_NUM_DIRS);
	k_mem_slab_init(&nffs_inode_entry_pool,
			_k_mem_slab_buf_nffs_inode_entry_pool,
			sizeof(struct nffs_inode_entry),
			CONFIG_FS_NFFS_NUM_INODES);
	k_mem_slab_init(&nffs_block_entry_pool,
			_k_mem_slab_buf_nffs_block_entry_pool,
			sizeof(struct nffs_hash_entry),
			CONFIG_FS_NFFS_NUM_BLOCKS);
	k_mem_slab_init(&nffs_cache_inode_pool,
			_k_mem_slab_buf_nffs_cache_inode_pool,
			sizeof(struct nffs_cache_inode),
			CONFIG_FS_NFFS_NUM_CACHE_INODES);
	k_mem_slab_init(&nffs_cache_block_pool,
			_k_mem_slab_buf_nffs_cache_block_pool,
			sizeof(struct nffs_cache_block),
			CONFIG_FS_NFFS_NUM_CACHE_BLOCKS);

	return 0;
}

void *nffs_os_mempool_get(nffs_os_mempool_t *pool)
{
	int rc;
	void *ptr;

	rc = k_mem_slab_alloc(pool, &ptr, K_NO_WAIT);
	if (rc) {
		ptr = NULL;
	}

	return ptr;
}

int nffs_os_mempool_free(nffs_os_mempool_t *pool, void *block)
{
	k_mem_slab_free(pool, &block);

	return 0;
}

int nffs_os_flash_read(uint8_t id, uint32_t address, void *dst,
		uint32_t num_bytes)
{
	int rc;

	rc = flash_read(flash_dev, address, dst, num_bytes);

	return rc;
}

int nffs_os_flash_write(uint8_t id, uint32_t address, const void *src,
		uint32_t num_bytes)
{
	int rc;

	rc = flash_write_protection_set(flash_dev, false);
	if (rc) {
		return rc;
	}

	rc = flash_write(flash_dev, address, src, num_bytes);

	/* Ignore errors here - this does not affect write operation */
	(void) flash_write_protection_set(flash_dev, true);

	return rc;
}

int nffs_os_flash_erase(uint8_t id, uint32_t address, uint32_t num_bytes)
{
	int rc;

	rc = flash_write_protection_set(flash_dev, false);
	if (rc) {
		return rc;
	}

	rc = flash_erase(flash_dev, address, num_bytes);

	/* Ignore errors here - this does not affect erase operation */
	(void) flash_write_protection_set(flash_dev, true);

	return rc;
}

int nffs_os_flash_info(uint8_t id, uint32_t sector, uint32_t *address,
		uint32_t *size)
{
	struct flash_pages_info pi;
	int rc;

	rc = flash_get_page_info_by_idx(flash_dev, sector, &pi);
	__ASSERT(rc == 0, "Failed to obtain flash page data");

	*address = pi.start_offset;
	*size = pi.size;

	return 0;
}

uint16_t nffs_os_crc16_ccitt(uint16_t initial_crc, const void *buf, int len,
		int final)
{
	return crc16(buf, len, 0x1021, initial_crc, final);
}

static int inode_to_dirent(struct nffs_inode_entry *inode,
			   struct fs_dirent *entry)
{
	u8_t name_len;
	uint32_t size;
	int rc;

	rc = nffs_inode_read_filename(inode, sizeof(entry->name), entry->name,
				      &name_len);
	if (rc) {
		return rc;
	}

	if (nffs_hash_id_is_dir(inode->nie_hash_entry.nhe_id)) {
		entry->type = FS_DIR_ENTRY_DIR;
		entry->size = 0;
	} else {
		entry->type = FS_DIR_ENTRY_FILE;
		rc = nffs_inode_data_len(inode, &size);
		if (rc) {
			return rc;
		}
		entry->size = size;
	}

	return rc;
}

static int nffs_open(struct fs_file_t *zfp, const char *file_name)
{
	int rc;

	file_name = fs_impl_strip_prefix(file_name, zfp->mp);

	k_mutex_lock(&nffs_lock, K_FOREVER);

	zfp->filep = NULL;

	if (!nffs_misc_ready()) {
		k_mutex_unlock(&nffs_lock);
		return -ENODEV;
	}

	rc = nffs_file_open((struct nffs_file **)&zfp->filep, file_name,
			    FS_ACCESS_READ | FS_ACCESS_WRITE);

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_close(struct fs_file_t *zfp)
{
	int rc;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	rc = nffs_file_close(zfp->filep);
	if (!rc) {
		zfp->filep = NULL;
	}

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_unlink(struct fs_mount_t *mountp, const char *path)
{
	int rc;

	path = fs_impl_strip_prefix(path, mountp);

	k_mutex_lock(&nffs_lock, K_FOREVER);

	rc = nffs_path_unlink(path);

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static ssize_t nffs_read(struct fs_file_t *zfp, void *ptr, size_t size)
{
	uint32_t br;
	int rc;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	rc = nffs_file_read(zfp->filep, size, ptr, &br);

	k_mutex_unlock(&nffs_lock);

	if (rc) {
		return translate_error(rc);
	}

	return br;
}

static ssize_t nffs_write(struct fs_file_t *zfp, const void *ptr, size_t size)
{
	int rc;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	rc = nffs_write_to_file(zfp->filep, ptr, size);

	k_mutex_unlock(&nffs_lock);

	if (rc) {
		return translate_error(rc);
	}

	/* We need to assume all bytes were written */
	return size;
}

static int nffs_seek(struct fs_file_t *zfp, off_t offset, int whence)
{
	uint32_t len;
	u32_t pos;
	int rc;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	switch (whence) {
	case FS_SEEK_SET:
		pos = offset;
		break;
	case FS_SEEK_CUR:
		pos = ((struct nffs_file *)zfp->filep)->nf_offset + offset;
		break;
	case FS_SEEK_END:
		rc = nffs_inode_data_len(((struct nffs_file *)zfp->filep)->nf_inode_entry, &len);
		if (rc) {
			k_mutex_unlock(&nffs_lock);
			return -EINVAL;
		}
		pos = len + offset;
		break;
	default:
		k_mutex_unlock(&nffs_lock);
		return -EINVAL;
	}

	rc = nffs_file_seek(zfp->filep, pos);

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static off_t nffs_tell(struct fs_file_t *zfp)
{
	u32_t offset;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	if (!zfp->filep) {
		k_mutex_unlock(&nffs_lock);
		return -EIO;
	}

	offset = ((struct nffs_file *)zfp->filep)->nf_offset;

	k_mutex_unlock(&nffs_lock);

	return offset;
}

static int nffs_truncate(struct fs_file_t *zfp, off_t length)
{
	/*
	 * FIXME:
	 * There is no API in NFFS to truncate opened file. For now we return
	 * ENOTSUP, but this should be revisited if truncation is implemented
	 * in NFFS at some point.
	 */

	return -ENOTSUP;
}

static int nffs_sync(struct fs_file_t *zfp)
{
	/*
	 * Files are written to flash immediately so we do not need to support
	 * sync call - just return success.
	 */

	return 0;
}

static int nffs_mkdir(struct fs_mount_t *mountp, const char *path)
{
	int rc;

	path = fs_impl_strip_prefix(path, mountp);

	k_mutex_lock(&nffs_lock, K_FOREVER);

	if (!nffs_misc_ready()) {
		k_mutex_unlock(&nffs_lock);
		return -ENODEV;
	}

	rc = nffs_path_new_dir(path, NULL);

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_opendir(struct fs_dir_t *zdp, const char *path)
{
	int rc;

	path = fs_impl_strip_prefix(path, zdp->mp);

	k_mutex_lock(&nffs_lock, K_FOREVER);

	zdp->dirp = NULL;

	if (!nffs_misc_ready()) {
		k_mutex_unlock(&nffs_lock);
		return -ENODEV;
	}

	rc = nffs_dir_open(path, (struct nffs_dir **)&zdp->dirp);

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_readdir(struct fs_dir_t *zdp, struct fs_dirent *entry)
{
	struct nffs_dirent *dirent;
	int rc;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	rc = nffs_dir_read(zdp->dirp, &dirent);
	switch (rc) {
	case 0:
		rc = inode_to_dirent(dirent->nde_inode_entry, entry);
		break;
	case FS_ENOENT:
		entry->name[0] = 0;
		rc = 0;
		break;
	default:
		break;
	}

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_closedir(struct fs_dir_t *zdp)
{
	int rc;

	k_mutex_lock(&nffs_lock, K_FOREVER);

	rc = nffs_dir_close(zdp->dirp);
	if (!rc) {
		zdp->dirp = NULL;
	}

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_stat(struct fs_mount_t *mountp,
		     const char *path, struct fs_dirent *entry)
{
	struct nffs_path_parser parser;
	struct nffs_inode_entry *parent;
	struct nffs_inode_entry *inode;
	int rc;

	path = fs_impl_strip_prefix(path, mountp);

	k_mutex_lock(&nffs_lock, K_FOREVER);

	nffs_path_parser_new(&parser, path);

	rc = nffs_path_find(&parser, &inode, &parent);
	if (rc == 0) {
		rc = inode_to_dirent(inode, entry);
	}

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_statvfs(struct fs_mount_t *mountp,
			const char *path, struct fs_statvfs *stat)
{
	/*
	 * FIXME:
	 * There is not API to retrieve such data in NFFS.
	 */

	return -ENOTSUP;
}

static int nffs_rename(struct fs_mount_t *mountp, const char *from,
		       const char *to)
{
	int rc;

	from = fs_impl_strip_prefix(from, mountp);
	to = fs_impl_strip_prefix(to, mountp);

	k_mutex_lock(&nffs_lock, K_FOREVER);

	if (!nffs_misc_ready()) {
		k_mutex_unlock(&nffs_lock);
		return -ENODEV;
	}

	rc = nffs_path_rename(from, to);

	k_mutex_unlock(&nffs_lock);

	return translate_error(rc);
}

static int nffs_mount(struct fs_mount_t *mountp)
{
	struct nffs_flash_desc *flash_desc =
				(struct nffs_flash_desc *)mountp->fs_data;
	int cnt;
	int rc;

	/* Set flash device */
	flash_dev = (struct device *)mountp->storage_dev;

	/* Set flash descriptor fields */
	flash_desc->id = 0;
	flash_desc->sector_count = flash_get_page_count(flash_dev);
	flash_desc->area_offset = DT_FLASH_AREA_STORAGE_OFFSET;
	flash_desc->area_size = DT_FLASH_AREA_STORAGE_SIZE;

	rc = nffs_misc_reset();
	if (rc) {
		return -EIO;
	}

	cnt = CONFIG_NFFS_FILESYSTEM_MAX_AREAS;
	rc = nffs_misc_desc_from_flash_area(flash_desc, &cnt, descs);
	if (rc) {
		return -EIO;
	}

	rc = nffs_restore_full(descs);
	switch (rc) {
	case 0:
		break;
	case FS_ECORRUPT:
		rc = nffs_format_full(descs);
		if (rc) {
			return -EIO;
		}
		break;
	default:
		return -EIO;
	}

	return 0;
}

/* File system interface */
static struct fs_file_system_t nffs_fs = {
	.open = nffs_open,
	.close = nffs_close,
	.read = nffs_read,
	.write = nffs_write,
	.lseek = nffs_seek,
	.tell = nffs_tell,
	.truncate = nffs_truncate,
	.sync = nffs_sync,
	.opendir = nffs_opendir,
	.readdir = nffs_readdir,
	.closedir = nffs_closedir,
	.mount = nffs_mount,
	.unlink = nffs_unlink,
	.rename = nffs_rename,
	.mkdir = nffs_mkdir,
	.stat = nffs_stat,
	.statvfs = nffs_statvfs,
};

static int nffs_init(struct device *dev)
{
	ARG_UNUSED(dev);

	k_mutex_init(&nffs_lock);

	return fs_register(FS_NFFS, &nffs_fs);
}

SYS_INIT(nffs_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
