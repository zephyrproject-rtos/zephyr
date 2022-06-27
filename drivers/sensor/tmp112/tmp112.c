/*
 * Copyright (c) 2016 Firmwave
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp112

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>
#include "tmp112.h"

LOG_MODULE_REGISTER(TMP112, CONFIG_SENSOR_LOG_LEVEL);

static int tmp112_reg_read(const struct tmp112_config *cfg,
			   uint8_t reg, uint16_t *val)
{
	if (i2c_burst_read_dt(&cfg->bus, reg, (uint8_t *)val, sizeof(*val)) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int tmp112_reg_write(const struct tmp112_config *cfg,
			    uint8_t reg, uint16_t val)
{
	uint8_t buf[3];

	buf[0] = reg;
	sys_put_be16(val, &buf[1]);

	return i2c_write_dt(&cfg->bus, buf, sizeof(buf));
}

static uint16_t set_config_flags(struct tmp112_data *data, uint16_t mask,
				 uint16_t value)
{
	return (data->config_reg & ~mask) | (value & mask);
}

static int tmp112_update_config(const struct device *dev, uint16_t mask,
				uint16_t val)
{
	int rc;
	struct tmp112_data *data = dev->data;
	const uint16_t new_val = set_config_flags(data, mask, val);

	rc = tmp112_reg_write(dev->config, TMP112_REG_CONFIG, new_val);
	if (rc == 0) {
		data->config_reg = new_val;
	}

	return rc;
}

static int tmp112_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	uint16_t value;
	uint16_t cr;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
#if CONFIG_TMP112_FULL_SCALE_RUNTIME
	case SENSOR_ATTR_FULL_SCALE:
		/* the sensor supports two ranges -55 to 128 and -55 to 150 */
		/* the value contains the upper limit */
		if (val->val1 == 128) {
			value = 0x0000;
		} else if (val->val1 == 150) {
			value = TMP112_CONFIG_EM;
		} else {
			return -ENOTSUP;
		}

		if (tmp112_update_config(dev, TMP112_CONFIG_EM, value) < 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}
		break;
#endif
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
#if CONFIG_TMP112_SAMPLING_FREQUENCY_RUNTIME
		/* conversion rate in mHz */
		cr = val->val1 * 1000 + val->val2 / 1000;

		/* the sensor supports 0.25Hz, 1Hz, 4Hz and 8Hz */
		/* conversion rate */
		switch (cr) {
		case 250:
			value = TMP112_CONV_RATE(TMP112_CONV_RATE_025);
			break;

		case 1000:
			value = TMP112_CONV_RATE(TMP112_CONV_RATE_1000);
			break;

		case 4000:
			value = TMP112_CONV_RATE(TMP112_CONV_RATE_4);
			break;

		case 8000:
			value = TMP112_CONV_RATE(TMP112_CONV_RATE_8);
			break;

		default:
			return -ENOTSUP;
		}

		if (tmp112_update_config(dev, TMP112_CONV_RATE_MASK, value) < 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		break;
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int tmp112_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct tmp112_data *drv_data = dev->data;
	const struct tmp112_config *cfg = dev->config;
	uint16_t val;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (tmp112_reg_read(cfg, TMP112_REG_TEMPERATURE, &val) < 0) {
		return -EIO;
	}

	if (val & TMP112_DATA_EXTENDED) {
		drv_data->sample = arithmetic_shift_right((int16_t)val,
							  TMP112_DATA_EXTENDED_SHIFT);
	} else {
		drv_data->sample = arithmetic_shift_right((int16_t)val,
							  TMP112_DATA_NORMAL_SHIFT);
	}

	return 0;
}

static int tmp112_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct tmp112_data *drv_data = dev->data;
	int32_t uval;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	uval = (int32_t)drv_data->sample * TMP112_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	return 0;
}

static const struct sensor_driver_api tmp112_driver_api = {
	.attr_set = tmp112_attr_set,
	.sample_fetch = tmp112_sample_fetch,
	.channel_get = tmp112_channel_get,
};

int tmp112_init(const struct device *dev)
{
	const struct tmp112_config *cfg = dev->config;
	struct tmp112_data *data = dev->data;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -EINVAL;
	}

	data->config_reg = TMP112_CONV_RATE(cfg->cr) | TMP112_CONV_RES_MASK
			   | (cfg->extended_mode ? TMP112_CONFIG_EM : 0);

	return tmp112_update_config(dev, 0, 0);
}


#define TMP112_INST(inst)						    \
	static struct tmp112_data tmp112_data_##inst;			    \
	static const struct tmp112_config tmp112_config_##inst = {	    \
		.bus = I2C_DT_SPEC_INST_GET(inst),			    \
		.cr = DT_INST_ENUM_IDX(inst, conversion_rate),		    \
		.extended_mode = DT_INST_PROP(inst, extended_mode),	    \
	};								    \
									    \
	DEVICE_DT_INST_DEFINE(inst, tmp112_init, NULL, &tmp112_data_##inst, \
			      &tmp112_config_##inst, POST_KERNEL,	    \
			      CONFIG_SENSOR_INIT_PRIORITY, &tmp112_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMP112_INST)
