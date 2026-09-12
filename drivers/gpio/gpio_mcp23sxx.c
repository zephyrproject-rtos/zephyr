/*
 *
 * Copyright (c) 2021 metraTec GmbH
 * Copyright (c) 2021 Peter Johanson
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Driver for MPC23Sxx SPI-based GPIO driver.
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#include <zephyr/drivers/gpio/gpio_utils.h>
#include "gpio_mcp23xxx.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_mcp23sxx);

static int mcp23sxx_read_regs(const struct device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
	const struct mcp23xxx_config *config = dev->config;
	uint8_t buffer_tx[2 + MCP23XXX_MAX_BURST] = {MCP23SXX_ADDR | MCP23SXX_READBIT, reg};
	uint8_t buffer_rx[2 + MCP23XXX_MAX_BURST];
	int ret;

	if (len > MCP23XXX_MAX_BURST) {
		return -EINVAL;
	}

	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 2 + len,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = 2 + len,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};

	ret = spi_transceive_dt(&config->bus.spi, &tx, &rx);
	if (ret < 0) {
		LOG_ERR("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	memcpy(buf, &buffer_rx[2], len);

	return 0;
}

static int mcp23sxx_write_regs(const struct device *dev, uint8_t reg, const uint8_t *buf,
			       size_t len)
{
	const struct mcp23xxx_config *config = dev->config;
	uint8_t buffer_tx[2 + MCP23XXX_MAX_BURST] = {MCP23SXX_ADDR, reg};
	int ret;

	if (len > MCP23XXX_MAX_BURST) {
		return -EINVAL;
	}

	memcpy(&buffer_tx[2], buf, len);

	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 2 + len,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	ret = spi_write_dt(&config->bus.spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write FAIL %d\n", ret);
		return ret;
	}

	return 0;
}

static int mcp23sxx_bus_is_ready(const struct device *dev)
{
	const struct mcp23xxx_config *config = dev->config;

	if (!spi_is_ready_dt(&config->bus.spi)) {
		LOG_ERR("SPI bus %s not ready", config->bus.spi.bus->name);
		return -ENODEV;
	}

	return 0;
}

#define GPIO_MCP23SXX_DEVICE(inst, num_gpios, open_drain, model)                              \
	static struct mcp23xxx_drv_data mcp##model##_##inst##_drvdata = {                     \
		/* Default for registers according to datasheet */                            \
		.reg_cache.iodir = 0xFFFF, .reg_cache.ipol = 0x0,   .reg_cache.gpinten = 0x0, \
		.reg_cache.defval = 0x0,   .reg_cache.intcon = 0x0, .reg_cache.iocon = 0x0,   \
		.reg_cache.gppu = 0x0,	   .reg_cache.intf = 0x0,   .reg_cache.intcap = 0x0,  \
		.reg_cache.gpio = 0x0,	   .reg_cache.olat = 0x0,                             \
	};                                                                                    \
	static struct mcp23xxx_config mcp##model##_##inst##_config = {                        \
		.config = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                              \
		.bus = {                                                                      \
			.spi = SPI_DT_SPEC_INST_GET(inst,                                     \
				SPI_OP_MODE_CONTROLLER | SPI_MODE_CPOL |                      \
				SPI_MODE_CPHA | SPI_WORD_SET(8))                              \
		},                                                                            \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                   \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),               \
		.ngpios =  num_gpios,				                              \
		.is_open_drain = open_drain,                                                  \
		.int_open_drain = DT_INST_PROP(inst, int_open_drain),                         \
		.read_fn = mcp23sxx_read_regs,                                                \
		.write_fn = mcp23sxx_write_regs,                                              \
		.bus_fn = mcp23sxx_bus_is_ready                                               \
	};                                                                                    \
	DEVICE_DT_INST_DEFINE(inst, gpio_mcp23xxx_init, NULL, &mcp##model##_##inst##_drvdata, \
			      &mcp##model##_##inst##_config, POST_KERNEL,                     \
			      CONFIG_GPIO_MCP23SXX_INIT_PRIORITY, &gpio_mcp23xxx_api_table);


#define DT_DRV_COMPAT microchip_mcp23s08
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP23SXX_DEVICE, 8, false, 23s08)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23s09
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP23SXX_DEVICE, 8, true, 23s09)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23s17
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP23SXX_DEVICE, 16, false, 23s17)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT microchip_mcp23s18
DT_INST_FOREACH_STATUS_OKAY_VARGS(GPIO_MCP23SXX_DEVICE, 16, true, 23s18)
#undef DT_DRV_COMPAT
