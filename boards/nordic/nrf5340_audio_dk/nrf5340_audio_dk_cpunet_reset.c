/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include <nrf53_cpunet_mgmt.h>
#include <hal/nrf_gpiote.h>

LOG_MODULE_REGISTER(nrf5340_audio_dk_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

#if defined(CONFIG_BT_CTLR_DEBUG_PINS_CPUAPP)
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>
#else
#define DEBUG_SETUP()
#endif

static int core_config(void)
{
	nrf_gpiote_latency_t latency;

	latency = nrf_gpiote_latency_get(NRF_GPIOTE);

	if (latency != NRF_GPIOTE_LATENCY_LOWPOWER) {
		LOG_DBG("Setting gpiote latency to low power");
		nrf_gpiote_latency_set(NRF_GPIOTE, NRF_GPIOTE_LATENCY_LOWPOWER);
	}

	return 0;
}

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
	int ret;


	ret = core_config();
	if (ret) {
		return ret;
	}

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
	nrf53_cpunet_enable(true);

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
