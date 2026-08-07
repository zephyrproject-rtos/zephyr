/*
 * Copyright (c) Copyright (c) 2021 BrainCo Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	SystemInit();
}
