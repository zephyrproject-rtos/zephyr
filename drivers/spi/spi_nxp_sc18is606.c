/*
 * Copyright (c) 2025, tinyvision.ai
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include "spi_context.h"

#define DT_DRV_COMPAT nxp_sc18is606

LOG_MODULE_REGISTER(nxp_sc18is606, CONFIG_SPI_LOG_LEVEL);


#define SC18IS606_CONFIG_SPI 0xF0
#define CLEAR_INTERRUPT      0xF1
#define IDLE_MODE            0xF2
#define READ_VERSION         0xFE

struct nxp_sc18is606_data {
	struct spi_context ctx;
	const struct device *i2c_dev;
	uint8_t i2c_addr;
	uint32_t spi_clock_freq;
	uint8_t spi_mode;
};

struct nxp_sc18is606_config {
	 const struct i2c_dt_spec i2c_controller;
};

static int sc18is606_write_reg(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t value)
{
	uint8_t buffer[2] = {reg, value};
	return i2c_write_dt(i2c, buffer, sizeof(buffer));
}

static int sc18is606_spi_configure(const struct nxp_sc18is606_config *info,
		struct nxp_sc18is606_data *data,
		const struct spi_config *config)
{
	if(config->operation & SPI_OP_MODE_SLAVE) {
		LOG_ERR("SC18IS606 does not support Slave mode");
		return -ENOTSUP;
	}

	if(config->operation & (SPI_LINES_DUAL | SPI_LINES_QUAD | SPI_LINES_OCTAL)) {
		LOG_ERR("Unsupported line configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if(bits > 8){
		LOG_ERR("Word sizes > 8 bits not supported");
		return -ENOTSUP;
	}


	/* Since I am using a hardware device and configuration is by writing a register
	 * Via i2c do I need to set config as below or is the register write sufficient
	 */
	data->ctx.config = config;

	int ret;
	ret = sc18is606_write_reg(&info->i2c_controller, SC18IS606_CONFIG_SPI, data->spi_mode);

	return ret;
}





static int sc18is606_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_buffer_set,
				    const struct spi_buf_set *rx_buffer_set)
{
	struct nxp_sc18is606_data *data = dev->data;
	const struct device *i2c_dev = data->i2c_dev;
	const struct nxp_sc18is606_config *info = dev->config;
	int ret, reconfig;


	reconfig = sc18is606_spi_configure(info, data, spi_cfg);
	if(reconfig < 0)
	{
		return reconfig;
	}

	if (!tx_buffer_set || !rx_buffer_set) {
		LOG_ERR("SC18IS606 Invalid Buffers");
		return -EINVAL;
	}

	/* CS line to be Used */
	uint8_t ss_idx = 0;
	if(spi_cfg->cs.gpio.port) {
		ss_idx  = spi_cfg->cs.gpio.pin;
		if(ss_idx > 2) {
			LOG_ERR("SC18IS606: Invalid SS Index (%u)", ss_idx);
			return -EINVAL;
		}
	}
	uint8_t function_id = (1 << ss_idx) & 0x07;

	if(tx_buffer_set && tx_buffer_set->buffers && tx_buffer_set->count > 0) {
		const struct spi_buf *tx_buf = &tx_buffer_set->buffers[0];
		const size_t len = tx_buf->len;

		uint8_t sc18_buf[1 + len];
		sc18_buf[0] = function_id;
		memcpy(&sc18_buf[1], tx_buf->buf, len);

		ret = i2c_write(i2c_dev, sc18_buf, sizeof(sc18_buf), data->i2c_addr);
		if (ret) {
			LOG_ERR("SPI write failed: %d", ret);
			return ret;
		}
	}

	if (rx_buffer_set && rx_buffer_set->buffers && rx_buffer_set->count > 0) {
		/* Function ID first to select the device */
		uint8_t cmd_buf[1] = {function_id};
		ret = i2c_write(i2c_dev, cmd_buf, sizeof(cmd_buf), data->i2c_addr);
		if(ret) {
			LOG_ERR("SPI read preamnble write failed %d", ret);
			return ret;
		}
		const struct spi_buf *rx_buf = &rx_buffer_set->buffers[0];
		ret = i2c_read(i2c_dev, rx_buf->buf, rx_buf->len, data->i2c_addr);
		if (ret) {
			LOG_ERR("SPI read failed: %d", ret);
			return ret;
		}
	}

	return 0;
}

int sc18is606_release(const struct device *dev, const struct spi_config *config)
{
	struct nxp_sc18is606_data *data = dev -> data;

	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);

	return 0;
}




static DEVICE_API(spi, sc18is606_api) = {
	.transceive = sc18is606_spi_transceive,
	.release = sc18is606_release,
};

static int sc18is606_init(const struct device *dev)
{
	const struct nxp_sc18is606_config *cfg = dev->config;
	struct nxp_sc18is606_data *data = dev->data;
	int ret;

	data->i2c_dev = cfg->i2c_controller.bus;
	if (!data->i2c_dev) {
		LOG_ERR("I2C controller %s not found", data->i2c_dev->name);
		return -ENODEV;
	}

	LOG_INF("Using I2C controller: %s", data->i2c_dev->name);

	ret = sc18is606_write_reg(&cfg->i2c_controller, SC18IS606_CONFIG_SPI, data->spi_mode);
	if (ret) {
		LOG_ERR("Failed to CONFIGURE the SC18IS606: %d", ret);
		return ret;
	}

	LOG_INF("SC18IS606 initialized");
	return 0;
}

#define NXP_SC18IS606_DEFINE(inst)                                                                 \
	static struct nxp_sc18is606_data sc18is606_data_##inst = {                                 \
		.i2c_addr = DT_INST_REG_ADDR(inst),                                                \
		.spi_clock_freq = DT_INST_PROP(inst, spi_clock_frequency),                         \
		.spi_mode = DT_INST_PROP(inst, spi_mode),                                          \
	};                                                                                         \
	static const struct nxp_sc18is606_config sc18is606_config_##inst = {                       \
		.i2c_controller = I2C_DT_SPEC_INST_GET(inst),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sc18is606_init, NULL, &sc18is606_data_##inst,                  \
			      &sc18is606_config_##inst, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,     \
			      &sc18is606_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SC18IS606_DEFINE)
