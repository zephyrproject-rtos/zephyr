/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tsl2540

#include "tsl2540.h"

#include <stdlib.h>

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define TSL2540_INTEGRATION_TIME_MS	(2.81)
#define TSL2540_DEVICE_FACTOR		(53.0)

#define FIXED_ATTENUATION_TO_DBL(x)	(x * 0.00001)

LOG_MODULE_REGISTER(tsl2540, CONFIG_SENSOR_LOG_LEVEL);

static int tsl2540_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tsl2540_config *cfg = dev->config;
	struct tsl2540_data *data = dev->data;
	int ret = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT ||
			chan == SENSOR_CHAN_IR);
	k_sem_take(&data->sem, K_FOREVER);

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT) {
		uint16_t le16_buffer;

		ret = i2c_burst_read_dt(&cfg->i2c_spec, TSL2540_REG_VIS_LOW,
					(uint8_t *)&le16_buffer, sizeof(le16_buffer));
		if (ret) {
			LOG_ERR("Could not fetch ambient light (visible)");
			k_sem_give(&data->sem);
			return -EIO;
		}

		data->count_vis = sys_le16_to_cpu(le16_buffer);
	}

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_IR) {
		uint16_t le16_buffer;

		ret = i2c_burst_read_dt(&cfg->i2c_spec, TSL2540_REG_IR_LOW, (uint8_t *)&le16_buffer,
					sizeof(le16_buffer));
		if (ret) {
			LOG_ERR("Could not fetch ambient light (IR)");
			k_sem_give(&data->sem);
			return -EIO;
		}

		data->count_ir = sys_le16_to_cpu(le16_buffer);
	}

	k_sem_give(&data->sem);

	return ret;
}

static int tsl2540_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct tsl2540_config *cfg = dev->config;
	struct tsl2540_data *data = dev->data;
	int ret = 0;
	double cpl;
	double glass_attenuation = FIXED_ATTENUATION_TO_DBL(cfg->glass_attenuation);
	double glass_ir_attenuation = FIXED_ATTENUATION_TO_DBL(cfg->glass_ir_attenuation);

	k_sem_take(&data->sem, K_FOREVER);

	cpl = (data->integration_time + 1) * TSL2540_INTEGRATION_TIME_MS;
	cpl *= data->again;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		sensor_value_from_double(val, data->count_vis / cpl *
					 TSL2540_DEVICE_FACTOR * glass_attenuation);
		break;
	case SENSOR_CHAN_IR:
		sensor_value_from_double(val, data->count_ir / cpl *
					 TSL2540_DEVICE_FACTOR * glass_ir_attenuation);
		break;
	default:
		ret = -ENOTSUP;
	}

	k_sem_give(&data->sem);

	return ret;
}

static int tsl2540_attr_set_gain(const struct device *dev, enum sensor_gain_tsl2540 gain)
{
	const struct tsl2540_config *cfg = dev->config;
	struct tsl2540_data *data = dev->data;
	uint8_t value = 0;
	double again = 0.0;

	switch (gain) {
	case TSL2540_SENSOR_GAIN_1_2:
		value = TSL2540_CFG1_G1_2;
		again = TSL2540_AGAIN_S1_2;
		break;
	case TSL2540_SENSOR_GAIN_1:
		value = TSL2540_CFG1_G1;
		again = TSL2540_AGAIN_S1;
		break;
	case TSL2540_SENSOR_GAIN_4:
		value = TSL2540_CFG1_G4;
		again = TSL2540_AGAIN_S4;
		break;
	case TSL2540_SENSOR_GAIN_16:
		value = TSL2540_CFG1_G16;
		again = TSL2540_AGAIN_S16;
		break;
	case TSL2540_SENSOR_GAIN_64:
		value = TSL2540_CFG1_G64;
		again = TSL2540_AGAIN_S64;
		break;
	case TSL2540_SENSOR_GAIN_128:
		value = TSL2540_CFG1_G128;
		again = TSL2540_CFG2_G128;
		break;
	}

	if (i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2540_REG_CFG_1, value) < 0) {
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2540_REG_CFG_2, value) < 0) {
		return -EIO;
	}

	data->again = again;

	return 0;
}

