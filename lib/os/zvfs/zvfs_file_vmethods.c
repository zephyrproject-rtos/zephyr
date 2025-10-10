/*
 * Copyright (c) 2025 Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/fdtable.h>
#include <zephyr/fs/fs.h>

int zvfs_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	int rc = 0;
	struct fs_file_t *ptr = obj;

	switch (request) {
	case ZFD_IOCTL_STAT: {
		struct zvfs_stat *buf = va_arg(args, struct zvfs_stat *);
		long offset = fs_tell(ptr);
		long current;

		if (offset < 0) {
			return offset;
		}

		*buf = (struct zvfs_stat){0};

		rc = fs_seek(ptr, 0, FS_SEEK_END);
		if (rc < 0) {
			return rc;
		}

		current = fs_tell(ptr);
		if (current >= 0) {
			buf->size = current;
			buf->mode = ZVFS_MODE_IFREG;
		}

		rc = fs_seek(ptr, offset, FS_SEEK_SET);

		if (current < 0) {
			rc = current;
		}
		break;
	}
	case ZFD_IOCTL_FSYNC: {
		rc = fs_sync(ptr);
		break;
	}
	case ZFD_IOCTL_LSEEK: {
		off_t offset;
		int whence;

		offset = va_arg(args, off_t);
		whence = va_arg(args, int);

		rc = fs_seek(ptr, offset, whence);
		if (rc == 0) {
			rc = fs_tell(ptr);
		}
		break;
	}
	case ZFD_IOCTL_TRUNCATE: {
		off_t length;

		length = va_arg(args, off_t);

		rc = fs_truncate(ptr, length);
		if (rc < 0) {
			errno = -rc;
			return -1;
		}
		break;
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

/*
 * zvfs_open() accepts a name as an argument, so we can be sure it will only operate
 * on files. zvfs_close(), on the other hand, can be used to close any fd, so the
 * underlying object needs to be deallocated here.
 */
extern struct k_mem_slab file_desc_slab;

/**
 * @brief Close and free Zephyr's struct fs_file_t.
 */
int zvfs_close_vmeth(void *obj)
{
	struct fs_file_t *ptr = obj;
	int rc;

	rc = fs_close(ptr);
	k_mem_slab_free(&file_desc_slab, ptr);

	return rc;
}

/**
 * @brief Write to a file.
 *
 * See IEEE 1003.1
 */
ssize_t zvfs_write_vmeth(void *obj, const void *buffer, size_t count)
{
	ssize_t rc;
	struct fs_file_t *ptr = obj;

	rc = fs_write(ptr, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}

/**
 * @brief Read from a file.
 *
 * See IEEE 1003.1
 */
ssize_t zvfs_read_vmeth(void *obj, void *buffer, size_t count)
{
	ssize_t rc;
	struct fs_file_t *ptr = obj;

	rc = fs_read(ptr, buffer, count);
	if (rc < 0) {
		errno = -rc;
		return -1;
	}

	return rc;
}
