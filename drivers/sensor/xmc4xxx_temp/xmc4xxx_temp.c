/*
 * Copyright (c) 2023 Andriy Gelman <andriy.gelman@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_temp

#include <xmc_scu.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(xmc4xxx_temp, CONFIG_SENSOR_LOG_LEVEL);

struct xmc4xxx_temp_data {
	float temp_out;
};

static int xmc4xxx_temp_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct xmc4xxx_temp_data *data = dev->data;
	int32_t val;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	ret = XMC_SCU_StartTemperatureMeasurement();
	if (ret != 0) {
		return -EBUSY;
	}

	while (XMC_SCU_IsTemperatureSensorBusy()) {
	};

	val = XMC_SCU_GetTemperatureMeasurement();
	/* See Infineon XMC4500 Reference Manual Section 11.2.5.1 */
	data->temp_out = (val - 605) / 2.05f;

	return ret;
}

static int xmc4xxx_temp_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	struct xmc4xxx_temp_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, data->temp_out);
}

static const struct sensor_driver_api xmc4xxx_temp_driver_api = {
	.sample_fetch = xmc4xxx_temp_sample_fetch,
	.channel_get = xmc4xxx_temp_channel_get,
};

static int xmc4xxx_temp_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	XMC_SCU_EnableTemperatureSensor();

	BUILD_ASSERT(CONFIG_XMC4XXX_TEMP_CALIBRATE_OFFSET >= -64 &&
		     CONFIG_XMC4XXX_TEMP_CALIBRATE_OFFSET <= 63);

	BUILD_ASSERT(CONFIG_XMC4XXX_TEMP_CALIBRATE_GAIN >= 0 &&
		     CONFIG_XMC4XXX_TEMP_CALIBRATE_OFFSET <= 63);

	XMC_SCU_CalibrateTemperatureSensor(CONFIG_XMC4XXX_TEMP_CALIBRATE_OFFSET & 0x7f,
					   CONFIG_XMC4XXX_TEMP_CALIBRATE_GAIN);
	return 0;
}

#define XMC4XXX_TEMP_DEFINE(inst)								\
	static struct xmc4xxx_temp_data xmc4xxx_temp_dev_data_##inst;				\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, xmc4xxx_temp_init, NULL,				\
			      &xmc4xxx_temp_dev_data_##inst, NULL,				\
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,				\
			      &xmc4xxx_temp_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_TEMP_DEFINE)
