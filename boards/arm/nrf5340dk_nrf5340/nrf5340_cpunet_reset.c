/*
 * Copyright (c) 2019-2021 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>
#include <logging/log.h>

#include <soc.h>
#include <soc_secure.h>

LOG_MODULE_REGISTER(nrf5340dk_nrf5340_cpuapp, CONFIG_LOG_DEFAULT_LEVEL);

/* TODO: This should come from DTS, possibly an overlay. */
#define CPUNET_UARTE_PIN_TX 33
#define CPUNET_UARTE_PIN_RX 32
#define CPUNET_UARTE_PIN_RTS 11
#define CPUNET_UARTE_PIN_CTS 10

#if defined(CONFIG_BT_CTLR_DEBUG_PINS_CPUAPP)
#include <../subsys/bluetooth/controller/ll_sw/nordic/hal/nrf5/debug.h>
#else
#define DEBUG_SETUP()
#endif

static void remoteproc_mgr_config(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(CONFIG_BUILD_WITH_TFM)
	/* UARTE */
	/* Assign specific GPIOs that will be used to get UARTE from
	 * nRF5340 Network MCU.
	 */
	soc_secure_gpio_pin_mcu_select(CPUNET_UARTE_PIN_TX, NRF_GPIO_PIN_MCUSEL_NETWORK);
	soc_secure_gpio_pin_mcu_select(CPUNET_UARTE_PIN_RX, NRF_GPIO_PIN_MCUSEL_NETWORK);
	soc_secure_gpio_pin_mcu_select(CPUNET_UARTE_PIN_RTS, NRF_GPIO_PIN_MCUSEL_NETWORK);
	soc_secure_gpio_pin_mcu_select(CPUNET_UARTE_PIN_CTS, NRF_GPIO_PIN_MCUSEL_NETWORK);

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

static int remoteproc_mgr_boot(const struct device *dev)
{
	ARG_UNUSED(dev);

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
	NRF_RESET->NETWORK.FORCEOFF = RESET_NETWORK_FORCEOFF_FORCEOFF_Release;

	LOG_DBG("Network MCU released.");
#endif /* !CONFIG_TRUSTED_EXECUTION_SECURE */

	return 0;
}

SYS_INIT(remoteproc_mgr_boot, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
