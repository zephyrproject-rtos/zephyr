/*
 * Copyright (c) 2020 M2I Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/spi.h>
#include <drivers/dac.h>
#include <logging/log.h>
#include <dt-bindings/dac/dacx0508.h>

LOG_MODULE_REGISTER(dac_dacx0508, CONFIG_DAC_LOG_LEVEL);

#define DACX0508_REG_DEVICE_ID   0x01U
#define DACX0508_REG_CONFIG      0x03U
#define DACX0508_REG_GAIN        0x04U
#define DACX0508_REG_TRIGGER     0x05U
#define DACX0508_REG_STATUS      0x07U
#define DACX0508_REG_DAC0        0x08U

#define DACX0508_MASK_DEVICE_ID_8CH          BIT(11)
#define DACX0508_MASK_CONFIG_REF_PWDWN       BIT(8)
#define DACX0508_MASK_GAIN_BUFF_GAIN(x)      BIT(x)
#define DACX0508_MASK_GAIN_REFDIV_EN         BIT(8)
#define DACX0508_MASK_TRIGGER_SOFT_RESET     (BIT(1) | BIT(3))
#define DACX0508_MASK_STATUS_REF_ALM         BIT(0)

#define DACX0508_READ_CMD       0x80
#define DACX0508_POR_DELAY      250
#define DACX0508_MAX_CHANNEL    8

struct dacx0508_config {
	const char *spi_dev_name;
	const char *spi_cs_dev_name;
	gpio_pin_t spi_cs_pin;
	gpio_dt_flags_t spi_cs_dt_flags;
	struct spi_config spi_cfg;
	uint8_t resolution;
	uint8_t reference;
	uint8_t gain[8];
};

struct dacx0508_data {
	const struct device *spi_dev;
	struct spi_cs_control spi_cs;
	struct spi_config spi_cfg;
	uint8_t configured;
};

static int dacx0508_reg_read(const struct device *dev, uint8_t addr,
			     uint8_t *data)
{
	struct dacx0508_data *dev_data = dev->data;
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
	struct spi_buf_set rx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf)
	};
	uint8_t tmp;
	int ret;

	if (k_is_in_isr()) {
		/* Prevent SPI transactions from an ISR */
		return -EWOULDBLOCK;
	}

	tmp = addr |= DACX0508_READ_CMD;

	ret = spi_write(dev_data->spi_dev, &dev_data->spi_cfg, &tx);
	if (ret) {
		return ret;
	}

	ret = spi_read(dev_data->spi_dev, &dev_data->spi_cfg, &rx);
	if (ret) {
		return ret;
	}

	if (addr != tmp) {
		return -EIO;
	}

	return 0;
}

static int dacx0508_reg_write(const struct device *dev, uint8_t addr,
			      	uint8_t *data)
{
	struct dacx0508_data *dev_data = dev->data;
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

	return spi_write(dev_data->spi_dev, &dev_data->spi_cfg, &tx);
}

int dacx0508_reg_update(const struct device *dev, uint8_t addr,
			 uint16_t mask, bool setting)
{
	uint8_t regval[2] = {0, };
	uint16_t tmp;
	int ret;

	ret = dacx0508_reg_read(dev, addr, regval);
	if (ret < 0) {
		return ret;
	}
	tmp = (regval[0] << 8) | regval[1];

	if (setting) {
		tmp |= mask;
	} else {
		tmp &= ~mask;
	}

	regval[0] = tmp >> 8;
	regval[1] = tmp & 0xFF;

	ret = dacx0508_reg_write(dev, addr, regval);
	if (ret) {
		return ret;
	}

	return 0;
}

