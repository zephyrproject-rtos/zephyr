/*
 * Copyright (c) 2024 Calian Advanced Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT national_lm95234

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/lm95234.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(LM95234, CONFIG_SENSOR_LOG_LEVEL);

#define LM95234_REG_LOCAL_TEMP_SIGNED_MSB				0x10
#define LM95234_REG_LOCAL_TEMP_SIGNED_LSB				0x20
#define LM95234_REG_REMOTE_TEMP_1_SIGNED_MSB				0x11
#define LM95234_REG_REMOTE_TEMP_1_SIGNED_LSB				0x21
#define LM95234_REG_REMOTE_TEMP_2_SIGNED_MSB				0x12
#define LM95234_REG_REMOTE_TEMP_2_SIGNED_LSB				0x22
#define LM95234_REG_REMOTE_TEMP_3_SIGNED_MSB				0x13
#define LM95234_REG_REMOTE_TEMP_3_SIGNED_LSB				0x23
#define LM95234_REG_REMOTE_TEMP_4_SIGNED_MSB				0x14
#define LM95234_REG_REMOTE_TEMP_4_SIGNED_LSB				0x24
#define LM95234_REG_REMOTE_TEMP_1_UNSIGNED_MSB				0x19
#define LM95234_REG_REMOTE_TEMP_1_UNSIGNED_LSB				0x29
#define LM95234_REG_REMOTE_TEMP_2_UNSIGNED_MSB				0x1a
#define LM95234_REG_REMOTE_TEMP_2_UNSIGNED_LSB				0x2a
#define LM95234_REG_REMOTE_TEMP_3_UNSIGNED_MSB				0x1b
#define LM95234_REG_REMOTE_TEMP_3_UNSIGNED_LSB				0x2b
#define LM95234_REG_REMOTE_TEMP_4_UNSIGNED_MSB				0x1c
#define LM95234_REG_REMOTE_TEMP_4_UNSIGNED_LSB				0x2c
#define LM95234_REG_DIODE_MODEL_SELECT					0x30
#define LM95234_REG_REMOTE_1_OFFSET					0x31
#define LM95234_REG_REMOTE_2_OFFSET					0x32
#define LM95234_REG_REMOTE_3_OFFSET					0x33
#define LM95234_REG_REMOTE_4_OFFSET					0x34
#define LM95234_REG_CONFIG						0x03
#define LM95234_REG_CONV_RATE						0x04
#define LM95234_REG_CHANNEL_CONV_ENABLE					0x05
#define LM95234_REG_FILTER_SETTING					0x06
#define LM95234_REG_ONESHOT						0x0f
#define LM95234_REG_COMMON_STATUS					0x02
#define LM95234_REG_STATUS_1						0x07
#define LM95234_REG_STATUS_2						0x08
#define LM95234_REG_STATUS_3						0x09
#define LM95234_REG_STATUS_4						0x0a
#define LM95234_REG_DIODE_MODEL_STATUS					0x38
#define LM95234_REG_TCRIT1_MASK						0x0c
#define LM95234_REG_TCRIT2_MASK						0x0d
#define LM95234_REG_TCRIT3_MASK						0x0e
#define LM95234_REG_LOCAL_TCRIT_LIMIT					0x40
#define LM95234_REG_REMOTE1_TCRIT1_LIMIT				0x41
#define LM95234_REG_REMOTE2_TCRIT1_LIMIT				0x42
#define LM95234_REG_REMOTE3_TCRIT_LIMIT					0x43
#define LM95234_REG_REMOTE4_TCRIT_LIMIT					0x44
#define LM95234_REG_REMOTE1_TCRIT23_LIMIT				0x49
#define LM95234_REG_REMOTE2_TCRIT23_LIMIT				0x4a
#define LM95234_REG_COMMON_TCRIT_HYSTERESIS				0x5a
#define LM95234_REG_MANUF_ID						0xfe
#define LM95234_REG_REV_ID						0xff

#define LM95234_MAN_ID							0x01
#define LM95234_CHIP_ID							0x79

#define LM95234_CONFIG_STANDBY						BIT(6)

struct lm95234_data {
	/** Temperatures in raw format read from sensor */
	int32_t local;
	int32_t remote[4];
};

struct lm95234_config {
	struct i2c_dt_spec i2c;
};

static inline int lm95234_fetch_temp(const struct lm95234_config *cfg, struct lm95234_data *data,
				     enum sensor_channel chan, int32_t *output)
{
	int ret;
	uint8_t val;
	int32_t result = 0;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		uint16_t temp;
		int offset = (chan - SENSOR_CHAN_LM95234_REMOTE_TEMP_1);

		ret = i2c_reg_read_byte_dt(&cfg->i2c,
					   LM95234_REG_REMOTE_TEMP_1_UNSIGNED_MSB + offset, &val);
		if (ret) {
			return ret;
		}
		temp = val << 8;
		ret = i2c_reg_read_byte_dt(&cfg->i2c,
					  LM95234_REG_REMOTE_TEMP_1_UNSIGNED_LSB + offset, &val);
		if (ret) {
			return ret;
		}
		temp |= val;
		result = temp;
	}

	/* Read signed temperature if unsigned temperature is 0, or for local sensor */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP || result == 0) {
		int offset = chan == SENSOR_CHAN_AMBIENT_TEMP ? 0 :
			(chan - SENSOR_CHAN_LM95234_REMOTE_TEMP_1 + 1);
		int16_t temp;

		ret = i2c_reg_read_byte_dt(&cfg->i2c,
					  LM95234_REG_LOCAL_TEMP_SIGNED_MSB + offset, &val);
		if (ret) {
			return ret;
		}
		temp = val << 8;
		ret = i2c_reg_read_byte_dt(&cfg->i2c,
					   LM95234_REG_LOCAL_TEMP_SIGNED_LSB + offset, &val);
		if (ret) {
			return ret;
		}
		temp |= val;
		result = temp;
	}
	*output = result;
	return 0;
}

