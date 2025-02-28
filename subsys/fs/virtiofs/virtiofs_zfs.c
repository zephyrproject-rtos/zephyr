/*
 * Copyright (c) 2025 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/fs_sys.h>
#include <zephyr/fs/virtiofs.h>
#include <zephyr/posix/fcntl.h>
#include "../fs_impl.h"
#include <string.h>
#include "virtiofs.h"

#define MODE_FTYPE_MASK 0170000
#define MODE_FTYPE_DIR 040000

#define DT_DIR 4
#define DT_REG 8

struct virtiofs_file {
	uint64_t fh;
	uint64_t nodeid;
	uint64_t offset;
	uint32_t open_flags;
};

struct virtiofs_dir {
	uint64_t fh;
	uint64_t nodeid;
	uint64_t offset;
};

K_MEM_SLAB_DEFINE_STATIC(
	file_struct_slab, sizeof(struct virtiofs_file), CONFIG_VIRTIOFS_MAX_FILES, sizeof(void *)
);
K_MEM_SLAB_DEFINE_STATIC(
	dir_struct_slab, sizeof(struct virtiofs_dir), CONFIG_VIRTIOFS_MAX_FILES, sizeof(void *)
);

static int zephyr_mode_to_posix(int m)
{
	int mode = (m & FS_O_CREATE) ? O_CREAT : 0;

	mode |= (m & FS_O_APPEND) ? O_APPEND : 0;
	mode |= (m & FS_O_TRUNC) ? O_TRUNC : 0;

	switch (m & FS_O_MODE_MASK) {
	case FS_O_READ:
		mode |= O_RDONLY;
		break;
	case FS_O_WRITE:
		mode |= O_WRONLY;
		break;
	case FS_O_RDWR:
		mode |= O_RDWR;
		break;
	default:
		break;
	}

	return mode;
}

static const char *virtiofs_strip_prefix(const char *path, const struct fs_mount_t *mp)
{
	const char *virtiofs_path = fs_impl_strip_prefix(path, mp);

	if (virtiofs_path[0] == '/') {
		virtiofs_path++;
	}
	return virtiofs_path;
}

static const char *strip_path(const char *fpath)
{
	const char *c = fpath + strlen(fpath);

	for (; c > fpath; c--) {
		if (*c == '/') {
			c++;
			break;
		}
	}

	return c;
}

/*
 * despite the similarity of fuse/virtiofs to posix fs functions there are some notable differences:
 * - open() is split into lookup+open in case of existing files and lookup+create in case of
 *   O_CREATE
 * - opendir() is split into lookup+opendir
 * - lookups are non-recursive, we have to traverse through each directory in the path
 * - close()/closedir() is split into release+forget/releasedir+forget
 * - read()/write()/readdir() takes offset as a parameter
 * - there is sort of reverse stat() - settatr, that can be used to i.e. truncate the file
 */

static int virtiofs_zfs_open_existing(
	struct fs_file_t *filp, struct fuse_entry_out lookup_ret, int flags)
{
	struct fuse_open_out open_ret;
	struct virtiofs_file *file;

	int ret = virtiofs_open(
		filp->mp->storage_dev, lookup_ret.nodeid, zephyr_mode_to_posix(flags), &open_ret,
		FUSE_FILE
	);
	if (ret == 0) {
		ret = k_mem_slab_alloc(&file_struct_slab, (void **)&file, K_NO_WAIT);
		if (ret != 0) {
			virtiofs_release(
				filp->mp->storage_dev, lookup_ret.nodeid, open_ret.fh, FUSE_FILE
			);
			return ret;
		}

		file->fh = open_ret.fh;
		file->nodeid = lookup_ret.nodeid;
		file->offset = 0;
		file->open_flags = flags;

		filp->filep = file;
	}

	return ret;
}

static int virtiofs_zfs_open_create(
	struct fs_file_t *filp, int flags, const char *path, uint64_t parent_inode)
{
	struct fuse_create_out create_ret;
	const char *fname = strip_path(path);
	struct virtiofs_file *file;

