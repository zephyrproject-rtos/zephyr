/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>

/* Initialization chain of Thingy:53 board requires some delays before on board sensors
 * could be accessed after power up. In particular bme680 and bmm150 sensors require,
 * respectively 2ms and 1ms power on delay. In order to avoid delays sum, common delay is
 * introduced in the board start up file. Below asserts ensure correct initialization order:
 * on board regulators, board init (this), sensors init.
 */
#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
#if defined(CONFIG_REGULATOR_FIXED)
BUILD_ASSERT(CONFIG_THINGY53_INIT_PRIORITY > CONFIG_REGULATOR_FIXED_INIT_PRIORITY,
	"CONFIG_THINGY53_INIT_PRIORITY must be higher than CONFIG_REGULATOR_FIXED_INIT_PRIORITY");
#endif /* CONFIG_REGULATOR_FIXED */
#if defined(CONFIG_IEEE802154_NRF5)
BUILD_ASSERT(CONFIG_THINGY53_INIT_PRIORITY < CONFIG_IEEE802154_NRF5_INIT_PRIO,
	"CONFIG_THINGY53_INIT_PRIORITY must be less than CONFIG_IEEE802154_NRF5_INIT_PRIO");
#endif /* CONFIG_IEEE802154_NRF5 */
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

#if defined(CONFIG_SENSOR)
BUILD_ASSERT(CONFIG_THINGY53_INIT_PRIORITY < CONFIG_SENSOR_INIT_PRIORITY,
	"CONFIG_THINGY53_INIT_PRIORITY must be less than CONFIG_SENSOR_INIT_PRIORITY");
#endif

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE) && defined(CONFIG_SENSOR)
static int setup(void)
{
	/* Initialization chain of Thingy:53 board requires some delays before on board
	 * sensors could be accessed after power up. In particular bme680 and bmm150
	 * sensors require, 2ms and 1ms power on delay respectively. In order not to sum
	 * delays, common delay is introduced in the board start up file. This code is
	 * executed after sensors are powered up and before their initialization.
	 * It's ensured by build asserts at the beginning of this file.
	 */
	k_msleep(2);

	return 0;
}

SYS_INIT(setup, POST_KERNEL, CONFIG_THINGY53_INIT_PRIORITY);
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE && CONFIG_SENSOR */
