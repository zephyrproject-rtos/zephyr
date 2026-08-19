/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT m5stack_m5pm1_adc

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/mfd/m5pm1.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_m5pm1, CONFIG_ADC_LOG_LEVEL);

#define M5PM1_ADC_RESOLUTION 12U

#define M5PM1_REG_VREF_L    0x20
#define M5PM1_REG_VBAT_L    0x22
#define M5PM1_REG_VIN_L     0x24
#define M5PM1_REG_5VOUT_L   0x26
#define M5PM1_REG_ADC_RES_L 0x28
#define M5PM1_REG_ADC_CTRL  0x2a

#define M5PM1_ADC_CTRL_START      BIT(0)
#define M5PM1_ADC_CTRL_CH_SEL(ch) ((uint8_t)((ch) << 1))

#define M5PM1_ADC_HW_CH_GPIO1 1U
#define M5PM1_ADC_HW_CH_GPIO2 2U
#define M5PM1_ADC_HW_CH_TEMP  6U

#define M5PM1_ADC_POLL_INTERVAL_MS 1
#define M5PM1_ADC_POLL_MAX         50

enum adc_m5pm1_channel {
	ADC_M5PM1_CHANNEL_VREF = 0,
	ADC_M5PM1_CHANNEL_VBAT,
	ADC_M5PM1_CHANNEL_VIN,
	ADC_M5PM1_CHANNEL_5VOUT,
	ADC_M5PM1_CHANNEL_GPIO1,
	ADC_M5PM1_CHANNEL_GPIO2,
	ADC_M5PM1_CHANNEL_TEMP,
	ADC_M5PM1_CHANNEL_MAX = ADC_M5PM1_CHANNEL_TEMP,
};

struct adc_m5pm1_config {
	const struct device *mfd;
};

struct adc_m5pm1_data {
	struct k_mutex lock;
};

static int adc_m5pm1_read_u16(const struct device *mfd, uint8_t reg_l, uint16_t *val)
{
	uint8_t buf[2];
	int ret;

	ret = mfd_m5pm1_burst_read(mfd, reg_l, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_le16(buf);

	return 0;
}

static int adc_m5pm1_convert(const struct device *dev, uint8_t hw_channel, uint16_t *value)
{
	const struct adc_m5pm1_config *config = dev->config;
	struct adc_m5pm1_data *data = dev->data;
	uint8_t ctrl;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = mfd_m5pm1_write_reg(config->mfd, M5PM1_REG_ADC_CTRL,
				  M5PM1_ADC_CTRL_CH_SEL(hw_channel) | M5PM1_ADC_CTRL_START);
	if (ret < 0) {
		goto out;
	}

	for (int i = 0; i < M5PM1_ADC_POLL_MAX; i++) {
		ret = mfd_m5pm1_read_reg(config->mfd, M5PM1_REG_ADC_CTRL, &ctrl);
		if (ret < 0) {
			goto out;
		}

		if ((ctrl & M5PM1_ADC_CTRL_START) == 0U) {
			ret = adc_m5pm1_read_u16(config->mfd, M5PM1_REG_ADC_RES_L, value);
			goto out;
		}

		k_msleep(M5PM1_ADC_POLL_INTERVAL_MS);
	}

	ret = -ETIMEDOUT;
out:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int adc_m5pm1_sample(const struct device *dev, uint8_t channel, uint16_t *value)
{
	const struct adc_m5pm1_config *config = dev->config;

	switch (channel) {
	case ADC_M5PM1_CHANNEL_VREF:
		return adc_m5pm1_read_u16(config->mfd, M5PM1_REG_VREF_L, value);
	case ADC_M5PM1_CHANNEL_VBAT:
		return adc_m5pm1_read_u16(config->mfd, M5PM1_REG_VBAT_L, value);
	case ADC_M5PM1_CHANNEL_VIN:
		return adc_m5pm1_read_u16(config->mfd, M5PM1_REG_VIN_L, value);
	case ADC_M5PM1_CHANNEL_5VOUT:
		return adc_m5pm1_read_u16(config->mfd, M5PM1_REG_5VOUT_L, value);
	case ADC_M5PM1_CHANNEL_GPIO1:
		return adc_m5pm1_convert(dev, M5PM1_ADC_HW_CH_GPIO1, value);
	case ADC_M5PM1_CHANNEL_GPIO2:
		return adc_m5pm1_convert(dev, M5PM1_ADC_HW_CH_GPIO2, value);
	case ADC_M5PM1_CHANNEL_TEMP:
		return adc_m5pm1_convert(dev, M5PM1_ADC_HW_CH_TEMP, value);
	default:
		return -EINVAL;
	}
}

static int adc_m5pm1_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	const struct adc_m5pm1_config *config = dev->config;

	if (channel_cfg->channel_id > ADC_M5PM1_CHANNEL_MAX) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		return -ENOTSUP;
	}

	switch (channel_cfg->channel_id) {
	case ADC_M5PM1_CHANNEL_GPIO1:
		return mfd_m5pm1_pin_request(config->mfd, dev, M5PM1_ADC_HW_CH_GPIO1,
					     M5PM1_PIN_FUNC_ADC);
	case ADC_M5PM1_CHANNEL_GPIO2:
		return mfd_m5pm1_pin_request(config->mfd, dev, M5PM1_ADC_HW_CH_GPIO2,
					     M5PM1_PIN_FUNC_ADC);
	default:
		/*
		 * Other channels do not need to be claimed as they are internal and do not use
		 * external pins
		 */
		return 0;
	}
}

