/*
 * Copyright (c) 2025  Andreas Wolf <awolf002@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file  dac_dacx311.c
 * @brief Driver for the TI x311 and x411 single channel DAC chips.
 *
 * This driver supports multiple variants of the Texas Instrument DAC chip.
 *
 *  - 16-bit write register size -
 *  DAC5311 : 8-bit resolution
 *  DAC6311 : 10-bit resolution
 *  DAC7311 : 12-bit resolution
 *  DAC8311 : 14-bit resolution
 *
 *  - 24-bit write register size -
 *  DAC8411 : 16-bit resolution
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_dacx311, CONFIG_DAC_LOG_LEVEL);

#define DAC8411_RESOLUTION     16U
#define DAC8311_RESOLUTION     14U
#define DAC7311_RESOLUTION     12U
#define DAC6311_RESOLUTION     10U
#define DAC5311_RESOLUTION     8U

#define DACX311_MAX_CHANNEL    1U

#define DACX311_SPI_HZ_MAX     MHZ(50)

struct dacx311_config {
	struct spi_dt_spec bus;
	uint8_t  resolution;
	uint8_t  power_down_mode;
	uint8_t  reg_size;
};

struct dacx311_data {
	uint32_t limit_value;
	uint32_t power_down_mode;
	uint8_t bit_shift;
	uint8_t configured;
};

static int dac_xx11_reg_write(const struct device *dev, uint32_t val)
{
	const struct dacx311_config *cfg = dev->config;
	const bool reg_24bit = (cfg->reg_size > 16);
	uint8_t tx_bytes[3];

	/* Construct write buffer for SPI API */
	const struct spi_buf tx_buf[1] = {
		{
			/* skips tx_bytes[0] if not needed */
			.buf = &tx_bytes[((reg_24bit)?0:1)],
			.len = (reg_24bit)?3:2
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = ARRAY_SIZE(tx_buf)
	};

	/* Set register bits */
	if (reg_24bit) {
		tx_bytes[0] = val >> 16;
	}

	tx_bytes[1] = (val >> 8) & 0xFF;
	tx_bytes[2] = val & 0xFF;

	/* Write to bus */
	return spi_write_dt(&cfg->bus, &tx);
}

static int dac_xx11_channel_setup(const struct device *dev,
			const struct dac_channel_cfg *channel_cfg)
{
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

	if (data->configured & BIT(channel_cfg->channel_id)) {
		LOG_DBG("Channel %d already configured", channel_cfg->channel_id);
		return 0;
	}

	/* Mark channel as configured */
	data->configured |= BIT(channel_cfg->channel_id);

	LOG_DBG("Channel %d initialized", channel_cfg->channel_id);

	return 0;
}

static int dac_xx11_write_value(const struct device *dev, uint8_t channel,
			uint32_t value)
{
	struct dacx311_data *data = dev->data;
	uint32_t regval = 0;
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

	if (value >= data->limit_value) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	/*
	 * (See https://www.ti.com/document-viewer/dac6311/datasheet)
	 *
	 * Shift given value to align MSB bit position to register bit 13.
	 *
	 * DAC output register format (16 bits):
	 *
	 * | 15 14 | 13 12 11 10  9  8  7  6  5  4  3  2  1  0         |
	 * |-------|---------------------------------------------------|
	 * | Mode  | 8311[13:0] / 7311[13:2] / 6311[13:4] / 5311[13:6] |
	 *
	 *-------------------------------------------------------------------------------
	 *
	 * (See https://www.ti.com/document-viewer/dac8411/datasheet)
	 *
	 * Shift given value to align MSB bit position to register bit 21.
	 *
	 * DAC output register format (24 bits):
	 *
	 * | 23 22 | 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0 |
	 * |-------|-------------------------------------------------------------------|
	 * | Mode  | 8411[21:6]                                       X  X  X  X  X  X |
	 */
	regval = value << data->bit_shift;

	/*
	 * Set mode bits to value taken from configuration.
	 *
	 * MODE = 0 0  -> Normal Operation
	 *        0 1  -> Output 1 kΩ to GND
	 *        1 0  -> Output 100 kΩ to GND
	 *        1 1  -> High-Z
	 */
	regval |= data->power_down_mode;

	/* Write to output */
	ret = dac_xx11_reg_write(dev, regval);

	if (ret) {
		LOG_ERR("Unable to set value %d on channel %d", value, channel);
		return -EIO;
	}

	return 0;
}

static int dac_xx11_init(const struct device *dev)
{
	const struct dacx311_config *config = dev->config;
	struct dacx311_data *data = dev->data;

	/* Set correct bit shift for the resolution of this chip variant */
	data->bit_shift = config->reg_size - config->resolution - 2;

	/* Lowest value that is outside the range of valid DAC bits */
	data->limit_value = 1 << (config->resolution);

	/* Set the power mode (shifted to upper bits) */
	data->power_down_mode = FIELD_PREP(BIT_MASK(2) << (config->reg_size - 2),
			config->power_down_mode);

	return 0;
}

static DEVICE_API(dac, dac_xx11_driver_api) = {
	.channel_setup = dac_xx11_channel_setup,
	.write_value = dac_xx11_write_value
};

#define INST_DT_DACX311(inst, t) DT_INST(inst, ti_dac##t)

#define DACX311_DEVICE(t, n, sze, res)                          \
	BUILD_ASSERT(DT_INST_ENUM_IDX(n, power_down_mode) <= 3, \
				"Invalid power down mode");\
	BUILD_ASSERT(DT_PROP(INST_DT_DACX311(n, t), spi_max_frequency) \
				<= DACX311_SPI_HZ_MAX, "Invalid SPI frequency");\
	static struct dacx311_data dac##t##_data_##n; \
	static const struct dacx311_config dac##t##_config_##n = { \
		.bus = SPI_DT_SPEC_GET(INST_DT_DACX311(n, t), \
					SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
					SPI_MODE_CPHA | \
					SPI_WORD_SET(8)),  \
		.resolution = res, \
		.reg_size = sze,                                         \
		.power_down_mode = DT_INST_ENUM_IDX(n, power_down_mode), \
	}; \
	DEVICE_DT_DEFINE(INST_DT_DACX311(n, t), \
			dac_xx11_init, NULL, \
			&dac##t##_data_##n,			   \
			&dac##t##_config_##n, POST_KERNEL,	   \
			CONFIG_DAC_INIT_PRIORITY,		   \
			&dac_xx11_driver_api)

/*
 * DAC8411: 16-bit
 */
#define DAC8411_DEVICE(n) DACX311_DEVICE(8411, n, 24, DAC8411_RESOLUTION)
/*
 * DAC8311: 14-bit
 */
#define DAC8311_DEVICE(n) DACX311_DEVICE(8311, n, 16, DAC8311_RESOLUTION)
/*
 * DAC7311: 12-bit
 */
#define DAC7311_DEVICE(n) DACX311_DEVICE(7311, n, 16, DAC7311_RESOLUTION)
/*
 * DAC6311: 10-bit
 */
#define DAC6311_DEVICE(n) DACX311_DEVICE(6311, n, 16, DAC6311_RESOLUTION)
/*
 * DAC5311: 8-bit
 */
#define DAC5311_DEVICE(n) DACX311_DEVICE(5311, n, 16, DAC5311_RESOLUTION)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_DACX311_FOREACH(t, inst_expr) \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(ti_dac##t), \
		CALL_WITH_ARG, (), inst_expr)

#define DT_DRV_COMPAT ti_dac8411
INST_DT_DACX311_FOREACH(8411, DAC8411_DEVICE);
#undef DT_DRV_COMPAT
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
