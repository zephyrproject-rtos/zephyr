/*
 * Copyright (c) 2024, Maxjta
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	SystemInit();
}

