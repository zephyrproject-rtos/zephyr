/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <sys/types.h>

#include <zephyr/posix/sys/mman.h>

int mprotect(void *addr, size_t len, int prot)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(len);
	ARG_UNUSED(prot);

	errno = ENOSYS;
	return -1;
}
