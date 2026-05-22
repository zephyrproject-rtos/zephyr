/*
 * Copyright (c) 2026 Jonathan Elliot Peace <jep@alphabetiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Raspberry Pi BCM283x SoC die-temperature sensor.
 *
 * The temperature is read through the VideoCore firmware
 * GET_TEMPERATURE property tag -- the source behind `vcgencmd
 * measure_temp`. The sensor owns no MMIO; it is a child of the
 * raspberrypi,bcm283x-firmware node and forwards requests through
 * rpi_fw. Exposes SENSOR_CHAN_DIE_TEMP.
 */

#define DT_DRV_COMPAT raspberrypi_bcm283x_vc_thermal

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <rpi_fw.h>

LOG_MODULE_REGISTER(bcm283x_vc_thermal, CONFIG_SENSOR_LOG_LEVEL);

struct bcm283x_vc_thermal_config {
	const struct device *fw;
};

struct bcm283x_vc_thermal_data {
	int32_t millideg;
};

static int bcm283x_vc_thermal_sample_fetch(const struct device *dev,
					   enum sensor_channel chan)
{
	const struct bcm283x_vc_thermal_config *config = dev->config;
	struct bcm283x_vc_thermal_data *data = dev->data;
	/* GET_TEMPERATURE: u32 temperature id in, {id, milli-degC} out. */
	uint32_t buf[2] = {0U, 0U};
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	ret = rpi_fw_transfer(config->fw, RPI_FW_TAG_GET_TEMPERATURE, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	data->millideg = (int32_t)buf[1];

	return 0;
}

static int bcm283x_vc_thermal_channel_get(const struct device *dev,
					  enum sensor_channel chan,
					  struct sensor_value *val)
{
	const struct bcm283x_vc_thermal_data *data = dev->data;

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* Firmware reports milli-degrees Celsius. */
	val->val1 = data->millideg / 1000;
	val->val2 = (data->millideg % 1000) * 1000;

	return 0;
}

static int bcm283x_vc_thermal_init(const struct device *dev)
{
	const struct bcm283x_vc_thermal_config *config = dev->config;

	if (!device_is_ready(config->fw)) {
		LOG_ERR("VideoCore firmware device not ready");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, bcm283x_vc_thermal_api) = {
	.sample_fetch = bcm283x_vc_thermal_sample_fetch,
	.channel_get = bcm283x_vc_thermal_channel_get,
};

#define BCM283X_VC_THERMAL_DEFINE(inst)                                                            \
	static struct bcm283x_vc_thermal_data bcm283x_vc_thermal_data_##inst;                      \
                                                                                                   \
	static const struct bcm283x_vc_thermal_config bcm283x_vc_thermal_cfg_##inst = {            \
		.fw = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bcm283x_vc_thermal_init, NULL,                          \
				     &bcm283x_vc_thermal_data_##inst,                              \
				     &bcm283x_vc_thermal_cfg_##inst, POST_KERNEL,                  \
				     CONFIG_SENSOR_INIT_PRIORITY, &bcm283x_vc_thermal_api);

DT_INST_FOREACH_STATUS_OKAY(BCM283X_VC_THERMAL_DEFINE)
