/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZVFS path open via the Zephyr file system.
 *
 * Wraps Zephyr fs_open() and registers the result in the generic fd table so that
 * ISO C fopen() and POSIX open() can share the same fd-backed I/O path.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>

#if defined(CONFIG_MINIMAL_LIBC)
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#endif

#include <sys/stat.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/zvfs_fs.h>

#define ZVFS_O_ACCMODE (ZVFS_O_RDONLY | ZVFS_O_WRONLY | ZVFS_O_RDWR)

static struct zvfs_fs_desc desc_array[CONFIG_ZVFS_FS_DESC_MAX];

static struct fd_op_vtable fs_fd_op_vtable;

struct zvfs_fs_desc *zvfs_fs_desc_alloc(bool is_dir)
{
	struct zvfs_fs_desc *ptr = NULL;
	unsigned int key = irq_lock();

	for (int i = 0; i < CONFIG_ZVFS_FS_DESC_MAX; i++) {
		if (!desc_array[i].used) {
			ptr = &desc_array[i];
			ptr->used = true;
			ptr->is_dir = is_dir;
			break;
		}
	}

	irq_unlock(key);

	return ptr;
}

void zvfs_fs_desc_free(struct zvfs_fs_desc *ptr)
{
	if (ptr != NULL) {
		ptr->used = false;
	}
}

static int posix_mode_to_zephyr(int mf)
{
	int mode = (mf & ZVFS_O_CREAT) ? FS_O_CREATE : 0;

	mode |= (mf & ZVFS_O_APPEND) ? FS_O_APPEND : 0;
	mode |= (mf & ZVFS_O_TRUNC) ? FS_O_TRUNC : 0;

	switch (mf & ZVFS_O_ACCMODE) {
	case ZVFS_O_RDONLY:
		mode |= FS_O_READ;
		break;
	case ZVFS_O_WRONLY:
		mode |= FS_O_WRITE;
		break;
	case ZVFS_O_RDWR:
		mode |= FS_O_RDWR;
		break;
	default:
		break;
	}

	return mode;
}

