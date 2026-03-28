/*
 * Copyright (c) 2018, 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>

#include "adt7420.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ADT7420, CONFIG_SENSOR_LOG_LEVEL);

static int adt7420_probe(const struct device *dev);

static int adt7420_temp_reg_read(const struct device *dev, uint8_t reg,
				 uint16_t *val)
{
	const struct adt7420_dev_config *cfg = dev->config;

	if (i2c_burst_read_dt(&cfg->i2c, reg, (uint8_t *) val, 2) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int adt7420_temp_reg_write(const struct device *dev, uint8_t reg,
				  int16_t val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write_dt(&cfg->i2c, buf, ARRAY_SIZE(buf));
}

static int adt7420_set_sampling_frequency(const struct device *dev,
					   const struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	int ret;

	if (val->val1 < ADT7420_OP_MODE_CONT_CONV || val->val1 > ADT7420_OP_MODE_SHUTDOWN) {
		LOG_DBG("Invalid operation mode!");
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG,
				     ADT7420_CONFIG_OP_MODE_MASK,
				     ADT7420_CONFIG_OP_MODE(val->val1));
	if (ret < 0) {
		LOG_DBG("Failed to set operation mode!");
		return ret;
	}

	return 0;
}

static int adt7420_set_threshold(const struct device *dev, uint8_t reg,
				  const struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	int64_t value;
	int16_t payload;
	int ret;

	if ((val->val1 < cfg->min_temp) || (val->val1 > cfg->max_temp)) {
		return -EINVAL;
	}

	/* threshold values will always be in 16-bit, regardless of bit mode */
	value = ((int64_t)val->val1 * ADT7420_SCALE_UVAL_TO_VAL + val->val2) *
		ADT7420_TEMP_CONV_16BIT_DIV;
	if (value < 0) {
		value = value + (ADT7420_TEMP_NEG_16BIT * ADT7420_SCALE_UVAL_TO_VAL);
	}

	payload = (int16_t)(value / ADT7420_SCALE_UVAL_TO_VAL);

	ret = adt7420_temp_reg_write(dev, reg, payload);
	if (ret < 0) {
		LOG_DBG("Failed to set threshold!");
		return ret;
	}

	return 0;
}

static int adt7420_set_hysteresis(const struct device *dev,
				   const struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	int ret;

	if (val->val1 < ADT7420_HYST_MIN || val->val1 > ADT7420_HYST_MAX) {
		LOG_DBG("Invalid Hysteresis Value!");
		return -EINVAL;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT7420_REG_HIST, (uint8_t)val->val1);
	if (ret < 0) {
		LOG_DBG("Failed to set hysteresis!");
		return ret;
	}

	return 0;
}

static int adt7420_set_configuration(const struct device *dev,
				      const struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	int ret;

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT7420_REG_RESET,
				    (val->val1 > 0) ? ADT7420_REG_RESET : 0);
	if (ret < 0) {
		LOG_DBG("Failed to  device!");
		return ret;
	}

	/* wait 200us as stated on datasheet, page 19 */
	k_usleep(200);

	/* re-initialize configs */
	return adt7420_probe(dev);
}

static int adt7420_set_fault_queue(const struct device *dev,
				    const struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	int ret;

	if (val->val1 < ADT7420_FAULT_QUEUE_MIN_IDX || val->val1 > ADT7420_FAULT_QUEUE_MAX_IDX) {
		LOG_DBG("Invalid configuration for Fault Queue!");
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG,
				     ADT7420_CONFIG_FAULT_QUEUE(~0),
				     ADT7420_CONFIG_FAULT_QUEUE(val->val1));
	if (ret < 0) {
		LOG_DBG("Failed to set fault queue count!");
		return ret;
	}

	return 0;
}

static int adt7420_set_resolution(const struct device *dev,
				   const struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	struct adt7420_data *dev_data = dev->data;
	int ret;

	if (val->val1 < 0 || val->val1 > 1) {
		LOG_DBG("Invalid Resolution!");
		return -EINVAL;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG,
				     ADT7420_CONFIG_RESOLUTION,
				     FIELD_PREP(ADT7420_CONFIG_RESOLUTION, val->val1));
	if (ret < 0) {
		LOG_DBG("Failed to set resolution!");
		return ret;
	}

	dev_data->resolution_16_bit = !!val->val1;

	return 0;
}

