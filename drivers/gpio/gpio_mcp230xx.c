/*
 *
 * Copyright (c) 2021 metraTec GmbH
 * Copyright (c) 2021 Peter Johanson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for MPC230xx I2C-based GPIO driver.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_mcp23xxx.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp230xx);

static int mcp230xx_read_regs(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
	const struct mcp23xxx_config *config = dev->config;
	int ret;

	ret = i2c_burst_read_dt(&config->bus.i2c, reg, buf, len);
	if (ret < 0) {
		LOG_ERR("i2c_read failed!");
		return ret;
	}

	return 0;
}

static int mcp230xx_write_regs(const struct device *dev, uint8_t reg, const uint8_t *buf,
			       size_t len)
{
	const struct mcp23xxx_config *config = dev->config;
	uint8_t tx[1 + MCP23XXX_MAX_BURST];
	int ret;

	if (len > MCP23XXX_MAX_BURST) {
		return -EINVAL;
	}

	tx[0] = reg;
	memcpy(&tx[1], buf, len);

	ret = i2c_write_dt(&config->bus.i2c, tx, 1 + len);
	if (ret < 0) {
		LOG_ERR("i2c_write failed!");
		return ret;
	}

	return 0;
}

static int mcp230xx_bus_is_ready(const struct device *dev)
{
	const struct mcp23xxx_config *config = dev->config;

	if (!device_is_ready(config->bus.i2c.bus)) {
		LOG_ERR("I2C bus %s not ready", config->bus.i2c.bus->name);
		return -ENODEV;
	}

	return 0;
}

#define GPIO_MCP230XX_DEVICE(inst, num_gpios, open_drain, model)                                   \
	static struct mcp23xxx_drv_data mcp##model##_##inst##_drvdata = {                          \
		/* Default for registers according to datasheet */                                 \
		.reg_cache.iodir = 0xFFFF, .reg_cache.ipol = 0x0,   .reg_cache.gpinten = 0x0,      \
		.reg_cache.defval = 0x0,   .reg_cache.intcon = 0x0, .reg_cache.iocon = 0x0,        \
		.reg_cache.gppu = 0x0,     .reg_cache.intf = 0x0,   .reg_cache.intcap = 0x0,       \
		.reg_cache.gpio = 0x0,     .reg_cache.olat = 0x0,                                  \
	};                                                                                         \
	static const struct mcp23xxx_config mcp##model##_##inst##_config = {                       \
		.config = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		.bus = {                                                                           \
			.i2c = I2C_DT_SPEC_INST_GET(inst),                                         \
		},                                                                                 \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                        \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),                    \
		.ngpios =  num_gpios,                                                              \
		.is_open_drain = open_drain,                                                       \
		.int_open_drain = DT_INST_PROP(inst, int_open_drain),                              \
		.read_fn = mcp230xx_read_regs,                                                     \
		.write_fn = mcp230xx_write_regs,                                                   \
		.bus_fn = mcp230xx_bus_is_ready,                                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_mcp23xxx_init, NULL, &mcp##model##_##inst##_drvdata,      \
		&mcp##model##_##inst##_config, POST_KERNEL,                                        \
		CONFIG_GPIO_MCP230XX_INIT_PRIORITY, &gpio_mcp23xxx_api_table);

#define DT_DRV_COMPAT microchip_mcp23008
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP230XX_DEVICE, 8, false, 23008)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23009
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP230XX_DEVICE, 8, true, 23009)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23016
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP230XX_DEVICE, 16, false, 23016)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23017
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP230XX_DEVICE, 16, false, 23017)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23018
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP230XX_DEVICE, 16, true, 23018)
#undef DT_DRV_COMPAT
