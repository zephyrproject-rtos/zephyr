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

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/kernel_structs.h>

/*
 * @brief Read 64-bit timestamp value
 *
 * This function returns a 64-bit bit time stamp value that is clocked
 * at the same frequency as the CPU.
 *
 * @return 64-bit time stamp value
 */
uint64_t z_tsc_read(void)
{
	unsigned int key;
	uint64_t t;
	uint32_t count;

	key = arch_irq_lock();
	t = (uint64_t)sys_clock_tick_get();
	count = z_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
	arch_irq_unlock(key);
	t *= k_ticks_to_cyc_floor64(1);
	t += (uint64_t)count;
	return t;
}
