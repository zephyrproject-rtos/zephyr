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
	/* Artificial delay (~2S) to have a possibility to flash using ICEman
	 * - press and hold both reset buttons (flash and MCU)
	 * - release MCU reset
	 * - release flash reset
	 * - start ICEman
	 */
	for (volatile uint32_t i = 0; i < 8000000; i++) {
		__asm("nop");
	}
	printk("ICEman guard finished\n");

	while (1) {
		printk("cnt: %u\n", cnt);
		cnt++;
		k_msleep(SLEEP_TIME_MS);
	}
	return 0;
}
