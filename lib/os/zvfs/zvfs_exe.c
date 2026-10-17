/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/zvfs.h>
#include <zephyr/sys/zvfs_fs.h>

/* the opened candidate stays open only when it is an executable regular file */
static int exe_candidate(int fd, bool *found)
{
	struct zvfs_stat st;

	if (zvfs_fstat(fd, &st) < 0) {
		int err = errno;

		(void)zvfs_close(fd);
		return -err;
	}
	if (((st.mode & ZVFS_MODE_IFMT) == ZVFS_MODE_IFREG) &&
	    ((st.mode & (ZVFS_S_IXUSR | ZVFS_S_IXGRP | ZVFS_S_IXOTH)) != 0)) {
		return fd;
	}
	(void)zvfs_close(fd);
	*found = true;

	return -ENOENT;
}

int zvfs_exe_resolve(const char *paths, const char *exe, char *buf, size_t buf_size)
{
	bool found = false;
	const char *seg = paths;
	struct zvfs_stat st;

	if ((paths == NULL) || (exe == NULL) || (exe[0] == '\0') || (strchr(exe, '/') != NULL) ||
	    (buf == NULL) || (buf_size == 0)) {
		return -EINVAL;
	}

	while (true) {
		const char *end = strchr(seg, ':');
		size_t len;
		int fd;
		int n;

		if (end == NULL) {
			end = seg + strlen(seg);
		}
		len = (size_t)(end - seg);

		/* an empty prefix is the working directory: the bare name, relative to it */
		n = (len == 0) ? snprintf(buf, buf_size, "%s", exe)
			       : snprintf(buf, buf_size, "%.*s/%s", (int)len, seg, exe);
		if ((n < 0) || ((size_t)n >= buf_size)) {
			return -ERANGE;
		}
		fd = zvfs_open(buf, ZVFS_O_RDONLY, 0);

		if (fd >= 0) {
			fd = exe_candidate(fd, &found);
			if (fd != -ENOENT) {
				return fd;
			}
		} else {
			switch (errno) {
			case ENOENT:
			case ENOTDIR:
				/* a directory of that name does not open as a file; it still counts */
				if (zvfs_stat(buf, &st) == 0) {
					found = true;
				}
				break;
			case EACCES:
			case EISDIR:
				found = true;
				break;
			default:
				return -errno;
			}
		}

		if (*end == '\0') {
			break;
		}
		seg = end + 1;
	}

	return found ? -EPERM : -ENOENT;
}
