/*
 * Copyright (c) 2023 Panasonic Industrial Devices Europe GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include <nrf53_cpunet_mgmt.h>

#if defined(CONFIG_BOARD_PAN1783_EVB_NRF5340_CPUAPP)
LOG_MODULE_REGISTER(pan1783_evb_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);
#elif defined(CONFIG_BOARD_PAN1783A_EVB_NRF5340_CPUAPP)
LOG_MODULE_REGISTER(pan1783a_evb_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);
#elif defined(CONFIG_BOARD_PAN1783A_PA_EVB_NRF5340_CPUAPP)
LOG_MODULE_REGISTER(pan1783a_pa_evb_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);
#else
#error "No board selected!"
#endif

#if defined(CONFIG_BT_CTLR_DEBUG_PINS_CPUAPP)
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>
#else
#define DEBUG_SETUP()
#endif

static void remoteproc_mgr_config(void)
{
	/* Route Bluetooth Controller Debug Pins */
	DEBUG_SETUP();

	/* Retain nRF5340 Network MCU */
	NRF_SPU->EXTDOMAIN[0].PERM = 1 << 4;
}

static int remoteproc_mgr_boot(void)
{
	/* Configure permissions for the Network MCU. */
	remoteproc_mgr_config();

	/* Release the Network MCU, 'Release force off signal' */
	nrf53_cpunet_enable(true);

	LOG_DBG("Network MCU released.");

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
