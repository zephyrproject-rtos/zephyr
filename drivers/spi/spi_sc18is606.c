/*
 * Copyright (c) 2026, tinyvision.ai
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
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>
#include "spi_context.h"
#include <mfd_sc18is606.h>

struct spi_sc18is606_data {
	struct spi_context ctx;
	uint8_t frequency_idx;
	uint8_t spi_mode;
};

struct spi_sc18is606_config {
	const struct device *bridge;
};

#define SC18IS606_SPI_FREQUENCY_CNT  4
#define SC18IS606_SPI_FREQUENCY_1875 0
#define SC18IS606_SPI_FREQUENCY_455  1
#define SC18IS606_SPI_FREQUENCY_115  2
#define SC18IS606_SPI_FREQUENCY_58   3


static int sc18is606_spi_configure(const struct device *dev, const struct spi_config *config)
{
	const struct spi_sc18is606_config *cfg = dev->config;
	struct spi_sc18is606_data *data = dev->data;
	uint8_t cfg_byte = 0;
	uint8_t buffer[2];
	uint8_t freq_idx;
	int ret;

	if ((config->operation & SPI_OP_MODE_SLAVE) != 0U) {
		LOG_ERR("SC18IS606 does not support Slave mode");
		return -ENOTSUP;
	}

	if ((config->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Unsupported line configuration");
		return -ENOTSUP;
	}

	const int bits = SPI_WORD_SIZE_GET(config->operation);

	if (bits > 8) {
		LOG_ERR("Word sizes > 8 bits not supported");
		return -ENOTSUP;
	}

	ret = nxp_sc18is606_set_pin_mode(cfg->bridge, config->slave, false, SC18IS606_GPIO_CS_CONF);

	if (ret < 0) {
		LOG_ERR("Failed to set pin mode (%d)", ret);
	}

	/* Build SC18IS606  configuration byte*/
	cfg_byte |= FIELD_PREP(SC18IS606_LSB_MASK, (config->operation & SPI_TRANSFER_LSB) >> 4);

	cfg_byte |= FIELD_PREP(SC18IS606_MODE_MASK, (SPI_MODE_GET(config->operation) >> 1));

	if (config->frequency >= KHZ(1875)) {
		freq_idx = SC18IS606_SPI_FREQUENCY_1875;
	} else if (config->frequency >= KHZ(455)) {
		freq_idx = SC18IS606_SPI_FREQUENCY_455;
	} else if (config->frequency >= KHZ(115)) {
		freq_idx = SC18IS606_SPI_FREQUENCY_115;
	} else {
		freq_idx = SC18IS606_SPI_FREQUENCY_58;
	}

	cfg_byte |= FIELD_PREP(SC18IS606_FREQ_MASK, freq_idx);

	data->ctx.config = config;

	buffer[0] = SC18IS606_CONFIG_SPI;
	buffer[1] = cfg_byte;
	cfg_byte |= ((config->operation & SPI_TRANSFER_LSB) >> 4) << 5;

	return nxp_sc18is606_transfer(cfg->bridge, buffer, sizeof(buffer), NULL, 0, NULL);
}

static int sc18is606_spi_transceive(const struct device *dev, const struct spi_config *spi_cfg,
				    const struct spi_buf_set *tx_buffer_set,
				    const struct spi_buf_set *rx_buffer_set)
{
	const struct spi_sc18is606_config *cfg = dev->config;
	struct spi_sc18is606_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

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

	spi_context_cs_control(ctx, true);

	if (tx_buffer_set && tx_buffer_set->buffers && rx_buffer_set && rx_buffer_set->buffers) {
		for (size_t i = 0; i < tx_buffer_set->count; i++) {
			const struct spi_buf *tx_buf = &tx_buffer_set->buffers[i];
			const struct spi_buf *rx_buf = &rx_buffer_set->buffers[i];

			uint8_t id_buf[1] = {function_id};

			ret = nxp_sc18is606_transfer(cfg->bridge, tx_buf->buf, tx_buf->len,
						     rx_buf->buf, rx_buf->len, id_buf);
			if (ret < 0) {
				LOG_ERR("SC18IS606: TX of size: %d failed %s", tx_buf->len,
					dev->name);
				return ret;
			}
		}
	} else {
		if (tx_buffer_set && tx_buffer_set->buffers && tx_buffer_set->count > 0) {
			for (size_t i = 0; i < tx_buffer_set->count; i++) {
				const struct spi_buf *tx_buf = &tx_buffer_set->buffers[i];

				uint8_t id_buf[1] = {function_id};

				ret = nxp_sc18is606_transfer(cfg->bridge, tx_buf->buf, tx_buf->len,
							     NULL, 0, id_buf);

				if (ret < 0) {
					LOG_ERR("SC18IS606: TX of size: %d failed on  (%s)",
						tx_buf->len, dev->name);
					return ret;
				}
			}
		}

		if (rx_buffer_set && rx_buffer_set->buffers && rx_buffer_set->count > 0) {
			for (size_t i = 0; i < rx_buffer_set->count; i++) {
				/* Function ID to select the device */
				uint8_t cmd_buf[1] = {function_id};

				const struct spi_buf *rx_buf = &rx_buffer_set->buffers[i];

				ret = nxp_sc18is606_transfer(cfg->bridge, cmd_buf, sizeof(cmd_buf),
							     rx_buf->buf, rx_buf->len, NULL);

				if (ret < 0) {
					LOG_ERR("SC18IS606: Rx of size: %d failed on (%s)",
						rx_buf->len, dev->name);
					return ret;
				}
			}
		}
	}

	spi_context_cs_control(ctx, false);

	spi_context_complete(ctx, dev, 0);

	return ret;
}

int sc18is606_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct spi_sc18is606_data *data = dev->data;

	struct spi_context *ctx = &data->ctx;

	spi_context_unlock_unconditionally(ctx);

	return 0;
}

static DEVICE_API(spi, sc18is606_api) = {
	.transceive = sc18is606_spi_transceive,
	.release = sc18is606_spi_release,
};

static int sc18is606_spi_init(const struct device *dev)
{
	struct spi_sc18is606_data *data = dev->data;
	int ret;

	struct spi_config my_config = {
		.frequency = data->frequency_idx,
		.operation = data->spi_mode,
		.slave = 0,
	};

	ret = sc18is606_spi_configure(dev, &my_config);
	if (ret != 0) {
		LOG_ERR("Failed to CONFIGURE the SC18IS606: %d", ret);
		return ret;
	}

	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("Failed to configure CS pins: %d", ret);
		return ret;
	}

	LOG_DBG("SC18IS606 SPI initialized");
	return 0;
}

#define SPI_SC18IS606_DEFINE(inst)                                                                 \
	static struct spi_sc18is606_data spi_sc18is606_data_##inst = {                             \
		.frequency_idx = DT_INST_ENUM_IDX(inst, frequency),                                \
		.spi_mode = DT_INST_PROP(inst, spi_mode),                                          \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(inst), ctx)};                          \
	static const struct spi_sc18is606_config spi_sc18is606_config##inst = {                    \
		.bridge = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, sc18is606_spi_init, NULL, &spi_sc18is606_data_##inst,          \
			      &spi_sc18is606_config##inst, POST_KERNEL,                            \
			      CONFIG_SPI_SC18IS606_INIT_PRIORITY, &sc18is606_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SC18IS606_DEFINE)
