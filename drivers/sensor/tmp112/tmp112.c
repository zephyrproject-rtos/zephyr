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

LOG_MODULE_REGISTER(TMP112, CONFIG_SENSOR_LOG_LEVEL);


#define TMP112_REG_TEMPERATURE		0x00
#define TMP112_D0_BIT			BIT(0)

#define TMP112_REG_CONFIG		0x01
#define TMP112_EM_BIT			BIT(4)
#define TMP112_CR0_BIT			BIT(6)
#define TMP112_CR1_BIT			BIT(7)

/* scale in micro degrees Celsius */
#define TMP112_TEMP_SCALE		62500

struct tmp112_data {
	int16_t sample;
};

struct tmp112_config {
	struct i2c_dt_spec bus;
};

static int tmp112_reg_read(const struct tmp112_config *cfg,
			   uint8_t reg, uint16_t *val)
{
	uint8_t buf[sizeof(val)];

	if (i2c_burst_read_dt(&cfg->bus, reg, buf, sizeof(buf)) < 0) {
		return -EIO;
	}

	*val = sys_get_be16(buf);

	return 0;
}

static int tmp112_reg_write(const struct tmp112_config *cfg,
			    uint8_t reg, uint16_t val)
{
	uint16_t val_be = sys_cpu_to_be16(val);

	return i2c_burst_write_dt(&cfg->bus, reg, (uint8_t *)&val_be, 2);
}

static int tmp112_reg_update(const struct tmp112_config *cfg, uint8_t reg,
			     uint16_t mask, uint16_t val)
{
	uint16_t old_val = 0U;
	uint16_t new_val;

	if (tmp112_reg_read(cfg, reg, &old_val) < 0) {
		return -EIO;
	}

	new_val = old_val & ~mask;
	new_val |= val & mask;

	return tmp112_reg_write(cfg, reg, new_val);
}

static int tmp112_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	const struct tmp112_config *cfg = dev->config;
	int64_t value;
	uint16_t cr;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		/* the sensor supports two ranges -55 to 128 and -55 to 150 */
		/* the value contains the upper limit */
		if (val->val1 == 128) {
			value = 0x0000;
		} else if (val->val1 == 150) {
			value = TMP112_EM_BIT;
		} else {
			return -ENOTSUP;
		}

		if (tmp112_reg_update(cfg, TMP112_REG_CONFIG,
				      TMP112_EM_BIT, value) < 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		return 0;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		/* conversion rate in mHz */
		cr = val->val1 * 1000 + val->val2 / 1000;

		/* the sensor supports 0.25Hz, 1Hz, 4Hz and 8Hz */
		/* conversion rate */
		switch (cr) {
		case 250:
			value = 0x0000;
			break;

		case 1000:
			value = TMP112_CR0_BIT;
			break;

		case 4000:
			value = TMP112_CR1_BIT;
			break;

		case 8000:
			value = TMP112_CR0_BIT | TMP112_CR1_BIT;
			break;

		default:
			return -ENOTSUP;
		}

		if (tmp112_reg_update(cfg, TMP112_REG_CONFIG,
				      TMP112_CR0_BIT | TMP112_CR1_BIT,
				      value) < 0) {
			LOG_DBG("Failed to set attribute!");
			return -EIO;
		}

		return 0;

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

	if (val & TMP112_D0_BIT) {
		drv_data->sample = arithmetic_shift_right((int16_t)val, 3);
	} else {
		drv_data->sample = arithmetic_shift_right((int16_t)val, 4);
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

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C dev %s not ready", cfg->bus.bus->name);
		return -EINVAL;
	}

	return 0;
}

#define TMP112_INST(inst)                                             \
static struct tmp112_data tmp112_data_##inst;                         \
static const struct tmp112_config tmp112_config_##inst = {            \
	.bus = I2C_DT_SPEC_INST_GET(inst)                             \
};                                                                    \
DEVICE_DT_INST_DEFINE(inst, tmp112_init, NULL, &tmp112_data_##inst,   \
		      &tmp112_config_##inst, POST_KERNEL,             \
		      CONFIG_SENSOR_INIT_PRIORITY, &tmp112_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TMP112_INST)
