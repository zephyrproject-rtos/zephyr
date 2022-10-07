/* vl53l0x.c - Driver for ST VL53L0X time of flight sensor */

#define DT_DRV_COMPAT st_vl53l0x

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vl53l0x_api.h"
#include "vl53l0x_api_core.h"
#include "vl53l0x_platform.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(VL53L0X, CONFIG_SENSOR_LOG_LEVEL);

/* All the values used in this driver are coming from ST datasheet and examples.
 * It can be found here:
 *   http://www.st.com/en/embedded-software/stsw-img005.html
 * There are also examples of use in the L4 cube FW:
 *   http://www.st.com/en/embedded-software/stm32cubel4.html
 */
#define VL53L0X_INITIAL_ADDR		       0x29
#define VL53L0X_REG_WHO_AM_I		       0xC0
#define VL53L0X_CHIP_ID			       0xEEAA
#define VL53L0X_SETUP_SIGNAL_LIMIT	       (0.1 * 65536)
#define VL53L0X_SETUP_SIGMA_LIMIT	       (60 * 65536)
#define VL53L0X_SETUP_MAX_TIME_FOR_RANGING     33000
#define VL53L0X_SETUP_PRE_RANGE_VCSEL_PERIOD   18
#define VL53L0X_SETUP_FINAL_RANGE_VCSEL_PERIOD 14

struct vl53l0x_config {
	struct i2c_dt_spec i2c;
	struct gpio_dt_spec interrupt;
	struct gpio_dt_spec xshut;
};

struct vl53l0x_data {
	const struct device *dev;
	bool started;
	VL53L0X_Dev_t vl53l0x;
	VL53L0X_RangingMeasurementData_t measurement;
	VL53L0X_DeviceModes current_mode;
	struct gpio_callback interrupt_cb;
	struct k_work interrupt_work;
	struct k_sem wait_for_interrupt;
	sensor_trigger_handler_t data_ready_handler;
};

static inline bool vl53l0x_supports_chan(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_ALL:
	case SENSOR_CHAN_DISTANCE:
	case SENSOR_CHAN_PROX:
		return true;
	default:
		return false;
	}
}

static inline bool vl53l0x_has_interrupt(const struct device *dev)
{
	const struct vl53l0x_config *const config = dev->config;

	return config->interrupt.port != NULL;
}

static void vl53l0x_interrupt_callback(const struct device *port, struct gpio_callback *cb,
				       uint32_t pin)
{
	struct vl53l0x_data *drv_data = CONTAINER_OF(cb, struct vl53l0x_data, interrupt_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pin);
	k_work_submit(&drv_data->interrupt_work);
	LOG_DBG("[%s] Got interrupt !", drv_data->dev->name);
}

static void vl53l0x_interrupt_work(struct k_work *work)
{
	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_DISTANCE,
	};
	struct vl53l0x_data *drv_data = CONTAINER_OF(work, struct vl53l0x_data, interrupt_work);
	uint32_t reason;
	int r;

	r = VL53L0X_GetInterruptMaskStatus(&drv_data->vl53l0x, &reason);
	if (r) {
		LOG_ERR("[%s] VL53L0X_GetInterruptMaskStatus failed", drv_data->dev->name);
		return;
	}

	k_sem_give(&drv_data->wait_for_interrupt);

	if (reason == VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY && drv_data->data_ready_handler) {
		drv_data->data_ready_handler(drv_data->dev, &trig);
	}

	r = VL53L0X_ClearInterruptMask(&drv_data->vl53l0x, 0);
	if (r) {
		LOG_ERR("[%s] VL53L0X_ClearInterruptMask failed", drv_data->dev->name);
	}
}

static int vl53l0x_configure(const struct device *dev)
{
	struct vl53l0x_data *drv_data = dev->data;
	int ret;
	uint8_t VhvSettings;
	uint8_t PhaseCal;
	uint32_t refSpadCount;
	uint8_t isApertureSpads;

	ret = VL53L0X_StaticInit(&drv_data->vl53l0x);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_StaticInit failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_PerformRefCalibration(&drv_data->vl53l0x, &VhvSettings, &PhaseCal);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_PerformRefCalibration failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_PerformRefSpadManagement(&drv_data->vl53l0x, (uint32_t *)&refSpadCount,
					       &isApertureSpads);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_PerformRefSpadManagement failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckEnable(&drv_data->vl53l0x, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					  1);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckEnable sigma failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckEnable(&drv_data->vl53l0x,
					  VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckEnable signal rate failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckValue(&drv_data->vl53l0x,
					 VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
					 VL53L0X_SETUP_SIGNAL_LIMIT);

	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckValue signal rate failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckValue(&drv_data->vl53l0x, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					 VL53L0X_SETUP_SIGMA_LIMIT);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetLimitCheckValue sigma failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l0x,
							     VL53L0X_SETUP_MAX_TIME_FOR_RANGING);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetMeasurementTimingBudgetMicroSeconds failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetVcselPulsePeriod(&drv_data->vl53l0x, VL53L0X_VCSEL_PERIOD_PRE_RANGE,
					  VL53L0X_SETUP_PRE_RANGE_VCSEL_PERIOD);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetVcselPulsePeriod pre range failed", dev->name);
		goto exit;
	}

	ret = VL53L0X_SetVcselPulsePeriod(&drv_data->vl53l0x, VL53L0X_VCSEL_PERIOD_FINAL_RANGE,
					  VL53L0X_SETUP_FINAL_RANGE_VCSEL_PERIOD);
	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetVcselPulsePeriod final range failed", dev->name);
		goto exit;
	}

