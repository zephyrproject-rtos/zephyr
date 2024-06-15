/*
 * Copyright (c) 2024 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/unistd.h>

/* prototypes for external, not-yet-public, functions in fdtable.c */
int zvfs_fsync(int fd);

int fsync(int fd)
{
	return zvfs_fsync(fd);
}
#ifdef CONFIG_POSIX_FILE_SYSTEM_ALIAS_FSYNC
FUNC_ALIAS(fsync, _fsync, int);
#endif
