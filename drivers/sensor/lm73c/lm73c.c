/*
 * Copyright (c) 2020 SER Consulting LLC / Steven Riedl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_lm73c

#include "lm73c.h"
#include <logging/log.h>

LOG_MODULE_REGISTER(LM73C, CONFIG_SENSOR_LOG_LEVEL);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "LM73C driver enabled without any devices"
#endif

/**
 * @brief fetch a sample from the sensor
 *
 * @return 0
 */
static int lm73c_sample_fetch(const struct device *dev,
			enum sensor_channel chan)
{
	int retval;
	struct lm73c_data *lm73_data = dev->data;
	const struct lm73c_config *cfg = dev->config;
	uint8_t temp[2];

	retval = i2c_burst_read(lm73_data->i2c_dev,
				cfg->i2c_address,
				LM73_REG_TEMP,
				temp, sizeof(temp));

	if (retval == 0) {
		lm73_data->temp = (int32_t)(int16_t)((temp[0] << 8) | temp[1]);
		lm73_data->temp = (lm73_data->temp >> 2) * 31250;
	} else {
		/* if we didn't read the temp, set it to an invalid value*/
		lm73_data->temp = 1;
		LOG_ERR("lm73c read register err");
	}

	return retval;
}

/**
 * @brief sensor value get
 *
 * @return -ENOTSUP for unsupported channels
 */
static int lm73c_channel_get(const struct device *dev,
			enum sensor_channel chan,
			struct sensor_value *val)
{
	struct lm73c_data *lm73_data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}
	/* If the temp is not valid */
	if (lm73_data->temp == 1) {
		return -EINVAL;
	}
	val->val1 = lm73_data->temp / 1000000;
	val->val2 = lm73_data->temp % 1000000;
	return 0;
}

/**
 * @brief initialize the sensor registers
 *
 * @return 0 for success
 */

static int lm73c_chip_init(const struct device *dev)
{
	const char *name = dev->name;
	struct lm73c_data *drv_data = dev->data;
	const struct lm73c_config *config = dev->config;
	uint8_t temp[2];

	/* set resolution to config, default .03125 deg C */
	temp[0] = config->temp_bits << 5;
	if (i2c_burst_write(drv_data->i2c_dev,
				config->i2c_address,
				LM73_REG_CTRL, temp, 1)) {
		LOG_ERR("%s: Set resolution err", name);
		return -EIO;
	}
	/* kick off the first read */
	if (i2c_burst_read(drv_data->i2c_dev,
				config->i2c_address, LM73_REG_TEMP,
				temp, sizeof(temp))) {
		drv_data->temp = 1;
		LOG_ERR("%s: First temp read err", name);
		return -EIO;
	}
	drv_data->temp = (int32_t)(int16_t)((temp[0] << 8) | temp[1]);
	drv_data->temp = (drv_data->temp >> 2) * 31250;
	LOG_DBG("%s: Initialized", name);
	return 0;
}

static const struct sensor_driver_api lm73c_api = {
	.sample_fetch = &lm73c_sample_fetch,
	.channel_get = &lm73c_channel_get,
};

/**
 * @brief initialize the sensor
 *
 * @return 0 for success
 */

static int lm73c_init(const struct device *dev)
{
	const char *name = dev->name;
	struct lm73c_data *drv_data = dev->data;
	const struct lm73c_config *config = dev->config;
	uint8_t temp[2];

	LOG_DBG("%s: Initializing", name);
	drv_data->i2c_dev = device_get_binding(DT_INST_BUS_LABEL(0));

	if (drv_data->i2c_dev == NULL) {
		LOG_ERR("%s: i2c master not found: 0", name);
		return -EINVAL;
	}

	if (i2c_burst_read(drv_data->i2c_dev, config->i2c_address,
					   LM73_REG_ID, temp, sizeof(temp))) {
		LOG_ERR("%s: Read ID register err", name);
		return -EIO;
	}

	uint16_t id = (temp[0] << 8) | temp[1];

	if (id != LM73_ID) {
		LOG_ERR("%s: incorrect deviceid (0x0190) = %x", name, id);
		return -EIO;
	}

	return(lm73c_chip_init(dev));
}

#define LM73C_INIT(index)						\
static struct lm73c_data lm73c_driver_##index;		\
\
static const struct lm73c_config lm73c_config_##index = { \
.i2c_name = DT_INST_BUS_LABEL(index),			\
.i2c_address = DT_INST_REG_ADDR(index),			\
.temp_bits = DT_ENUM_IDX(DT_DRV_INST(index), temp_prec), \
};								\
\
DEVICE_AND_API_INIT(lm73c_##index, DT_INST_LABEL(index),	\
&lm73c_init, &lm73c_driver_##index, \
&lm73c_config_##index, POST_KERNEL,	\
CONFIG_SENSOR_INIT_PRIORITY,		\
&lm73c_api)

DT_INST_FOREACH_STATUS_OKAY(LM73C_INIT);
