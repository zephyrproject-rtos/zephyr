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

#include <device.h>
#include <i2c.h>
#include <gpio.h>
#include <nanokernel.h>
#include <sensor.h>

#include "sensor_hdc1008.h"

#ifndef CONFIG_SENSOR_DEBUG
#define DBG(...) { ; }
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_SENSOR_DEBUG */

static struct hdc1008_data hdc1008_driver;

static void hdc1008_gpio_callback(struct device *dev, uint32_t pin)
{
	gpio_pin_disable_callback(dev, pin);
	nano_sem_give(&hdc1008_driver.data_sem);
}

static int hdc1008_sample_fetch(struct device *dev)
{
	struct hdc1008_data *drv_data = dev->driver_data;
	uint8_t buf[4];
	int rc;

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HDC1008_GPIO_PIN_NUM);

	buf[0] = HDC1008_REG_TEMP;
	rc = i2c_write(drv_data->i2c, buf, 1, HDC1008_I2C_ADDRESS);
	if (rc != DEV_OK) {
		DBG("Failed to write address pointer\n");
		return DEV_FAIL;
	}

	nano_sem_take(&drv_data->data_sem, TICKS_UNLIMITED);

	rc = i2c_read(drv_data->i2c, buf, 4, HDC1008_I2C_ADDRESS);
	if (rc != DEV_OK) {
		DBG("Failed to read sample data\n");
		return DEV_FAIL;
	}

	drv_data->t_sample = (buf[0] << 8) + buf[1];
	drv_data->rh_sample = (buf[2] << 8) + buf[3];

	return DEV_OK;
}


static int hdc1008_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct hdc1008_data *drv_data = dev->driver_data;
	uint64_t tmp;

	/*
	 * See datasheet "Temperature Register" and "Humidity
	 * Register" sections for more details on processing
	 * sample data.
	 */
	if (chan == SENSOR_CHAN_TEMP) {
		/* val = -40 + 165 * sample / 2^16 */
		tmp = 165 * (uint64_t)drv_data->t_sample;
		val->type = SENSOR_TYPE_INT_PLUS_MICRO;
		val->val1 = (int32_t)(tmp >> 16) - 40;
		val->val2 = (1000000 * (tmp & 0xFFFF)) >> 16;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* val = 100000 * sample / 2^16 */
		tmp = 100000 * (uint64_t)drv_data->rh_sample;
		val->type = SENSOR_TYPE_INT_PLUS_MICRO;
		val->val1 = tmp >> 16;
		val->val2 = (1000000 * (tmp & 0xFFFF)) >> 16;
	} else {
		return DEV_INVALID_OP;
	}

	return DEV_OK;
}

static struct sensor_driver_api hdc1008_driver_api = {
	.sample_fetch = hdc1008_sample_fetch,
	.channel_get = hdc1008_channel_get,
};

int hdc1008_init(struct device *dev)
{
	struct hdc1008_data *drv_data = dev->driver_data;
	int rc;

	dev->driver_api = &hdc1008_driver_api;

	drv_data->i2c = device_get_binding(CONFIG_HDC1008_I2C_MASTER_DEV_NAME);
	if (drv_data->i2c == NULL) {
		DBG("Failed to get pointer to %s device!\n",
		    CONFIG_HDC1008_I2C_MASTER_DEV_NAME);
		return DEV_INVALID_CONF;
	}

	nano_sem_init(&drv_data->data_sem);

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_HDC1008_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		DBG("Failed to get pointer to %s device\n",
		    CONFIG_HDC1008_GPIO_DEV_NAME);
		return DEV_INVALID_CONF;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_HDC1008_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	rc = gpio_set_callback(drv_data->gpio, hdc1008_gpio_callback);
	if (rc != DEV_OK) {
		DBG("Failed to set GPIO callback\n");
		return DEV_FAIL;
	}

	return DEV_OK;
}

DEVICE_INIT(hdc1008, CONFIG_HDC1008_NAME, hdc1008_init, &hdc1008_driver,
	    NULL, SECONDARY, CONFIG_HDC1008_INIT_PRIORITY);
