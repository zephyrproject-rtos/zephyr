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

static int mcp23sxx_read_port_regs(const struct device *dev, uint8_t reg, uint16_t *buf)
{
	const struct mcp23xxx_config *config = dev->config;
	uint16_t port_data = 0;
	int ret;

	uint8_t nread = (config->ngpios == 8) ? 1 : 2;

	uint8_t addr = MCP23SXX_ADDR | MCP23SXX_READBIT;
	uint8_t buffer_tx[4] = { addr, reg, 0, 0 };
	uint8_t buffer_rx[4] = { 0 };

	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 4,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = 2 + nread,
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

	port_data = ((uint16_t)buffer_rx[3] << 8 | buffer_rx[2]);

	*buf = sys_le16_to_cpu(port_data);

	return 0;
}

static int mcp23sxx_write_port_regs(const struct device *dev, uint8_t reg, uint16_t value)
{
	const struct mcp23xxx_config *config = dev->config;
	int ret;

	uint8_t nwrite = (config->ngpios == 8) ? 1 : 2;
	uint16_t port_data = sys_cpu_to_le16(value);
	uint8_t port_a_data = port_data & 0xFF;
	uint8_t port_b_data = (port_data >> 8) & 0xFF;

	port_data = sys_cpu_to_le16(value);

	uint8_t addr = MCP23SXX_ADDR;
	uint8_t buffer_tx[4] = { addr, reg, port_a_data, port_b_data };

	const struct spi_buf tx_buf[1] = {
		{
			.buf = buffer_tx,
			.len = nwrite + 2,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf),
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

#define DT_DRV_COMPAT microchip_mcp23sxx

#define GPIO_MCP23SXX_DEVICE(inst)                                                               \
	static struct mcp23xxx_drv_data mcp23sxx_##inst##_drvdata = {                         \
		/* Default for registers according to datasheet */                            \
		.reg_cache.iodir = 0xFFFF, .reg_cache.ipol = 0x0,   .reg_cache.gpinten = 0x0, \
		.reg_cache.defval = 0x0,   .reg_cache.intcon = 0x0, .reg_cache.iocon = 0x0,   \
		.reg_cache.gppu = 0x0,	   .reg_cache.intf = 0x0,   .reg_cache.intcap = 0x0,  \
		.reg_cache.gpio = 0x0,	   .reg_cache.olat = 0x0,                             \
	};                                                                                    \
	static struct mcp23xxx_config mcp23sxx_##inst##_config = {                            \
		.config = {					                              \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),                  \
		},						                              \
		.bus = {                                                                      \
			.spi = SPI_DT_SPEC_INST_GET(inst,                                        \
				SPI_OP_MODE_MASTER | SPI_MODE_CPOL |                          \
				SPI_MODE_CPHA | SPI_WORD_SET(8), 0)                           \
		},                                                                            \
		.gpio_int = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),                   \
		.gpio_reset = GPIO_DT_SPEC_INST_GET_OR(inst, reset_gpios, {0}),               \
		.ngpios =  DT_INST_PROP(inst, ngpios),		                              \
		.read_fn = mcp23sxx_read_port_regs,                                           \
		.write_fn = mcp23sxx_write_port_regs,                                         \
		.bus_fn = mcp23sxx_bus_is_ready                                               \
	};                                                                                    \
	DEVICE_DT_INST_DEFINE(inst, gpio_mcp23xxx_init, NULL, &mcp23sxx_##inst##_drvdata,        \
			      &mcp23sxx_##inst##_config, POST_KERNEL,                         \
			      CONFIG_GPIO_MCP23SXX_INIT_PRIORITY, &gpio_mcp23xxx_api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MCP23SXX_DEVICE)
