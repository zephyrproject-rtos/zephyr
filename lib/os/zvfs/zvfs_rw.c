/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/internal/fdtable_priv.h>
#include <zephyr/internal/syscall_handler.h>

bool supports_pread_pwrite(uint32_t mode)
{
	switch (mode & ZVFS_MODE_IFMT) {
	case ZVFS_MODE_IFSHM:
		return true;
	default:
		return false;
	}
}

static int zvfs_rw_entry(struct fd_entry *entry, void *buf, size_t sz, bool is_write,
			 const size_t *from_offset)
{
	bool prw;
	int res;
	const size_t *off;

	prw = supports_pread_pwrite(entry->mode);
	if (from_offset != NULL && !prw) {
		errno = ENOTSUP;
		return -1;
	}

	off = (from_offset == NULL) ? &entry->offset : from_offset;

	if (is_write) {
		if (entry->vtable->write_offs == NULL) {
			errno = EIO;
			return -1;
		}

		res = entry->vtable->write_offs(entry->obj, buf, sz, *off);
	} else {
		if (entry->vtable->read_offs == NULL) {
			errno = EIO;
			return -1;
		}

		res = entry->vtable->read_offs(entry->obj, buf, sz, *off);
	}

	if (res > 0 && from_offset == NULL) {
		entry->offset += res;
	}

	return res;
}

int zvfs_rw_unlocked(int fd, void *buf, size_t sz, bool is_write, const size_t *from_offset)
{
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	return zvfs_rw_entry(entry, buf, sz, is_write, from_offset);
}

static ssize_t zvfs_rw(int fd, void *buf, size_t sz, bool is_write, const size_t *from_offset)
{
	ssize_t res;
	struct fd_entry *entry = zvfs_fd_entry_get(fd);

	if (entry == NULL) {
		return -1;
	}

	(void)k_mutex_lock(&entry->lock, K_FOREVER);

	res = zvfs_rw_entry(entry, buf, sz, is_write, from_offset);

	k_mutex_unlock(&entry->lock);

	return res;
}

ssize_t z_impl_zvfs_read_offset(int fd, void *buf, size_t sz, const size_t *from_offset)
{
	return zvfs_rw(fd, buf, sz, false, from_offset);
}

ssize_t z_impl_zvfs_write_offset(int fd, const void *buf, size_t sz, const size_t *from_offset)
{
	return zvfs_rw(fd, (void *)buf, sz, true, from_offset);
}

#ifdef CONFIG_USERSPACE
static ssize_t z_vrfy_zvfs_read_offset(int fd, void *buf, size_t sz, const size_t *from_offset)
{
	size_t off;
	const size_t *off_ptr = NULL;

	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	K_OOPS(K_SYSCALL_MEMORY_WRITE(buf, sz));

	if (from_offset != NULL) {
		K_OOPS(k_usermode_from_copy(&off, (void *)from_offset, sizeof(off)));
		off_ptr = &off;
	}

	return z_impl_zvfs_read_offset(fd, buf, sz, off_ptr);
}

static ssize_t z_vrfy_zvfs_write_offset(int fd, const void *buf, size_t sz,
					const size_t *from_offset)
{
	size_t off;
	const size_t *off_ptr = NULL;

	if (zvfs_fd_syscall_verify(fd) == NULL) {
		return -1;
	}

	K_OOPS(K_SYSCALL_MEMORY_READ(buf, sz));

	if (from_offset != NULL) {
		K_OOPS(k_usermode_from_copy(&off, (void *)from_offset, sizeof(off)));
		off_ptr = &off;
	}

	return z_impl_zvfs_write_offset(fd, buf, sz, off_ptr);
}

#include <zephyr/syscalls/zvfs_read_offset_mrsh.c>
#include <zephyr/syscalls/zvfs_write_offset_mrsh.c>
#endif /* CONFIG_USERSPACE */
