/*
 * Copyright (c) 2025  Andreas Wolf <awolf002@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  dac_tx311.c
 * @brief Driver for the TI x311 single channel DAC chips.
 *
 * This driver supports multiple variants of the Texas Instrument DAC chip.
 *
 *  DAC5311 : 8-bit resolution
 *  DAC6311 : 10-bit resolution
 *  DAC7311 : 12-bit resolution
 *  DAC8311 : 14-bit resolution
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_tx311, CONFIG_DAC_LOG_LEVEL);

#define DACX311_MIN_RESOLUTION 8U
#define DACX311_MAX_RESOLUTION 14U

#define DACX311_MAX_CHANNEL    1U

struct dacx311_config {
	struct spi_dt_spec bus;
	uint8_t  resolution;
	uint32_t max_freq_khz;
	uint8_t  power_down_mode;
};

struct dacx311_data {
	uint8_t resolution;
	uint16_t power_down_mode;
	uint8_t configured;
};

static int dacx311_reg_write(const struct device *dev, uint16_t val)
{
	const struct dacx311_config *cfg = dev->config;
	uint8_t tx_bytes[2];

	/* Construct write buffer for SPI API */
	const struct spi_buf tx_buf[1] = {
		{
			.buf = tx_bytes,
			.len = sizeof(tx_bytes)
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf)
	};

	/* Set register bits */
	tx_bytes[0] = val >> 8;
	tx_bytes[1] = val & 0xFF;

	/* Write to bus */
	return spi_write_dt(&cfg->bus, &tx);
}

static int dacx311_channel_setup(const struct device *dev,
				const struct dac_channel_cfg *channel_cfg)
{
	const struct dacx311_config *config = dev->config;
	struct dacx311_data *data = dev->data;

	/* Validate configuration */
	if (channel_cfg->channel_id > (DACX311_MAX_CHANNEL - 1)) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	if (1000 * config->max_freq_khz < config->bus.config.frequency) {
		LOG_ERR("SCLK frequency too high: '%d' Hz", config->bus.config.frequency);
		return -ENOTSUP;
	}

	if ((config->resolution > DACX311_MAX_RESOLUTION) ||
			(config->resolution < DACX311_MIN_RESOLUTION)) {
		LOG_ERR("Unsupported resolution %d", config->resolution);
		return -ENOTSUP;
	}

	if (config->power_down_mode > 3) {
		LOG_ERR("Unsupported power mode %d", config->power_down_mode);
		return -ENOTSUP;
	}

	if (data->configured & BIT(channel_cfg->channel_id)) {
		LOG_DBG("Channel %d already configured", channel_cfg->channel_id);
		return 0;
	}

	/* Mark channel as configured */
	data->configured |= BIT(channel_cfg->channel_id);

	/* Set bit resolution for this chip variant */
	data->resolution = config->resolution;

	/* Set the power mode (shifted to upper bits) */
	data->power_down_mode = FIELD_PREP(BIT_MASK(2) << DACX311_MAX_RESOLUTION,
			config->power_down_mode);

	LOG_DBG("Channel %d initialized", channel_cfg->channel_id);

	return 0;
}

static int dacx311_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	struct dacx311_data *data = dev->data;
	uint16_t regval;
	uint8_t shift;
	int ret;

	const bool brdcast = (channel == DAC_CHANNEL_BROADCAST) ? 1 : 0;

	if (!brdcast && (channel > (DACX311_MAX_CHANNEL - 1))) {
		LOG_ERR("Unsupported channel %d", channel);
		return -ENOTSUP;
	}

	/*
	 * Check if channel is initialized
	 * If broadcast channel is used, check if any channel is initialized
	 */
	if ((brdcast && !data->configured) ||
			(channel < DACX311_MAX_CHANNEL && !(data->configured & BIT(channel)))) {
		LOG_ERR("Channel %d not initialized", channel);
		return -EINVAL;
	}

	if (value >= (1 << (data->resolution))) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	/*
	 * (See https://www.ti.com/document-viewer/dac6311/datasheet)
	 *
	 * Shift given value to align MSB bit position to register bit 13.
	 *
	 * DAC output register format:
	 *
	 * | 15 14 | 13 12 11 10  9  8  7  6  5  4  3  2  1  0         |
	 * |-------|---------------------------------------------------|
	 * | Mode  | 8311[13:0] / 7311[13:2] / 6311[13:4] / 5311[13:6] |
	 */
	shift = DACX311_MAX_RESOLUTION - data->resolution;
	regval = value << shift;

	/*
	 * Set mode bits to value taken from configuration.
	 *
	 * MODE = 0 0  -> Normal Operation
	 *        0 1  -> Output 1 kΩ to GND
	 *        1 0  -> Output 100 kΩ to GND
	 *        1 1  -> High-Z
	 */
	regval &= 0x3FFF;
	regval |= data->power_down_mode;

	/* Write to output */
	ret = dacx311_reg_write(dev, regval);
	if (ret) {
		LOG_ERR("Unable to set value %d on channel %d", value, channel);
		return -EIO;
	}

	return 0;
}

static DEVICE_API(dac, dacx311_driver_api) = {
	.channel_setup = dacx311_channel_setup,
	.write_value = dacx311_write_value
};

#define INST_DT_DACX311(inst, t) DT_INST(inst, ti_dac##t)

#define DACX311_DEVICE(t, n, res) \
	static struct dacx311_data dac##t##_data_##n; \
	static const struct dacx311_config dac##t##_config_##n = { \
		.bus = SPI_DT_SPEC_GET(INST_DT_DACX311(n, t), \
					SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
					SPI_MODE_CPHA | \
					SPI_WORD_SET(16), 0),  \
		.resolution = res, \
		.power_down_mode = DT_INST_ENUM_IDX(n, power_down_mode), \
		.max_freq_khz = 50000,		\
	}; \
	DEVICE_DT_DEFINE(INST_DT_DACX311(n, t), \
			NULL, NULL, \
			&dac##t##_data_##n,			   \
			&dac##t##_config_##n, POST_KERNEL,	   \
			CONFIG_DAC_INIT_PRIORITY,		   \
			&dacx311_driver_api)

/*
 * DAC8311: 14-bit
 */
#define DAC8311_DEVICE(n) DACX311_DEVICE(8311, n, 14)
/*
 * DAC7311: 12-bit
 */
#define DAC7311_DEVICE(n) DACX311_DEVICE(7311, n, 12)
/*
 * DAC6311: 10-bit
 */
#define DAC6311_DEVICE(n) DACX311_DEVICE(6311, n, 10)
/*
 * DAC5311: 8-bit
 */
#define DAC5311_DEVICE(n) DACX311_DEVICE(5311, n, 8)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_DACX311_FOREACH(t, inst_expr) \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(ti_dac##t), \
		CALL_WITH_ARG, (), inst_expr)

#define DT_DRV_COMPAT ti_dac8311
INST_DT_DACX311_FOREACH(8311, DAC8311_DEVICE);
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_dac7311
INST_DT_DACX311_FOREACH(7311, DAC7311_DEVICE);
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_dac6311
INST_DT_DACX311_FOREACH(6311, DAC6311_DEVICE);
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_dac5311
INST_DT_DACX311_FOREACH(5311, DAC5311_DEVICE);
#undef DT_DRV_COMPAT
