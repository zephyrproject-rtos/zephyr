/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <nrf53_cpunet_mgmt.h>

LOG_MODULE_REGISTER(thingy53_board_init);

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

static void enable_cpunet(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE */

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/*
	 * Building Zephyr with CONFIG_TRUSTED_EXECUTION_SECURE=y implies
	 * building also a Non-Secure image. The Non-Secure image will, in
	 * this case do the remainder of actions to properly configure and
	 * boot the Network MCU.
	 */

	/* Release the Network MCU, 'Release force off signal' */
	nrf53_cpunet_enable(true);

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */
}

static int setup(void)
{

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	if (IS_ENABLED(CONFIG_SENSOR)) {
		/* Initialization chain of Thingy:53 board requires some delays before on board
		 * sensors could be accessed after power up. In particular bme680 and bmm150
		 * sensors require, 2ms and 1ms power on delay respectively. In order not to sum
		 * delays, common delay is introduced in the board start up file. This code is
		 * executed after sensors are powered up and before their initialization.
		 * It's ensured by build asserts at the beginning of this file.
		 */
		k_msleep(2);
	}

#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	if (IS_ENABLED(CONFIG_BOARD_ENABLE_CPUNET)) {
		enable_cpunet();
	}

	return 0;
}

SYS_INIT(setup, POST_KERNEL, CONFIG_THINGY53_INIT_PRIORITY);
