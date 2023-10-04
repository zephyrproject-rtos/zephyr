/*
 * Driver for Linear Technology LTC1660/LTC1665  DAC
 *
 * Copyright (C) 2023 Marcus Folkesson <marcus.folkesson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_ltc166x, CONFIG_DAC_LOG_LEVEL);

#define LTC166X_REG_MASK               GENMASK(15, 12)
#define LTC166X_DATA8_MASK             GENMASK(11, 4)
#define LTC166X_DATA10_MASK            GENMASK(12, 2)

struct ltc166x_config {
	struct spi_dt_spec bus;
	uint8_t resolution;
	uint8_t nchannels;
};

static int ltc166x_reg_write(const struct device *dev, uint8_t addr,
			uint32_t data)
{
	const struct ltc166x_config *config = dev->config;
	uint16_t regval;

	regval = FIELD_PREP(LTC166X_REG_MASK, addr);

	if (config->resolution == 10) {
		regval |= FIELD_PREP(LTC166X_DATA10_MASK, data);
	} else {
		regval |= FIELD_PREP(LTC166X_DATA8_MASK, data);
	}

	const struct spi_buf buf = {
			.buf = &regval,
			.len = sizeof(regval),
	};

	struct spi_buf_set tx = {
		.buffers = &buf,
		.count = 1,
	};

	return spi_write_dt(&config->bus, &tx);
}


static int ltc166x_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct ltc166x_config *config = dev->config;

	if (channel_cfg->channel_id > config->nchannels - 1) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	return 0;
}

static int ltc166x_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct ltc166x_config *config = dev->config;

	if (channel > config->nchannels - 1) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (value >= (1 << config->resolution)) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	return ltc166x_reg_write(dev, channel + 1, value);
}

static int ltc166x_init(const struct device *dev)
{
	const struct ltc166x_config *config = dev->config;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}
	return 0;
}

static const struct dac_driver_api ltc166x_driver_api = {
	.channel_setup = ltc166x_channel_setup,
	.write_value = ltc166x_write_value,
};


#define INST_DT_LTC166X(inst, t) DT_INST(inst, lltc_ltc##t)

#define LTC166X_DEVICE(t, n, res, nchan) \
	static const struct ltc166x_config ltc##t##_config_##n = { \
		.bus = SPI_DT_SPEC_GET(INST_DT_LTC166X(n, t), \
			SPI_OP_MODE_MASTER | \
			SPI_WORD_SET(8), 0), \
		.resolution = res, \
		.nchannels = nchan, \
	}; \
	DEVICE_DT_DEFINE(INST_DT_LTC166X(n, t), \
			    &ltc166x_init, NULL, \
			    NULL, \
			    &ltc##t##_config_##n, POST_KERNEL, \
			    CONFIG_DAC_LTC166X_INIT_PRIORITY, \
			    &ltc166x_driver_api)

/*
 * LTC1660: 10-bit
 */
#define LTC1660_DEVICE(n) LTC166X_DEVICE(1660, n, 10, 8)

/*
 * LTC1665: 8-bit
 */
#define LTC1665_DEVICE(n) LTC166X_DEVICE(1665, n, 8, 8)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_LTC166X_FOREACH(t, inst_expr) \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(lltc_ltc##t), \
		     CALL_WITH_ARG, (), inst_expr)

INST_DT_LTC166X_FOREACH(1660, LTC1660_DEVICE);
INST_DT_LTC166X_FOREACH(1665, LTC1665_DEVICE);