int zvfs_open(const char *name, int flags, int mode)
{
	int rc, fd;
	struct zvfs_fs_desc *ptr = NULL;
	int zmode = posix_mode_to_zephyr(flags);

	if (zmode < 0) {
		return zmode;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	ptr = zvfs_fs_desc_alloc(false);
	if (ptr == NULL) {
		rc = -EMFILE;
		goto out_err;
	}

	fs_file_t_init(&ptr->file);

	if (flags & ZVFS_O_CREAT) {
		flags &= ~ZVFS_O_CREAT;

		rc = fs_open(&ptr->file, name, FS_O_CREATE | (zmode & FS_O_RDWR));
		if (rc < 0) {
			goto out_err;
		}
		rc = fs_close(&ptr->file);
		if (rc < 0) {
			goto out_err;
		}
	}

	rc = fs_open(&ptr->file, name, zmode);
	if (rc < 0) {
		goto out_err;
	}

	ptr->flags = flags & (ZVFS_O_ACCMODE | ZVFS_O_APPEND | ZVFS_O_NONBLOCK);
	zvfs_finalize_typed_fd(fd, ptr, &fs_fd_op_vtable, ZVFS_MODE_IFREG);

	return fd;

out_err:
	if (ptr != NULL) {
		zvfs_fs_desc_free(ptr);
	}

	zvfs_free_fd(fd);
	errno = -rc;
	return -1;
}

static int fs_close_vmeth(void *obj)
{
	struct zvfs_fs_desc *ptr = obj;
	int rc;

	rc = fs_close(&ptr->file);
	zvfs_record_lock_drop_obj(ptr);
	zvfs_fs_desc_free(ptr);

	return rc;
}

static int fs_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	int rc = 0;
	struct zvfs_fs_desc *ptr = obj;

	switch (request) {
	case ZFD_IOCTL_STAT: {
		struct zvfs_stat *buf = va_arg(args, struct zvfs_stat *);
		long offset = fs_tell(&ptr->file);
		long current;

		if (offset < 0) {
			return offset;
		}

		*buf = (struct zvfs_stat){0};

		rc = fs_seek(&ptr->file, 0, FS_SEEK_END);
		if (rc < 0) {
			return rc;
		}

		current = fs_tell(&ptr->file);
		if (current >= 0) {
			buf->size = current;
			buf->mode = ptr->is_dir ? ZVFS_MODE_IFDIR : ZVFS_MODE_IFREG;
		}

		rc = fs_seek(&ptr->file, offset, FS_SEEK_SET);

		if (current < 0) {
			rc = current;
		}
		break;
	}
	case ZFD_IOCTL_FSYNC: {
		rc = fs_sync(&ptr->file);
		break;
	}
	case ZFD_IOCTL_LSEEK: {
		off_t offset;
		int whence;
		size_t cur_offset;

		offset = va_arg(args, off_t);
		whence = va_arg(args, int);
		cur_offset = va_arg(args, size_t);

		/* reads and writes go through the fd offset, not the file position */
		if (whence == FS_SEEK_CUR) {
			offset += (off_t)cur_offset;
			whence = FS_SEEK_SET;
		}

		rc = fs_seek(&ptr->file, offset, whence);
		if (rc == 0) {
			rc = fs_tell(&ptr->file);
		}
		break;
	}
	case ZFD_IOCTL_TRUNCATE: {
		off_t length;

		length = va_arg(args, off_t);

		rc = fs_truncate(&ptr->file, length);
		if (rc < 0) {
			errno = -rc;
			return -1;
		}
		break;
	}
	case ZVFS_F_GETFL:
		return ptr->flags;
	case ZVFS_F_SETFL: {
		int set = va_arg(args, int);

		/* the file system honors neither append nor non-blocking mode */
		ptr->flags = (ptr->flags & ZVFS_O_ACCMODE) |
			     (set & (ZVFS_O_APPEND | ZVFS_O_NONBLOCK));
		return 0;
	}
	default:
		errno = EOPNOTSUPP;
		return -1;
	}

	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

static ssize_t fs_write_vmeth(void *obj, const void *buffer, size_t count)
{
	ssize_t rc;
	struct zvfs_fs_desc *ptr = obj;

	rc = fs_write(&ptr->file, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

static ssize_t fs_write_offs_vmeth(void *obj, const void *buffer, size_t count,
				   size_t offset)
{
	struct zvfs_fs_desc *ptr = obj;
	ssize_t rc;
	long saved;

	saved = fs_tell(&ptr->file);
	if (saved < 0) {
		errno = -saved;
		return -1;
	}

	rc = fs_seek(&ptr->file, (off_t)offset, FS_SEEK_SET);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	rc = fs_write_vmeth(obj, buffer, count);
	if (rc < 0) {
		return rc;
	}

	if (fs_seek(&ptr->file, saved, FS_SEEK_SET) < 0) {
		errno = EIO;
		return -1;
	}

	return rc;
}

static ssize_t fs_read_vmeth(void *obj, void *buffer, size_t count)
{
	ssize_t rc;
	struct zvfs_fs_desc *ptr = obj;

	rc = fs_read(&ptr->file, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

static ssize_t fs_read_offs_vmeth(void *obj, void *buffer, size_t count, size_t offset)
{
	struct zvfs_fs_desc *ptr = obj;
	ssize_t rc;
	long saved;

	saved = fs_tell(&ptr->file);
	if (saved < 0) {
		errno = -saved;
		return -1;
	}

	rc = fs_seek(&ptr->file, (off_t)offset, FS_SEEK_SET);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	rc = fs_read_vmeth(obj, buffer, count);
	if (rc < 0) {
		return rc;
	}

	if (fs_seek(&ptr->file, saved, FS_SEEK_SET) < 0) {
		errno = EIO;
		return -1;
	}

	return rc;
}

static struct fd_op_vtable fs_fd_op_vtable = {
	.read = fs_read_vmeth,
	.read_offs = fs_read_offs_vmeth,
	.write = fs_write_vmeth,
	.write_offs = fs_write_offs_vmeth,
	.close = fs_close_vmeth,
	.ioctl = fs_ioctl_vmeth,
};