static int tsl2540_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct tsl2540_config *cfg = dev->config;
	struct tsl2540_data *data = dev->data;
	int ret = 0;
	uint8_t temp;
	double it;

	if ((chan != SENSOR_CHAN_IR) & (chan != SENSOR_CHAN_LIGHT)) {
		return -ENOTSUP;
	}

	k_sem_take(&data->sem, K_FOREVER);

	ret = i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2540_ENABLE_ADDR, TSL2540_ENABLE_MASK &
				    ~TSL2540_ENABLE_CONF);
	if (ret) {
		k_sem_give(&data->sem);
		return ret;
	}

#if CONFIG_TSL2540_TRIGGER
	if (chan == SENSOR_CHAN_LIGHT) {
		if (attr == SENSOR_ATTR_UPPER_THRESH) {
			double cpl;
			uint16_t thld, le16_buffer;
			double glass_attenuation = FIXED_ATTENUATION_TO_DBL(cfg->glass_attenuation);

			cpl = ((data->integration_time + 1) * TSL2540_INTEGRATION_TIME_MS);
			cpl *= data->again;
			cpl /= (TSL2540_DEVICE_FACTOR * glass_attenuation);
			thld = sensor_value_to_double(val) * cpl;
			LOG_DBG("attr: %d, cpl: %g, thld: %x\n", attr, cpl, thld);

			le16_buffer = sys_cpu_to_le16(thld);
			ret = i2c_burst_write_dt(
				&((const struct tsl2540_config *)dev->config)->i2c_spec,
				TSL2540_REG_AIHT_LOW, (uint8_t *)&le16_buffer, sizeof(le16_buffer));

			goto exit;
		}
		if (attr == SENSOR_ATTR_LOWER_THRESH) {
			double cpl;
			uint16_t thld, le16_buffer;
			double glass_attenuation = FIXED_ATTENUATION_TO_DBL(cfg->glass_attenuation);

			cpl = ((data->integration_time + 1) * TSL2540_INTEGRATION_TIME_MS);
			cpl *= data->again;
			cpl /= (TSL2540_DEVICE_FACTOR * glass_attenuation);
			thld = sensor_value_to_double(val) * cpl;
			LOG_DBG("attr: %d, cpl: %g, thld: %x\n", attr, cpl, thld);

			le16_buffer = sys_cpu_to_le16(sys_cpu_to_le16(thld));

			ret = i2c_burst_write_dt(
				&((const struct tsl2540_config *)dev->config)->i2c_spec,
				TSL2540_REG_AILT_LOW, (uint8_t *)&le16_buffer, sizeof(le16_buffer));

			goto exit;
		}

	}
#endif /* CONFIG_TSL2540_TRIGGER */

	if (attr == SENSOR_ATTR_GAIN) {
		tsl2540_attr_set_gain(dev, (enum sensor_gain_tsl2540)val->val1);
		goto exit;
	}

	switch ((enum sensor_attribute_tsl2540)attr) {
	case SENSOR_ATTR_INT_APERS:
		temp = (uint8_t)val->val1;

		if (temp > 15) {
			ret = -EINVAL;
			goto exit;
		}

		if (i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2540_REG_PERS, temp)) {
			ret = -EIO;
			goto exit;
		}
		break;
	case SENSOR_ATTR_INTEGRATION_TIME:
		it = sensor_value_to_double(val);
		it /= TSL2540_INTEGRATION_TIME_MS;
		if (it < 1 || it > 256) {
			ret = -EINVAL;
			goto exit;
		}
		it -= 1;
		temp = (uint8_t)it;
		if (i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2540_REG_ATIME, temp)) {
			ret = -EIO;
			goto exit;
		}

		data->integration_time = temp;
		ret = 0;
		break;
	case SENSOR_ATTR_TSL2540_SHUTDOWN_MODE:
		data->enable_mode = TSL2540_ENABLE_DISABLE;
		ret = i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_CFG3_ADDR, TSL2540_CFG3_MASK,
						TSL2540_CFG3_CONF);
		break;
	case SENSOR_ATTR_TSL2540_CONTINUOUS_MODE:
		data->enable_mode = TSL2540_ENABLE_CONF;
		ret = i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_CFG3_ADDR, TSL2540_CFG3_MASK,
						TSL2540_CFG3_CONF);
		break;
	case SENSOR_ATTR_TSL2540_CONTINUOUS_NO_WAIT_MODE:
		data->enable_mode = TSL2540_ENABLE_AEN_PON;
		ret = i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_CFG3_ADDR, TSL2540_CFG3_MASK,
						TSL2540_CFG3_DFLT);
		break;
	}