	int ret = virtiofs_create(
		filp->mp->storage_dev, parent_inode, fname,
		zephyr_mode_to_posix(flags), CONFIG_VIRTIOFS_CREATE_MODE_VALUE, &create_ret
	);
	if (ret == 0) {
		ret = k_mem_slab_alloc(&file_struct_slab, (void **)&file, K_NO_WAIT);
		if (ret != 0) {
			virtiofs_release(
				filp->mp->storage_dev, create_ret.entry_out.nodeid,
				create_ret.open_out.fh, FUSE_FILE
			);
			return ret;
		}

		file->fh = create_ret.open_out.fh;
		file->nodeid = create_ret.entry_out.nodeid;
		file->offset = 0;
		file->open_flags = flags;

		filp->filep = file;
	}

	return ret;
}

static int virtiofs_zfs_open(struct fs_file_t *filp, const char *fs_path, fs_mode_t flags)
{
	int ret = 0;
	struct fuse_entry_out lookup_ret;
	const char *path = virtiofs_strip_prefix(fs_path, filp->mp);
	uint64_t parent_inode = FUSE_ROOT_INODE;

	ret = virtiofs_lookup(
		filp->mp->storage_dev, FUSE_ROOT_INODE, path, &lookup_ret, &parent_inode
	);
	if (ret == 0) {
		ret = virtiofs_zfs_open_existing(filp, lookup_ret, flags & ~FS_O_CREATE);
	} else if ((flags & FS_O_CREATE) && parent_inode != 0) {
		ret = virtiofs_zfs_open_create(filp, flags, path, parent_inode);
	} else {
		if (parent_inode != 0) {
			virtiofs_forget(filp->mp->storage_dev, parent_inode, 1);
		}
		return ret;
	}

	if (parent_inode != 0) {
		virtiofs_forget(filp->mp->storage_dev, parent_inode, 1);
	}

	if (ret != 0) {
		virtiofs_forget(filp->mp->storage_dev, lookup_ret.nodeid, 1);
	}

	return ret;
}

static int virtiofs_zfs_close(struct fs_file_t *filp)
{
	struct virtiofs_file *file = filp->filep;
	uint64_t nodeid = file->nodeid;
	int ret = virtiofs_release(filp->mp->storage_dev, file->nodeid, file->fh, FUSE_FILE);

	if (ret == 0) {
		k_mem_slab_free(&file_struct_slab, file);
	} else {
		return ret;
	}

	virtiofs_forget(filp->mp->storage_dev, nodeid, 1);

	return 0;
}

static ssize_t virtiofs_zfs_read(struct fs_file_t *filp, void *dest, size_t nbytes)
{
	struct virtiofs_file *file = filp->filep;
	int read_c = virtiofs_read(
		filp->mp->storage_dev, file->nodeid, file->fh, file->offset, nbytes, dest
	);

	if (read_c >= 0) {
		file->offset += read_c;
	}

	return read_c;
}

#define FUSE_SEEK_SET 0
#define FUSE_SEEK_CUR 1
#define FUSE_SEEK_END 2

static int zephyr_whence_to_posix(int whence)
{
	switch (whence) {
	case SEEK_SET:
		return FUSE_SEEK_SET;
	case SEEK_CUR:
		return FUSE_SEEK_CUR;
	case SEEK_END:
		return FUSE_SEEK_END;
	default:
		return whence;
	}
}

static int virtio_zfs_lseek(struct fs_file_t *filp, off_t off, int whence)
{
	struct virtiofs_file *file = filp->filep;
	struct fuse_lseek_out lseek_ret;
	uint64_t off_arg = off;

	whence = zephyr_whence_to_posix(whence);

	/*
	 * SEEK_CUR is kind of broken with FUSE_LSEEK as reads/writes don't update the file
	 * offset on the host side so if we never used FUSE_LSEEK since opening the file, but
	 * did some reads/writes in the meantime and then used FUSE_LSEEK with SEEK_CUR+x,
	 * the returned offset would've been x instead of sum of read/written bytes + x. One
	 * solution is to pair each read/write with lseek(SEEK_CUR, read_c/write_c) to keep
	 * the offset updated on the host side, but we just don't use SEEK_CUR+x and instead
	 * use SEEK_SET with file->offset+x. Essentially the only thing FUSE_LSEEK provides
	 * for us is bounds checking and easier handling of SEEK_END (otherwise we would have
	 * to use other fuse call to determine file size)
	 */
	if (whence == FUSE_SEEK_CUR) {
		whence = FUSE_SEEK_SET;
		off_arg = file->offset + off;
	}

	int ret = virtiofs_lseek(
		filp->mp->storage_dev, file->nodeid, file->fh, off_arg, whence, &lseek_ret
	);

	if (ret != 0) {
		return ret;
	}

	file->offset = lseek_ret.offset;

	return file->offset;
}

