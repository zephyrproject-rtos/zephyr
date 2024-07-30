/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int main(void)
{
	printk("Start SMBUS shell sample %s\n", CONFIG_BOARD);
	return 0;
}
