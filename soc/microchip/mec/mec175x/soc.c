/*
 * Copyright (c) 2024 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <soc_cmn_init.h>

void soc_prep_hook(void)
{
	mec5_soc_common_init();
}