exit:
	i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_ENABLE_ADDR, TSL2540_ENABLE_MASK,
				data->enable_mode);

	k_sem_give(&data->sem);

	return ret;
}

static int tsl2540_setup(const struct device *dev)
{
	struct sensor_value integration_time;

	/* Set ALS integration time */
	tsl2540_attr_set(dev, (enum sensor_channel)SENSOR_CHAN_LIGHT,
			 (enum sensor_attribute)SENSOR_ATTR_GAIN,
			 &(struct sensor_value){.val1 = TSL2540_SENSOR_GAIN_1_2, .val2 = 0});

	sensor_value_from_double(&integration_time, 500.0);
	tsl2540_attr_set(dev, (enum sensor_channel)SENSOR_CHAN_LIGHT,
			 (enum sensor_attribute)SENSOR_ATTR_INTEGRATION_TIME, &integration_time);

	return 0;
}

static int tsl2540_init(const struct device *dev)
{
	const struct tsl2540_config *cfg = dev->config;
	struct tsl2540_data *data = dev->data;
	int ret;

	data->enable_mode = TSL2540_ENABLE_DISABLE;

	k_sem_init(&data->sem, 1, K_SEM_MAX_LIMIT);

	if (!i2c_is_ready_dt(&cfg->i2c_spec)) {
		LOG_ERR("I2C dev %s not ready", cfg->i2c_spec.bus->name);
		return -ENODEV;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2540_REG_PERS, 1);
	if (ret) {
		LOG_ERR("Failed to setup interrupt persistence filter");
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_CFG3_ADDR, TSL2540_CFG3_MASK,
				     TSL2540_CFG3_DFLT);
	if (ret) {
		LOG_ERR("Failed to set configuration");
		return ret;
	}

	if (tsl2540_setup(dev)) {
		LOG_ERR("Failed to setup ambient light functionality");
		return -EIO;
	}

#if CONFIG_TSL2540_TRIGGER
	if (tsl2540_trigger_init(dev)) {
		LOG_ERR("Could not initialize interrupts");
		return -EIO;
	}
#endif

	LOG_DBG("Init complete");

	return 0;
}

static const struct sensor_driver_api tsl2540_driver_api = {
	.sample_fetch = tsl2540_sample_fetch,
	.channel_get = tsl2540_channel_get,
	.attr_set = tsl2540_attr_set,
#ifdef CONFIG_TSL2540_TRIGGER
	.trigger_set = tsl2540_trigger_set,
#endif
};

#ifdef CONFIG_PM_DEVICE
static int tsl2540_pm_action(const struct device *dev, enum pm_device_action action)
{

	const struct tsl2540_config *cfg = dev->config;
	struct tsl2540_data *data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_ENABLE_ADDR,
					    TSL2540_ENABLE_MASK, data->enable_mode);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2540_ENABLE_ADDR,
					    TSL2540_ENABLE_MASK, TSL2540_ENABLE_DISABLE);
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

#define TSL2540_GLASS_ATTEN(inst)						\
	.glass_attenuation = DT_INST_PROP(inst, glass_attenuation),		\
	.glass_ir_attenuation = DT_INST_PROP(inst, glass_ir_attenuation),	\

#define TSL2540_DEFINE(inst)									\
	static struct tsl2540_data tsl2540_prv_data_##inst;					\
	static const struct tsl2540_config tsl2540_config_##inst = {				\
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_TSL2540_TRIGGER,						\
		(.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),))				\
		TSL2540_GLASS_ATTEN(inst)							\
	};											\
	PM_DEVICE_DT_INST_DEFINE(inst, tsl2540_pm_action);					\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &tsl2540_init, PM_DEVICE_DT_INST_GET(inst),		\
				&tsl2540_prv_data_##inst, &tsl2540_config_##inst, POST_KERNEL,	\
				CONFIG_SENSOR_INIT_PRIORITY, &tsl2540_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TSL2540_DEFINE)
