/*
 * Copyright (c) 2025, tinyvision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_sc18is606_spi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sc18is606, CONFIG_SPI_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include "spi_context.h"

#define SC18IS606_CONFIG_SPI 0xF0
#define CLEAR_INTERRUPT      0xF1
#define IDLE_MODE            0xF2

#include "spi_sc18is606.h"

struct nxp_sc18is606_data {
	struct k_mutex bridge_lock;
	struct spi_context ctx;
	uint32_t frequency;
	uint8_t spi_mode;
};

struct nxp_sc18is606_config {
	const struct i2c_dt_spec i2c_controller;
};

int nxp_sc18is606_claim(const struct device *dev)
{
	struct nxp_sc18is606_data *data = dev->data;

	return k_mutex_lock(&data->bridge_lock, K_FOREVER);
}

int nxp_sc18is606_release(const struct device *dev)
{
	struct nxp_sc18is606_data *data = dev->data;

	return k_mutex_unlock(&data->bridge_lock);
}

int nxp_sc18is606_transfer(const struct device *dev, const uint8_t *tx_data, uint8_t tx_len,
			   uint8_t *rx_data, uint8_t rx_len)
{
	struct nxp_sc18is606_data *data = dev->data;
	const struct nxp_sc18is606_config *info = dev->config;
	int ret;

	ret = k_mutex_lock(&data->bridge_lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	if (tx_data != NULL) {
		ret = i2c_write_dt(&info->i2c_controller, tx_data, tx_len);
		if (ret) {
			LOG_ERR("SPI write failed: %d", ret);
			return ret;
		}
	}

	if (rx_data != NULL) {
		/*What is the time*/
		k_timepoint_t end;

		/*Set a deadline in a second*/
		end = sys_timepoint_calc(K_MSEC(1));

		do {
			ret = i2c_read(info->i2c_controller.bus, rx_data, rx_len,
				       info->i2c_controller.addr);
		} while (!sys_timepoint_expired(end)); /*Keep reading while in the deadline*/

		if (ret < 0) {
			LOG_ERR("Failed to read data (%d)", ret);
			return ret;
		}
	}

	k_mutex_unlock(&data->bridge_lock);

	return 0;
}

static int sc18is606_spi_configure(const struct device *dev, const struct spi_config *config)
{
	struct nxp_sc18is606_data *data = dev->data;

	if (config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("SC18IS606 does not support Slave mode");
		return -ENOTSUP;
	}

	if (config->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD | SPI_LINES_OCTAL)) {
		LOG_ERR("Unsupported line configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if (bits > 8) {
		LOG_ERR("Word sizes > 8 bits not supported");
		return -ENOTSUP;
	}

	/* Build SC18IS606  configuration byte*/
	uint8_t cfg_byte = 0;

	cfg_byte |= ((config->operation & SPI_TRANSFER_LSB) >> 4) << 5;

	uint8_t mode = (SPI_MODE_GET(config->operation) >> 1) & 0x3;

	cfg_byte |= (mode << 2);

	cfg_byte |= (config->frequency & 0x3);

	data->ctx.config = config;

	uint8_t buffer[2] = {SC18IS606_CONFIG_SPI, cfg_byte};

	cfg_byte = nxp_sc18is606_transfer(dev, buffer, sizeof(buffer), NULL, 0);

	return 0;
}

static int sc18is606_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_buffer_set,
				    const struct spi_buf_set *rx_buffer_set)
{
	int ret = 1;

	ret = sc18is606_spi_configure(dev, spi_cfg);
	if (ret < 0) {
		return ret;
	}

	if (!tx_buffer_set && !rx_buffer_set) {
		LOG_ERR("SC18IS606 at least one buffer_set should be set");
		return -EINVAL;
	}

	/* CS line to be Used */
	uint8_t ss_idx = spi_cfg->slave;

	if (ss_idx > 2) {
		LOG_ERR("SC18IS606: Invalid SS Index (%u) must be 0-2", ss_idx);
		return -EINVAL;
	}

	uint8_t function_id = (1 << ss_idx) & 0x07;

	if (tx_buffer_set && tx_buffer_set->buffers && tx_buffer_set->count > 0) {
		for (size_t i = 0; i < tx_buffer_set->count; i++) {
			const struct spi_buf *tx_buf = &tx_buffer_set->buffers[i];
			const uint8_t *data_bytes = (const uint8_t *)tx_buf->buf;
			const size_t len = tx_buf->len;
			const size_t sc18_len = len + 1;

			uint8_t sc18_buf[sc18_len];

			sc18_buf[0] = function_id;

			for (size_t buf_i = 1; buf_i < sc18_len; buf_i++) {
				sc18_buf[buf_i] = data_bytes[buf_i - 1];
			}

			ret = nxp_sc18is606_transfer(dev, sc18_buf, sizeof(sc18_buf), NULL, 0);
			if (ret < 0) {
				LOG_ERR("SC18IS606: TX of size: %d failed %s", tx_buf->len,
					dev->name);
				return ret;
			}
		}
	}

	if (rx_buffer_set && rx_buffer_set->buffers && rx_buffer_set->count > 0) {
		for (size_t i = 0; i < rx_buffer_set->count; i++) {
			/* Function ID first to select the device */
			uint8_t cmd_buf[1] = {function_id};

			const struct spi_buf *rx_buf = &rx_buffer_set->buffers[i];

			ret = nxp_sc18is606_transfer(dev, cmd_buf, sizeof(cmd_buf), rx_buf->buf,
						     rx_buf->len);

			if (ret < 0) {
				LOG_ERR("SC18IS606: RX of size: %d failed on  (%s)", rx_buf->len,
					dev->name);
				return ret;
			}
		}
	}

	return ret;
}

int sc18is606_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct nxp_sc18is606_data *data = dev->data;

	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static DEVICE_API(spi, sc18is606_api) = {
	.transceive = sc18is606_spi_transceive,
	.release = sc18is606_spi_release,
};

static int sc18is606_init(const struct device *dev)
{
	const struct nxp_sc18is606_config *cfg = dev->config;
	struct nxp_sc18is606_data *data = dev->data;
	int ret;

	struct spi_config my_config = {
		.frequency = data->frequency,
		.operation = data->spi_mode,
		.slave = data->spi_mode,
	};

	if (!device_is_ready(cfg->i2c_controller.bus)) {
		LOG_ERR("I2C controller %s not found", cfg->i2c_controller.bus->name);
		return -ENODEV;
	}

	LOG_INF("Using I2C controller: %s", cfg->i2c_controller.bus->name);

	ret = sc18is606_spi_configure(dev, &my_config);
	if (ret != 0) {
		LOG_ERR("Failed to CONFIGURE the SC18IS606: %d", ret);
		return ret;
	}

	LOG_INF("SC18IS606 initialized");
	return 0;
}

#define SPI_SC18IS606_DEFINE(inst)                                                                 \
	static struct nxp_sc18is606_data sc18is606_data_##inst = {                                 \
		.frequency = DT_PROP(DT_DRV_INST(inst), frequency),                                \
		.spi_mode = DT_INST_PROP(inst, spi_mode),                                          \
	};                                                                                         \
	static const struct nxp_sc18is606_config sc18is606_config_##inst = {                       \
		.i2c_controller = I2C_DT_SPEC_GET(DT_PARENT(DT_DRV_INST(inst))),                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sc18is606_init, NULL, &sc18is606_data_##inst,                  \
			      &sc18is606_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
			      &sc18is606_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SC18IS606_DEFINE)
