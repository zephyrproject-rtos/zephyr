/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/adc_npcx_v2t.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adc_v2t_npcx.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT nuvoton_npcx_adc_v2t

LOG_MODULE_REGISTER(adc_v2t_npcx, CONFIG_SENSOR_LOG_LEVEL);

/* 1 fraction step = 0.125°C = 125 milli-°C */
#define NPCX_V2T_FRAC_STEP_MILLIC 125

struct adc_v2t_npcx_config {
	/*
	 * Pointer of ADC device that will be performing measurement, this
	 * must be provied by device tree.
	 */
	const struct device *adc_dev;
};

struct adc_v2t_npcx_data {
	/* ADC conversion result buffer */
	uint16_t buffer;
	struct adc_sequence adc_seq;
	struct adc_channel_cfg adc_ch_cfg;
};

static inline void adc_v2t_npcx_set_channel(const struct device *dev, uint32_t v2t_ch)
{
	struct adc_v2t_npcx_data *data = dev->data;

	data->adc_ch_cfg.channel_id = v2t_ch;
	data->adc_seq.channels = BIT(v2t_ch);
}

static int adc_v2t_npcx_attr_set(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct adc_v2t_npcx_config *const config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_NPCX_V2T_CHANNEL_CFG:
		/* Set V2T channel bit-mask */
		ret = adc_npcx_v2t_set_channels(config->adc_dev, (uint32_t)val->val1);
		if (ret != 0) {
			return ret;
		}

		/* Set ADC channel */
		adc_v2t_npcx_set_channel(dev, find_lsb_set(val->val1) - 1);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int adc_v2t_npcx_attr_get(const struct device *dev, enum sensor_channel chan,
				 enum sensor_attribute attr, struct sensor_value *val)
{
	const struct adc_v2t_npcx_config *const config = dev->config;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_NPCX_V2T_CHANNEL_CFG:
		val->val1 = adc_npcx_v2t_get_channels(config->adc_dev);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int adc_v2t_npcx_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret;
	const struct adc_v2t_npcx_config *config = dev->config;
	struct adc_v2t_npcx_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = adc_channel_setup(config->adc_dev, &data->adc_ch_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to configure ADC channel (ret %d)", ret);
		return ret;
	}

	return adc_read(config->adc_dev, &data->adc_seq);
}

static int adc_v2t_npcx_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct adc_v2t_npcx_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* Convert register data to temperature in degree Celsius */
	/* Integer part */
	val->val1 = (int8_t)GET_FIELD(data->buffer, NPCX_V2T_TCHNDAT_DAT);
	/* Fraction part*/
	val->val2 = GET_FIELD(data->buffer, NPCX_V2T_TCHNDAT_DAT_FRACION) *
			NPCX_V2T_FRAC_STEP_MILLIC;

	return 0;
}

static int adc_v2t_npcx_init(const struct device *dev)
{
	const struct adc_v2t_npcx_config *const config = dev->config;

	if (!device_is_ready(config->adc_dev)) {
		LOG_ERR("ADC device is not ready");
		return -EINVAL;
	}

	return 0;
}

static DEVICE_API(sensor, adc_v2t_npcx_api) = {
	.attr_set = adc_v2t_npcx_attr_set,
	.attr_get = adc_v2t_npcx_attr_get,
	.sample_fetch = adc_v2t_npcx_sample_fetch,
	.channel_get = adc_v2t_npcx_channel_get,
};

#define NPCX_ADC_V2T_INIT(inst)                                                                    \
	static struct adc_v2t_npcx_data adc_v2t_npcx_data_##inst = {                               \
		.adc_ch_cfg = {                                                                    \
			.gain = ADC_GAIN_1,                                                        \
			.reference = ADC_REF_INTERNAL,                                             \
			.acquisition_time = ADC_ACQ_TIME_DEFAULT,                                  \
		},                                                                                 \
		.adc_seq = {                                                                       \
			.buffer = &adc_v2t_npcx_data_##inst.buffer,                                \
			.buffer_size = sizeof(adc_v2t_npcx_data_##inst.buffer),                    \
			.resolution = 10,                                                          \
		},                                                                                 \
	};                                                                                         \
	static const struct adc_v2t_npcx_config adc_v2t_npcx_config_##inst = {                     \
		.adc_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, adc_dev)),                          \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adc_v2t_npcx_init, NULL, &adc_v2t_npcx_data_##inst,     \
				     &adc_v2t_npcx_config_##inst, POST_KERNEL,                     \
				     CONFIG_SENSOR_INIT_PRIORITY, &adc_v2t_npcx_api);
DT_INST_FOREACH_STATUS_OKAY(NPCX_ADC_V2T_INIT)
