/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA.
 * Copyright (c) 2021-2023 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include <hal/nrf_reset.h>

LOG_MODULE_REGISTER(bl5340_dvk_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

#if defined(CONFIG_BT_CTLR_DEBUG_PINS_CPUAPP)
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>
#else
#define DEBUG_SETUP()
#endif

static void remoteproc_mgr_config(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(CONFIG_BUILD_WITH_TFM)
	/* Route Bluetooth Controller Debug Pins */
	DEBUG_SETUP();
#endif /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(CONFIG_BUILD_WITH_TFM) */

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Retain nRF5340 Network MCU in Secure domain (bus
	 * accesses by Network MCU will have Secure attribute set).
	 */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
#endif /* !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) */
}

static int remoteproc_mgr_boot(void)
{

	/* Secure domain may configure permissions for the Network MCU. */
	remoteproc_mgr_config();

#if !defined(CONFIG_TRUSTED_EXECUTION_SECURE)
	/*
	 * Building Zephyr with CONFIG_TRUSTED_EXECUTION_SECURE=y implies
	 * building also a Non-Secure image. The Non-Secure image will, in
	 * this case do the remainder of actions to properly configure and
	 * boot the Network MCU.
	 */

	/* Release the Network MCU, 'Release force off signal' */
	nrf_reset_network_force_off(NRF_RESET, false);

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
