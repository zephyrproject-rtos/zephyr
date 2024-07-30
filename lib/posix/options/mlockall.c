/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include <zephyr/posix/sys/mman.h>

int mlockall(int flags)
{
	ARG_UNUSED(flags);

	errno = ENOSYS;
	return -1;
}

int munlockall(void)
{
	errno = ENOSYS;
	return -1;
}
