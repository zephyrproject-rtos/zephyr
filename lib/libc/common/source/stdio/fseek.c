/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/fdtable.h>

int fileptr_to_fd(FILE *stream);

int fseek(FILE *stream, long offset, int whence)
{
	int fd = fileptr_to_fd(stream);

	if (fd < 0) {
		return EOF;
	}
	int z_whence = 0;

	switch (whence) {
	case SEEK_SET:
		z_whence = FS_SEEK_SET;
		break;
	case SEEK_CUR:
		z_whence = FS_SEEK_CUR;
		break;
	case SEEK_END:
		z_whence = FS_SEEK_END;
		break;
	default:
		errno = EINVAL;
		return EOF;
	}
	int rc = zvfs_lseek(fd, offset, z_whence);

	if (rc >= 0) { /* zvfs_lseek also calls fs_tell and returns its result */
		rc = 0;
	}
	return rc;
}
