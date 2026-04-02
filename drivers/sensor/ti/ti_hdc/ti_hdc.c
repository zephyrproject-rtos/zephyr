/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_hdc

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "ti_hdc.h"

LOG_MODULE_REGISTER(TI_HDC, CONFIG_SENSOR_LOG_LEVEL);

static void ti_hdc_gpio_callback(const struct device *dev,
				  struct gpio_callback *cb, uint32_t pins)
{
	struct ti_hdc_data *drv_data =
		CONTAINER_OF(cb, struct ti_hdc_data, gpio_cb);
	const struct ti_hdc_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->drdy, GPIO_INT_DISABLE);
	k_sem_give(&drv_data->data_sem);
}

static int ti_hdc_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct ti_hdc_data *drv_data = dev->data;
	const struct ti_hdc_config *cfg = dev->config;
	uint8_t buf[4];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (cfg->drdy.port) {
		gpio_pin_interrupt_configure_dt(&cfg->drdy, GPIO_INT_EDGE_TO_ACTIVE);
	}

	buf[0] = TI_HDC_REG_TEMP;
	if (i2c_write_dt(&cfg->i2c, buf, 1) < 0) {
		LOG_DBG("Failed to write address pointer");
		return -EIO;
	}

	if (cfg->drdy.port) {
		k_sem_take(&drv_data->data_sem, K_FOREVER);
	} else {
		/* wait for the conversion to finish */
		k_msleep(HDC_CONVERSION_TIME);
	}

	if (i2c_read_dt(&cfg->i2c, buf, 4) < 0) {
		LOG_DBG("Failed to read sample data");
		return -EIO;
	}

	drv_data->t_sample = (buf[0] << 8) + buf[1];
	drv_data->rh_sample = (buf[2] << 8) + buf[3];

	return 0;
}


static int ti_hdc_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ti_hdc_data *drv_data = dev->data;
	uint64_t tmp;

	/*
	 * See datasheet "Temperature Register" and "Humidity
	 * Register" sections for more details on processing
	 * sample data.
	 */
	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* val = -40 + 165 * sample / 2^16 */
		tmp = (uint64_t)drv_data->t_sample * 165U;
		val->val1 = (int32_t)(tmp >> 16) - 40;
		val->val2 = ((tmp & 0xFFFF) * 1000000U) >> 16;
	} else if (chan == SENSOR_CHAN_HUMIDITY) {
		/* val = 100 * sample / 2^16 */
		tmp = (uint64_t)drv_data->rh_sample * 100U;
		val->val1 = tmp >> 16;
		/* x * 1000000 / 65536 == x * 15625 / 1024 */
		val->val2 = ((tmp & 0xFFFF) * 15625U) >> 10;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, ti_hdc_driver_api) = {
	.sample_fetch = ti_hdc_sample_fetch,
	.channel_get = ti_hdc_channel_get,
};

static uint16_t read16(const struct i2c_dt_spec *i2c, uint8_t d)
{
	uint8_t buf[2];
	if (i2c_burst_read_dt(i2c, d, (uint8_t *)buf, 2) < 0) {
		LOG_ERR("Error reading register.");
	}
	return (buf[0] << 8 | buf[1]);
}

static int ti_hdc_init(const struct device *dev)
{
	const struct ti_hdc_config *cfg = dev->config;
	uint16_t tmp;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	if (read16(&cfg->i2c, TI_HDC_REG_MANUFID) != TI_HDC_MANUFID) {
		LOG_ERR("Failed to get correct manufacturer ID");
		return -EINVAL;
	}
	tmp = read16(&cfg->i2c, TI_HDC_REG_DEVICEID);
	if (tmp != TI_HDC1000_DEVID && tmp != TI_HDC1050_DEVID) {
		LOG_ERR("Unsupported device ID");
		return -EINVAL;
	}

	if (cfg->drdy.port) {
		struct ti_hdc_data *drv_data = dev->data;

		drv_data->dev = dev;

		k_sem_init(&drv_data->data_sem, 0, K_SEM_MAX_LIMIT);

		/* setup data ready gpio interrupt */
		if (!gpio_is_ready_dt(&cfg->drdy)) {
			LOG_ERR("%s: device %s is not ready", dev->name,
					cfg->drdy.port->name);
			return -ENODEV;
		}

		gpio_pin_configure_dt(&cfg->drdy, GPIO_INPUT);

		gpio_init_callback(&drv_data->gpio_cb,
				ti_hdc_gpio_callback,
				BIT(cfg->drdy.pin));

		if (gpio_add_callback(cfg->drdy.port, &drv_data->gpio_cb) < 0) {
			LOG_DBG("Failed to set GPIO callback");
			return -EIO;
		}

		gpio_pin_interrupt_configure_dt(&cfg->drdy, GPIO_INT_EDGE_TO_ACTIVE);
	}

	LOG_INF("Initialized device successfully");

	return 0;
}

#define TI_HDC_DEFINE(inst)								\
	static struct ti_hdc_data ti_hdc_data_##inst;					\
											\
	static const struct ti_hdc_config ti_hdc_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
		.drdy = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 }),		\
	};										\
											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ti_hdc_init, NULL,				\
			      &ti_hdc_data_##inst, &ti_hdc_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &ti_hdc_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(TI_HDC_DEFINE)
