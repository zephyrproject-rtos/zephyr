/*
 * Copyright (c) 2016 Firmwave
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp112

#include <device.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include "tmp112.h"

LOG_MODULE_REGISTER(TMP112, CONFIG_SENSOR_LOG_LEVEL);


static int tmp112_reg_read(const struct device *dev,
			   uint8_t reg, uint16_t *val)
{

	const struct tmp112_config *config = dev->config;

	if (i2c_burst_read(config->bus, config->i2c_slv_addr,
			   reg, (uint8_t *)val, 2) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int tmp112_reg_write(const struct device *dev,
			    uint8_t reg, uint16_t val)
{
	const struct tmp112_config *config = dev->config;

	uint16_t val_be = sys_cpu_to_be16(val);

	return i2c_burst_write(config->bus, config->i2c_slv_addr,
			       reg, (uint8_t *)&val_be, 2);
}

static uint16_t set_config_flags(const struct device *dev, uint16_t mask,
				 uint16_t value)
{
	struct tmp112_data *data = dev->data;

	return (data->config_reg & ~mask) | (value & mask);
}

static int tmp112_update_config(const struct device *dev, uint16_t mask,
				uint16_t val)
{
	int rc;
	struct tmp112_data *data = dev->data;
	const uint16_t new_val = set_config_flags(dev, mask, val);

	rc = tmp112_reg_write(dev, TMP112_REG_CONFIG, new_val);
	if (rc == 0) {
		data->config_reg = val;
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
	uint16_t val;

	__ASSERT_NO_MSG(
		chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_AMBIENT_TEMP);

	if (tmp112_reg_read(dev, TMP112_REG_TEMPERATURE, &val) < 0) {
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
	struct tmp112_data *drv_data = dev->data;
	const struct tmp112_config *config = dev->config;

	if (!device_is_ready(config->bus)) {
		LOG_ERR("device %s not ready!", config->bus->name);
		return -EINVAL;
	}

	drv_data->config_reg = TMP112_CONV_RATE(config->cr) | TMP112_CONV_RES_MASK
			       | (config->extended_mode ? TMP112_CONFIG_EM : 0);

	return tmp112_update_config(dev, 0, 0);
}



#define TMP112_DEVICE_DEFINE(n)								 \
	static struct tmp112_data tmp112_driver_##n;					 \
	static const struct tmp112_config tmp112_cfg_##n = {				 \
		.bus = DEVICE_DT_GET(DT_INST_BUS(n)),					 \
		.i2c_slv_addr = DT_INST_REG_ADDR(n),					 \
		.cr = DT_ENUM_IDX(DT_DRV_INST(n), sampling_rate),			 \
		.extended_mode = DT_ENUM_IDX(DT_DRV_INST(n), full_scale),		 \
	};										 \
	DEVICE_DT_INST_DEFINE(n, tmp112_init, NULL, &tmp112_driver_##n,			 \
			      &tmp112_cfg_##n, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, \
			      &tmp112_driver_api);					 \


DT_INST_FOREACH_STATUS_OKAY(TMP112_DEVICE_DEFINE);