exit:
	return ret;
}

static int vl53l0x_set_ranging_mode(const struct device *dev, VL53L0X_DeviceModes mode)
{
	struct vl53l0x_data *drv_data = dev->data;
	int ret = VL53L0X_SetDeviceMode(&drv_data->vl53l0x, mode);

	if (ret) {
		LOG_ERR("[%s] VL53L0X_SetDeviceMode failed", dev->name);
		return ret;
	}
	drv_data->current_mode = mode;
	return 0;
}

static int vl53l0x_start(const struct device *dev)
{
	const struct vl53l0x_config *const config = dev->config;
	struct vl53l0x_data *drv_data = dev->data;
	int r;
	VL53L0X_Error ret;
	uint16_t vl53l0x_id = 0U;
	VL53L0X_DeviceInfo_t vl53l0x_dev_info;

	LOG_DBG("[%s] Starting", dev->name);

	/* Pull XSHUT high to start the sensor */
	if (config->xshut.port) {
		r = gpio_pin_set_dt(&config->xshut, 1);
		if (r < 0) {
			LOG_ERR("[%s] Unable to set XSHUT gpio (error %d)", dev->name, r);
			return -EIO;
		}
		k_sleep(K_MSEC(2));
	}

#ifdef CONFIG_VL53L0X_RECONFIGURE_ADDRESS
	if (config->i2c.addr != VL53L0X_INITIAL_ADDR) {
		ret = VL53L0X_SetDeviceAddress(&drv_data->vl53l0x, 2 * config->i2c.addr);
		if (ret != 0) {
			LOG_ERR("[%s] Unable to reconfigure I2C address", dev->name);
			return -EIO;
		}

		drv_data->vl53l0x.I2cDevAddr = config->i2c.addr;
		LOG_DBG("[%s] I2C address reconfigured", dev->name);
		k_sleep(K_MSEC(2));
	}
#endif

	/* Get info from sensor */
	(void)memset(&vl53l0x_dev_info, 0, sizeof(VL53L0X_DeviceInfo_t));

	ret = VL53L0X_GetDeviceInfo(&drv_data->vl53l0x, &vl53l0x_dev_info);
	if (ret < 0) {
		LOG_ERR("[%s] Could not get info from device.", dev->name);
		return -ENODEV;
	}

	LOG_DBG("[%s] VL53L0X_GetDeviceInfo = %d", dev->name, ret);
	LOG_DBG("   Device Name : %s", vl53l0x_dev_info.Name);
	LOG_DBG("   Device Type : %s", vl53l0x_dev_info.Type);
	LOG_DBG("   Device ID : %s", vl53l0x_dev_info.ProductId);
	LOG_DBG("   ProductRevisionMajor : %d", vl53l0x_dev_info.ProductRevisionMajor);
	LOG_DBG("   ProductRevisionMinor : %d", vl53l0x_dev_info.ProductRevisionMinor);

	ret = VL53L0X_RdWord(&drv_data->vl53l0x, VL53L0X_REG_WHO_AM_I, (uint16_t *)&vl53l0x_id);
	if ((ret < 0) || (vl53l0x_id != VL53L0X_CHIP_ID)) {
		LOG_ERR("[%s] Issue on device identification", dev->name);
		return -ENOTSUP;
	}

	/* sensor init */
	ret = VL53L0X_DataInit(&drv_data->vl53l0x);
	if (ret < 0) {
		LOG_ERR("[%s] VL53L0X_DataInit return error (%d)", dev->name, ret);
		return -ENOTSUP;
	}

	ret = vl53l0x_configure(dev);
	if (ret < 0) {
		return -ENOTSUP;
	}

	ret = vl53l0x_set_ranging_mode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
	if (ret) {
		return -EIO;
	}

	if (vl53l0x_has_interrupt(dev)) {
		r = VL53L0X_SetGpioConfig(&drv_data->vl53l0x, 0, VL53L0X_DEVICEMODE_SINGLE_RANGING,
					  VL53L0X_GPIOFUNCTIONALITY_NEW_MEASURE_READY,
					  VL53L0X_INTERRUPTPOLARITY_HIGH);
		if (r < 0) {
			LOG_ERR("[%s] Unable to setup interrupt config on device: %d", dev->name,
				r);
			return -EIO;
		}

		gpio_init_callback(&drv_data->interrupt_cb, vl53l0x_interrupt_callback,
				   BIT(config->interrupt.pin));
		gpio_add_callback(config->interrupt.port, &drv_data->interrupt_cb);
		gpio_pin_interrupt_configure_dt(&config->interrupt, GPIO_INT_EDGE_TO_ACTIVE);
		LOG_DBG("[%s] Interrupt configured", dev->name);
	}

	drv_data->started = true;
	LOG_DBG("[%s] Started", dev->name);
	return 0;
}

