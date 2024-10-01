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

static int soc_init(void)
{
	mec5_soc_common_init();
	return 0;
}

/* Enabling HW debug and initializing the MEC interrupt aggregator should be done
 * before driver are loaded to not overwrite driver interrupt configuration.
 * Use early initialization category called soon after Zephyr z_cstart and before
 * Zephyr starts making driver initialization calls.
 */
SYS_INIT(soc_init, EARLY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
