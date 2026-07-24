/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(dac_dacx3508, CONFIG_DAC_LOG_LEVEL);

#define DACX3508_REG_CONFIG      0x01U
#define DACX3508_REG_TRIGGER     0x02U
#define DACX3508_REG_DAC0        0x08U

#define DACX3508_MASK_TRIGGER_SOFT_RESET     (BIT(1) | BIT(3))

#define DACX3508_MAX_CHANNEL    8

struct dacx3508_config {
	struct spi_dt_spec bus;
	uint8_t resolution;
};

struct dacx3508_data {
	uint8_t configured;
};

static int dacx3508_reg_write(const struct device *dev, uint8_t addr, uint8_t *data)
{
	const struct dacx3508_config *config = dev->config;
	const struct spi_buf buf[2] = {
		{
			.buf = &addr,
			.len = sizeof(addr)
		},
		{
			.buf = data,
			.len = 2
		}
	};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	if (k_is_in_isr()) {
		/* Prevent SPI transactions from an ISR */
		return -EWOULDBLOCK;
	}

	return spi_write_dt(&config->bus, &tx);
}

static int dacx3508_channel_setup(const struct device *dev,
	const struct dac_channel_cfg *channel_cfg)
{
	const struct dacx3508_config *config = dev->config;
	struct dacx3508_data *data = dev->data;
	uint8_t regval[2] = { 0 };


	if (channel_cfg->channel_id > DACX3508_MAX_CHANNEL - 1) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	if (channel_cfg->internal) {
		LOG_ERR("Internal channels not supported");
		return -ENOTSUP;
	}

	regval[1] = ~(data->configured | BIT(channel_cfg->channel_id));

	int ret = dacx3508_reg_write(dev, DACX3508_REG_CONFIG, regval);

	if (ret) {
		return -EIO;
	}

	data->configured |= BIT(channel_cfg->channel_id);

	return 0;
}

static int dacx3508_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dacx3508_config *config = dev->config;
	struct dacx3508_data *data = dev->data;
	uint8_t regval[2];
	int ret;

	if (channel > DACX3508_MAX_CHANNEL - 1) {
		LOG_ERR("unsupported channel %d", channel);
		return -ENOTSUP;
	}

	if (!(data->configured & BIT(channel))) {
		LOG_ERR("Channel not initialized");
		return -EINVAL;
	}

	if (value >= (1 << config->resolution)) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	regval[0] = value >> 8;
	regval[1] = value & 0xFF;

	ret = dacx3508_reg_write(dev, DACX3508_REG_DAC0 + channel, regval);
	if (ret) {
		return -EIO;
	}

	return 0;
}

static int dacx3508_soft_reset(const struct device *dev)
{
	uint8_t regval[2] = {0, DACX3508_MASK_TRIGGER_SOFT_RESET};
	int ret;

	ret = dacx3508_reg_write(dev, DACX3508_REG_TRIGGER, regval);
	if (ret) {
		return -EIO;
	}

	return 0;
}

static int dacx3508_init(const struct device *dev)
{
	const struct dacx3508_config *config = dev->config;
	struct dacx3508_data *data = dev->data;
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	ret = dacx3508_soft_reset(dev);
	if (ret) {
		LOG_ERR("Soft-reset failed");
		return ret;
	}

	data->configured = 0;

	return 0;
}

static DEVICE_API(dac, dacx3508_driver_api) = {
	.channel_setup = dacx3508_channel_setup,
	.write_value = dacx3508_write_value,
};

#define INST_DT_DACX3508(inst, t) DT_INST(inst, ti_dac##t)

#define DACX3508_DEVICE(t, n, res) \
	static struct dacx3508_data dac##t##_data_##n; \
	static const struct dacx3508_config dac##t##_config_##n = { \
		.bus = SPI_DT_SPEC_GET(INST_DT_DACX3508(n, t), \
			SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
			SPI_WORD_SET(8) | SPI_MODE_CPHA), \
		.resolution = res, \
	}; \
	DEVICE_DT_DEFINE(INST_DT_DACX3508(n, t), \
			    &dacx3508_init, NULL, \
			    &dac##t##_data_##n, \
			    &dac##t##_config_##n, POST_KERNEL, \
			    CONFIG_DAC_DACX3508_INIT_PRIORITY, \
			    &dacx3508_driver_api);

/*
 * DAC43508: 8-bit
 */
#define DAC43508_DEVICE(n) DACX3508_DEVICE(43508, n, 8)

/*
 * DAC53508: 10-bit
 */
#define DAC53508_DEVICE(n) DACX3508_DEVICE(53508, n, 10)

/*
 * DAC63508: 12-bit
 */
#define DAC63508_DEVICE(n) DACX3508_DEVICE(63508, n, 12)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_DACX3508_FOREACH(t, inst_expr) \
	LISTIFY(DT_NUM_INST_STATUS_OKAY(ti_dac##t), \
		     CALL_WITH_ARG, (), inst_expr)

INST_DT_DACX3508_FOREACH(43508, DAC43508_DEVICE);
INST_DT_DACX3508_FOREACH(53508, DAC53508_DEVICE);
INST_DT_DACX3508_FOREACH(63508, DAC63508_DEVICE);
