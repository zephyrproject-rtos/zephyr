/*
 * Copyright (c) 2026 Paweł Pielorz <pawel.pielorz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc13xx_cc26xx_vbat

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <driverlib/aon_batmon.h>

LOG_MODULE_REGISTER(cc13xx_cc26xx_vbat, CONFIG_SENSOR_LOG_LEVEL);

struct cc13xx_cc26xx_vbat_data {
	uint16_t raw; /* raw batmon voltage value */
};

static int cc13xx_cc26xx_vbat_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct cc13xx_cc26xx_vbat_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	data->raw = AONBatMonBatteryVoltageGet();

	return 0;
}

static int cc13xx_cc26xx_vbat_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct cc13xx_cc26xx_vbat_data *data = dev->data;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	val->val1 = data->raw >> 8;
	val->val2 = (data->raw & 0xFF) * 1000000 / 256;

	return 0;
}

static DEVICE_API(sensor, cc13xx_cc26xx_vbat_driver_api) = {
	.sample_fetch = cc13xx_cc26xx_vbat_sample_fetch,
	.channel_get = cc13xx_cc26xx_vbat_channel_get,
};

static int cc13xx_cc26xx_vbat_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	AONBatMonEnable();

	return 0;
}

static struct cc13xx_cc26xx_vbat_data cc13xx_cc26xx_vbat_dev_data;

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
		"Only one VBat sensor is available on CC13XX/CC26XX devices");

SENSOR_DEVICE_DT_INST_DEFINE(0, cc13xx_cc26xx_vbat_init, NULL, &cc13xx_cc26xx_vbat_dev_data, NULL,
				POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
				&cc13xx_cc26xx_vbat_driver_api);
