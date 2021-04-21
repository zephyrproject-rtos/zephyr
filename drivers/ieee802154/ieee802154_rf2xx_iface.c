/* ieee802154_rf2xx_iface.c - ATMEL RF2XX IEEE 802.15.4 Interface */

/*
 * Copyright (c) 2019-2020 Gerson Fernando Budke
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME ieee802154_rf2xx_iface
#define LOG_LEVEL CONFIG_IEEE802154_DRIVER_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <errno.h>

#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#include "ieee802154_rf2xx.h"
#include "ieee802154_rf2xx_regs.h"
#include "ieee802154_rf2xx_iface.h"

void rf2xx_iface_phy_rst(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	const struct rf2xx_context *ctx = dev->data;

	/* Ensure control lines have correct levels. */
	gpio_pin_set(ctx->reset_gpio, conf->reset.pin, 0);
	gpio_pin_set(ctx->slptr_gpio, conf->slptr.pin, 0);

	/* Wait typical time of timer TR1. */
	k_busy_wait(330);

	gpio_pin_set(ctx->reset_gpio, conf->reset.pin, 1);
	k_busy_wait(10);
	gpio_pin_set(ctx->reset_gpio, conf->reset.pin, 0);
}
void rf2xx_iface_phy_tx_start(const struct device *dev)
{
	const struct rf2xx_config *conf = dev->config;
	const struct rf2xx_context *ctx = dev->data;

	/* Start TX transmission at rise edge */
	gpio_pin_set(ctx->slptr_gpio, conf->slptr.pin, 1);
	/* 16.125[μs] delay to detect signal */
	k_busy_wait(20);
	/* restore initial pin state */
	gpio_pin_set(ctx->slptr_gpio, conf->slptr.pin, 0);
}

uint8_t rf2xx_iface_reg_read(const struct device *dev,
			     uint8_t addr)
{
	const struct rf2xx_context *ctx = dev->data;
	uint8_t status;
	uint8_t regval = 0;

	addr |= RF2XX_RF_CMD_REG_R;

	const struct spi_buf tx_buf = {
		.buf = &addr,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_buf rx_buf[2] = {
		{
			.buf = &status,
			.len = 1
		},
		{
			.buf = &regval,
			.len = 1
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx) != 0) {
		LOG_ERR("Failed to exec rf2xx_reg_read CMD at address %d",
			addr);
	}

	LOG_DBG("Read Address: %02X, PhyStatus: %02X, RegVal: %02X",
		(addr & ~(RF2XX_RF_CMD_REG_R)), status, regval);

	return regval;
}

void rf2xx_iface_reg_write(const struct device *dev,
			   uint8_t addr,
			   uint8_t data)
{
	const struct rf2xx_context *ctx = dev->data;
	uint8_t status;

	addr |= RF2XX_RF_CMD_REG_W;

	const struct spi_buf tx_buf[2] = {
		{
			.buf = &addr,
			.len = 1
		},
		{
			.buf = &data,
			.len = 1
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};
	const struct spi_buf rx_buf = {
		.buf = &status,
		.len = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	if (spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx) != 0) {
		LOG_ERR("Failed to exec rf2xx_reg_write at address %d",
			addr);
	}

	LOG_DBG("Write Address: %02X, PhyStatus: %02X, RegVal: %02X",
		(addr & ~(RF2XX_RF_CMD_REG_W)), status, data);
}

uint8_t rf2xx_iface_bit_read(const struct device *dev,
			     uint8_t addr,
			     uint8_t mask,
			     uint8_t pos)
{
	uint8_t ret;

	ret = rf2xx_iface_reg_read(dev, addr);
	ret &= mask;
	ret >>= pos;

	return ret;
}

void rf2xx_iface_bit_write(const struct device *dev,
			   uint8_t reg_addr,
			   uint8_t mask,
			   uint8_t pos,
			   uint8_t new_value)
{
	uint8_t current_reg_value;

	current_reg_value = rf2xx_iface_reg_read(dev, reg_addr);
	current_reg_value &= ~mask;
	new_value <<= pos;
	new_value &= mask;
	new_value |= current_reg_value;
	rf2xx_iface_reg_write(dev, reg_addr, new_value);
}

void rf2xx_iface_frame_read(const struct device *dev,
			    uint8_t *data,
			    uint8_t length)
{
	const struct rf2xx_context *ctx = dev->data;
	uint8_t cmd = RF2XX_RF_CMD_FRAME_R;

	const struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_buf rx_buf = {
		.buf = data,
		.len = length
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	if (spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx) != 0) {
		LOG_ERR("Failed to exec rf2xx_frame_read PHR");
	}

	LOG_DBG("Frame R: PhyStatus: %02X. length: %02X", data[0], length);
	LOG_HEXDUMP_DBG(data + RX2XX_FRAME_HEADER_SIZE, length, "payload");
}

void rf2xx_iface_frame_write(const struct device *dev,
			     uint8_t *data,
			     uint8_t length)
{
	const struct rf2xx_context *ctx = dev->data;
	uint8_t cmd = RF2XX_RF_CMD_FRAME_W;
	uint8_t status;
	uint8_t phr;

	/* Sanity check */
	if (length > 125) {
		length = 125;
	}

	phr = length + RX2XX_FRAME_FCS_LENGTH;

	const struct spi_buf tx_buf[3] = {
		{
			.buf = &cmd,
			.len = 1
		},
		{
			.buf = &phr,    /* PHR */
			.len = 1
		},
		{
			.buf = data,    /* PSDU */
			.len = length
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 3
	};
	const struct spi_buf rx_buf = {
		.buf = &status,
		.len = 1
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1
	};

	if (spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx) != 0) {
		LOG_ERR("Failed to exec rf2xx_frame_write");
	}

	LOG_DBG("Frame W: PhyStatus: %02X. length: %02X", status, length);
	LOG_HEXDUMP_DBG(data, length, "payload");
}

void rf2xx_iface_sram_read(const struct device *dev,
			    uint8_t address,
			    uint8_t *data,
			    uint8_t length)
{
	const struct rf2xx_context *ctx = dev->data;
	uint8_t cmd = RF2XX_RF_CMD_SRAM_R;
	uint8_t status[2];

	const struct spi_buf tx_buf[2] = {
		{
			.buf = &cmd,
			.len = 1
		},
		{
			.buf = &address,
			.len = 1
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};
	const struct spi_buf rx_buf[2] = {
		{
			.buf = status,
			.len = 2
		},
		{
			.buf = data,
			.len = length
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (spi_transceive(ctx->spi, &ctx->spi_cfg, &tx, &rx) != 0) {
		LOG_ERR("Failed to exec rf2xx_sram_read");
	}

	LOG_DBG("SRAM R: length: %02X, status: %02X", length, status[0]);
	LOG_HEXDUMP_DBG(data, length, "content");
}
