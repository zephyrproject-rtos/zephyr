/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <i2c.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/__assert.h>
#include <zephyr/types.h>
#include <device.h>
#include <logging/log.h>

#include "vl53l1x.h"
#include "vl53l1_api.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(VL53L1X);

static int vl53l1x_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct vl53l1x_data *drv_data = dev->driver_data;
	VL53L1_Error ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL
			|| chan == SENSOR_CHAN_DISTANCE
			|| chan == SENSOR_CHAN_PROX);

	ret = VL53L1_GetRangingMeasurementData(&drv_data->vl53l1x,
					&drv_data->RangingMeasurementData);
	if (ret < 0) {
		LOG_ERR("Could not perform measurment (error=%d)", ret);
		return -EINVAL;
	}

	return 0;
}


static int vl53l1x_channel_get(struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct vl53l1x_data *drv_data = (struct vl53l1x_data *)dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_DISTANCE
			|| chan == SENSOR_CHAN_PROX);

	if (chan == SENSOR_CHAN_PROX) {
		if (drv_data->RangingMeasurementData.RangeMilliMeter <=
		    CONFIG_VL53L1X_PROXIMITY_THRESHOLD) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}
		val->val2 = 0;
	} else {
		val->val1 = drv_data->RangingMeasurementData.RangeMilliMeter / 1000;
		val->val2 = (drv_data->RangingMeasurementData.RangeMilliMeter % 1000) * 1000;
	}

	VL53L1_ClearInterruptAndStartMeasurement(&drv_data->vl53l1x);

	return 0;
}

static const struct sensor_driver_api vl53l1x_api_funcs = {
#if CONFIG_VL53L1X_TRIGGER
	.trigger_set = vl53l1x_trigger_set,
#endif
	.sample_fetch = vl53l1x_sample_fetch,
	.channel_get = vl53l1x_channel_get,
};


static int vl53l1x_init(struct device *dev)
{
	struct vl53l1x_data *drv_data = dev->driver_data;
	VL53L1_Error ret;
	u16_t vl53l1x_id = 0U;
	VL53L1_DeviceInfo_t vl53l1x_dev_info;

	LOG_DBG("enter in %s", __func__);

#ifdef CONFIG_VL53L1X_XSHUT_CONTROL_ENABLE
	struct device *gpio;

	/* configure and set VL53L1X_XSHUT_Pin */
	gpio = device_get_binding(CONFIG_VL53L1X_XSHUT_GPIO_DEV_NAME);
	if (gpio == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
		CONFIG_VL53L1X_XSHUT_GPIO_DEV_NAME);
		return -EINVAL;
	}

	if (gpio_pin_configure(gpio,
			      CONFIG_VL53L1X_XSHUT_GPIO_PIN_NUM,
			      GPIO_DIR_OUT | GPIO_PUD_PULL_UP) < 0) {
		LOG_ERR("Could not configure GPIO %s %d).",
			CONFIG_VL53L1X_XSHUT_GPIO_DEV_NAME,
			CONFIG_VL53L1X_XSHUT_GPIO_PIN_NUM);
		return -EINVAL;
	}

	gpio_pin_write(gpio, CONFIG_VL53L1X_XSHUT_GPIO_PIN_NUM, 1);
	k_sleep(100);
#endif

	drv_data->i2c = device_get_binding(DT_ST_VL53L1X_0_BUS_NAME);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			DT_ST_VL53L1X_0_BUS_NAME);
		return -EINVAL;
	}

	drv_data->vl53l1x.i2c = drv_data->i2c;
	drv_data->vl53l1x.I2cDevAddr = DT_ST_VL53L1X_0_BASE_ADDRESS;

	/* Get info from sensor */
	(void)memset(&vl53l1x_dev_info, 0, sizeof(VL53L1_DeviceInfo_t));

	ret = VL53L1_GetDeviceInfo(&drv_data->vl53l1x, &vl53l1x_dev_info);
	if (ret < 0) {
		LOG_ERR("Could not get info from device.");
		return -ENODEV;
	}

	LOG_DBG("VL53L1X_GetDeviceInfo = %d", ret);
	LOG_DBG("   Device Name : %s", vl53l1x_dev_info.Name);
	LOG_DBG("   Device Type : %s", vl53l1x_dev_info.Type);
	LOG_DBG("   Device ID : %s",   vl53l1x_dev_info.ProductId);
	LOG_DBG("   ProductRevisionMajor : %d",
		    vl53l1x_dev_info.ProductRevisionMajor);
	LOG_DBG("   ProductRevisionMinor : %d",
		    vl53l1x_dev_info.ProductRevisionMinor);


	ret = VL53L1_RdWord(&drv_data->vl53l1x,
			     VL53L1X_REG_WHO_AM_I,
			     (uint16_t *) &vl53l1x_id);
	if ((ret < 0) || (vl53l1x_id != VL53L1X_CHIP_ID)) {
		LOG_ERR("Issue on device identification");
		return -ENOTSUP;
	}

	/* sensor init */
	ret = VL53L1_WaitDeviceBooted(&drv_data->vl53l1x);
	ret = VL53L1_DataInit(&drv_data->vl53l1x);
	ret = VL53L1_StaticInit(&drv_data->vl53l1x);
	ret = VL53L1_SetDistanceMode(&drv_data->vl53l1x, VL53L1_DISTANCEMODE_LONG);
	ret = VL53L1_SetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l1x, 50000);
	ret = VL53L1_SetInterMeasurementPeriodMilliSeconds(&drv_data->vl53l1x, 500);
	ret = VL53L1_StartMeasurement(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_StartMeasurement failed");
		return -ENOTSUP;
	}

#ifdef CONFIG_VL53L1X_TRIGGER
	if (vl53l1x_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupt.");
		return -EIO;
	}
#endif

	return 0;
}


static struct vl53l1x_data vl53l1x_driver;

DEVICE_AND_API_INIT(vl53l1x, DT_ST_VL53L1X_0_LABEL, vl53l1x_init, &vl53l1x_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &vl53l1x_api_funcs);

