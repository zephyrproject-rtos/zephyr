/*
 * Copyright (c) 2017 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/printk.h>

#include "targets.h"

const uint32_t rodata_var = RODATA_VALUE;

uint8_t data_buf[BUF_SIZE] __aligned(sizeof(int));

int overwrite_target(int i)
{
	printk("text not modified\n");
	return (i - 1);
}
