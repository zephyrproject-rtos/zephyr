/*
 * Copyright (c) 2025 Jocelyn Masserot <jocemass@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/digipot.h>
#include <zephyr/logging/log.h>


#define AD528X_RDAC_SELECT(x)	FIELD_PREP(BIT(7), x)
#define AD528X_RDAC_RESET		BIT(6)
#define AD528X_RDAC_SHUTDN		BIT(5)
#define AD528X_RDAC_POS_NB		256

struct ad528x_config {
	struct i2c_dt_spec bus;
	uint8_t rdac_nb;
};

static int ad528x_write(const struct device *dev, uint8_t command, uint8_t value)
{
	const struct ad528x_config *config = dev->config;
	uint8_t tx_data[2];

	tx_data[0] = command;
	tx_data[1] = value;

	return i2c_write_dt(&config->bus, tx_data, sizeof(tx_data));
}

static int ad528x_read(const struct device *dev, uint8_t command, uint16_t *value)
{
	const struct ad528x_config *config = dev->config;
	uint8_t tx_data = command;
	uint8_t rx_data = 0;
	int ret;

	ret = i2c_write_dt(&config->bus, &tx_data, sizeof(tx_data));
	if (ret != 0) {
		return ret;
	}

	ret = i2c_read_dt(&config->bus, &rx_data, sizeof(rx_data));
	if (ret != 0) {
		return ret;
	}

	*value = (uint16_t)rx_data;

	return ret;
}

static int ad528x_wiper_set(const struct device *dev, uint8_t channel, uint16_t position)
{
	const struct ad528x_config *config = dev->config;
	uint8_t cmd = 0;

	if (channel > (config->rdac_nb-1)) {
		return -EINVAL;
	}

	if (position >= AD528X_RDAC_POS_NB) {
		return -EINVAL;
	}

	cmd = AD528X_RDAC_SELECT(channel);

	return ad528x_write(dev, cmd, position);
}

static int ad528x_wiper_get(const struct device *dev, uint8_t channel, uint16_t *position)
{
	const struct ad528x_config *config = dev->config;
	uint8_t cmd = 0;

	if (channel > (config->rdac_nb-1)) {
		return -EINVAL;
	}

	cmd = AD528X_RDAC_SELECT(channel);

	return ad528x_read(dev, cmd, position);
}

static int ad528x_wiper_reset(const struct device *dev, uint8_t channel)
{
	const struct ad528x_config *config = dev->config;
	uint8_t cmd = 0;

	if (channel > (config->rdac_nb-1)) {
		return -EINVAL;
	}

	cmd = AD528X_RDAC_SELECT(channel);
	cmd |= AD528X_RDAC_RESET;

	return ad528x_write(dev, cmd, 0);
}

static int ad528x_init(const struct device *dev)
{
	const struct ad528x_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(digipot, ad528x_driver_api) = {
	.set_position = ad528x_wiper_set,
	.get_position = ad528x_wiper_get,
	.reset_position = ad528x_wiper_reset,
};

/* Helper that instantiates a driver for AD5280 (1 RDAC) */
#define INST_AD5280(index)												\
	static const struct ad528x_config config_ad5280_##index = {			\
		.bus = I2C_DT_SPEC_INST_GET(index),								\
		.rdac_nb = 1,													\
	};																	\
	DEVICE_DT_INST_DEFINE(index, ad528x_init, NULL, NULL, 				\
							&config_ad5280_##index,						\
							POST_KERNEL, CONFIG_DIGIPOT_INIT_PRIORITY,	\
							&ad528x_driver_api);

/* Helper that instantiates a driver for AD5282 (2 RDACs) */
#define INST_AD5282(index)												\
	static const struct ad528x_config config_ad5282_##index = {			\
		.bus = I2C_DT_SPEC_INST_GET(index),								\
		.rdac_nb = 2,													\
	};																	\
	DEVICE_DT_INST_DEFINE(index, ad528x_init, NULL, NULL,				\
							&config_ad5282_##index,						\
							POST_KERNEL, CONFIG_DIGIPOT_INIT_PRIORITY,	\
							&ad528x_driver_api);

/* Instantiate for AD5280 compatibles (if any) */
#define DT_DRV_COMPAT adi_ad5280
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY(INST_AD5280)
#endif
#undef DT_DRV_COMPAT

/* Instantiate for AD5282 compatibles (if any) */
#define DT_DRV_COMPAT adi_ad5282
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
DT_INST_FOREACH_STATUS_OKAY(INST_AD5282)
#endif
#undef DT_DRV_COMPAT