/*
 * Copyright 2019-2026, NXP
 * Copyright (c) 2022 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "i2c-priv.h"
#include "i2c_bitbang.h"
#include "i2c_mcux_lpi2c_common.h"

LOG_MODULE_REGISTER(mcux_lpi2c_common, CONFIG_I2C_LOG_LEVEL);

struct mcux_lpi2c_recovery_io {
	const struct gpio_dt_spec *scl;
	const struct gpio_dt_spec *sda;
	int error;
};

static void mcux_lpi2c_bitbang_set_scl(void *context, int state)
{
	struct mcux_lpi2c_recovery_io *io = context;
	int ret = gpio_pin_set_dt(io->scl, state);

	if (ret < 0 && io->error == 0) {
		io->error = ret;
	}
}

static void mcux_lpi2c_bitbang_set_sda(void *context, int state)
{
	struct mcux_lpi2c_recovery_io *io = context;
	int ret = gpio_pin_set_dt(io->sda, state);

	if (ret < 0 && io->error == 0) {
		io->error = ret;
	}
}

static int mcux_lpi2c_bitbang_get_sda(void *context)
{
	struct mcux_lpi2c_recovery_io *io = context;
	int ret = gpio_pin_get_dt(io->sda);

	if (ret < 0) {
		io->error = ret;
		return 1;
	}

	return ret;
}

#ifdef CONFIG_I2C_GPIO_CLOCK_STRETCHING
static int mcux_lpi2c_bitbang_get_scl(void *context)
{
	struct mcux_lpi2c_recovery_io *io = context;
	int ret = gpio_pin_get_dt(io->scl);

	if (ret < 0) {
		io->error = ret;
		return 1;
	}

	return ret;
}
#endif

int mcux_lpi2c_recover_bus_common(const struct gpio_dt_spec *scl,
				const struct gpio_dt_spec *sda,
				const struct pinctrl_dev_config *pincfg, uint32_t bitrate)
{
	struct mcux_lpi2c_recovery_io io = {.scl = scl, .sda = sda};
	const struct i2c_bitbang_io bitbang_io = {
		.set_scl = mcux_lpi2c_bitbang_set_scl,
		.set_sda = mcux_lpi2c_bitbang_set_sda,
		.get_sda = mcux_lpi2c_bitbang_get_sda,
#ifdef CONFIG_I2C_GPIO_CLOCK_STRETCHING
		.get_scl = mcux_lpi2c_bitbang_get_scl,
#endif
	};
	struct i2c_bitbang bitbang;
	int ret;
	int restore_ret;

	if (!gpio_is_ready_dt(scl) || !gpio_is_ready_dt(sda)) {
		LOG_ERR("Bus recovery GPIOs not ready");
		return -EIO;
	}

	ret = gpio_pin_configure_dt(scl, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		LOG_ERR("Failed to configure SCL GPIO: %d", ret);
		goto restore;
	}

	ret = gpio_pin_configure_dt(sda, GPIO_OUTPUT_HIGH);
	if (ret != 0) {
		LOG_ERR("Failed to configure SDA GPIO: %d", ret);
		goto restore;
	}

	i2c_bitbang_init(&bitbang, &bitbang_io, &io);
	ret = i2c_bitbang_configure(&bitbang, i2c_map_dt_bitrate(bitrate) | I2C_MODE_CONTROLLER);
	if (ret != 0) {
		LOG_ERR("Failed to configure I2C bitbang: %d", ret);
		goto restore;
	}

	ret = i2c_bitbang_recover_bus(&bitbang);
	if (io.error != 0) {
		ret = io.error;
	}
	if (ret != 0) {
		LOG_ERR("Bus recovery failed: %d", ret);
	}

restore:
	restore_ret = pinctrl_apply_state(pincfg, PINCTRL_STATE_DEFAULT);
	if (restore_ret != 0) {
		LOG_ERR("Failed to restore I2C pins: %d", restore_ret);
		if (ret == 0) {
			ret = restore_ret;
		}
	}

	return ret;
}
