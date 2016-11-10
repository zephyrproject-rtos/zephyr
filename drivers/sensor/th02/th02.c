/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <kernel.h>
#include <device.h>
#include <i2c.h>
#include <misc/byteorder.h>
#include <misc/util.h>
#include <sensor.h>
#include <misc/__assert.h>

#include "th02.h"

static uint8_t read8(struct device *dev, uint8_t d)
{
	uint8_t buf;

	if (i2c_reg_read_byte(dev, TH02_I2C_DEV_ID, d, &buf) < 0) {
		SYS_LOG_ERR("Error reading register.");
	}
	return buf;
}

static int is_ready(struct device *dev)
{

	uint8_t status;

	if (i2c_reg_read_byte(dev, TH02_I2C_DEV_ID,
			      TH02_REG_STATUS, &status) < 0) {
		SYS_LOG_ERR("error reading status register");
	}

	if (status & TH02_STATUS_RDY_MASK) {
		return 0;
	} else {
		return 1;
	}
}

static uint16_t get_humi(struct device *dev)
{
	uint16_t humidity = 0;

	if (i2c_reg_write_byte(dev, TH02_I2C_DEV_ID,
			       TH02_REG_CONFIG, TH02_CMD_MEASURE_HUMI) < 0) {
		SYS_LOG_ERR("Error writing register");
		return 0;
	}
	while (!is_ready(dev)) {
	}
	;

	humidity = read8(dev, TH02_REG_DATA_H) << 8;
	humidity |= read8(dev, TH02_REG_DATA_L);
	humidity >>= 4;

	return humidity;
}

uint16_t get_temp(struct device *dev)
{
	uint16_t temperature = 0;

	if (i2c_reg_write_byte(dev, TH02_I2C_DEV_ID,
			       TH02_REG_CONFIG, TH02_CMD_MEASURE_TEMP) < 0) {
		SYS_LOG_ERR("Error writing register");
		return 0;
	}
	while (!is_ready(dev)) {
	}
	;

	temperature = read8(dev, TH02_REG_DATA_H) << 8;
	temperature |= read8(dev, TH02_REG_DATA_L);
	temperature >>= 2;

	return temperature;
}

static int th02_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct th02_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_TEMP);

	drv_data->t_sample = get_temp(drv_data->i2c);
	SYS_LOG_INF("temp: %u", drv_data->t_sample);
	drv_data->rh_sample = get_humi(drv_data->i2c);
	SYS_LOG_INF("rh: %u", drv_data->rh_sample);

	return 0;
}

static int th02_channel_get(struct device *dev, enum sensor_channel chan,
			    struct sensor_value *val)
{
	struct th02_data *drv_data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_TEMP ||
			chan == SENSOR_CHAN_HUMIDITY);

	if (chan == SENSOR_CHAN_TEMP) {
		val->type = SENSOR_VALUE_TYPE_DOUBLE;
		val->dval = (double)drv_data->t_sample  / 32.0 - 50.0;
	} else {
		val->type = SENSOR_VALUE_TYPE_DOUBLE;
		val->dval = (double)drv_data->rh_sample / 16.0 - 24.0;
	}

	return 0;
}

static const struct sensor_driver_api th02_driver_api = {
	.sample_fetch = th02_sample_fetch,
	.channel_get = th02_channel_get,
};

static int th02_init(struct device *dev)
{
	struct th02_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_TH02_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
			    CONFIG_TH02_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	dev->driver_api = &th02_driver_api;

	return 0;
}

static struct th02_data th02_driver;

DEVICE_INIT(th02, CONFIG_TH02_NAME, th02_init, &th02_driver,
	    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY);
