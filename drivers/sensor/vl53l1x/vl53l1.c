/* vl53l1.c - Driver for ST VL53L1X time of flight sensor */
/*
 * Copyright (c) 2017 STMicroelectronics
 * Copyright (c) 2019 Makaio GmbH
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



#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(VL53L1X);

struct vl53l1x_pin_config {
	bool use;
	u32_t gpios_pin;
	char *gpios_ctrl;
};

struct vl53l1x_data {
	char *i2c_dev_name;
	u16_t i2c_addr;
	VL53L1_Dev_t vl53l1x;
	VL53L1_RangingMeasurementData_t RangingMeasurementData;
	int inter_measurement_period;
	int timing_budget;
	struct vl53l1x_pin_config irq_config;
	struct vl53l1x_pin_config xshut_config;
	u8_t instance_id;
};

static void vl53l1x_print_device_settings(struct device *dev)
{
	struct vl53l1x_data *drv_data = dev->driver_data;

	struct vl53l1x_pin_config *irq_cfg = &drv_data->irq_config;

	LOG_DBG("Name:\t\t%s", log_strdup(dev->config->name));
	LOG_DBG("Instance ID:\t%u", drv_data->instance_id);
	LOG_DBG("I2C Address:\t0x%02x", drv_data->i2c_addr);

	if (irq_cfg && irq_cfg->use) {
		LOG_DBG("IRQ:\t\tenabled, ctrl %s, pin %u",
				irq_cfg->gpios_ctrl, irq_cfg->gpios_pin);
	} else {
		LOG_DBG("IRQ:\t\tdisabled");
	}

	struct vl53l1x_pin_config *xshut_cfg = &drv_data->xshut_config;

	if (xshut_cfg && xshut_cfg->use) {
		LOG_DBG("XSHUT:\t\tenabled, ctrl %s, pin %u",
				xshut_cfg->gpios_ctrl, xshut_cfg->gpios_pin);
	} else {
		LOG_DBG("XSHUT:\t\tdisabled");
	}


}

static int vl53l1x_init(struct device *dev)
{
	struct vl53l1x_data *drv_data = dev->driver_data;
	VL53L1_Error ret;
	u16_t vl53l1x_id = 0U;
	VL53L1_DeviceInfo_t vl53l1x_dev_info;

	LOG_DBG("enter in %s", __func__);

	vl53l1x_print_device_settings(dev);

	VL53L1_Dev_t vl53l1dev = {
			.I2cDevAddr = drv_data->i2c_addr,
			.I2cHandle = device_get_binding(drv_data->i2c_dev_name)
	};
	drv_data->vl53l1x = vl53l1dev;

	if (vl53l1dev.I2cHandle == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
				drv_data->i2c_dev_name);
		return -EINVAL;
	}

	ret = VL53L1_WaitDeviceBooted(&drv_data->vl53l1x);

	if (ret) {
		return -ETIMEDOUT;
	}

	LOG_DBG("VL53L1_WaitDeviceBooted succeeded");

	/* sensor init */
	ret = VL53L1_DataInit(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1X_DataInit return error (%d)", ret);
		return -ENOTSUP;
	}

	LOG_DBG("VL53L1_DataInit succeeded");

	/* static init */
	ret = VL53L1_StaticInit(&drv_data->vl53l1x);
	if (ret < 0) {
		LOG_ERR("VL53L1_StaticInit return error (%d)", ret);
		return -ENOTSUP;
	}

	LOG_DBG("VL53L1_StaticInit succeeded");

	ret = VL53L1_SetDistanceMode(&drv_data->vl53l1x,
			VL53L1_DISTANCEMODE_LONG);
	if (ret < 0) {
		LOG_ERR("VL53L1_SetDistanceMode return error (%d)", ret);
		return -ENOTSUP;
	}

	LOG_DBG("VL53L1_SetDistanceMode succeeded");

	ret = VL53L1_StartMeasurement(&drv_data->vl53l1x);

	return 0;
}

#define VL53L1X_PIN_CFG(id, pintype)					\
static const struct vl53l1x_pin_config\
	vl53l1x_##pintype##_##id##_cfg = {	\
		.use = true,	\
		.gpios_pin =	\
		DT_INST_##id##_ST_VL53L1X_##pintype##_GPIOS_PIN,	\
		.gpios_ctrl =	\
		DT_INST_##id##_ST_VL53L1X_##pintype##_GPIOS_CONTROLLER	\
}

#define VL53L1X_NOPIN_CFG(id, pintype)					\
static const struct vl53l1x_pin_config	\
	vl53l1x_##pintype##_##id##_cfg = {	\
/* this is unnecessary, but let's keep it for clarity */	\
		.use = false,	\
	}




#define VL53L1X_INST_INIT(id)	\
static  struct vl53l1x_data vl53l1x_driver_##id##_data = {	\
		.instance_id = id,	\
		.i2c_dev_name = DT_INST_##id##_ST_VL53L1X_BUS_NAME,	\
		.i2c_addr = DT_INST_##id##_ST_VL53L1X_BASE_ADDRESS,	\
		.irq_config = vl53l1x_IRQ_##id##_cfg,	\
		.xshut_config = vl53l1x_XSHUT_##id##_cfg,	\
		.inter_measurement_period =	\
		DT_INST_##id##_ST_VL53L1X_INTER_MEASUREMENT_PERIOD,	\
		.timing_budget =	\
		DT_INST_##id##_ST_VL53L1X_MEASUREMENT_TIMING_BUDGET	\
};	\
DEVICE_AND_API_INIT(vl53l1x,	\
		DT_INST_##id##_ST_VL53L1X_LABEL,	\
		vl53l1x_init,	\
		&vl53l1x_driver_##id##_data,	\
NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, NULL)

#ifdef DT_INST_0_ST_VL53L1X_BASE_ADDRESS
#ifndef DT_INST_0_ST_VL53L1X_INTER_MEASUREMENT_PERIOD
#define DT_INST_0_ST_VL53L1X_INTER_MEASUREMENT_PERIOD
			CONFIG_VL53L1X_INTERMEASUREMENT_PERIOD
#endif
#ifndef DT_INST_0_ST_VL53L1X_MEASUREMENT_TIMING_BUDGET
#define DT_INST_0_ST_VL53L1X_MEASUREMENT_TIMING_BUDGET
			CONFIG_VL53L1X_MEAS_TIMING_BUDGET
#endif
#ifdef DT_INST_0_ST_VL53L1X_IRQ_GPIOS_PIN
VL53L1X_PIN_CFG(0, IRQ);
#else
VL53L1X_NOPIN_CFG(0, IRQ);
#endif
#ifdef DT_INST_0_ST_VL53L1X_XSHUT_GPIOS_PIN
VL53L1X_PIN_CFG(0, XSHUT);
#else
VL53L1X_NOPIN_CFG(0, XSHUT);
#endif
VL53L1X_INST_INIT(0);
#endif

