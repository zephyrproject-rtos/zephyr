/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Linumiz
 */

#define DT_DRV_COMPAT memsic_mc3419

#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "mc3419.h"

LOG_MODULE_REGISTER(MC3419, CONFIG_SENSOR_LOG_LEVEL);

static const uint16_t mc3419_accel_sense_map[] = {1, 2, 4, 6, 8};

static int mc3419_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	ret = i2c_burst_read_dt(&cfg->i2c, MC3419_REG_XOUT_L, (uint8_t *)data->samples,
								MC3419_SAMPLE_READ_SIZE);
	k_sem_give(&data->sem);
	return ret;
}

static void mc3419_data_convert(double sensitivity, int16_t *raw_data,
							struct sensor_value *val)
{
	double value = 0.0;

	*raw_data = sys_le16_to_cpu(*raw_data);
	value = (double)(*raw_data) * (double)sensitivity * SENSOR_GRAVITY_DOUBLE / 1000;
	val->val1 = (int32_t)value;
	val->val2 = (((int32_t)(value * 1000)) % 1000) * 1000;
}

static int mc3419_channel_get(const struct device *dev, enum sensor_channel chan,
							struct sensor_value *val)
{
	struct mc3419_driver_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		mc3419_data_convert(data->sensitivity, &data->samples[0], val);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		mc3419_data_convert(data->sensitivity, &data->samples[1], val);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		mc3419_data_convert(data->sensitivity, &data->samples[2], val);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		mc3419_data_convert(data->sensitivity, &data->samples[0], &val[0]);
		mc3419_data_convert(data->sensitivity, &data->samples[1], &val[1]);
		mc3419_data_convert(data->sensitivity, &data->samples[2], &val[2]);
		break;
	default:
		LOG_ERR("Unsupported channel");
		k_sem_give(&data->sem);
		return -EINVAL;
	}

	k_sem_give(&data->sem);
	return 0;
}

static int mc3419_set_accel_range(const struct device *dev, uint16_t range)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;
	struct mc3419_driver_data *data = dev->data;

	if (range >= MC3419_ACCL_RANGE_END) {
		LOG_ERR("Accel resolution is out of range");
		return -EINVAL;
	}

	ret = mc3419_set_op_mode(cfg, MC3419_MODE_STANDBY);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_reg_update_byte_dt(&cfg->i2c, MC3419_REG_RANGE_SELECT_CTRL,
							MC3419_RANGE_MASK, range << 4);
	if (ret < 0) {
		LOG_ERR("Failed to set resolution (%d)", ret);
		return ret;
	}

	data->sensitivity = (double)(mc3419_accel_sense_map[range] * SENSOR_GRAIN_VALUE);

	return 0;
}

static int mc3419_set_odr(const struct device *dev, const struct sensor_value *val)
{
	int ret = 0;
	int data_rate = 0;
	const struct mc3419_config *cfg = dev->config;

	ret = mc3419_set_op_mode(cfg, MC3419_MODE_STANDBY);
	if (ret < 0) {
		return ret;
	}

	if (val->val1 == 25 && val->val2 == 0) {
		data_rate = MC3419_ODR_25;
	} else if (val->val1 == 50 && val->val2 == 0) {
		data_rate = MC3419_ODR_50;
	} else if (val->val1 == 62 && val->val2 == 500) {
		data_rate = MC3419_ODR_62_5;
	} else if (val->val1 == 100 && val->val2 == 0) {
		data_rate = MC3419_ODR_100;
	} else if (val->val1 == 125 && val->val2 == 0) {
		data_rate = MC3419_ODR_125;
	} else if (val->val1 == 250 && val->val2 == 0) {
		data_rate = MC3419_ODR_250;
	} else if (val->val1 == 500 && val->val2 == 0) {
		data_rate = MC3419_ODR_500;
	} else if (val->val1 == 1000 && val->val2 == 0) {
		data_rate = MC3419_ODR_500;
	} else {
		return -EINVAL;
	}

	LOG_DBG("Set ODR Rate to 0x%x", data_rate);
	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_SAMPLE_RATE, data_rate);
	if (ret < 0) {
		LOG_ERR("Failed to set ODR (%d)", ret);
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&cfg->i2c, MC3419_REG_SAMPLE_RATE_2,
					CONFIG_MC3419_DECIMATION_RATE);
	if (ret < 0) {
		LOG_ERR("Failed to set decimation rate (%d)", ret);
		return ret;
	}

	return 0;
}

