/*
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/sys_clock.h>

int sensor_clock_get_cycles(uint64_t *cycles)
{
	if (cycles == NULL) {
		return -EINVAL;
	}

#ifdef CONFIG_TIMER_HAS_64BIT_CYCLE_COUNTER
	*cycles = k_cycle_get_64();
#else
	*cycles = (uint64_t)k_cycle_get_32();
#endif

	return 0;
}

uint64_t sensor_clock_cycles_to_ns(uint64_t cycles)
{
	return (cycles * NSEC_PER_SEC) / sys_clock_hw_cycles_per_sec();
}
