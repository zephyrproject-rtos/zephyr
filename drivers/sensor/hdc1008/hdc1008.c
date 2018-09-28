/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <i2c.h>
#include <gpio.h>
#include <kernel.h>
#include <sensor.h>
#include <misc/util.h>
#include <misc/__assert.h>

#include "hdc1008.h"

static void hdc1008_gpio_callback(struct device *dev,
				  struct gpio_callback *cb, u32_t pins)
{
	struct hdc1008_data *drv_data =
		CONTAINER_OF(cb, struct hdc1008_data, gpio_cb);

	ARG_UNUSED(pins);

	gpio_pin_disable_callback(dev, CONFIG_HDC1008_GPIO_PIN_NUM);
	k_sem_give(&drv_data->data_sem);
}

static int hdc1008_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct hdc1008_data *drv_data = dev->driver_data;
	u8_t buf[4];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	gpio_pin_enable_callback(drv_data->gpio, CONFIG_HDC1008_GPIO_PIN_NUM);

	buf[0] = HDC1008_REG_TEMP;
	if (i2c_write(drv_data->i2c, buf, 1, CONFIG_HDC1008_I2C_ADDR) < 0) {
		SYS_LOG_DBG("Failed to write address pointer");
		return -EIO;
	}

	k_sem_take(&drv_data->data_sem, K_FOREVER);

	if (i2c_read(drv_data->i2c, buf, 4, CONFIG_HDC1008_I2C_ADDR) < 0) {
		SYS_LOG_DBG("Failed to read sample data");
		return -EIO;
	}

	drv_data->t_sample = (buf[0] << 8) + buf[1];
	drv_data->rh_sample = (buf[2] << 8) + buf[3];

	return 0;
}


static int hdc1008_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct hdc1008_data *drv_data = dev->driver_data;
	u64_t tmp;

	/*
	 * See datasheet "Temperature Register" and "Humidity
	 * Register" sections for more details on processing
	 * sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* val = -40 + 165 * sample / 2^16 */
		tmp = 165 * (u64_t)drv_data->t_sample;
		val->val1 = (s32_t)(tmp >> 16) - 40;
		val->val2 = (1000000 * (tmp & 0xFFFF)) >> 16;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* val = 100 * sample / 2^16 */
		u32_t tmp2;
		tmp2 = 100 * (u32_t)drv_data->rh_sample;
		val->val1 = tmp2 >> 16;
		/* x * 1000000 / 65536 == x * 15625 / 1024 */
		val->val2 = (15625 * (tmp2 & 0xFFFF)) >> 10;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api hdc1008_driver_api = {
	.sample_fetch = hdc1008_sample_fetch,
	.channel_get = hdc1008_channel_get,
};

static u16_t read16(struct device *dev, u8_t a, u8_t d)
{
	u8_t buf[2];
	if (i2c_burst_read(dev, a, d, (u8_t *)buf, 2) < 0) {
		SYS_LOG_ERR("Error reading register.");
	}
	return (buf[0] << 8 | buf[1]);
}

static int hdc1008_init(struct device *dev)
{
	struct hdc1008_data *drv_data = dev->driver_data;

	drv_data->i2c = device_get_binding(CONFIG_HDC1008_I2C_MASTER_DEV_NAME);

	if (drv_data->i2c == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device!",
			    CONFIG_HDC1008_I2C_MASTER_DEV_NAME);
		return -EINVAL;
	}

	if (read16(drv_data->i2c, CONFIG_HDC1008_I2C_ADDR, HDC1000_MANUFID)
	    != 0x5449) {
		SYS_LOG_ERR("Failed to get correct manufacturer ID");
		return -EINVAL;
	}
	if (read16(drv_data->i2c, CONFIG_HDC1008_I2C_ADDR, HDC1000_DEVICEID)
	    != 0x1000) {
		SYS_LOG_ERR("Failed to get correct device ID");
		return -EINVAL;
	}

	k_sem_init(&drv_data->data_sem, 0, UINT_MAX);

	/* setup data ready gpio interrupt */
	drv_data->gpio = device_get_binding(CONFIG_HDC1008_GPIO_DEV_NAME);
	if (drv_data->gpio == NULL) {
		SYS_LOG_DBG("Failed to get pointer to %s device",
			    CONFIG_HDC1008_GPIO_DEV_NAME);
		return -EINVAL;
	}

	gpio_pin_configure(drv_data->gpio, CONFIG_HDC1008_GPIO_PIN_NUM,
			   GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE |
#if defined(CONFIG_HDC1008_GPIO_FLAGS)
			   CONFIG_HDC1008_GPIO_FLAGS |
#endif
			   GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE);

	gpio_init_callback(&drv_data->gpio_cb,
			   hdc1008_gpio_callback,
			   BIT(CONFIG_HDC1008_GPIO_PIN_NUM));

	if (gpio_add_callback(drv_data->gpio, &drv_data->gpio_cb) < 0) {
		SYS_LOG_DBG("Failed to set GPIO callback");
		return -EIO;
	}

	return 0;
}

static struct hdc1008_data hdc1008_data;

DEVICE_AND_API_INIT(hdc1008, CONFIG_HDC1008_NAME, hdc1008_init, &hdc1008_data,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &hdc1008_driver_api);
