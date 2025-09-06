/*
 * Copyright (c) 2025 Syswonder
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/linker-defs.h>

#define VECTOR_ADDRESS ((uintptr_t)_vector_start)

void relocate_vector_table(void)
{
	write_sctlr(read_sctlr() & ~HIVECS);
	write_vbar(VECTOR_ADDRESS & VBAR_MASK);
	barrier_isync_fence_full();
}
