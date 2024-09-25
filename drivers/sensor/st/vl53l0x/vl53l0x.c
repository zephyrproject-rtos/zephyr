/* vl53l0x.c - Driver for ST VL53L0X time of flight sensor */

#define DT_DRV_COMPAT st_vl53l0x

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

LOG_MODULE_REGISTER(VL53L0X, CONFIG_SENSOR_LOG_LEVEL);

/* All the values used in this driver are coming from ST datasheet and examples.
 * It can be found here:
 *   https://www.st.com/en/embedded-software/stsw-img005.html
 * There are also examples of use in the L4 cube FW:
 *   https://www.st.com/en/embedded-software/stm32cubel4.html
 */
#define VL53L0X_INITIAL_ADDR                    0x29
#define VL53L0X_REG_WHO_AM_I                    0xC0
#define VL53L0X_CHIP_ID                         0xEEAA
#define VL53L0X_SETUP_SIGNAL_LIMIT              (0.1 * 65536)
#define VL53L0X_SETUP_SIGMA_LIMIT               (60 * 65536)
#define VL53L0X_SETUP_MAX_TIME_FOR_RANGING      33000
#define VL53L0X_SETUP_PRE_RANGE_VCSEL_PERIOD    18
#define VL53L0X_SETUP_FINAL_RANGE_VCSEL_PERIOD  14

/* tBOOT (1.2ms max.) VL53L0X firmware boot period */
#define T_BOOT K_USEC(1200)

struct vl53l0x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec xshut;
};

struct vl53l0x_data {
	bool started;
	VL53L0X_Dev_t vl53l0x;
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
};

static int vl53l0x_setup_single_shot(const struct device *dev)
{
	struct vl53l0x_data *drv_data = dev->data;
	int ret;
	uint8_t VhvSettings;
	uint8_t PhaseCal;
	uint32_t refSpadCount;
	uint8_t isApertureSpads;

	ret = VL53L0X_StaticInit(&drv_data->vl53l0x);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_StaticInit failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_PerformRefCalibration(&drv_data->vl53l0x,
					    &VhvSettings,
					    &PhaseCal);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_PerformRefCalibration failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_PerformRefSpadManagement(&drv_data->vl53l0x,
					       &refSpadCount,
					       &isApertureSpads);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_PerformRefSpadManagement failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetDeviceMode(&drv_data->vl53l0x,
				    VL53L0X_DEVICEMODE_SINGLE_RANGING);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetDeviceMode failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckEnable(&drv_data->vl53l0x,
					  VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					  1);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckEnable sigma failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckEnable(&drv_data->vl53l0x,
					  VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
					  1);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckEnable signal rate failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckValue(&drv_data->vl53l0x,
					 VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
					 VL53L0X_SETUP_SIGNAL_LIMIT);

	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckValue signal rate failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckValue(&drv_data->vl53l0x,
					 VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					 VL53L0X_SETUP_SIGMA_LIMIT);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckValue sigma failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l0x,
							     VL53L0X_SETUP_MAX_TIME_FOR_RANGING);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetMeasurementTimingBudgetMicroSeconds failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetVcselPulsePeriod(&drv_data->vl53l0x,
					  VL53L0X_VCSEL_PERIOD_PRE_RANGE,
					  VL53L0X_SETUP_PRE_RANGE_VCSEL_PERIOD);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetVcselPulsePeriod pre range failed",
			dev->name);
		goto exit;
	}

	ret = VL53L0X_SetVcselPulsePeriod(&drv_data->vl53l0x,
					  VL53L0X_VCSEL_PERIOD_FINAL_RANGE,
					  VL53L0X_SETUP_FINAL_RANGE_VCSEL_PERIOD);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetVcselPulsePeriod final range failed",
			dev->name);
		goto exit;
	}

exit:
	return ret;
}

static int vl53l0x_start(const struct device *dev)
{
	const struct vl53l0x_config *const config = dev->config;
	struct vl53l0x_data *drv_data = dev->data;
	int r;
	VL53L0X_Error ret;
	uint16_t vl53l0x_id = 0U;
	VL53L0X_DeviceInfo_t vl53l0x_dev_info = { 0 };

	LOG_DBG("[%s] Starting", dev->name);

	if (config->xshut.port) {
		r = gpio_pin_configure_dt(&config->xshut, GPIO_OUTPUT_INACTIVE);
		if (r < 0) {
			LOG_ERR("[%s] Unable to inactivate XSHUT: %d",
				dev->name, r);
			return -EIO;
		}
		k_sleep(T_BOOT);
	}

#ifdef CONFIG_VL53L0X_RECONFIGURE_ADDRESS
	if (config->i2c.addr != VL53L0X_INITIAL_ADDR) {
		ret = VL53L0X_SetDeviceAddress(&drv_data->vl53l0x, 2 * config->i2c.addr);
		if (ret != 0) {
			LOG_ERR("[%s] Unable to reconfigure I2C address",
				dev->name);
			return -EIO;
		}

		drv_data->vl53l0x.I2cDevAddr = config->i2c.addr;
		LOG_DBG("[%s] I2C address reconfigured", dev->name);
		k_sleep(T_BOOT);
	}
#endif

	ret = VL53L0X_GetDeviceInfo(&drv_data->vl53l0x, &vl53l0x_dev_info);
	if (ret < 0) {
		LOG_ERR("[%s] Could not get info from device.", dev->name);
		return -ENODEV;
	}

	LOG_DBG("[%s] VL53L0X_GetDeviceInfo = %d", dev->name, ret);
	LOG_DBG("   Device Name : %s", vl53l0x_dev_info.Name);
	LOG_DBG("   Device Type : %s", vl53l0x_dev_info.Type);
	LOG_DBG("   Device ID : %s", vl53l0x_dev_info.ProductId);
	LOG_DBG("   ProductRevisionMajor : %d",
		vl53l0x_dev_info.ProductRevisionMajor);
	LOG_DBG("   ProductRevisionMinor : %d",
		vl53l0x_dev_info.ProductRevisionMinor);

	ret = VL53L0X_RdWord(&drv_data->vl53l0x,
			     VL53L0X_REG_WHO_AM_I,
			     &vl53l0x_id);
	if ((ret < 0) || (vl53l0x_id != VL53L0X_CHIP_ID)) {
		LOG_ERR("[%s] Issue on device identification", dev->name);
		return -ENOTSUP;
	}

	/* sensor init */
	ret = VL53L0X_DataInit(&drv_data->vl53l0x);
	if (ret < 0) {
		LOG_ERR("[%s] VL53L0X_DataInit return error (%d)",
			dev->name, ret);
		return -ENOTSUP;
	}

	ret = vl53l0x_setup_single_shot(dev);
	if (ret < 0) {
		return -ENOTSUP;
	}

	drv_data->started = true;
	LOG_DBG("[%s] Started", dev->name);
	return 0;
}