static int vl53l0x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct vl53l0x_data *drv_data = dev->data;
	int r;

	if (!vl53l0x_supports_chan(chan)) {
		return -ENOTSUP;
	}

	if (!drv_data->started) {
		r = vl53l0x_start(dev);
		if (r < 0) {
			return r;
		}
	}

	if (drv_data->current_mode == VL53L0X_DEVICEMODE_SINGLE_RANGING) {
		r = VL53L0X_StartMeasurement(&drv_data->vl53l0x);
		if (r) {
			LOG_ERR("[%s] VL53L0X_PerformSingleMeasurement failed", dev->name);
			return -EIO;
		}
	}

	if (vl53l0x_has_interrupt(dev)) {
		LOG_DBG("[%s] Waiting for interrupt", dev->name);
		k_sem_take(&drv_data->wait_for_interrupt, K_FOREVER);
	} else {
		LOG_DBG("[%s] Polling for measurement completion", dev->name);
		VL53L0X_measurement_poll_for_completion(&drv_data->vl53l0x);
	}

	LOG_DBG("[%s] Getting measurement data", dev->name);
	r = VL53L0X_GetRangingMeasurementData(&drv_data->vl53l0x, &drv_data->measurement);
	if (r) {
		LOG_ERR("[%s] VL53L0X_GetRangingMeasurementData failed", dev->name);
		return -EINVAL;
	}

	return 0;
}

static int vl53l0x_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct vl53l0x_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_PROX) {
		if (drv_data->measurement.RangeMilliMeter <= CONFIG_VL53L0X_PROXIMITY_THRESHOLD) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_DISTANCE) {
		val->val1 = drv_data->measurement.RangeMilliMeter / 1000;
		val->val2 = (drv_data->measurement.RangeMilliMeter % 1000) * 1000;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int vl53l0x_get_sampling_freq(const struct device *dev, struct sensor_value *val)
{
	uint32_t timing;
	struct vl53l0x_data *drv_data = dev->data;
	int ret = VL53L0X_GetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l0x, &timing);

	if (ret) {
		LOG_ERR("[%s] Unable to get measurement timing budget", dev->name);
		return ret;
	}
	if (timing == 0) {
		return -EINVAL;
	}
	val->val1 = USEC_PER_SEC / timing;
	val->val2 = USEC_PER_SEC / (USEC_PER_SEC % timing) / timing;
	return ret;
}

static int vl53l0x_set_sampling_freq(const struct device *dev, const struct sensor_value *val)
{
	struct vl53l0x_data *drv_data = dev->data;
	uint64_t freq_microhertz = USEC_PER_SEC * val->val1 + val->val2;
	uint64_t timing;
	int r;

	if (freq_microhertz == 0) {
		return -EINVAL;
	}

	timing = USEC_PER_SEC * USEC_PER_SEC / freq_microhertz;

	r = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l0x, (uint32_t)timing);
	if (r) {
		LOG_ERR("[%s] Unable to set measurement timing budget", dev->name);
		return r;
	}
	return r;
}

