/*
 * Copyright (c) 2023 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#define SLEEP_TIME_MS   1000

int main(void)
{
	uint32_t cnt = 0;

	printk("retention demo start\n");
	/******************************************************************************
	 * Artificial delay (~2S @ 48MHz) to have a possibility to flash using ICEman *
	 * - press and hold both reset buttons (flash and MCU)                        *
	 * - release MCU reset                                                        *
	 * - release flash reset                                                      *
	 * - start ICEman                                                             *
	 ******************************************************************************/
	for (volatile uint32_t i = 0; i < 7410838; i++) {
		__asm("nop");
	}
	printk("ICEman guard finished\n");

	while (1) {
		uint64_t active_time_ms = k_uptime_get();

		printk("cnt: %u, mtimer: %llu\n", cnt++, sys_clock_cycle_get_64());
		k_sleep(K_TIMEOUT_ABS_MS(active_time_ms + SLEEP_TIME_MS));
	}
	return 0;
}