static int lm95234_sample_fetch(const struct device *dev,
			     enum sensor_channel chan)
{
	struct lm95234_data *data = dev->data;
	const struct lm95234_config *cfg = dev->config;
	enum pm_device_state pm_state;
	int ret;

	(void)pm_device_state_get(dev, &pm_state);
	if (pm_state != PM_DEVICE_STATE_ACTIVE) {
		ret = -EIO;
		return ret;
	}

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_ALL:
		ret = lm95234_fetch_temp(cfg, data, SENSOR_CHAN_AMBIENT_TEMP, &data->local);
		if (ret)
			return ret;
		for (int i = 0; i < ARRAY_SIZE(data->remote); i++) {
			ret = lm95234_fetch_temp(cfg, data,
						 SENSOR_CHAN_LM95234_REMOTE_TEMP_1 + i,
						 &data->remote[i]);
			if (ret)
				return ret;
		}
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		ret = lm95234_fetch_temp(cfg, data, chan, &data->local);
		break;
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_1:
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_2:
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_3:
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_4:
		ret = lm95234_fetch_temp(cfg, data, chan,
					 &data->remote[chan - SENSOR_CHAN_LM95234_REMOTE_TEMP_1]);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int lm95234_channel_get(const struct device *dev,
			    enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct lm95234_data *data = dev->data;
	int32_t raw_temp;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		raw_temp = data->local;
		break;
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_1:
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_2:
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_3:
	case SENSOR_CHAN_LM95234_REMOTE_TEMP_4:
		raw_temp = data->remote[chan - SENSOR_CHAN_LM95234_REMOTE_TEMP_1];
		break;
	default:
		return -ENOTSUP;
	}

	/* Raw data format is 8 bits integer, 5 bits fractional, 3 bits zero */
	val->val1 = raw_temp / 256;
	val->val2 = (raw_temp % 256) * 1000000 / 256;
	return 0;
}

static const struct sensor_driver_api lm95234_driver_api = {
	.sample_fetch = lm95234_sample_fetch,
	.channel_get = lm95234_channel_get,
};

static int lm95234_init(const struct device *dev)
{
	const struct lm95234_config *cfg = dev->config;
	int ret = 0;
	uint8_t value, model_select, model_status;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C dev not ready");
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, LM95234_REG_MANUF_ID, &value);
	if (ret) {
		LOG_ERR("Could not read manufacturer ID: %d", ret);
		return ret;
	}
	if (value != LM95234_MAN_ID) {
		LOG_ERR("Invalid manufacturer ID: %02x", value);
		return -ENODEV;
	}
	ret = i2c_reg_read_byte_dt(&cfg->i2c, LM95234_REG_REV_ID, &value);
	if (ret) {
		LOG_ERR("Could not read revision ID: %d", ret);
		return ret;
	}
	if (value != LM95234_CHIP_ID) {
		LOG_ERR("Invalid chip ID: %02x", value);
		return -ENODEV;
	}
	ret = i2c_reg_read_byte_dt(&cfg->i2c, LM95234_REG_CONFIG, &value);
	if (ret) {
		LOG_ERR("Could not read config: %d", ret);
		return ret;
	}
	if (value & LM95234_CONFIG_STANDBY) {
		value &= ~LM95234_CONFIG_STANDBY;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, LM95234_REG_CONFIG, value);
		if (ret) {
			LOG_ERR("Could not write config: %d", ret);
			return ret;
		}
	}
	ret = i2c_reg_read_byte_dt(&cfg->i2c, LM95234_REG_DIODE_MODEL_SELECT, &model_select);
	if (ret) {
		LOG_ERR("Could not read diode model select: %d", ret);
		return ret;
	}
	ret = i2c_reg_read_byte_dt(&cfg->i2c, LM95234_REG_DIODE_MODEL_STATUS, &model_status);
	if (ret) {
		LOG_ERR("Could not read diode model status: %d", ret);
		return ret;
	}
	/**
	 * Check if any remote inputs have a 3904 transistor detected but are not configured
	 * as such. If so, configure them as 3904 transistors.
	 */
	if (model_select & model_status) {
		model_select &= ~model_status;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, LM95234_REG_DIODE_MODEL_SELECT,
					    model_select);
		if (ret) {
			LOG_ERR("Could not write diode model select: %d", ret);
			return ret;
		}
	}

#ifdef CONFIG_PM_DEVICE_RUNTIME
	pm_device_init_suspended(dev);

	ret = pm_device_runtime_enable(dev);
	if (ret < 0 && ret != -ENOTSUP) {
		LOG_ERR("Failed to enable runtime power management");
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int lm95234_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
	case PM_DEVICE_ACTION_RESUME:
	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#endif

#define LM95234_INST(inst)                                     \
static struct lm95234_data lm95234_data_##inst;                \
static const struct lm95234_config lm95234_config_##inst = {   \
	.i2c = I2C_DT_SPEC_INST_GET(inst),                     \
};                                                             \
PM_DEVICE_DT_INST_DEFINE(inst, lm95234_pm_action);	       \
SENSOR_DEVICE_DT_INST_DEFINE(inst, lm95234_init,               \
			     PM_DEVICE_DT_INST_GET(inst),      \
			     &lm95234_data_##inst,	       \
			     &lm95234_config_##inst,           \
			     POST_KERNEL,                      \
			     CONFIG_SENSOR_INIT_PRIORITY,      \
			     &lm95234_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LM95234_INST)