static int adt7420_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_DBG("Invalid channel");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adt7420_set_sampling_frequency(dev, val);
	case SENSOR_ATTR_UPPER_THRESH:
		return adt7420_set_threshold(dev, ADT7420_REG_T_HIGH_MSB, val);
	case SENSOR_ATTR_LOWER_THRESH:
		return adt7420_set_threshold(dev, ADT7420_REG_T_LOW_MSB, val);
	case SENSOR_ATTR_HYSTERESIS:
		return adt7420_set_hysteresis(dev, val);
	case SENSOR_ATTR_CONFIGURATION:
		return adt7420_set_configuration(dev, val);
	case SENSOR_ATTR_FEATURE_MASK:
		return adt7420_set_fault_queue(dev, val);
	case SENSOR_ATTR_RESOLUTION:
		return adt7420_set_resolution(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int adt7420_get_sampling_frequency(const struct device *dev,
					   struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t uval8;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, &uval8);
	if (ret < 0) {
		LOG_DBG("Failed to get sampling mode!");
		return ret;
	}

	val->val1 = (ADT7420_CONFIG_OP_MODE_MASK & uval8) >> 5;
	val->val2 = 0;

	return 0;
}

static int adt7420_get_threshold(const struct device *dev, uint8_t reg,
				  struct sensor_value *val)
{
	uint16_t uval16;
	int64_t value;
	int ret;

	ret = adt7420_temp_reg_read(dev, reg, &uval16);
	if (ret < 0) {
		LOG_DBG("Failed to get attribute!");
		return ret;
	}

	value = (int64_t)uval16 * ADT7420_SCALE_UVAL_TO_VAL;
	if (uval16 & ADT7420_TEMP_SIGN_16BIT) {
		value = (value - ADT7420_TEMP_NEG_16BIT) /
			ADT7420_TEMP_CONV_16BIT_DIV;
	} else {
		value = value / ADT7420_TEMP_CONV_16BIT_DIV;
	}

	val->val1 = value / ADT7420_SCALE_UVAL_TO_VAL;
	val->val2 = value % ADT7420_SCALE_UVAL_TO_VAL;

	return 0;
}

static int adt7420_get_configuration(const struct device *dev,
				      struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t uval8;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, &uval8);
	if (ret < 0) {
		LOG_DBG("Failed to get configuration!");
		return ret;
	}

	val->val1 = uval8;
	val->val2 = 0;

	return 0;
}

static int adt7420_get_hysteresis(const struct device *dev,
				   struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t uval8;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_HIST, &uval8);
	if (ret < 0) {
		LOG_DBG("Failed to get hysteresis!");
		return ret;
	}

	val->val1 = uval8;
	val->val2 = 0;

	return 0;
}

static int adt7420_get_resolution(const struct device *dev,
				   struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t uval8;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, &uval8);
	if (ret < 0) {
		LOG_DBG("Failed to get resolution!");
		return ret;
	}

	val->val1 = (ADT7420_CONFIG_RESOLUTION & uval8) ? 13 : 16;
	val->val2 = 0;

	return 0;
}

static int adt7420_get_alert(const struct device *dev,
			      struct sensor_value *val)
{
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t uval8;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_STATUS, &uval8);
	if (ret < 0) {
		LOG_DBG("Failed to get alerts!");
		return ret;
	}

	val->val1 = uval8;
	val->val2 = 0;

	return 0;
}

