/*
 * Copyright (c) 2023 Balthazar Deliers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aosong_ags10

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include "ags10.h"

LOG_MODULE_REGISTER(AGS10, CONFIG_SENSOR_LOG_LEVEL);

#define AGS10_MAX_PAYLOAD_SIZE 5U	/* Payload will be max 4 bytes + CRC (datasheet 3.1) */

static int ags10_read(const struct device *dev, uint8_t cmd, uint8_t *data, uint8_t rx_bytes)
{
	if (rx_bytes > AGS10_MAX_PAYLOAD_SIZE) {
		return -EINVAL;
	}

	const struct ags10_config *conf = dev->config;

	uint8_t recv_buf[AGS10_MAX_PAYLOAD_SIZE] = {0};
	int ret = i2c_write_read_dt(&conf->bus, &cmd, sizeof(cmd), &recv_buf, rx_bytes);

	if (ret < 0) {
		return ret;
	}

	memcpy(data, recv_buf, rx_bytes);

	return 0;
}

static int ags10_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_VOC && chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	struct ags10_data *data = dev->data;
	int ret = -ENOTSUP;
	uint8_t recv_buf[5] = {0};

	ret = ags10_read(dev, AGS10_CMD_DATA_ACQUISITION, recv_buf, 5);

	if (ret == 0) {
		/* If CRC is valid and data is valid too */
		if (crc8(&recv_buf[0], 4, 0x31, 0xFF, false) == recv_buf[4] &&
		    ((recv_buf[0] & AGS10_MSK_STATUS) == AGS10_REG_STATUS_NRDY_READY)) {
			data->status = recv_buf[0] & AGS10_MSK_STATUS;
			data->tvoc_ppb = sys_get_be24(&recv_buf[1]);
			return 0;
		}

		LOG_WRN("Bad CRC or data not ready");
		ret = -EIO;
	}

	return ret;
}

static int ags10_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct ags10_data *data = dev->data;

	if (chan == SENSOR_CHAN_VOC) {
		val->val1 = data->tvoc_ppb;
	} else {
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

static int ags10_init(const struct device *dev)
{
	const struct ags10_config *conf = dev->config;
	struct ags10_data *data = dev->data;
	int ret;

	if (!i2c_is_ready_dt(&conf->bus)) {
		LOG_ERR("Device not ready");
		return -ENODEV;
	}

	/* Set initial data values */
	data->tvoc_ppb = 0;
	data->status = 0xFF;
	data->version = 0;

	/* Read firmware version and check CRC */
	uint8_t recv_buf[5] = {0};

	ret = ags10_read(dev, AGS10_CMD_READ_VERSION, recv_buf, 5);

	/* Bytes 0 to 2 are reserved, byte 3 is version, byte 4 is CRC */
	if (ret == 0 && crc8(&recv_buf[0], 4, 0x31, 0xFF, false) == recv_buf[4]) {
		data->version = recv_buf[3];
		LOG_DBG("Sensor detected");
	} else if (ret != 0) {
		LOG_ERR("No reply from sensor");
		ret = -ENODEV;
	} else {
		LOG_WRN("Bad CRC");
		ret = -EIO;
	}

	return ret;
}

static const struct sensor_driver_api ags10_api = {.sample_fetch = ags10_sample_fetch,
						   .channel_get = ags10_channel_get};

#define AGS10_INIT(n)                                                                              \
	static struct ags10_data ags10_data_##n;                                                   \
                                                                                                   \
	static const struct ags10_config ags10_config_##n = {                                      \
		.bus = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, ags10_init, NULL, &ags10_data_##n, &ags10_config_##n,      \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ags10_api);

DT_INST_FOREACH_STATUS_OKAY(AGS10_INIT)
