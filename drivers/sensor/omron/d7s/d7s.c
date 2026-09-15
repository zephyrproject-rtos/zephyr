/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT omron_d7s

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/d7s.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "d7s.h"

LOG_MODULE_REGISTER(D7S, CONFIG_SENSOR_LOG_LEVEL);

int d7s_read_reg(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len)
{
	const struct d7s_config *cfg = dev->config;
	uint8_t addr[2];

	sys_put_be16(reg, addr);

	return i2c_write_read_dt(&cfg->i2c, addr, sizeof(addr), buf, len);
}

int d7s_write_reg(const struct device *dev, uint16_t reg, uint8_t val)
{
	const struct d7s_config *cfg = dev->config;
	uint8_t buf[3];

	sys_put_be16(reg, buf);
	buf[2] = val;

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int d7s_update_ctrl(const struct device *dev, uint8_t mask, uint8_t val)
{
	uint8_t ctrl;
	int ret;

	ret = d7s_read_reg(dev, D7S_REG_CTRL, &ctrl, sizeof(ctrl));
	if (ret < 0) {
		return ret;
	}

	ctrl = (ctrl & ~mask) | (val & mask);

	return d7s_write_reg(dev, D7S_REG_CTRL, ctrl);
}

static int d7s_set_mode(const struct device *dev, uint8_t mode)
{
	uint8_t state;
	int ret;

	ret = d7s_read_reg(dev, D7S_REG_STATE, &state, sizeof(state));
	if (ret < 0) {
		return ret;
	}

	/* The sensor only accepts a mode change from Normal Mode. */
	if (FIELD_GET(D7S_STATE_MASK, state) > D7S_STATE_NORMAL_BUSY) {
		return -EBUSY;
	}

	return d7s_write_reg(dev, D7S_REG_MODE, mode);
}

static int d7s_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct d7s_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP &&
	    chan != (enum sensor_channel)SENSOR_CHAN_D7S_SI &&
	    chan != (enum sensor_channel)SENSOR_CHAN_D7S_PGA) {
		return -ENOTSUP;
	}

	return d7s_read_reg(dev, D7S_REG_LATEST(0), data->record, sizeof(data->record));
}

static int d7s_channel_get(const struct device *dev, enum sensor_channel chan,
			   struct sensor_value *val)
{
	struct d7s_data *data = dev->data;
	int64_t milli;

	switch ((int)chan) {
	case SENSOR_CHAN_D7S_SI:
		milli = (int64_t)sys_get_be16(&data->record[D7S_RECORD_SI]) * 100;
		break;
	case SENSOR_CHAN_D7S_PGA:
		milli = (int64_t)sys_get_be16(&data->record[D7S_RECORD_PGA]) * 100;
		break;
	case SENSOR_CHAN_DIE_TEMP:
		milli = (int64_t)(int16_t)sys_get_be16(&data->record[D7S_RECORD_T_AVE]) * 100;
		break;
	default:
		return -ENOTSUP;
	}

	return sensor_value_from_milli(val, milli);
}

static int d7s_attr_set(const struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_D7S_AXIS_MODE:
		if (val->val1 < D7S_AXIS_MODE_YZ || val->val1 > D7S_AXIS_MODE_SWITCH_AT_INSTALL) {
			return -EINVAL;
		}
		return d7s_update_ctrl(dev, D7S_CTRL_AXIS_MASK,
				       FIELD_PREP(D7S_CTRL_AXIS_MASK, val->val1));
	case SENSOR_ATTR_D7S_SHUTOFF_THRESHOLD:
		if (val->val1 != D7S_SHUTOFF_THRESHOLD_HIGH &&
		    val->val1 != D7S_SHUTOFF_THRESHOLD_LOW) {
			return -EINVAL;
		}
		return d7s_update_ctrl(dev, D7S_CTRL_THRESHOLD,
				       val->val1 == D7S_SHUTOFF_THRESHOLD_LOW ? D7S_CTRL_THRESHOLD
									      : 0);
	case SENSOR_ATTR_D7S_SELFTEST:
		return d7s_set_mode(dev, D7S_MODE_SELF_DIAG);
	case SENSOR_ATTR_D7S_INSTALL:
		return d7s_set_mode(dev, D7S_MODE_INITIAL_INSTALL);
	case SENSOR_ATTR_D7S_CLEAR:
		if ((val->val1 & ~D7S_CLEAR_MASK) != 0) {
			return -EINVAL;
		}
		return d7s_write_reg(dev, D7S_REG_CLEAR_COMMAND, val->val1);
	default:
		return -ENOTSUP;
	}
}