static int adt7420_attr_get(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    struct sensor_value *val)
{
	if (val == NULL) {
		LOG_DBG("sensor value is null!");
		return -EINVAL;
	}

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_DBG("Invalid channel");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adt7420_get_sampling_frequency(dev, val);
	case SENSOR_ATTR_UPPER_THRESH:
		return adt7420_get_threshold(dev, ADT7420_REG_T_HIGH_MSB, val);
	case SENSOR_ATTR_LOWER_THRESH:
		return adt7420_get_threshold(dev, ADT7420_REG_T_LOW_MSB, val);
	case SENSOR_ATTR_CONFIGURATION:
		return adt7420_get_configuration(dev, val);
	case SENSOR_ATTR_HYSTERESIS:
		return adt7420_get_hysteresis(dev, val);
	case SENSOR_ATTR_RESOLUTION:
		return adt7420_get_resolution(dev, val);
	case SENSOR_ATTR_ALERT:
		return adt7420_get_alert(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int adt7420_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct adt7420_data *drv_data = dev->data;
	int16_t value;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (adt7420_temp_reg_read(dev, ADT7420_REG_TEMP_MSB, &value) < 0) {
		return -EIO;
	}

	drv_data->sample = value;

	return 0;
}

static int adt7420_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adt7420_data *drv_data = dev->data;
	int64_t value;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	if (drv_data->resolution_16_bit) {
		if (drv_data->sample & ADT7420_TEMP_SIGN_16BIT) {
			value = (int64_t)drv_data->sample * ADT7420_SCALE_UVAL_TO_VAL;
			value = (value - ADT7420_TEMP_NEG_16BIT) /
				ADT7420_TEMP_CONV_16BIT_DIV;
		} else {
			value = (int64_t)drv_data->sample * ADT7420_SCALE_UVAL_TO_VAL /
				ADT7420_TEMP_CONV_16BIT_DIV;
		}
	} else {
		if (drv_data->sample & ADT7420_TEMP_SIGN_13BIT) {
			value = (int64_t)(drv_data->sample >> 3) *
				ADT7420_SCALE_UVAL_TO_VAL;
			value = (value - ADT7420_TEMP_NEG_13BIT) /
				ADT7420_TEMP_CONV_13BIT_DIV;
		} else {
			value = (int64_t)(drv_data->sample >> 3) * ADT7420_SCALE_UVAL_TO_VAL /
				ADT7420_TEMP_CONV_13BIT_DIV;
		}
	}

	val->val1 = value / ADT7420_SCALE_UVAL_TO_VAL;
	val->val2 = value % ADT7420_SCALE_UVAL_TO_VAL;

	return 0;
}

static DEVICE_API(sensor, adt7420_driver_api) = {
	.attr_set = adt7420_attr_set,
	.attr_get = adt7420_attr_get,
	.sample_fetch = adt7420_sample_fetch,
	.channel_get = adt7420_channel_get,
#ifdef CONFIG_ADT7420_TRIGGER
	.trigger_set = adt7420_trigger_set,
#endif
};

static int adt7420_probe(const struct device *dev)
{
	const struct adt7420_dev_config *cfg = dev->config;
	struct adt7420_data *drv_data = dev->data;
	int64_t temp_crit;
	uint8_t value;
	int ret;

	ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_ID, &value);
	if (ret) {
		return ret;
	}

	if ((value & cfg->id_mask) != cfg->id_mask) {
		return -ENODEV;
	}

	value = FIELD_PREP(ADT7420_CONFIG_CT_POL, cfg->ct_pol) |
		FIELD_PREP(ADT7420_CONFIG_INT_POL, cfg->int_pol) |
		FIELD_PREP(ADT7420_CONFIG_INT_CT_MODE, cfg->ct_mode) |
		ADT7420_CONFIG_OP_MODE(cfg->init_op_mode) |
		FIELD_PREP(ADT7420_CONFIG_RESOLUTION, drv_data->resolution_16_bit);

	ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, value);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    ADT7420_REG_HIST, CONFIG_ADT7420_TEMP_HYST);
	if (ret) {
		return ret;
	}

	temp_crit = CONFIG_ADT7420_TEMP_CRIT * ADT7420_TEMP_CONV_16BIT_DIV;
	if (temp_crit < 0) {
		temp_crit = temp_crit + ADT7420_TEMP_NEG_16BIT;
	}

	ret = adt7420_temp_reg_write(dev, ADT7420_REG_T_CRIT_MSB,
				    (int16_t) temp_crit);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_ADT7420_TRIGGER
	if (!cfg->int_gpio.port && !cfg->ct_gpio.port) {
		LOG_ERR("No GPIO pins to initialize!");
		return -ENODEV;
	}

	ret = adt7420_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return ret;
	}