static int adc_m5pm1_read(const struct device *dev, const struct adc_sequence *sequence)
{
	uint16_t *buffer = sequence->buffer;
	uint32_t channels = sequence->channels;
	uint8_t channel;
	size_t needed_size;
	int ret;

	if (sequence->resolution != M5PM1_ADC_RESOLUTION) {
		return -ENOTSUP;
	}

	if (sequence->oversampling != 0 || sequence->options != NULL) {
		return -ENOTSUP;
	}

	if (find_msb_set(channels) > ADC_M5PM1_CHANNEL_MAX + 1U) {
		return -EINVAL;
	}

	needed_size = POPCOUNT(channels) * sizeof(uint16_t);
	if (sequence->buffer_size < needed_size) {
		return -ENOMEM;
	}

	while (channels != 0U) {
		channel = find_lsb_set(channels) - 1U;
		ret = adc_m5pm1_sample(dev, channel, buffer);
		if (ret < 0) {
			return ret;
		}

		buffer++;
		WRITE_BIT(channels, channel, 0);
	}

	return 0;
}

#define M5PM1_ADC_REF_INTERNAL_MV 4096

static DEVICE_API(adc, adc_m5pm1_api) = {
	.channel_setup = adc_m5pm1_channel_setup,
	.read = adc_m5pm1_read,
	.ref_internal = M5PM1_ADC_REF_INTERNAL_MV,
};

static int adc_m5pm1_init(const struct device *dev)
{
	const struct adc_m5pm1_config *config = dev->config;
	struct adc_m5pm1_data *data = dev->data;

	if (!device_is_ready(config->mfd)) {
		LOG_ERR_DEVICE_NOT_READY(config->mfd);
		return -ENODEV;
	}

	k_mutex_init(&data->lock);

	return 0;
}

#define ADC_M5PM1_DEFINE(inst)                                                                     \
	static const struct adc_m5pm1_config adc_m5pm1_config_##inst = {                           \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                        \
	};                                                                                         \
	static struct adc_m5pm1_data adc_m5pm1_data_##inst;                                        \
	DEVICE_DT_INST_DEFINE(inst, adc_m5pm1_init, NULL, &adc_m5pm1_data_##inst,                  \
			      &adc_m5pm1_config_##inst, POST_KERNEL,                               \
			      CONFIG_ADC_M5PM1_INIT_PRIORITY, &adc_m5pm1_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_M5PM1_DEFINE)