static int vl53l0x_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct vl53l0x_data *drv_data = dev->data;
	VL53L0X_Error ret;
	int r;

	__ASSERT_NO_MSG((chan == SENSOR_CHAN_ALL)
			|| (chan == SENSOR_CHAN_DISTANCE)
			|| (chan == SENSOR_CHAN_PROX));

	if (!drv_data->started) {
		r = vl53l0x_start(dev);
		if (r < 0) {
			return r;
		}
	}

	ret = VL53L0X_PerformSingleRangingMeasurement(&drv_data->vl53l0x,
						      &drv_data->RangingMeasurementData);
	if (ret < 0) {
		LOG_ERR("[%s] Could not perform measurment (error=%d)",
			dev->name, ret);
		return -EINVAL;
	}

	return 0;
}


static int vl53l0x_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct vl53l0x_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_PROX) {
		if (drv_data->RangingMeasurementData.RangeMilliMeter <=
		    CONFIG_VL53L0X_PROXIMITY_THRESHOLD) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_DISTANCE) {
		val->val1 = drv_data->RangingMeasurementData.RangeMilliMeter / 1000;
		val->val2 = (drv_data->RangingMeasurementData.RangeMilliMeter % 1000) * 1000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api vl53l0x_api_funcs = {
	.sample_fetch = vl53l0x_sample_fetch,
	.channel_get = vl53l0x_channel_get,
};

static int vl53l0x_init(const struct device *dev)
{
	int r;
	struct vl53l0x_data *drv_data = dev->data;
	const struct vl53l0x_config *const config = dev->config;

	/* Initialize the HAL peripheral with the default sensor address,
	 * ie. the address on power up
	 */
	drv_data->vl53l0x.I2cDevAddr = VL53L0X_INITIAL_ADDR;
	drv_data->vl53l0x.i2c = config->i2c.bus;

#if defined(CONFIG_VL53L0X_RECONFIGURE_ADDRESS) || defined(CONFIG_PM_DEVICE)
	if (config->xshut.port == NULL) {
		LOG_ERR("[%s] Missing XSHUT gpio spec", dev->name);
		return -ENOTSUP;
	}
#endif

#ifdef CONFIG_VL53L0X_RECONFIGURE_ADDRESS
	/*
	 * Shutdown all vl53l0x sensors so at each sensor's 1st fetch call
	 * they can be enabled one at a time and programmed with their address.
	 */
	r = gpio_pin_configure_dt(&config->xshut, GPIO_OUTPUT_ACTIVE);
	if (r < 0) {
		LOG_ERR("[%s] Unable to shutdown sensor", dev->name);
		return -EIO;
	}
	LOG_DBG("[%s] Shutdown", dev->name);
#else
	if (config->i2c.addr != VL53L0X_INITIAL_ADDR) {
		LOG_ERR("[%s] Invalid device address (should be 0x%X or "
			"CONFIG_VL53L0X_RECONFIGURE_ADDRESS should be enabled)",
			dev->name, VL53L0X_INITIAL_ADDR);
		return -ENOTSUP;
	}

	r = vl53l0x_start(dev);
	if (r) {
		return r;
	}
#endif

	LOG_DBG("[%s] Initialized", dev->name);
	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int vl53l0x_pm_action(const struct device *dev,
			     enum pm_device_action action)
{
	const struct vl53l0x_config *const config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = vl53l0x_init(dev);
		if (ret != 0) {
			LOG_ERR("resume init: %d", ret);
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* HW Standby */
		ret = gpio_pin_set_dt(&config->xshut, 1);
		if (ret < 0) {
			LOG_ERR("[%s] XSHUT pin active", dev->name);
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

#define VL53L0X_INIT(inst)						 \
	static struct vl53l0x_config vl53l0x_##inst##_config = {	 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),			 \
		.xshut = GPIO_DT_SPEC_INST_GET_OR(inst, xshut_gpios, {}) \
	};								 \
									 \
	static struct vl53l0x_data vl53l0x_##inst##_driver;		 \
									 \
	PM_DEVICE_DT_INST_DEFINE(inst, vl53l0x_pm_action);		 \
									 \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, vl53l0x_init,		 \
			      PM_DEVICE_DT_INST_GET(inst),		 \
			      &vl53l0x_##inst##_driver,			 \
			      &vl53l0x_##inst##_config,			 \
			      POST_KERNEL,				 \
			      CONFIG_SENSOR_INIT_PRIORITY,		 \
			      &vl53l0x_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(VL53L0X_INIT)
