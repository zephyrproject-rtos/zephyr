/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include "bus_force.h"

#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

static const struct gpio_dt_spec sda = GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, sda_gpios, {0});
static const struct gpio_dt_spec scl = GPIO_DT_SPEC_GET_OR(ZEPHYR_USER_NODE, scl_gpios, {0});

BUILD_ASSERT(!IS_ENABLED(CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED) ||
		     (DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, sda_gpios) &&
		      DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, scl_gpios)),
	     "CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED needs zephyr,user sda-gpios and scl-gpios");

/*
 * A pin is an output only while it holds a line low, so the forcing GPIOs
 * leave the bus alone at rest and a test that aborts cannot leave it held.
 */
int bus_force_init(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED)) {
		return 0;
	}

	if (!gpio_is_ready_dt(&sda) || !gpio_is_ready_dt(&scl)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&sda, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_configure_dt(&scl, GPIO_INPUT);
} /* bus_force_init() */

int bus_force_low(uint32_t lines)
{
	int ret;

	if (!IS_ENABLED(CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED)) {
		return -ENODEV;
	}

	if ((lines == 0U) || ((lines & ~(BUS_FORCE_SDA | BUS_FORCE_SCL)) != 0U)) {
		return -EINVAL;
	}

	if ((lines & BUS_FORCE_SDA) != 0U) {
		ret = gpio_pin_configure_dt(&sda, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if ((lines & BUS_FORCE_SCL) != 0U) {
		ret = gpio_pin_configure_dt(&scl, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
} /* bus_force_low() */

int bus_force_release(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&sda, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	return gpio_pin_configure_dt(&scl, GPIO_INPUT);
} /* bus_force_release() */

/* Read @p pin as an input, and add @p bit to @p high when the line is high. */
static int read_line(const struct gpio_dt_spec *pin, uint32_t bit, uint32_t *high)
{
	int ret;

	/* A pin driving the line low would read back its own drive, not the bus. */
	ret = gpio_pin_configure_dt(pin, GPIO_INPUT);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_get_dt(pin);
	if (ret < 0) {
		return ret;
	}

	if (ret == 1) {
		*high |= bit;
	}

	return 0;
} /* read_line() */

int bus_force_level(uint32_t lines)
{
	uint32_t high = 0U;
	int ret;

	if (!IS_ENABLED(CONFIG_I2C_FAULT_TEST_BUS_FAULT_WIRED)) {
		return -ENODEV;
	}

	if ((lines == 0U) || ((lines & ~(BUS_FORCE_SDA | BUS_FORCE_SCL)) != 0U)) {
		return -EINVAL;
	}

	if ((lines & BUS_FORCE_SDA) != 0U) {
		ret = read_line(&sda, BUS_FORCE_SDA, &high);
		if (ret < 0) {
			return ret;
		}
	}

	if ((lines & BUS_FORCE_SCL) != 0U) {
		ret = read_line(&scl, BUS_FORCE_SCL, &high);
		if (ret < 0) {
			return ret;
		}
	}

	return (int)high;
} /* bus_force_level() */
