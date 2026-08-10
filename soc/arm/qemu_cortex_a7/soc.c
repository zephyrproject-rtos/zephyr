/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <cmsis_core.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/linker/linker-defs.h>

void relocate_vector_table(void)
{
	uint32_t vector_address = (uint32_t)_vector_start;

	write_sctlr(read_sctlr() & ~HIVECS);
	write_vbar(vector_address & VBAR_MASK);
	barrier_isync_fence_full();
}