static int d7s_attr_get(const struct device *dev, enum sensor_channel chan,
			enum sensor_attribute attr, struct sensor_value *val)
{
	uint8_t reg;
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_D7S_STATE:
		ret = d7s_read_reg(dev, D7S_REG_STATE, &reg, sizeof(reg));
		reg = FIELD_GET(D7S_STATE_MASK, reg);
		break;
	case SENSOR_ATTR_D7S_AXIS_MODE:
		ret = d7s_read_reg(dev, D7S_REG_CTRL, &reg, sizeof(reg));
		reg = FIELD_GET(D7S_CTRL_AXIS_MASK, reg);
		break;
	case SENSOR_ATTR_D7S_SHUTOFF_THRESHOLD:
		ret = d7s_read_reg(dev, D7S_REG_CTRL, &reg, sizeof(reg));
		reg = (reg & D7S_CTRL_THRESHOLD) ? D7S_SHUTOFF_THRESHOLD_LOW
						 : D7S_SHUTOFF_THRESHOLD_HIGH;
		break;
	case SENSOR_ATTR_D7S_EVENT:
		ret = d7s_read_reg(dev, D7S_REG_EVENT, &reg, sizeof(reg));
		reg &= D7S_EVENT_MASK;
		break;
	default:
		return -ENOTSUP;
	}

	if (ret < 0) {
		return ret;
	}

	val->val1 = reg;
	val->val2 = 0;

	return 0;
}

static int d7s_init(const struct device *dev)
{
	const struct d7s_config *cfg = dev->config;
	uint8_t state;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	/* The D7S has no identification register, so STATE stands in for one. */
	ret = d7s_read_reg(dev, D7S_REG_STATE, &state, sizeof(state));
	if (ret < 0) {
		LOG_ERR("Failed to read STATE (%d)", ret);
		return ret;
	}

	if (FIELD_GET(D7S_STATE_MASK, state) > D7S_STATE_SELF_DIAGNOSTIC) {
		LOG_ERR("Unexpected STATE 0x%02x", state);
		return -ENODEV;
	}

	ret = d7s_update_ctrl(dev, D7S_CTRL_AXIS_MASK | D7S_CTRL_THRESHOLD,
			      FIELD_PREP(D7S_CTRL_AXIS_MASK, cfg->axis_mode) |
				      (cfg->shutoff_threshold ? D7S_CTRL_THRESHOLD : 0));
	if (ret < 0) {
		LOG_ERR("Failed to write CTRL (%d)", ret);
		return ret;
	}

#ifdef CONFIG_D7S_TRIGGER
	ret = d7s_trigger_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialise triggers (%d)", ret);
		return ret;
	}
#endif

	return 0;
}

static DEVICE_API(sensor, d7s_driver_api) = {
	.sample_fetch = d7s_sample_fetch,
	.channel_get = d7s_channel_get,
	.attr_set = d7s_attr_set,
	.attr_get = d7s_attr_get,
#ifdef CONFIG_D7S_TRIGGER
	.trigger_set = d7s_trigger_set,
#endif
};

#define D7S_AXIS_MODE(inst) UTIL_CAT(D7S_AXIS_MODE_, DT_INST_STRING_UPPER_TOKEN(inst, axis_mode))

#define D7S_SHUTOFF_THRESHOLD(inst)                                                                \
	UTIL_CAT(D7S_SHUTOFF_THRESHOLD_, DT_INST_STRING_UPPER_TOKEN(inst, shutoff_threshold))

#ifdef CONFIG_D7S_TRIGGER
#define D7S_TRIGGER_INIT(inst)                                                                     \
	.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0}),                                   \
	.int2 = GPIO_DT_SPEC_INST_GET_OR(inst, int2_gpios, {0}),
#else
#define D7S_TRIGGER_INIT(inst)
#endif

#define D7S_DEFINE(inst)                                                                           \
	static struct d7s_data d7s_data_##inst;                                                    \
                                                                                                   \
	static const struct d7s_config d7s_config_##inst = {.i2c = I2C_DT_SPEC_INST_GET(inst),     \
							    .axis_mode = D7S_AXIS_MODE(inst),      \
							    .shutoff_threshold =                   \
								    D7S_SHUTOFF_THRESHOLD(inst),   \
							    D7S_TRIGGER_INIT(inst)};               \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, d7s_init, NULL, &d7s_data_##inst, &d7s_config_##inst,   \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &d7s_driver_api);

DT_INST_FOREACH_STATUS_OKAY(D7S_DEFINE)