static int dacx0508_channel_setup(const struct device *dev,
				   const struct dac_channel_cfg *channel_cfg)
{
	const struct dacx0508_config *config = dev->config;
	struct dacx0508_data *data = dev->data;

	if (channel_cfg->channel_id > DACX0508_MAX_CHANNEL - 1) {
		LOG_ERR("Unsupported channel %d", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %d", channel_cfg->resolution);
		return -ENOTSUP;
	}

	data->configured |= BIT(channel_cfg->channel_id);

	return 0;
}

static int dacx0508_write_value(const struct device *dev, uint8_t channel,
				uint32_t value)
{
	const struct dacx0508_config *config = dev->config;
	struct dacx0508_data *data = dev->data;
	uint8_t regval[2];
	int ret;

	if (channel > DACX0508_MAX_CHANNEL - 1) {
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

	value <<= (16 - config->resolution);
	regval[0] = value >> 8;
	regval[1] = value & 0xFF;

	ret = dacx0508_reg_write(dev, DACX0508_REG_DAC0 + channel, regval);
	if (ret) {
		return -EIO;
	}

	return 0;
}

static int dacx0508_soft_reset(const struct device *dev)
{
	uint8_t regval[2] = {0, DACX0508_MASK_TRIGGER_SOFT_RESET};
	int ret;

	ret = dacx0508_reg_write(dev, DACX0508_REG_TRIGGER, regval);
	if (ret) {
		return -EIO;
	}
	k_usleep(DACX0508_POR_DELAY);

	return 0;
}

static int dacx0508_device_id_check(const struct device *dev)
{
	const struct dacx0508_config *config = dev->config;
	uint8_t regval[2] = {0, };
	uint8_t resolution;
	uint16_t dev_id;
	int ret;

	ret = dacx0508_reg_read(dev, DACX0508_REG_DEVICE_ID, regval);
	if (ret) {
		LOG_ERR("Unable to read Device ID");
		return -EIO;
	}
	dev_id = (regval[0] << 8) | regval[1];

	resolution = dev_id >> 12;
	if (resolution != (16 - config->resolution) >> 1) {
		LOG_ERR("Not match chip resolution");
		return -EINVAL;
	}

	if ((dev_id & DACX0508_MASK_DEVICE_ID_8CH) !=
				DACX0508_MASK_DEVICE_ID_8CH) {
		LOG_ERR("Support channels mismatch");
		return -EINVAL;
	}

	return 0;
}

static int dacx0508_setup(const struct device *dev)
{
	const struct dacx0508_config *config = dev->config;
	uint8_t regval[2] = {0, }, tmp = 0;
	bool ref_pwdwn, refdiv_en;
	int ret;

	switch (config->reference) {
	case DACX0508_REF_INTERNAL_1:
		ref_pwdwn = false;
		refdiv_en = false;
		break;
	case DACX0508_REF_INTERNAL_1_2:
		ref_pwdwn = false;
		refdiv_en = true;
		break;
	case DACX0508_REF_EXTERNAL_1:
		ref_pwdwn = true;
		refdiv_en = false;
		break;
	case DACX0508_REF_EXTERNAL_1_2:
		ref_pwdwn = true;
		refdiv_en = true;
		break;
	default:
		LOG_ERR("unsupported channel reference type '%d'",
			config->reference);
		return -ENOTSUP;
	}

	ret = dacx0508_reg_update(dev, DACX0508_REG_CONFIG,
				  DACX0508_MASK_CONFIG_REF_PWDWN, ref_pwdwn);
	if (ret) {
		LOG_ERR("GAIN Register update failed");
		return -EIO;
	}

	ret = dacx0508_reg_update(dev, DACX0508_REG_GAIN,
				  DACX0508_MASK_GAIN_REFDIV_EN, refdiv_en);
	if (ret) {
		LOG_ERR("GAIN Register update failed");
		return -EIO;
	}


	for (int i = 0; i < 8; i++) {
		tmp |= config->gain[i] << i;
	}

	ret = dacx0508_reg_read(dev, DACX0508_REG_GAIN, regval);
	if (ret) {
		LOG_ERR("Unable to read GAIN Register");
		return -EIO;
	}

	regval[1] = tmp;
	ret = dacx0508_reg_write(dev, DACX0508_REG_GAIN, regval);
	if (ret) {
		LOG_ERR("Unable to write GAIN Register");
		return -EIO;
	}

	ret = dacx0508_reg_read(dev, DACX0508_REG_STATUS, regval);
	if (ret) {
		LOG_ERR("Unable to read STATUS Register");
		return -EIO;
	}
	if ((regval[1] & DACX0508_MASK_STATUS_REF_ALM) ==
				DACX0508_MASK_STATUS_REF_ALM) {
		LOG_ERR("Difference between VREF/DIV and VDD is "
			"below the required minimum analog threshold");
		return -EIO;
	}

	return 0;
}

static int dacx0508_init(const struct device *dev)
{
	const struct dacx0508_config *config = dev->config;
	struct dacx0508_data *data = dev->data;
	int ret;

	data->spi_dev = device_get_binding(config->spi_dev_name);
	if (!data->spi_dev) {
		LOG_ERR("Cannot get pointer to %s device",
			config->spi_dev_name);
		return -EINVAL;
	}

	if (config->spi_cs_dev_name) {
		data->spi_cs.gpio_dev =
				device_get_binding(config->spi_cs_dev_name);
		if (!data->spi_cs.gpio_dev) {
			LOG_ERR("Cannot get pointer to %s device",
				config->spi_cs_dev_name);
			return -EINVAL;
		}
		data->spi_cs.gpio_pin = config->spi_cs_pin;
		data->spi_cs.gpio_dt_flags = config->spi_cs_dt_flags;
		data->spi_cfg = config->spi_cfg;
		data->spi_cfg.cs = &data->spi_cs;
	}

	ret = dacx0508_soft_reset(dev);
	if (ret) {
		LOG_ERR("Soft-reset failed");
		return ret;
	}

	ret = dacx0508_device_id_check(dev);
	if (ret) {
		return ret;
	}

	ret = dacx0508_setup(dev);
	if (ret) {
		return ret;
	}

	data->configured = 0;

	return 0;
}

static const struct dac_driver_api dacx0508_driver_api = {
	.channel_setup = dacx0508_channel_setup,
	.write_value = dacx0508_write_value,
};

#define INST_DT_DACX0508(inst, t) DT_INST(inst, ti_dac##t)

#define DACX0508_DEVICE(t, n, res) \
	static struct dacx0508_data dac##t##_data_##n; \
	static const struct dacx0508_config dac##t##_config_##n = { \
		.spi_dev_name = DT_BUS_LABEL(INST_DT_DACX0508(n, t)), \
		.spi_cs_dev_name = \
			UTIL_AND( \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_DACX0508(n, t)), \
			DT_SPI_DEV_CS_GPIOS_LABEL(INST_DT_DACX0508(n, t)) \
			), \
		.spi_cs_pin = \
			UTIL_AND( \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_DACX0508(n, t)), \
			DT_SPI_DEV_CS_GPIOS_PIN(INST_DT_DACX0508(n, t)) \
			), \
		.spi_cs_dt_flags = UTIL_AND( \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_DACX0508(n, t)), \
			DT_SPI_DEV_CS_GPIOS_FLAGS(INST_DT_DACX0508(n, t)) \
			), \
		.spi_cfg = { \
			.operation = (SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | \
				     SPI_WORD_SET(8) | SPI_MODE_CPHA), \
			.frequency = DT_PROP(INST_DT_DACX0508(n, t), \
					     spi_max_frequency), \
			.slave = DT_REG_ADDR(INST_DT_DACX0508(n, t)), \
			.cs = &dac##t##_data_##n.spi_cs, \
		}, \
		.resolution = res, \
		.reference = DT_PROP(INST_DT_DACX0508(n, t), \
					     voltage_reference), \
		.gain[0] = DT_PROP(INST_DT_DACX0508(n, t), channel0_gain), \
		.gain[1] = DT_PROP(INST_DT_DACX0508(n, t), channel1_gain), \
		.gain[2] = DT_PROP(INST_DT_DACX0508(n, t), channel2_gain), \
		.gain[3] = DT_PROP(INST_DT_DACX0508(n, t), channel3_gain), \
		.gain[4] = DT_PROP(INST_DT_DACX0508(n, t), channel4_gain), \
		.gain[5] = DT_PROP(INST_DT_DACX0508(n, t), channel5_gain), \
		.gain[6] = DT_PROP(INST_DT_DACX0508(n, t), channel6_gain), \
		.gain[7] = DT_PROP(INST_DT_DACX0508(n, t), channel7_gain), \
	}; \
	DEVICE_DT_DEFINE(INST_DT_DACX0508(n, t), \
			    &dacx0508_init, NULL, \
			    &dac##t##_data_##n, \
			    &dac##t##_config_##n, POST_KERNEL, \
			    CONFIG_DAC_DACX0508_INIT_PRIORITY, \
			    &dacx0508_driver_api)

/*
 * DAC60508: 12-bit
 */
#define DAC60508_DEVICE(n) DACX0508_DEVICE(60508, n, 12)

/*
 * DAC70508: 14-bit
 */
#define DAC70508_DEVICE(n) DACX0508_DEVICE(70508, n, 14)

/*
 * DAC80508: 16-bit
 */
#define DAC80508_DEVICE(n) DACX0508_DEVICE(80508, n, 16)

#define CALL_WITH_ARG(arg, expr) expr(arg)

#define INST_DT_DACX0508_FOREACH(t, inst_expr) \
	UTIL_LISTIFY(DT_NUM_INST_STATUS_OKAY(ti_dac##t), \
		     CALL_WITH_ARG, inst_expr)

INST_DT_DACX0508_FOREACH(60508, DAC60508_DEVICE);
INST_DT_DACX0508_FOREACH(70508, DAC70508_DEVICE);
INST_DT_DACX0508_FOREACH(80508, DAC80508_DEVICE);
