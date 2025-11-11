/*
 * Copyright (c) 2021, Teslabs Engineering S.L.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>

void soc_early_init_hook(void)
{
	SystemInit();
}
