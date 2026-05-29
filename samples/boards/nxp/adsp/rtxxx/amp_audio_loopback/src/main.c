/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "dsp.h"

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);

	printk("[ARM] Starting DSP...\n");
	dsp_start();

	return 0;
}
