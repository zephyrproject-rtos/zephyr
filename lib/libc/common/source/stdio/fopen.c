/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <zephyr/sys/zvfs_fd_file.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/util_macro.h>

static struct fd_op_vtable libc_op_vtable = {
	.read = zvfs_read_vmeth,
	.write = zvfs_write_vmeth,
	.close = zvfs_close_vmeth,
	.ioctl = zvfs_ioctl_vmeth,
};

FILE *fopen(const char *ZRESTRICT filename, const char *ZRESTRICT mode)
{
	if (!IS_ENABLED(CONFIG_FILE_SYSTEM)) {
		errno = ENOTSUP;
		return NULL;
	}

	int rwflag = 0;
	int openflag = 0;
	bool exclusive = false;

	while (*mode != '\0') {
		switch (*mode) {
		case 'a':
			rwflag = FS_O_WRITE;
			openflag = FS_O_CREATE | FS_O_APPEND;
			break;
		case 'r':
			rwflag = FS_O_READ;
			break;
		case 'w':
			rwflag = FS_O_WRITE;
			openflag = FS_O_CREATE | FS_O_TRUNC;
			break;
		case 'x':
			exclusive = true;
			break;
		case '+':
			rwflag = FS_O_RDWR;
			break;
		default:
			break;
		}
		mode++;
	}
	/* exclusive mode: opening without FS_O_CREATE should fail */
	if (exclusive) {
		openflag &= ~FS_O_CREATE;
	}
	int rc = zvfs_open(filename, rwflag | openflag, &libc_op_vtable);

	if (rc < 0) {
		if (!exclusive) {
			/* file doesn't exist or couldn't create; fail */
			return NULL;
		}
		openflag |= FS_O_CREATE;
		rc = zvfs_open(filename, rwflag | openflag, &libc_op_vtable);
		if (rc >= 0) {
			/* file didn't exist; creating it was successful */
			return zvfs_fdopen(rc, NULL);
		}
	} else if (exclusive) {
		/* file already exists; fail */
		zvfs_close(rc);
		errno = EEXIST;
		return NULL;
	}
	return zvfs_fdopen(rc, NULL);
}
