/* vl53l1x.c - Driver for ST VL53L1X time of flight sensor */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <zephyr/types.h>
#include <device.h>
#include <logging/log.h>

#include "vl53l1_api.h"
#include "vl53l1_platform.h"

LOG_MODULE_REGISTER(VL53L1X, CONFIG_SENSOR_LOG_LEVEL);
#define MEASUREMENT_BUDGET_MS 50
#define INTER_MEASUREMENT_PERIOD_MS 55


/* All the values used in this driver are coming from ST datasheet and examples.
 * It can be found here:
 *   http://www.st.com/en/embedded-software/stsw-img005.html
 * There are also examples of use in the L4 cube FW:
 *   http://www.st.com/en/embedded-software/stm32cubel4.html
 */
#define VL53L1X_REG_WHO_AM_I   0xC0
#define VL53L1X_CHIP_ID        0xEEAA
#define VL53L1X_SETUP_SIGNAL_LIMIT         (0.1*65536)
#define VL53L1X_SETUP_SIGMA_LIMIT          (60*65536)
#define VL53L1X_SETUP_MAX_TIME_FOR_RANGING     33000
#define VL53L1X_SETUP_PRE_RANGE_VCSEL_PERIOD   18
#define VL53L1X_SETUP_FINAL_RANGE_VCSEL_PERIOD 14

#define VL53L1_MODEL_ID_REG			0x010F
#define VL53L1_MODEL_ID   			0xEA
#define VL53L1_MODULE_TYPE_REG   	0X0110
#define VL53L1_MODULE_TYPE   		0XCC
#define VL53L1_MASK_REVISION_REG   	0X0111
#define VL53L1_MASK_REVISION   		0X10

struct vl53l1x_data {
	struct device *i2c;
	VL53L1_Dev_t vl53l1x;
	VL53L1_RangingMeasurementData_t RangingMeasurementData;
};

static int vl53l1x_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct vl53l1x_data *drv_data = dev->driver_data;
	VL53L1_Error ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL
			|| chan == SENSOR_CHAN_DISTANCE
			|| chan == SENSOR_CHAN_PROX);

	ret = VL53L1_WaitMeasurementDataReady(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_WaitMeasurementDataReady return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_GetRangingMeasurementData(&drv_data->vl53l1x, &drv_data->RangingMeasurementData);
	if (ret < 0) {
		LOG_ERR("VL53L1_GetRangingMeasurementData return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_ClearInterruptAndStartMeasurement(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_ClearInterruptAndStartMeasurement return error (%d)", ret);
		return -ENOTSUP;
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
		val->val1 = drv_data->RangingMeasurementData.RangeMilliMeter;
		val->val2 = drv_data->RangingMeasurementData.RangeFractionalPart/256;
	}

	return 0;
}

static const struct sensor_driver_api vl53l1x_api_funcs = {
	.sample_fetch = vl53l1x_sample_fetch,
	.channel_get = vl53l1x_channel_get,
};

static int vl53l1x_init(struct device *dev)
{
	struct vl53l1x_data *drv_data = dev->driver_data;
	VL53L1_Error ret;

	LOG_DBG("enter in %s", __func__);

#ifdef DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_CONTROLLER
	struct device *gpio;

	/* configure and set VL53L1X_XSHUT_Pin */
	gpio = device_get_binding(DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_CONTROLLER);
	if (gpio == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
		DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_CONTROLLER);
		return -EINVAL;
	}

	if (gpio_pin_configure(gpio,
			      DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_PIN,
			      GPIO_DIR_OUT | GPIO_PUD_PULL_UP) < 0) {
		LOG_ERR("Could not configure GPIO %s %d).",
			DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_CONTROLLER,
			DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_PIN);
		return -EINVAL;
	}

	gpio_pin_write(gpio, DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_PIN, 1);
	k_sleep(K_MSEC(100));
#endif

	drv_data->i2c = device_get_binding(DT_INST_0_ST_VL53L1X_BUS_NAME);
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			DT_INST_0_ST_VL53L1X_BUS_NAME);
		return -EINVAL;
	}

	drv_data->vl53l1x.I2cHandle = drv_data->i2c;
	drv_data->vl53l1x.I2cDevAddr = DT_INST_0_ST_VL53L1X_BASE_ADDRESS;

	ret = VL53L1_software_reset(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_software_reset return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_WaitDeviceBooted(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_WaitDeviceBooted return error (%d)", ret);
		return -ENOTSUP;
	}

	u16_t vl53l1_id = 0U;
	ret = VL53L1_RdWord(&drv_data->vl53l1x,
			     VL53L1_MODEL_ID_REG,
			     (uint16_t *) &vl53l1_id);

	if ((ret < 0) || (vl53l1_id != 0xEACC)) {
		LOG_ERR("Issue on device identification");
		return -ENOTSUP;
	}

	/* sensor init */
	ret = VL53L1_DataInit(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_DataInit return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_StaticInit(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_StaticInit return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_SetDistanceMode(&drv_data->vl53l1x, VL53L1_DISTANCEMODE_LONG);
	if (ret < 0) {
		LOG_ERR("VL53L1_SetDistanceMode return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_SetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l1x, MEASUREMENT_BUDGET_MS * 1000);
	if (ret < 0) {
		LOG_ERR("VL53L1_SetMeasurementTimingBudgetMicroSeconds return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_SetInterMeasurementPeriodMilliSeconds(&drv_data->vl53l1x, INTER_MEASUREMENT_PERIOD_MS);
	if (ret < 0) {
		LOG_ERR("VL53L1_SetInterMeasurementPeriodMilliSeconds return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = VL53L1_StartMeasurement(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_StartMeasurement return error (%d)", ret);
		return -ENOTSUP;
	}

	return 0;
}


static struct vl53l1x_data vl53l1x_driver;

DEVICE_AND_API_INIT(vl53l1x, DT_INST_0_ST_VL53L1X_LABEL, vl53l1x_init, &vl53l1x_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &vl53l1x_api_funcs);
