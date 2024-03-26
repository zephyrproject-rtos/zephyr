/*
 * Copyright (c) 2023 Grinn
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad559x_dac

#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/drivers/mfd/ad559x.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dac_ad559x, CONFIG_DAC_LOG_LEVEL);

#define AD559X_DAC_RESOLUTION        12
#define AD559X_DAC_WR_POINTER        0x10
#define AD559X_DAC_WR_MSB_BIT        BIT(15)
#define AD559X_DAC_CHANNEL_SHIFT_VAL 12

struct dac_ad559x_config {
	const struct device *mfd_dev;
};

struct dac_ad559x_data {
	uint8_t dac_conf;
};

static int dac_ad559x_channel_setup(const struct device *dev,
				    const struct dac_channel_cfg *channel_cfg)
{
	const struct dac_ad559x_config *config = dev->config;
	struct dac_ad559x_data *data = dev->data;

	if (channel_cfg->channel_id >= AD559X_PIN_MAX) {
		LOG_ERR("Invalid channel number %d", channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->resolution != AD559X_DAC_RESOLUTION) {
		LOG_ERR("Invalid resolution %d", channel_cfg->resolution);
		return -EINVAL;
	}

	data->dac_conf |= BIT(channel_cfg->channel_id);

	return mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_LDAC_EN, data->dac_conf);
}

static int dac_ad559x_write_value(const struct device *dev, uint8_t channel, uint32_t value)
{
	const struct dac_ad559x_config *config = dev->config;
	uint16_t msg;

	if (channel >= AD559X_PIN_MAX) {
		LOG_ERR("Invalid channel number %d", channel);
		return -EINVAL;
	}

	if (value >= (1 << AD559X_DAC_RESOLUTION)) {
		LOG_ERR("Value %d out of range", value);
		return -EINVAL;
	}

	if (mfd_ad559x_has_pointer_byte_map(config->mfd_dev)) {
		return mfd_ad559x_write_reg(config->mfd_dev, AD559X_DAC_WR_POINTER | channel,
					    value);
	} else {
		msg = sys_cpu_to_be16(AD559X_DAC_WR_MSB_BIT |
				      channel << AD559X_DAC_CHANNEL_SHIFT_VAL | value);

		return mfd_ad559x_write_raw(config->mfd_dev, (uint8_t *)&msg, sizeof(msg));
	}
}

static const struct dac_driver_api dac_ad559x_api = {
	.channel_setup = dac_ad559x_channel_setup,
	.write_value = dac_ad559x_write_value,
};

static int dac_ad559x_init(const struct device *dev)
{
	const struct dac_ad559x_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->mfd_dev)) {
		return -ENODEV;
	}

	ret = mfd_ad559x_write_reg(config->mfd_dev, AD559X_REG_PD_REF_CTRL, AD559X_EN_REF);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

#define DAC_AD559X_DEFINE(inst)                                                                    \
	static const struct dac_ad559x_config dac_ad559x_config##inst = {                          \
		.mfd_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                    \
	};                                                                                         \
                                                                                                   \
	struct dac_ad559x_data dac_ad559x_data##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dac_ad559x_init, NULL, &dac_ad559x_data##inst,                 \
			      &dac_ad559x_config##inst, POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,     \
			      &dac_ad559x_api);

DT_INST_FOREACH_STATUS_OKAY(DAC_AD559X_DEFINE)
