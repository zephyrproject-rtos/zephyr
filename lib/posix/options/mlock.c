/*
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/posix/sys/mman.h>

int mlock(const void *addr, size_t len)
{
	k_mem_pin(addr, len);

	return 0;
}

int munlock(const void *addr, size_t len)
{
	k_mem_unpin(addr, len);

	return 0;
}
