/*
 * Copyright (c) 2017 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Time Stamp API for ARCv2
 *
 * Provide 64-bit time stamp API
 */

#include <kernel.h>
#include <toolchain.h>
#include <kernel_structs.h>

/*
 * @brief Read 64-bit timestamp value
 *
 * This function returns a 64-bit bit time stamp value that is clocked
 * at the same frequency as the CPU.
 *
 * @return 64-bit time stamp value
 */
u64_t z_tsc_read(void)
{
	unsigned int key;
	u64_t t;
	u32_t count;

	key = irq_lock();
	t = (u64_t)z_tick_get();
	count = z_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
	irq_unlock(key);
	t *= (u64_t)sys_clock_hw_cycles_per_tick();
	t += (u64_t)count;
	return t;
}
