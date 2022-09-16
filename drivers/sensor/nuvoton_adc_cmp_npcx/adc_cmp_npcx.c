/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc/adc_npcx_threshold.h>
#include <zephyr/drivers/sensor/adc_cmp_npcx.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(adc_cmp_npcx, CONFIG_SENSOR_LOG_LEVEL);

struct adc_cmp_npcx_data {
	/* Work queue to be notified when threshold assertion happens */
	struct k_work work;
	/* Sensor trigger hanlder to notify user of assetion */
	sensor_trigger_handler_t handler;
	/* ADC NPCX driver reference */
	const struct device *dev;
	/* Driver user sensor trigger reference */
	const struct sensor_trigger *trigger;
};

struct adc_cmp_npcx_config {
	/*
	 * Pointer of ADC device that will be performing measurement, this
	 * must be provied by device tree.
	 */
	const struct device *adc;
	/*
	 * ADC channel that will be used to measure signal, this must be
	 * provided by device tree.
	 */
	uint8_t chnsel;
	/* Threshold selection number assigned during initialization */
	uint8_t th_sel;
	/* Threshold assert value in millivolts */
	uint32_t thr_mv;
	/*
	 * Condition to be met between measured signal and threshold assert
	 * value that will trigger event
	 */
	enum adc_cmp_npcx_comparison comparison;
};

#define DT_DRV_COMPAT nuvoton_adc_cmp

#define ADC_CMP_NPCX_UNDEFINED		(-1)
static void adc_cmp_npcx_trigger_work_handler(struct k_work *item)
{
	struct adc_cmp_npcx_data *data =
			CONTAINER_OF(item, struct adc_cmp_npcx_data, work);

	if (data->handler) {
		data->handler(data->dev, data->trigger);
	}
}

static int adc_cmp_npcx_init(const struct device *dev)
{
	const struct adc_cmp_npcx_config *const config = dev->config;
	struct adc_cmp_npcx_data *data = dev->data;
	struct adc_npcx_threshold_param param;
	int ret;

	LOG_DBG("Initialize ADC CMP threshold selection (%d)", config->th_sel);
	/* Data must keep device reference for worker handler*/
	data->dev = dev;

	/* Set ADC channel selection */
	param.type = ADC_NPCX_THRESHOLD_PARAM_CHNSEL;
	param.val = (uint32_t)config->chnsel;
	ret = adc_npcx_threshold_ctrl_set_param(config->adc, config->th_sel,
						&param);
	if (ret) {
		goto init_error;
	}

	/* Init and set Worker queue to enable notifications */
	k_work_init(&data->work, adc_cmp_npcx_trigger_work_handler);
	param.type = ADC_NPCX_THRESHOLD_PARAM_WORK;
	param.val = (uint32_t)&data->work;
	ret = adc_npcx_threshold_ctrl_set_param(config->adc, config->th_sel,
						&param);
	if (ret) {
		goto init_error;
	}

	/* Set threshold value if set on device tree */
	if (config->thr_mv != ADC_CMP_NPCX_UNDEFINED) {
		param.type = ADC_NPCX_THRESHOLD_PARAM_THVAL;
		/* Convert from millivolts to ADC raw register value */
		ret = adc_npcx_threshold_mv_to_thrval(config->thr_mv,
						&param.val);
		if (ret) {
			goto init_error;
		}

		ret = adc_npcx_threshold_ctrl_set_param(config->adc,
				config->th_sel,	&param);
		if (ret) {
			goto init_error;
		}
	}

	/* Set threshold comparison if set on device tree */
	if (config->comparison == ADC_CMP_NPCX_GREATER ||
	    config->comparison == ADC_CMP_NPCX_LESS_OR_EQUAL) {
		param.type = ADC_NPCX_THRESHOLD_PARAM_L_H;
		param.val =
			config->comparison == ADC_CMP_NPCX_GREATER ?
			ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER :
			ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER;
		ret = adc_npcx_threshold_ctrl_set_param(config->adc,
				config->th_sel,	&param);
	}

init_error:
	if (ret) {
		LOG_ERR("Error setting parameter %d - value %d",
			(uint32_t)param.type, param.val);
	}

	return ret;
}