#if defined CONFIG_MC3419_MOTION
static int mc3419_set_anymotion_threshold(const struct device *dev,
						const struct sensor_value *val)
{
	int ret = 0;
	const struct mc3419_config *cfg = dev->config;

	if (val->val1 > MC3419_ANY_MOTION_THRESH_MAX) {
		return -EINVAL;
	}

	ret = mc3419_set_op_mode(cfg, MC3419_MODE_STANDBY);
	if (ret < 0) {
		return ret;
	}

	ret = i2c_burst_write_dt(&cfg->i2c, MC3419_REG_ANY_MOTION_THRES,
					(uint8_t *)&val->val1, sizeof(int16_t));
	if (ret < 0) {
		LOG_ERR("Failed to set anymotion threshold (%d)", ret);
		return ret;
	}

	return 0;
}
#endif

static int mc3419_attr_set(const struct device *dev,
					enum sensor_channel chan,
					enum sensor_attribute attr,
					const struct sensor_value *val)
{
	int ret = 0;

	if (chan != SENSOR_CHAN_ACCEL_X &&
		chan != SENSOR_CHAN_ACCEL_Y &&
		chan != SENSOR_CHAN_ACCEL_Z &&
		chan != SENSOR_CHAN_ACCEL_XYZ) {
		LOG_ERR("Not supported on this channel.");
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		ret = mc3419_set_accel_range(dev, val->val1);
		break;
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = mc3419_set_odr(dev, val);
		break;
#if defined CONFIG_MC3419_MOTION
	case SENSOR_ATTR_SLOPE_TH:
		ret = mc3419_set_anymotion_threshold(dev, val);
		break;
#endif
	default:
		LOG_ERR("ACCEL attirbute is not supported");
		return -EINVAL;
	}

	if (!ret) {
		ret = mc3419_set_op_mode(dev->config, MC3419_MODE_WAKE);
		if (ret < 0) {
			LOG_ERR("Failed to set wake mode");
		}
	}

	return ret;
}

static int mc3419_init(const struct device *dev)
{
	int ret = 0;
	struct mc3419_driver_data *data = dev->data;
	const struct mc3419_config *cfg = dev->config;
	struct sensor_value odr = {.val1 = 62, .val2 = 500};

	if (!(i2c_is_ready_dt(&cfg->i2c))) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	ret = mc3419_set_odr(dev, &odr);
	if (ret < 0) {
		return ret;
	}

	ret = mc3419_set_accel_range(dev, MC3419_ACCl_RANGE_2G);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
#if CONFIG_MC3419_TRIGGER
	ret = mc3419_trigger_init(dev);
	if (ret < 0) {
		LOG_ERR("Could not initialize interrupts");
		return ret;
	}
#endif
	ret = mc3419_set_op_mode(cfg, MC3419_MODE_WAKE);
	if (ret < 0) {
		LOG_ERR("Failed to set wake mode");
		return ret;
	}

	k_sem_give(&data->sem);

	LOG_INF("MC3419 Initialized");

	return 0;
}

static const struct sensor_driver_api mc3419_api = {
	.attr_set = mc3419_attr_set,
#if defined CONFIG_MC3419_TRIGGER
	.trigger_set = mc3419_trigger_set,
#endif
	.sample_fetch = mc3419_sample_fetch,
	.channel_get = mc3419_channel_get,
};

#define MC3419_DEFINE(idx)								\
	static const struct mc3419_config mc3419_config_##idx = {			\
		.i2c = I2C_DT_SPEC_INST_GET(idx),					\
		.op_mode = DT_INST_ENUM_IDX(idx, op_mode),				\
		IF_ENABLED(CONFIG_MC3419_TRIGGER,					\
		(.int_gpio = GPIO_DT_SPEC_INST_GET_OR(idx, int_gpios, {0}),))		\
	};										\
	static struct mc3419_driver_data mc3419_data_##idx;				\
	SENSOR_DEVICE_DT_INST_DEFINE(idx,						\
				mc3419_init, NULL,					\
				&mc3419_data_##idx,					\
				&mc3419_config_##idx,					\
				POST_KERNEL,						\
				CONFIG_SENSOR_INIT_PRIORITY,				\
				&mc3419_api);

DT_INST_FOREACH_STATUS_OKAY(MC3419_DEFINE)