static int vl53l0x_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	int r;
	struct vl53l0x_data *drv_data = dev->data;

	if (!vl53l0x_supports_chan(chan)) {
		return -ENOTSUP;
	}

	if (!drv_data->started) {
		r = vl53l0x_start(dev);
		if (r) {
			return r;
		}
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return vl53l0x_get_sampling_freq(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int vl53l0x_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	int r;
	struct vl53l0x_data *drv_data = dev->data;

	if (!vl53l0x_supports_chan(chan)) {
		return -ENOTSUP;
	}

	if (!drv_data->started) {
		r = vl53l0x_start(dev);
		if (r) {
			return r;
		}
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return vl53l0x_set_sampling_freq(dev, val);
	default:
		return -ENOTSUP;
	}
}

int vl53l0x_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	struct vl53l0x_data *drv_data = dev->data;
	bool was_in_continuous_mode =
		(drv_data->current_mode == VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
	bool to_continuous_mode = (handler != NULL);

	if (!vl53l0x_has_interrupt(dev)) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	if (!vl53l0x_supports_chan(trig->chan)) {
		return -ENOTSUP;
	}

	drv_data->data_ready_handler = handler;

	if (!was_in_continuous_mode && to_continuous_mode) {
		if (vl53l0x_set_ranging_mode(dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING)) {
			return -EIO;
		}
		if (VL53L0X_StartMeasurement(&drv_data->vl53l0x)) {
			LOG_ERR("[%s] VL53L0X_StartMeasurement failed", dev->name);
			return -EIO;
		}
	} else if (was_in_continuous_mode && !to_continuous_mode) {
		if (VL53L0X_StopMeasurement(&drv_data->vl53l0x)) {
			LOG_ERR("[%s] VL53L0X_StopMeasurement failed", dev->name);
			return -EIO;
		}
		if (vl53l0x_set_ranging_mode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING)) {
			return -EIO;
		}
	}
	return 0;
}

static const struct sensor_driver_api vl53l0x_api_funcs = {
	.sample_fetch = vl53l0x_sample_fetch,
	.channel_get = vl53l0x_channel_get,
	.attr_get = vl53l0x_attr_get,
	.attr_set = vl53l0x_attr_set,
	.trigger_set = vl53l0x_trigger_set,
};

static int vl53l0x_init(const struct device *dev)
{
	int r;
	struct vl53l0x_data *drv_data = dev->data;
	const struct vl53l0x_config *const config = dev->config;

	k_work_init(&drv_data->interrupt_work, vl53l0x_interrupt_work);
	k_sem_init(&drv_data->wait_for_interrupt, 0, 1);
	drv_data->dev = dev;

	/* Initialize the HAL peripheral with the default sensor address,
	 * ie. the address on power up
	 */
	drv_data->vl53l0x.I2cDevAddr = VL53L0X_INITIAL_ADDR;
	drv_data->vl53l0x.i2c = config->i2c.bus;

#ifdef CONFIG_VL53L0X_RECONFIGURE_ADDRESS
	if (!config->xshut.port) {
		LOG_ERR("[%s] Missing XSHUT gpio spec", dev->name);
		return -ENOTSUP;
	}
#else
	if (config->i2c.addr != VL53L0X_INITIAL_ADDR) {
		LOG_ERR("[%s] Invalid device address (should be 0x%X or "
			"CONFIG_VL53L0X_RECONFIGURE_ADDRESS should be enabled)",
			dev->name, VL53L0X_INITIAL_ADDR);
		return -ENOTSUP;
	}
#endif

	if (config->xshut.port) {
		r = gpio_pin_configure_dt(&config->xshut, GPIO_OUTPUT);
		if (r < 0) {
			LOG_ERR("[%s] Unable to configure xshut as output", dev->name);
			return -EIO;
		}
	}

	if (vl53l0x_has_interrupt(dev)) {
		r = gpio_pin_configure_dt(&config->interrupt, GPIO_INPUT);
		if (r < 0) {
			LOG_ERR("[%s] Unable to configure interrupt as input", dev->name);
			return -EIO;
		}
	}

#ifdef CONFIG_VL53L0X_RECONFIGURE_ADDRESS
	/* Pull XSHUT low to shut down the sensor for now */
	r = gpio_pin_set_dt(&config->xshut, 0);
	if (r < 0) {
		LOG_ERR("[%s] Unable to shutdown sensor", dev->name);
		return -EIO;
	}
	LOG_DBG("[%s] Shutdown", dev->name);
#else
	r = vl53l0x_start(dev);
	if (r) {
		return r;
	}
#endif

	LOG_DBG("[%s] Initialized", dev->name);
	return 0;
}

#define VL53L0X_INIT(inst)                                                                         \
	static struct vl53l0x_config vl53l0x_##inst##_config = {                                   \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.interrupt = GPIO_DT_SPEC_INST_GET_OR(inst, interrupt_gpios, {}),                  \
		.xshut = GPIO_DT_SPEC_INST_GET_OR(inst, xshut_gpios, {})};                         \
                                                                                                   \
	static struct vl53l0x_data vl53l0x_##inst##_driver;                                        \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vl53l0x_init, NULL, &vl53l0x_##inst##_driver,                  \
			      &vl53l0x_##inst##_config, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,  \
			      &vl53l0x_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(VL53L0X_INIT)
