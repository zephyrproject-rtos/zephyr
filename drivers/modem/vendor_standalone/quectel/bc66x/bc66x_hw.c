/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "bc66x.h"
#include "bc66x_hw.h"

LOG_MODULE_REGISTER(bc66x_hw, CONFIG_MODEM_QUECTEL_BC66X_LOG_LEVEL);

/**
 * Initialize GPIO pins for BC660K / BC66.
 * Configures the BOOT, RST, and PSM pins as outputs (inactive).
 */
int bc66x_hw_init(const struct bc66x_config *dev_cfg)
{
	/* Check if all required GPIO pins are ready */
	if (!gpio_is_ready_dt(&dev_cfg->boot_gpio) || !gpio_is_ready_dt(&dev_cfg->rst_gpio) ||
	    !gpio_is_ready_dt(&dev_cfg->psm_gpio)) {
		LOG_ERR("GPIO pins not ready");
		return -ENODEV;
	}

	int ret;

	ret = gpio_pin_configure_dt(&dev_cfg->boot_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure BOOT pin");
		return ret;
	}

	ret = gpio_pin_configure_dt(&dev_cfg->rst_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure RST pin");
		return ret;
	}

	ret = gpio_pin_configure_dt(&dev_cfg->psm_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure PSM pin");
		return ret;
	}

	LOG_DBG("GPIO pins initialized successfully");
	return 0;
}

/**
 * Boot BC660K module with proper power-on sequence.
 * Performs the BC660K module startup sequence with mandatory timing requirements.
 * The sequence involves controlling BOOT, RST, and PSM pins in a specific order.
 */
int bc660k_hw_start(const struct bc66x_config *dev_cfg)
{
	LOG_DBG("Starting module...");

	int ret;

	/* Execute startup sequence with mandatory timings */
	ret = gpio_pin_set_dt(&dev_cfg->boot_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Cannot set boot pin");
		return ret;
	}

	ret = gpio_pin_set_dt(&dev_cfg->rst_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Cannot set reset pin");
		return ret;
	}

	ret = gpio_pin_set_dt(&dev_cfg->psm_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Cannot set psm pin");
		return ret;
	}

	k_msleep(500); /* Hold reset for 500ms */

	ret = gpio_pin_set_dt(&dev_cfg->rst_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Cannot set reset pin");
		return ret;
	}

	k_msleep(600); /* Wait for module to boot */

	LOG_DBG("Module started successfully");
	return 0;
}

/**
 * Boot BC66 module with proper power-on sequence.
 * Performs the BC66 module startup sequence with mandatory timing requirements.
 * The sequence involves controlling BOOT pin.
 */
int bc66_hw_start(const struct bc66x_config *dev_cfg)
{
	LOG_DBG("Starting module...");

	int ret;

	/* Execute startup sequence with mandatory timings */
	ret = gpio_pin_set_dt(&dev_cfg->boot_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Cannot set boot pin");
		return ret;
	}

	k_msleep(550); /* Hold boot for >500ms */
	ret = gpio_pin_set_dt(&dev_cfg->boot_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Cannot set boot pin");
		return ret;
	}

	k_msleep(600); /* Wait for module to boot (>= 500ms) */

	LOG_DBG("Module started successfully");
	return 0;
}

/**
 * Pulses the modem PSM pin high, waits, then brings it Low.
 */
int bc66x_hw_psm_pulse_ms(const struct bc66x_config *dev_cfg, int wait_ms)
{
	LOG_DBG("Pulsing PSM Pin");

	int ret;

	ret = gpio_pin_set_dt(&dev_cfg->psm_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Cannot set psm pin");
		return ret;
	}

	k_msleep(wait_ms);

	ret = gpio_pin_set_dt(&dev_cfg->psm_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Cannot set psm pin");
		return ret;
	}

	LOG_DBG("PSM Pin pulsed (%d ms)", wait_ms);

	return 0;
}