static int adc_cmp_npcx_set_threshold(const struct device *dev, bool is_upper,
				      bool is_mv, uint32_t value)
{
	const struct adc_cmp_npcx_config *const config = dev->config;
	struct adc_npcx_threshold_param param;
	int ret;

	param.type = ADC_NPCX_THRESHOLD_PARAM_THVAL;
	if (is_mv) {
		ret = adc_npcx_threshold_mv_to_thrval(value, &param.val);
		if (ret) {
			return ret;
		}
	}

	ret = adc_npcx_threshold_ctrl_set_param(config->adc,
						config->th_sel, &param);
	if (ret) {
		return ret;
	}

	param.type = ADC_NPCX_THRESHOLD_PARAM_L_H;
	param.val = is_upper ? ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER :
		ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER;

	ret = adc_npcx_threshold_ctrl_set_param(config->adc,
						config->th_sel, &param);

	return ret;
}

static int adc_cmp_npcx_attr_set(const struct device *dev,
			  enum sensor_channel chan,
			  enum sensor_attribute attr,
			  const struct sensor_value *val)
{
	const struct adc_cmp_npcx_config *const config = dev->config;
	int ret;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		__fallthrough;
	case SENSOR_ATTR_UPPER_THRESH:
		__fallthrough;
	case SENSOR_ATTR_LOWER_VOLTAGE_THRESH:
		__fallthrough;
	case SENSOR_ATTR_UPPER_VOLTAGE_THRESH:
		ret = adc_cmp_npcx_set_threshold(dev,
			/* Is upper? */
			attr == SENSOR_ATTR_UPPER_THRESH ||
			(uint16_t)attr == SENSOR_ATTR_UPPER_VOLTAGE_THRESH,
			/* Is mV? */
			(uint16_t)attr == SENSOR_ATTR_LOWER_VOLTAGE_THRESH ||
			(uint16_t)attr == SENSOR_ATTR_UPPER_VOLTAGE_THRESH,
			val->val1);
		break;
	case SENSOR_ATTR_ALERT:
		ret = adc_npcx_threshold_ctrl_enable(config->adc,
						config->th_sel, !!val->val1);
		break;
	default:
		ret = -ENOTSUP;
	}
	return ret;
}

static int adc_cmp_npcx_attr_get(const struct device *dev,
			  enum sensor_channel chan,
			  enum sensor_attribute attr,
			  struct sensor_value *val)
{
	return -ENOTSUP;
}

static int adc_cmp_npcx_trigger_set(const struct device *dev,
			     const struct sensor_trigger *trig,
			     sensor_trigger_handler_t handler)
{
	const struct adc_cmp_npcx_config *const config = dev->config;
	struct adc_cmp_npcx_data *data = dev->data;
	struct adc_npcx_threshold_param param;

	if (trig == NULL || handler == NULL) {
		return -EINVAL;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD ||
	    trig->chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	data->handler = handler;
	data->trigger = trig;

	param.type = ADC_NPCX_THRESHOLD_PARAM_WORK;
	param.val = (uint32_t)&data->work;
	return adc_npcx_threshold_ctrl_set_param(config->adc, config->th_sel,
						&param);
}

static int adc_cmp_npcx_sample_fetch(const struct device *dev,
			      enum sensor_channel chan)
{
	return -ENOTSUP;
}

static int adc_cmp_npcx_channel_get(const struct device *dev,
			     enum sensor_channel chan,
			     struct sensor_value *val)
{
	return -ENOTSUP;
}

static const struct sensor_driver_api adc_cmp_npcx_api = {
	.attr_set = adc_cmp_npcx_attr_set,
	.attr_get = adc_cmp_npcx_attr_get,
	.trigger_set = adc_cmp_npcx_trigger_set,
	.sample_fetch = adc_cmp_npcx_sample_fetch,
	.channel_get = adc_cmp_npcx_channel_get,
};

#define NPCX_ADC_CMP_INIT(inst)                                               \
	static struct adc_cmp_npcx_data adc_cmp_npcx_data_##inst;             \
	static const struct adc_cmp_npcx_config adc_cmp_npcx_config_##inst = {\
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),         \
		.chnsel = DT_INST_IO_CHANNELS_INPUT(inst),                    \
		.th_sel = inst,                                               \
		.thr_mv = DT_INST_PROP_OR(inst, threshold_mv,                 \
			ADC_CMP_NPCX_UNDEFINED),                              \
		.comparison = DT_INST_STRING_TOKEN_OR(inst,                   \
			comparison, ADC_CMP_NPCX_UNDEFINED)                   \
	};                                                                    \
	DEVICE_DT_INST_DEFINE(inst, adc_cmp_npcx_init, NULL,                  \
			      &adc_cmp_npcx_data_##inst,                      \
			      &adc_cmp_npcx_config_##inst, PRE_KERNEL_2,      \
			      CONFIG_SENSOR_INIT_PRIORITY,                    \
			      &adc_cmp_npcx_api);
DT_INST_FOREACH_STATUS_OKAY(NPCX_ADC_CMP_INIT)