static ssize_t virtio_zfs_write(struct fs_file_t *filp, const void *src, size_t nbytes)
{
	struct virtiofs_file *file = filp->filep;
	struct virtiofs_fs_data *fs_data = filp->mp->fs_data;
	const uint8_t *curr_addr = src;
	int write_c = 0;

	while (nbytes > 0) {
		/*
		 * max write size is limited to max_write from fuse_init_out received during fs
		 * initalization, so we have to split bigger writes into multiple smaller ones.
		 * If we try to write more the recent virtiofsd it will return 12 (Not enough
		 * space), but the older one will assert, rendering fs unusable until restart.
		 */
		size_t curr_size = MIN(nbytes, fs_data->max_write);

		/*
		 * while FUSE_WRITE will always write to the end if O_APPEND was passed when opening
		 * file (ignoring offset param) the file offset itself will remain unmodified on
		 * zephyr side, so we have to update it here
		 */
		if (file->open_flags & FS_O_APPEND) {
			virtio_zfs_lseek(filp, 0, SEEK_END);
		}

		int ret = virtiofs_write(
			filp->mp->storage_dev, file->nodeid, file->fh, file->offset + write_c,
			curr_size, curr_addr
		);

		if (ret >= 0) {
			write_c += ret;
		} else {
			/*
			 * according to fs_write comment in fs.h zephyr handles partial
			 * failures like that
			 */
			if (write_c > 0) {
				errno = ret;
				file->offset += write_c;
				return write_c;
			} else {
				return ret;
			}
		}

		nbytes -= curr_size;
		curr_addr += curr_size;
	}

	file->offset += write_c;
	return write_c;
}

static off_t virtiofs_zfs_tell(struct fs_file_t *filp)
{
	struct virtiofs_file *file = filp->filep;

	return file->offset;
}

static int virtiofs_zfs_truncate(struct fs_file_t *filp, off_t length)
{
	struct virtiofs_file *file = filp->filep;
	struct fuse_setattr_in attrs;
	struct fuse_attr_out setattr_ret;

	attrs.fh = file->fh;
	attrs.size = length;
	attrs.valid = FATTR_SIZE;

	return virtiofs_setattr(filp->mp->storage_dev, file->nodeid, &attrs, &setattr_ret);
}

static int virtiofs_zfs_sync(struct fs_file_t *filp)
{
	struct virtiofs_file *file = filp->filep;

	return virtiofs_fsync(filp->mp->storage_dev, file->nodeid, file->fh);
}

static int virtiofs_zfs_mkdir(struct fs_mount_t *mountp, const char *name)
{
	const char *path = virtiofs_strip_prefix(name, mountp);
	struct fuse_entry_out lookup_ret;
	uint64_t parent_inode = FUSE_ROOT_INODE;
	int ret = virtiofs_lookup(
		mountp->storage_dev, FUSE_ROOT_INODE, path, &lookup_ret, &parent_inode
	);

	if (parent_inode != 0) {
		ret = virtiofs_mkdir(
			mountp->storage_dev, parent_inode, strip_path(name),
			CONFIG_VIRTIOFS_CREATE_MODE_VALUE
		);
		virtiofs_forget(mountp->storage_dev, parent_inode, 1);
	}

	return ret;
}

static int virtiofs_zfs_opendir(struct fs_dir_t *dirp, const char *fs_path)
{
	int ret = 0;
	struct virtiofs_dir *dir;
	struct fuse_entry_out lookup_ret;
	const char *path = virtiofs_strip_prefix(fs_path, dirp->mp);

	if (path[0] == '\0') {
		/* looking up for "" or "/" yields nothing, so we have to use "." for root dir */
		path = ".";
	}

	ret = virtiofs_lookup(dirp->mp->storage_dev, FUSE_ROOT_INODE, path, &lookup_ret, NULL);
	if (ret == 0) {
		struct fuse_open_out open_ret;

		ret = virtiofs_open(
			dirp->mp->storage_dev, lookup_ret.nodeid, O_RDONLY, &open_ret, FUSE_DIR
		);
		if (ret == 0) {
			ret = k_mem_slab_alloc(&dir_struct_slab, (void **)&dir, K_NO_WAIT);
			if (ret != 0) {
				virtiofs_forget(dirp->mp->storage_dev, lookup_ret.nodeid, 1);
				return ret;
			}

			dir->fh = open_ret.fh;
			dir->nodeid = lookup_ret.nodeid;
			dir->offset = 0;
			dirp->dirp = dir;
		}
	} else {
		virtiofs_forget(dirp->mp->storage_dev, lookup_ret.nodeid, 1);
	}

	return ret;
}

