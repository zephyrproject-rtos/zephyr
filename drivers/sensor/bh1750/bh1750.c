/* bh1750.c - Driver for BH1750 ambient light sensor */
/*
 * Copyright (c) 2022, Michal Morsisko
 * Copyright (c) 2023, Sngular People SL. Jeronimo Agullo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rohm_bh1750

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(BH1750, CONFIG_SENSOR_LOG_LEVEL);

#define BH1750_POWER_DOWN		0x00
#define BH1750_POWER_ON			0x01
#define BH1750_RESET			0x07

#define BH1750_MTREG_HIGH_BYTE		0x40
#define BH1750_MTREG_LOW_BYTE		0x60

enum bh1750_resolution {
	BH1750_H_RESOLUTION_MODE = 0,
	BH1750_H_RESOLUTION_MODE2 = 1,
	BH1750_L_RESOLUTION_MODE = 3
};

enum bh1750_mode {
	BH1750_MODE_CONTINUOUSLY = 0,
	BH1750_MODE_ONE_TIME = 1
};

struct bh1750_data {
	float ambient_light;
};

struct bh1750_config {
	struct i2c_dt_spec i2c;
	enum bh1750_resolution resolution;
	enum bh1750_mode mode;
	uint8_t mtreg;
};

static int bh1750_channel_get(const struct device *dev,
			enum sensor_channel chan,
			struct sensor_value *val)
{
	struct bh1750_data *data = dev->data;

	if (chan != SENSOR_CHAN_LIGHT) {
		LOG_ERR("incorrect channel, please set SENSOR_CHAN_LIGHT");
		return -ENOTSUP;
	}

	val->val1 = (int32_t)(data->ambient_light);
	val->val2 = (data->ambient_light - val->val1) * 1000000;

	return 0;
}


static int bh1750_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct bh1750_data *drv_data = dev->data;
	const struct bh1750_config *cfg = dev->config;
	uint8_t buf[2];
	int rc = 0;

	/* read raw value data */
	rc = i2c_read_dt(&cfg->i2c, buf, 2);

	if (rc < 0) {
		LOG_ERR("Failed to read data sample.");
		return -EIO;
	}

	/* We need to divide by 1.2 since it is the accuracy according to datasheet */
	float data_value = (((uint16_t)buf[0] << 8) | buf[1]) / 1.2;

	/* We need to divide by 2 in case of resolution mode 2 */
	if (cfg->resolution == BH1750_H_RESOLUTION_MODE2) {
		data_value /= 2;
	}

	LOG_DBG("Light value: %f", data_value);
	drv_data->ambient_light = data_value;

	return 0;
}

static int bh1750_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct bh1750_config *cfg = dev->config;
	uint8_t mode;
	uint8_t buf;
	int rc = 0;

	if (chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_CONFIGURATION) {

		/* value in val1 sets the measurement mode */
		if (val->val1 != 0) {
			/* value in val1 sets the sensor mode */
			if (val->val1 == BH1750_MODE_CONTINUOUSLY) {
				mode = BIT(4);
				LOG_DBG("setting continuously mode");
			} else if (val->val1 == BH1750_MODE_ONE_TIME) {
				mode = BIT(5);
				LOG_DBG("setting one time mode");
			} else {
				return -EINVAL;
			}
			buf = cfg->resolution | mode;
			rc = i2c_write_dt(&cfg->i2c, &buf, 1);
			if (rc < 0) {
				LOG_ERR("Error writing in i2c bus: %d", rc);
				return -ENXIO;
			}
			LOG_INF("The mode was set correctly: 0x%x", buf);
		}

		/* value in val2 sets the mtreg time only if it is not zero */
		if (val->val2 != 0) {
			LOG_INF("mtreg: 0x%x", val->val2);
			/* mtreg high bit */
			LOG_INF("mask1: 0x%x", GENMASK(7, 5));
			buf = BH1750_MTREG_HIGH_BYTE | ((val->val2 & GENMASK(7, 5)) >> 5);
			LOG_INF("buf high: 0x%x", buf);
			rc = i2c_write_dt(&cfg->i2c, &buf, 1);

			/* mtreg low bit */
			buf = BH1750_MTREG_LOW_BYTE | (val->val2 & GENMASK(4, 0));
			LOG_INF("buf low: 0x%x", buf);
			rc = i2c_write_dt(&cfg->i2c, &buf, 1);
			LOG_INF("The mode was set correctly: 0x%x", buf);
		}

	} else {
		return -ENOTSUP;
	}

	/* NOTE: in case of BH1750_MODE_ONE_TIME,
	 * we need to wait for the new measurement to be taken before calling sample fetch
	 * According to datasheet page 5 section "Measurement mode explanation"
	 * 120 ms for H_RESOLUTION_MODE and H_RESOLUTION_MODE2
	 * 16 ms for L_RESOLUTION_MODE
	 */
	return 0;
}

static const struct sensor_driver_api bh1750_driver_api = {
	.sample_fetch = bh1750_sample_fetch,
	.channel_get = bh1750_channel_get,
	.attr_set = bh1750_attr_set,
};

static int bh1750_init(const struct device *dev)
{
	const struct bh1750_config *cfg = dev->config;
	uint8_t mode;
	int rc = 0;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	LOG_INF("Bus device is ready");

	/* Set mode */
	if (cfg->mode == BH1750_MODE_CONTINUOUSLY) {
		mode = BIT(4);
		LOG_DBG("setting continuously mode");
	} else if (cfg->mode == BH1750_MODE_ONE_TIME) {
		mode = BIT(5);
		LOG_DBG("setting one time mode");
	} else {
		return -EINVAL;
	}

	uint8_t buf = cfg->resolution | mode;

	rc = i2c_write_dt(&cfg->i2c, &buf, 1);
	if (rc < 0) {
		LOG_ERR("Error writing in i2c bus: %d", rc);
		return -ENXIO;
	}

	LOG_INF("The mode was set correctly: 0x%x", buf);

	return 0;
}

#define BH1750_DEFINE(inst)								\
	static struct bh1750_data bh1750_data_##inst;					\
											\
	/* pull instance configuration from Devicetree */				\
	static const struct bh1750_config bh1750_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.resolution = DT_INST_PROP(inst, resolution),				\
		.mode = DT_INST_PROP(inst, mode),					\
		.mtreg = DT_INST_PROP(inst, mtreg),					\
	};										\
											\
	/* define a new device inst. with the data and config defined above*/		\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bh1750_init, NULL,				\
				&bh1750_data_##inst, &bh1750_config_##inst,		\
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
				&bh1750_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(BH1750_DEFINE)