#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int adt7420_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct adt7420_dev_config *cfg = dev->config;
	struct adt7420_data *dev_data = dev->data;
	uint8_t value;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, &value);
		if (ret < 0) {
			LOG_DBG("Failed to read config for suspend");
			return ret;
		}

		dev_data->suspended_op_mode = (ADT7420_CONFIG_OP_MODE_MASK & value);

		value |= ADT7420_CONFIG_OP_MODE(ADT7420_OP_MODE_SHUTDOWN);
		ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, value);
		if (ret < 0) {
			LOG_DBG("Failed to suspend device");
			return ret;
		}

		LOG_DBG("ADT7420 is suspended");
		return 0;

	case PM_DEVICE_ACTION_RESUME:
		ret = i2c_reg_read_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, &value);
		if (ret < 0) {
			LOG_DBG("Failed to read config for resume");
			return ret;
		}

		value &= ~ADT7420_CONFIG_OP_MODE(ADT7420_OP_MODE_SHUTDOWN);
		value |= dev_data->suspended_op_mode;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, ADT7420_REG_CONFIG, value);
		if (ret < 0) {
			LOG_DBG("Failed to resume device");
			return ret;
		}

		/* wait 240us for initial temp conversion to complete */
		k_usleep(240);

		LOG_DBG("ADT7420 resumed");
		return 0;

	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static int adt7420_init(const struct device *dev)
{
	const struct adt7420_dev_config *cfg = dev->config;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	return adt7420_probe(dev);
}

#define ADT7420_DEFINE(inst, name, mask, min, max)					\
	static struct adt7420_data data_##name##_##inst = {				\
		.resolution_16_bit = DT_INST_ENUM_IDX(inst, set_resolution),		\
	};										\
											\
	static const struct adt7420_dev_config config_##name##_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.id_mask = mask,							\
		.min_temp = min,							\
		.max_temp = max,							\
		.int_pol = DT_INST_PROP(inst, int_polarity_active_high),		\
		.ct_pol = DT_INST_PROP(inst, ct_polarity_active_high),			\
		.ct_mode = DT_INST_PROP(inst, set_comparator_mode),			\
		.init_op_mode = DT_INST_ENUM_IDX(inst, op_mode),			\
											\
	IF_ENABLED(CONFIG_ADT7420_TRIGGER,						\
		   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),	\
		    .ct_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, ct_gpios, { 0 }),))	\
	};										\
											\
	PM_DEVICE_DT_INST_DEFINE(inst, adt7420_pm_action);				\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adt7420_init, PM_DEVICE_DT_INST_GET(inst),	\
			      &data_##name##_##inst, &config_##name##_##inst,		\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			\
			      &adt7420_driver_api);					\

#define DT_DRV_COMPAT adi_adt7410
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define ADT7410_DEFAULT_ID_MASK		0xC8
#define ADT7410_TEMP_MIN		(-55)
#define ADT7410_TEMP_MAX		150
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADT7420_DEFINE, DT_DRV_COMPAT, ADT7410_DEFAULT_ID_MASK,
				ADT7410_TEMP_MIN, ADT7410_TEMP_MAX)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_adt7420
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define ADT7420_DEFAULT_ID_MASK		0xC8
#define ADT7420_TEMP_MIN		(-40)
#define ADT7420_TEMP_MAX		150
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADT7420_DEFINE, DT_DRV_COMPAT, ADT7420_DEFAULT_ID_MASK,
				ADT7420_TEMP_MIN, ADT7420_TEMP_MAX)
#endif
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT adi_adt7422
#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)
#define ADT7422_DEFAULT_ID_MASK		0xC8
#define ADT7422_TEMP_MIN		(-40)
#define ADT7422_TEMP_MAX		125
DT_INST_FOREACH_STATUS_OKAY_VARGS(ADT7420_DEFINE, DT_DRV_COMPAT, ADT7422_DEFAULT_ID_MASK,
				ADT7422_TEMP_MIN, ADT7422_TEMP_MAX)
#endif
#undef DT_DRV_COMPAT
