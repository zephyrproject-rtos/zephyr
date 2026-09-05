/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/regulator.h>
#include <hal/nrf_regulators.h>
#include <soc.h>

#define DRIVER_NODE DT_NODELABEL(regulators)
#define DRIVER_REG_ADDR DT_REG_ADDR(DRIVER_NODE)
#define DRIVER_HIBERNATE_MIN_UPTIME_MS CONFIG_REGULATOR_NRF54L_HIBERNATE_MIN_UPTIME_MS

#if NRF_REGULATORS_HAS_HIBERNATOR
static NRF_REGULATORS_Type *driver_regs = (NRF_REGULATORS_Type *)DRIVER_REG_ADDR;
#endif

#if NRF_REGULATORS_HAS_HIBERNATOR
static int driver_api_ship_mode(const struct device *dev)
{
	/*
	 * Prevent immediately entering ship mode on boot which
	 * could deadlock the device as a debugger is not able to
	 * attach after ship mode is entered.
	 */
	if (k_uptime_get() < DRIVER_HIBERNATE_MIN_UPTIME_MS) {
		return -EPERM;
	}

	nrf_regulators_system_hibernate(driver_regs);

	/* Spin waiting for system to enter hibernate */
	while (1) {
	}

	return 0;
}
#endif

static DEVICE_API(regulator_parent, driver_api) = {
#if NRF_REGULATORS_HAS_HIBERNATOR
	.ship_mode = driver_api_ship_mode,
#endif
};

static int driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if NRF_REGULATORS_HAS_HIBERNATOR
	/*
	 * Unconditionally release GPIO retention on wakeup.
	 *
	 * According to PS, all GPIOs should be configured before
	 * releasing GPIO retention. This however is not supported
	 * by zephyr, where drivers expect GPIOs to be in an unknown
	 * and mutable state upon init. So we release the pins as
	 * early as possible.
	 */
	nrf_regulators_gpio_retention_release_set(driver_regs, true);
#endif

	return 0;
}

DEVICE_DT_DEFINE(
	DT_NODELABEL(regulators),
	driver_init,
	NULL,
	NULL,
	NULL,
	PRE_KERNEL_1,
	0,
	&driver_api
);
