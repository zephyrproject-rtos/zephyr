/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE) && defined(CONFIG_SENSOR)
void board_late_init_hook(void)
{
	/* Initialization chain of Thingy:53 board requires some delays before on board
	 * sensors could be accessed after power up. In particular bme680 and bmm150
	 * sensors require, 2ms and 1ms power on delay respectively. In order not to sum
	 * delays, common delay is introduced in the board start up file. This code is
	 * executed after sensors are powered up and before their initialization.
	 * It's ensured by build asserts at the beginning of this file.
	 */
	k_msleep(2);
}
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE && CONFIG_SENSOR */