static int virtiofs_zfs_readdir(struct fs_dir_t *dirp, struct fs_dirent *entry)
{
	struct virtiofs_dir *dir = dirp->dirp;
	struct fuse_dirent de;

	int read_c = virtiofs_readdir(
		dirp->mp->storage_dev, dir->nodeid, dir->fh, dir->offset,
		(uint8_t *)&de, sizeof(de), (uint8_t *)&entry->name, sizeof(entry->name)
	);

	/* end of dir */
	if (read_c == 0) {
		entry->name[0] = '\0';
		return 0;
	}

	if (read_c < sizeof(de) || de.namelen >= sizeof(entry->name) - 1) {
		return -EIO;
	}

	/*
	 * usually name is already null terminated, but sometimes name of the last entry
	 * in directory is not (maybe also in other cases), so we null terminate it here
	 */
	entry->name[de.namelen] = '\0';

	dir->offset = de.off;

	if (de.type == DT_REG) {
		struct fuse_entry_out lookup_ret;
		int ret = virtiofs_lookup(
			dirp->mp->storage_dev, dir->nodeid, entry->name, &lookup_ret, NULL
		);

		if (ret != 0) {
			return ret;
		}

		virtiofs_forget(dirp->mp->storage_dev, lookup_ret.nodeid, 1);

		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = lookup_ret.attr.size;
	} else if (de.type == DT_DIR) {
		entry->type = FS_DIR_ENTRY_DIR;
		entry->size = 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int virtiofs_zfs_closedir(struct fs_dir_t *dirp)
{
	struct virtiofs_dir *dir = dirp->dirp;
	uint64_t nodeid = dir->nodeid;
	int ret = virtiofs_release(dirp->mp->storage_dev, dir->nodeid, dir->fh, FUSE_DIR);

	if (ret == 0) {
		k_mem_slab_free(&dir_struct_slab, dir);
	} else {
		return ret;
	}

	virtiofs_forget(dirp->mp->storage_dev, nodeid, 1);

	return 0;
}

static int virtiofs_zfs_mount(struct fs_mount_t *mountp)
{
	struct fuse_init_out out;
	int ret = virtiofs_init(mountp->storage_dev, &out);

	if (ret == 0) {
		struct virtiofs_fs_data *fs_data = mountp->fs_data;

		fs_data->max_write = out.max_write;
	}

	return ret;
}

static int virtiofs_zfs_unmount(struct fs_mount_t *mountp)
{
	return virtiofs_destroy(mountp->storage_dev);
}

static int virtiofs_zfs_stat(
	struct fs_mount_t *mountp, const char *fs_path, struct fs_dirent *entry)
{
	const char *path = virtiofs_strip_prefix(fs_path, mountp);
	const char *name = strip_path(fs_path);

	if (strlen(name) + 1 > sizeof(entry->name)) {
		return -ENOBUFS;
	}

	struct fuse_entry_out lookup_ret;
	int ret = virtiofs_lookup(mountp->storage_dev, FUSE_ROOT_INODE, path, &lookup_ret, NULL);

	if (ret != 0) {
		return ret;
	}

	strcpy((char *)&entry->name, name);

	if ((lookup_ret.attr.mode & MODE_FTYPE_MASK) == MODE_FTYPE_DIR) {
		entry->type = FS_DIR_ENTRY_DIR;
		entry->size = 0;
	} else {
		entry->type = FS_DIR_ENTRY_FILE;
		entry->size = lookup_ret.attr.size;
	}

	virtiofs_forget(mountp->storage_dev, lookup_ret.nodeid, 1);

	return 0;
}

static int virtiofs_zfs_unlink(struct fs_mount_t *mountp, const char *name)
{
	const char *path = virtiofs_strip_prefix(name, mountp);
	struct fs_dirent d;
	int ret = virtiofs_zfs_stat(mountp, name, &d);

	if (ret != 0) {
		return ret;
	}

	if (d.type == FS_DIR_ENTRY_FILE) {
#ifdef CONFIG_VIRTIOFS_VIRTIOFSD_UNLINK_QUIRK
		struct fuse_entry_out lookup_ret;
		/*
		 * Even if unlink doesn't take nodeid as a param it still fails with -EIO if the
		 * file wasn't looked up using some virtiofsd versions. It happens at least with
		 * the one from Debian's package (Debian 1:7.2+dfsg-7+deb12u7). Virtiofsd 1.12.0
		 * built from sources doesn't need it
		 */
		ret = virtiofs_lookup(
			mountp->storage_dev, FUSE_ROOT_INODE, path, &lookup_ret, NULL
		);

		if (ret != 0) {
			return ret;
		}
#endif

		ret = virtiofs_unlink(mountp->storage_dev, path, FUSE_FILE);

#ifdef CONFIG_VIRTIOFS_VIRTIOFSD_UNLINK_QUIRK
		virtiofs_forget(mountp->storage_dev, lookup_ret.nodeid, 1);
#endif
	} else {
		ret = virtiofs_unlink(mountp->storage_dev, path, FUSE_DIR);
	}

	return ret;
}

static int virtiofs_zfs_rename(struct fs_mount_t *mountp, const char *from, const char *to)
{
	const char *old_path = virtiofs_strip_prefix(from, mountp);
	const char *new_path = virtiofs_strip_prefix(to, mountp);
	uint64_t old_dir = FUSE_ROOT_INODE;
	uint64_t new_dir = FUSE_ROOT_INODE;
	struct fuse_entry_out old_lookup_ret;
	struct fuse_entry_out new_lookup_ret;
	int ret;

	ret = virtiofs_lookup(
		mountp->storage_dev, FUSE_ROOT_INODE, old_path, &old_lookup_ret, &old_dir
	);

	if (ret != 0) {
		if (old_dir != 0 && old_dir != FUSE_ROOT_INODE) {
			virtiofs_forget(mountp->storage_dev, old_dir, 1);
		}
		return ret;
	}

	ret = virtiofs_lookup(
		mountp->storage_dev, FUSE_ROOT_INODE, new_path, &new_lookup_ret, &new_dir
	);

	/* there is no immediate parent of object's new path */
	if (ret != 0 && new_dir == 0) {
		virtiofs_forget(mountp->storage_dev, old_lookup_ret.nodeid, 1);
		return ret;
	}

	ret = virtiofs_rename(
		mountp->storage_dev,
		old_dir, strip_path(old_path),
		new_dir, strip_path(new_path)
	);

	virtiofs_forget(mountp->storage_dev, old_lookup_ret.nodeid, 1);
	virtiofs_forget(mountp->storage_dev, new_lookup_ret.nodeid, 1);
	virtiofs_forget(mountp->storage_dev, old_dir, 1);
	virtiofs_forget(mountp->storage_dev, new_dir, 1);

	return ret;
}

static int virtiofs_zfs_statvfs(
	struct fs_mount_t *mountp, const char *fs_path, struct fs_statvfs *stat)
{
	struct fuse_kstatfs statfs_out;
	int ret = virtiofs_statfs(mountp->storage_dev, &statfs_out);

	if (ret != 0) {
		return ret;
	}

	stat->f_bsize = statfs_out.bsize;
	stat->f_frsize = statfs_out.frsize;
	stat->f_blocks = statfs_out.blocks;
	stat->f_bfree = statfs_out.bfree;

	return 0;
}

static const struct fs_file_system_t virtiofs_ops = {
	.open = virtiofs_zfs_open,
	.close = virtiofs_zfs_close,
	.read = virtiofs_zfs_read,
	.write = virtio_zfs_write,
	.lseek = virtio_zfs_lseek,
	.tell = virtiofs_zfs_tell,
	.truncate = virtiofs_zfs_truncate,
	.sync = virtiofs_zfs_sync,
	.mkdir = virtiofs_zfs_mkdir,
	.opendir = virtiofs_zfs_opendir,
	.readdir = virtiofs_zfs_readdir,
	.closedir = virtiofs_zfs_closedir,
	.mount = virtiofs_zfs_mount,
	.unmount = virtiofs_zfs_unmount,
	.unlink = virtiofs_zfs_unlink,
	.rename = virtiofs_zfs_rename,
	.stat = virtiofs_zfs_stat,
	.statvfs = virtiofs_zfs_statvfs
};

static int virtiofs_register(void)
{
	return fs_register(FS_VIRTIOFS, &virtiofs_ops);
}

SYS_INIT(virtiofs_register, POST_KERNEL, 99);
