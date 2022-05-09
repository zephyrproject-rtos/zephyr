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
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include "gpio_utils.h"
#include "gpio_mcp23xxx.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp230xx);

static int mcp230xx_read_port_regs(const struct device *dev, uint8_t reg, uint16_t *buf)
{
	const struct mcp23xxx_config *config = dev->config;
	uint16_t port_data = 0;
	int ret;

	uint8_t nread = (config->ngpios == 8) ? 1 : 2;

	ret = i2c_burst_read_dt(&config->bus.i2c, reg, (uint8_t *)&port_data, nread);
	if (ret < 0) {
		LOG_ERR("i2c_read failed!");
		return ret;
	}

	*buf = sys_le16_to_cpu(port_data);

	return 0;
}

static int mcp230xx_write_port_regs(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct mcp23xxx_config *config = dev->config;
	int ret;

	uint8_t nwrite = (config->ngpios == 8) ? 2 : 3;
	uint8_t buf[3];

	buf[0] = reg;
	sys_put_le16(value, &buf[1]);

	ret = i2c_write_dt(&config->bus.i2c, buf, nwrite);
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

#define DT_DRV_COMPAT microchip_mcp230xx

#define GPIO_MCP230XX_DEVICE(n)                                                                    \
	static struct mcp23xxx_drv_data mcp230xx_##inst##_drvdata = {                              \
		/* Default for registers according to datasheet */                                 \
		.reg_cache.iodir = 0xFFFF, .reg_cache.ipol = 0x0,   .reg_cache.gpinten = 0x0,      \
		.reg_cache.defval = 0x0,   .reg_cache.intcon = 0x0, .reg_cache.iocon = 0x0,        \
		.reg_cache.gppu = 0x0,	   .reg_cache.intf = 0x0,   .reg_cache.intcap = 0x0,       \
		.reg_cache.gpio = 0x0,	   .reg_cache.olat = 0x0,                                  \
	};                                                                                         \
	static struct mcp23xxx_config mcp230xx_##inst##_config = {     \
		.config = {					       \
			.port_pin_mask =			       \
				GPIO_PORT_PIN_MASK_FROM_DT_INST(n),    \
		},						       \
		.bus =  \
	{                                                                                          \
		.i2c = I2C_DT_SPEC_INST_GET(n)                                          \
	}, \
		.ngpios =  DT_INST_PROP(n, ngpios),			       \
		.read_fn = mcp230xx_read_port_regs,              \
		.write_fn = mcp230xx_write_port_regs,              \
		.bus_fn = mcp230xx_bus_is_ready              \
	};                           \
	DEVICE_DT_INST_DEFINE(n, gpio_mcp23xxx_init, NULL, &mcp230xx_##inst##_drvdata,             \
			      &mcp230xx_##inst##_config, POST_KERNEL,                              \
			      CONFIG_GPIO_MCP230XX_INIT_PRIORITY, &gpio_mcp23xxx_api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MCP230XX_DEVICE)
