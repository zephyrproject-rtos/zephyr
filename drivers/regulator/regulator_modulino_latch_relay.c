/*
 * Copyright (c) 2026 Silicon Labs
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arduino_modulino_latch_relay

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(arduino_modulino_latch_relay, CONFIG_REGULATOR_LOG_LEVEL);

struct regulator_modulino_latch_relay_config {
	struct regulator_common_config common;
	struct i2c_dt_spec bus;
	uint8_t enable_mask;
};

struct regulator_modulino_latch_relay_data {
	struct regulator_common_data common;
};

static int regulator_modulino_latch_relay_set_enable(const struct device *dev, bool enable)
{
	int ret;
	const struct regulator_modulino_latch_relay_config *config = dev->config;
	uint8_t relay_state = enable ? 0x01 : 0x00;
	uint8_t command_out[3] = {relay_state, 0x00, 0x00};

	ret = i2c_write_dt(&config->bus, command_out, sizeof(command_out));
	if (ret < 0) {
		LOG_ERR("Failed to set relay state: %d", ret);
		return ret;
	}
	LOG_DBG("Relay state set to: %s", enable ? "ON" : "OFF");
	return 0;
}

static int regulator_modulino_latch_relay_enable(const struct device *dev)
{
	return regulator_modulino_latch_relay_set_enable(dev, true);
}

static int regulator_modulino_latch_relay_disable(const struct device *dev)
{
	return regulator_modulino_latch_relay_set_enable(dev, false);
}

static int regulator_modulino_latch_relay_init(const struct device *dev)
{
	const struct regulator_modulino_latch_relay_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->bus)) {
		LOG_ERR("I2C bus %s is not ready", config->bus.bus->name);
		return -ENODEV;
	}
	if (regulator_modulino_latch_relay_disable(dev) < 0) {
		LOG_ERR("Failed to initialize relay state");
		return -EIO;
	}
	LOG_DBG("Modulino Latch Relay initialized successfully");
	regulator_common_data_init(dev);
	return regulator_common_init(dev, false);
}

static DEVICE_API(regulator, modulino_latch_relay_api) = {
	.enable = regulator_modulino_latch_relay_enable,
	.disable = regulator_modulino_latch_relay_disable,
};

#define DEFINE_MODULINO_LATCH_RELAY(inst)                                                         \
	static const struct regulator_modulino_latch_relay_config                                  \
		regulator_modulino_latch_relay_config_##inst = {                                   \
			.common = REGULATOR_DT_COMMON_CONFIG_INIT(inst),                           \
			.bus = I2C_DT_SPEC_INST_GET(inst),                                         \
	};                                                                                         \
	static struct regulator_modulino_latch_relay_data                                          \
		regulator_modulino_latch_relay_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, regulator_modulino_latch_relay_init, NULL,                     \
			      &regulator_modulino_latch_relay_data_##inst,                         \
			      &regulator_modulino_latch_relay_config_##inst, POST_KERNEL,          \
			      CONFIG_REGULATOR_MODULINO_LATCH_RELAY_INIT_PRIORITY,                 \
			      &modulino_latch_relay_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_MODULINO_LATCH_RELAY)
