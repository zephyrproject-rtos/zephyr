/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util.h>

struct minifs_entry {
	const char *path;
	const char *contents;
};

static struct minifs_entry minifs[] = {
	{"/etc/passwd",
	 "user:x:1000:1000:user:/home/user:/bin/sh\nroot:x:0:0:root:/root:/bin/sh\n"},
	{"/etc/group", "user:x:1000:staff,admin\nroot:x:0:\n"},
};

static ssize_t minifs_read_offs(void *obj, void *buf, size_t sz, size_t offset)
{
	size_t len;

	ARRAY_FOR_EACH_PTR(minifs, entry) {
		if (obj == entry->path) {
			if (offset >= sizeof(entry->contents)) {
				return 0;
			}

			len = CLAMP(strlen(entry->contents) - offset, 0, sz);
			memcpy(buf, entry->contents + offset, len);

			return len;
		}
	}

	errno = EINVAL;
	return -1;
}

static int minifs_close2(void *obj, int fd)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(fd);

	return 0;
}

static int minifs_ioctl(void *obj, unsigned int request, va_list args)
{
	ARG_UNUSED(obj);
	ARG_UNUSED(request);
	ARG_UNUSED(args);

	return 0;
}

static const struct fd_op_vtable minifs_fops = {
	.read_offs = minifs_read_offs,
	.close2 = minifs_close2,
	.ioctl = minifs_ioctl,
};

/* override other zvfs_open() */
int zvfs_open(const char *name, int flags, ...)
{
	int fd;

	ARRAY_FOR_EACH_PTR(minifs, entry) {
		if (strcmp(name, entry->path) == 0) {
			fd = zvfs_reserve_fd();
			if (fd < 0) {
				errno = ENFILE;
				return -1;
			}
			zvfs_finalize_typed_fd(fd, (void *)entry->path, &minifs_fops,
					       ZVFS_MODE_IFREG);
			return fd;
		}
	}

	errno = ENOENT;
	return -1;
}
