/* vl6180x.c - Driver for ST VL6180X time of flight sensor */

#define DT_DRV_COMPAT st_vl6180x

/*
 * Copyright (c) 2017 STMicroelectronics
 * Copyright (c) 2022 Kamil Serwus
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "vl6180_api.h"
#include "vl6180_platform.h"

LOG_MODULE_REGISTER(VL6180X, CONFIG_SENSOR_LOG_LEVEL);

/* All the values used in this driver are coming from ST datasheet and examples.
 * It can be found here:
 *   http://www.st.com/en/embedded-software/stsw-img011.html
 */
#define VL6180X_INITIAL_ADDR                    0x29
#define VL6180X_REG_WHO_AM_I                    0x00
#define VL6180X_CHIP_ID                         0xB4

struct vl6180x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec xshut;
};

struct vl6180x_data {
	bool started;
	VL6180_Dev_t vl6180x;
	VL6180_RangeData_t RangingMeasurementData;
};

static int vl6180x_start(const struct device *dev)
{
	const struct vl6180x_config *const config = dev->config;
	struct vl6180x_data *drv_data = dev->data;
	int r;
	int ret;
	uint8_t vl6180x_id = 0U;

	LOG_DBG("[%s] Starting", dev->name);

	/* Pull XSHUT high to start the sensor */
	if (config->xshut.port) {
		r = gpio_pin_set_dt(&config->xshut, 1);
		if (r < 0) {
			LOG_ERR("[%s] Unable to set XSHUT gpio (error %d)",
				dev->name, r);
			return -EIO;
		}
		k_sleep(K_MSEC(1));
	}

#ifdef CONFIG_VL6180X_RECONFIGURE_ADDRESS
	if (config->i2c.addr != VL6180X_INITIAL_ADDR) {
		ret = VL6180_SetI2CAddress(&drv_data->vl6180x, 2 * config->i2c.addr);
		if (ret != 0) {
			LOG_ERR("[%s] Unable to reconfigure I2C address",
				dev->name);
			return -EIO;
		}

		drv_data->vl6180x.I2cDevAddr = config->i2c.addr;
		LOG_DBG("[%s] I2C address reconfigured", dev->name);
		k_sleep(K_MSEC(2));
	}
#endif

 	ret = VL6180_RdByte(&drv_data->vl6180x,
			     VL6180X_REG_WHO_AM_I,
			     &vl6180x_id);
	if ((ret < 0) || (vl6180x_id != VL6180X_CHIP_ID)) {
		LOG_ERR("[%s] Issue on device identification %x", dev->name, vl6180x_id);
		return -ENOTSUP;
	}

	/* sensor init */
	ret = VL6180_InitData(&drv_data->vl6180x);
	if (ret < 0) {
		LOG_ERR("[%s] VL6180_InitData return error (%d)",
			dev->name, ret);
		return -ENOTSUP;
	}

	ret = VL6180_Prepare(&drv_data->vl6180x);
	if (ret < 0) {
		LOG_ERR("[%s] VL6180_Prepare return error (%d)",
			dev->name, ret);
		return -ENOTSUP;
	}

	drv_data->started = true;
	LOG_DBG("[%s] Started", dev->name);
	return 0;
}

static int vl6180x_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct vl6180x_data *drv_data = dev->data;
	int ret;
	int r;

	__ASSERT_NO_MSG((chan == SENSOR_CHAN_ALL)
			|| (chan == SENSOR_CHAN_DISTANCE)
			|| (chan == SENSOR_CHAN_PROX));

	if (!drv_data->started) {
		r = vl6180x_start(dev);
		if (r < 0) {
			return r;
		}
	}

	ret = VL6180_RangePollMeasurement(&drv_data->vl6180x,
						      &drv_data->RangingMeasurementData);
	if (ret < 0) {
		LOG_ERR("[%s] Could not perform measurment (error=%d)",
			dev->name, ret);
		return -EINVAL;
	}

	return 0;
}


static int vl6180x_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct vl6180x_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_PROX) {
		if (drv_data->RangingMeasurementData.range_mm <=
		    CONFIG_VL6180X_PROXIMITY_THRESHOLD) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_DISTANCE) {
		val->val1 = drv_data->RangingMeasurementData.range_mm / 1000;
		val->val2 = (drv_data->RangingMeasurementData.range_mm % 1000) * 1000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api vl6180x_api_funcs = {
	.sample_fetch = vl6180x_sample_fetch,
	.channel_get = vl6180x_channel_get,
};

static int vl6180x_init(const struct device *dev)
{
	int r;
	struct vl6180x_data *drv_data = dev->data;
	const struct vl6180x_config *const config = dev->config;

	/* Initialize the HAL peripheral with the default sensor address,
	 * ie. the address on power up
	 */
	drv_data->vl6180x.I2cDevAddr = VL6180X_INITIAL_ADDR;
	drv_data->vl6180x.i2c = config->i2c.bus;

#ifdef CONFIGX_VL6180_RECONFIGURE_ADDRESS
	if (!config->xshut.port) {
		LOG_ERR("[%s] Missing XSHUT gpio spec", dev->name);
		return -ENOTSUP;
	}
#else
	if (config->i2c.addr != VL6180X_INITIAL_ADDR) {
		LOG_ERR("[%s] Invalid device address (should be 0x%X or "
			"CONFIG_VL6180_RECONFIGURE_ADDRESS should be enabled)",
			dev->name, VL6180X_INITIAL_ADDR);
		return -ENOTSUP;
	}
#endif

	if (config->xshut.port) {
		r = gpio_pin_configure_dt(&config->xshut, GPIO_OUTPUT);
		if (r < 0) {
			LOG_ERR("[%s] Unable to configure GPIO as output",
				dev->name);
		}
	}

#ifdef CONFIG_VL6180X_RECONFIGURE_ADDRESS
	/* Pull XSHUT low to shut down the sensor for now */
	r = gpio_pin_set_dt(&config->xshut, 0);
	if (r < 0) {
		LOG_ERR("[%s] Unable to shutdown sensor", dev->name);
		return -EIO;
	}
	LOG_DBG("[%s] Shutdown", dev->name);
#else
	r = vl6180x_start(dev);
	if (r) {
		return r;
	}
#endif

	LOG_DBG("[%s] Initialized", dev->name);
	return 0;
}

#define VL6180X_INIT(inst)						 \
	static struct vl6180x_config vl6180x_##inst##_config = {	 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),			 \
		.xshut = GPIO_DT_SPEC_INST_GET_OR(inst, xshut_gpios, {}) \
	};								 \
									 \
	static struct vl6180x_data vl6180x_##inst##_driver;		 \
									 \
	DEVICE_DT_INST_DEFINE(inst, vl6180x_init, NULL,			 \
			      &vl6180x_##inst##_driver,			 \
			      &vl6180x_##inst##_config,			 \
			      POST_KERNEL,				 \
			      CONFIG_SENSOR_INIT_PRIORITY,		 \
			      &vl6180x_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(VL6180X_INIT)

