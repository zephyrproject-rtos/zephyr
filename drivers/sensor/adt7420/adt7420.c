/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adt7420

#include <device.h>
#include <drivers/i2c.h>
#include <sys/byteorder.h>
#include <sys/util.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>

#include "adt7420.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ADT7420, CONFIG_SENSOR_LOG_LEVEL);

static int adt7420_temp_reg_read(const struct device *dev, uint8_t reg,
				 int16_t *val)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;

	if (i2c_burst_read(drv_data->i2c, cfg->i2c_addr,
			   reg, (uint8_t *) val, 2) < 0) {
		return -EIO;
	}

	*val = sys_be16_to_cpu(*val);

	return 0;
}

static int adt7420_temp_reg_write(const struct device *dev, uint8_t reg,
				  int16_t val)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t buf[3] = {reg, val >> 8, val & 0xFF};

	return i2c_write(drv_data->i2c, buf, sizeof(buf), cfg->i2c_addr);
}

static int adt7420_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t val8, reg = 0U;
	uint16_t rate;
	int64_t value;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		rate = val->val1 * 1000 + val->val2 / 1000; /* rate in mHz */

		switch (rate) {
		case 240:
			val8 = ADT7420_OP_MODE_CONT_CONV;
			break;
		case 1000:
			val8 = ADT7420_OP_MODE_1_SPS;
			break;
		default:
			return -EINVAL;
		}

		if (i2c_reg_update_byte(drv_data->i2c, cfg->i2c_addr,
					ADT7420_REG_CONFIG,
					ADT7420_CONFIG_OP_MODE(~0),
					ADT7420_CONFIG_OP_MODE(val8)) < 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		return 0;
	case SENSOR_ATTR_UPPER_THRESH:
		reg = ADT7420_REG_T_HIGH_MSB;
		__fallthrough;
	case SENSOR_ATTR_LOWER_THRESH:
		if (!reg) {
			reg = ADT7420_REG_T_LOW_MSB;
		}

		if ((val->val1 > 150) || (val->val1 < -40)) {
			return -EINVAL;
		}

		value = (int64_t)val->val1 * 1000000 + val->val2;
		value = (value / ADT7420_TEMP_SCALE) << 1;

		if (adt7420_temp_reg_write(dev, reg, value) < 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		return 0;
	default:
		return -ENOTSUP;
	}

	return 0;
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

	drv_data->sample = value >> 1; /* use 15-bit only */

	return 0;
}

static int adt7420_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adt7420_data *drv_data = dev->data;
	int32_t value;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	value = (int32_t)drv_data->sample * ADT7420_TEMP_SCALE;
	val->val1 = value / 1000000;
	val->val2 = value % 1000000;

	return 0;
}

static const struct sensor_driver_api adt7420_driver_api = {
	.attr_set = adt7420_attr_set,
	.sample_fetch = adt7420_sample_fetch,
	.channel_get = adt7420_channel_get,
#ifdef CONFIG_ADT7420_TRIGGER
	.trigger_set = adt7420_trigger_set,
#endif
};

static int adt7420_probe(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;
	uint8_t value;
	int ret;

	ret = i2c_reg_read_byte(drv_data->i2c, cfg->i2c_addr,
				ADT7420_REG_ID, &value);
	if (ret) {
		return ret;
	}

	if (value != ADT7420_DEFAULT_ID) {
		return -ENODEV;
	}

	ret = i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
			ADT7420_REG_CONFIG, ADT7420_CONFIG_RESOLUTION |
			ADT7420_CONFIG_OP_MODE(ADT7420_OP_MODE_CONT_CONV));
	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte(drv_data->i2c, cfg->i2c_addr,
				 ADT7420_REG_HIST, CONFIG_ADT7420_TEMP_HYST);
	if (ret) {
		return ret;
	}
	ret = adt7420_temp_reg_write(dev, ADT7420_REG_T_CRIT_MSB,
				     (CONFIG_ADT7420_TEMP_CRIT * 1000000 /
				     ADT7420_TEMP_SCALE) << 1);
	if (ret) {
		return ret;
	}

#ifdef CONFIG_ADT7420_TRIGGER
	if (adt7420_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return -EIO;
	}
#endif

	return 0;
}

static int adt7420_init(const struct device *dev)
{
	struct adt7420_data *drv_data = dev->data;
	const struct adt7420_dev_config *cfg = dev->config;

	drv_data->i2c = device_get_binding(cfg->i2c_port);
	if (drv_data->i2c == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			    cfg->i2c_port);
		return -EINVAL;
	}

	return adt7420_probe(dev);
}

static struct adt7420_data adt7420_driver;

static const struct adt7420_dev_config adt7420_config = {
	.i2c_port = DT_INST_BUS_LABEL(0),
	.i2c_addr = DT_INST_REG_ADDR(0),
#ifdef CONFIG_ADT7420_TRIGGER
	.int_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.int_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
	.int_name = DT_INST_GPIO_LABEL(0, int_gpios),
#endif
};

DEVICE_AND_API_INIT(adt7420, DT_INST_LABEL(0), adt7420_init, &adt7420_driver,
		    &adt7420_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &adt7420_driver_api);
