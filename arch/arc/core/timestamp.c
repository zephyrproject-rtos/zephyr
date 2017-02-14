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

extern int64_t _sys_clock_tick_count;
extern int sys_clock_hw_cycles_per_tick;

/*
 * @brief Read 64-bit timestamp value
 *
 * This function returns a 64-bit bit time stamp value that is clocked
 * at the same frequency as the CPU.
 *
 * @return 64-bit time stamp value
 */
uint64_t _tsc_read(void)
{
	unsigned int key;
	uint64_t t;
	uint32_t count;

	key = irq_lock();
	t = (uint64_t)_sys_clock_tick_count;
	count = _arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT);
	irq_unlock(key);
	t *= (uint64_t)sys_clock_hw_cycles_per_tick;
	t += (uint64_t)count;
	return t;
}
