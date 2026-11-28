/*
 * Copyright (c) 2026 Sacra Systems Private Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT murata_scl3300

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/scl3300.h>
#include <zephyr/logging/log.h>

#include "scl3300.h"

LOG_MODULE_REGISTER(scl3300, CONFIG_SENSOR_LOG_LEVEL);

static uint8_t scl3300_crc8_update(uint8_t bit_val, uint8_t crc)
{
	crc ^= bit_val;
	for (int i = 8; i > 0; i--) {
		crc = (crc & 0x80) ? (crc << 1) ^ 0x1D : (crc << 1);
	}
	return crc;
}

static bool scl3300_validate_crc(const uint8_t *rx_buf)
{
	uint32_t frame = ((uint32_t)rx_buf[0] << SCL3300_FRAME_SHIFT_B3) |
			 ((uint32_t)rx_buf[1] << SCL3300_FRAME_SHIFT_B2) |
			 ((uint32_t)rx_buf[2] << SCL3300_FRAME_SHIFT_B1) | (uint32_t)rx_buf[3];

	uint8_t crc = 0xFF;

	crc = scl3300_crc8_update((frame >> SCL3300_FRAME_SHIFT_B3) & 0xFF, crc);
	crc = scl3300_crc8_update((frame >> SCL3300_FRAME_SHIFT_B2) & 0xFF, crc);
	crc = scl3300_crc8_update((frame >> SCL3300_FRAME_SHIFT_B1) & 0xFF, crc);

	return ((~crc & 0xFF) == (frame & 0xFF));
}

static int scl3300_read_angle_response(const struct device *dev, uint32_t command, uint8_t *rx_buf)
{
	struct scl3300_data *data = dev->data;
	uint8_t command_phase_rx_buf[4];
	int ret;

	/* First frame: send the requested angle command. */
	ret = scl3300_transport_read(data->ctx, command, command_phase_rx_buf);
	if (ret != 0) {
		return ret;
	}

	/* Second frame: clock out the angle response. */
	ret = scl3300_transport_read(data->ctx, SCL3300_CMD_STATUS, rx_buf);

	return ret;
}

static int scl3300_fetch_angle(const struct device *dev, uint32_t command)
{
	struct scl3300_data *data = dev->data;
	uint8_t rx_buf[SCL3300_SPI_FRAME_SIZE];

	int ret = scl3300_read_angle_response(dev, command, rx_buf);

	if (ret != 0) {
		LOG_ERR("Angle Fetch failed");
		return ret;
	}

	if (!scl3300_validate_crc(rx_buf)) {
		LOG_ERR("CRC validation failed");
		return -EIO;
	}

	if (command == SCL3300_CMD_READ_ANGLE_X) {
		data->angle_x = (int16_t)(((uint16_t)rx_buf[SCL3300_RESPONSE_MSB_INDEX]
					   << SCL3300_FRAME_SHIFT_B1) |
					  rx_buf[SCL3300_RESPONSE_LSB_INDEX]);
	} else if (command == SCL3300_CMD_READ_ANGLE_Y) {
		data->angle_y = (int16_t)(((uint16_t)rx_buf[SCL3300_RESPONSE_MSB_INDEX]
					   << SCL3300_FRAME_SHIFT_B1) |
					  rx_buf[SCL3300_RESPONSE_LSB_INDEX]);
	} else if (command == SCL3300_CMD_READ_ANGLE_Z) {
		data->angle_z = (int16_t)(((uint16_t)rx_buf[SCL3300_RESPONSE_MSB_INDEX]
					   << SCL3300_FRAME_SHIFT_B1) |
					  rx_buf[SCL3300_RESPONSE_LSB_INDEX]);
	}

	return 0;
}

static int scl3300_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;

	switch ((int)chan) {
	case SENSOR_CHAN_INCLINATION_X:
		ret = scl3300_fetch_angle(dev, SCL3300_CMD_READ_ANGLE_X);
		break;
	case SENSOR_CHAN_INCLINATION_Y:
		ret = scl3300_fetch_angle(dev, SCL3300_CMD_READ_ANGLE_Y);
		break;
	case SENSOR_CHAN_INCLINATION_Z:
		ret = scl3300_fetch_angle(dev, SCL3300_CMD_READ_ANGLE_Z);
		break;
	case SENSOR_CHAN_INCLINATION_XYZ:
		ret = scl3300_fetch_angle(dev, SCL3300_CMD_READ_ANGLE_X);
		if (ret != 0) {
			break;
		}

		ret = scl3300_fetch_angle(dev, SCL3300_CMD_READ_ANGLE_Y);
		if (ret != 0) {
			break;
		}

		ret = scl3300_fetch_angle(dev, SCL3300_CMD_READ_ANGLE_Z);
		break;

	default:
		return -ENOTSUP;
	}

	return ret;
}

static void scl3300_convert_angle(int16_t raw, struct sensor_value *val)
{
	double angle;

	angle = ((double)raw * SCL3300_ANGLE_SCALE_FACTOR) / SCL3300_ANGLE_RESOLUTION;

	sensor_value_from_double(val, angle);
}

static int scl3300_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct scl3300_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_INCLINATION_X:
		scl3300_convert_angle(data->angle_x, val);
		break;

	case SENSOR_CHAN_INCLINATION_Y:
		scl3300_convert_angle(data->angle_y, val);
		break;

	case SENSOR_CHAN_INCLINATION_Z:
		scl3300_convert_angle(data->angle_z, val);
		break;

	case SENSOR_CHAN_INCLINATION_XYZ:
		scl3300_convert_angle(data->angle_x, &val[0]);
		scl3300_convert_angle(data->angle_y, &val[1]);
		scl3300_convert_angle(data->angle_z, &val[2]);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int scl3300_send_config_command(const struct device *dev, uint32_t command)
{
	struct scl3300_data *data = dev->data;

	return scl3300_transport_write(data->ctx, command);
}

static int scl3300_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	if (val == NULL || val->val2 != 0) {
		return -EINVAL;
	}

	switch (attr) {
	case SENSOR_ATTR_MEASUREMENT_MODE:
		switch (val->val1) {
		case SCL3300_MODE_1:
			return scl3300_send_config_command(dev, SCL3300_CMD_MODE1);

		case SCL3300_MODE_2:
			return scl3300_send_config_command(dev, SCL3300_CMD_MODE2);

		case SCL3300_MODE_3:
			return scl3300_send_config_command(dev, SCL3300_CMD_MODE3);

		case SCL3300_MODE_4:
			return scl3300_send_config_command(dev, SCL3300_CMD_MODE4);

		default:
			return -EINVAL;
		}

	default:
		return -ENOTSUP;
	}
}

static int scl3300_configure(const struct device *dev, const struct scl3300_config *cfg)
{
	struct scl3300_data *data = dev->data;
	int ret = 0;

	if ((data == NULL) || (data->ctx == NULL)) {
		LOG_ERR("SCL3300 context is not initialized");
		return -EINVAL;
	}

	if (cfg->init) {
		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_WHOAMI);
		if (ret < 0) {
			LOG_ERR("Failed to send WHOAMI command: %d", ret);
			return ret;
		}

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_WAKEUP);
		if (ret < 0) {
			LOG_ERR("Failed to send WAKEUP command: %d", ret);
			return ret;
		}

		k_msleep(2);

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_SW_RESET);
		if (ret < 0) {
			LOG_ERR("Failed to send SW_RESET command: %d", ret);
			return ret;
		}

		k_msleep(2);

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_MODE1);
		if (ret < 0) {
			LOG_ERR("Failed to send ANG_CTRL command: %d", ret);
			return ret;
		}

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_ANGLE_CTRL);
		if (ret < 0) {
			LOG_ERR("Failed to send ANG_CTRL command: %d", ret);
			return ret;
		}

		k_msleep(25);

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_STATUS);
		if (ret < 0) {
			LOG_ERR("Failed to send STATUS command: %d", ret);
			return ret;
		}

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_STATUS);
		if (ret < 0) {
			LOG_ERR("Failed to send STATUS command: %d", ret);
			return ret;
		}

		ret = scl3300_transport_write(data->ctx, SCL3300_CMD_STATUS);
		if (ret < 0) {
			LOG_ERR("Failed to send STATUS command: %d", ret);
			return ret;
		}
	}

	return ret;
}

static int scl3300_safely_configure(const struct device *dev, const struct scl3300_config *cfg)
{
	if ((dev == NULL) || (cfg == NULL) || (dev->config == NULL)) {
		return -EINVAL;
	}

	return scl3300_configure(dev, cfg);
}

static int scl3300_init_device(const struct device *dev)
{
	const struct scl3300_config *cfg = dev->config;
	int ret = 0;

	if (!spi_is_ready_iodev(cfg->spi_iodev)) {
		LOG_ERR("SCL3300 SPI bus is not ready");
		return -ENODEV;
	}

	ret = cfg->bus_init(dev);
	if (ret < 0) {
		LOG_ERR("SCL3300 SPI initialization failed: %d", ret);
		return ret;
	}

	ret = scl3300_safely_configure(dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure SCL3300: %d", ret);
		return ret;
	}

	return ret;
}

static DEVICE_API(sensor, scl3300_driver_api) = {
	.sample_fetch = scl3300_sample_fetch,
	.channel_get = scl3300_channel_get,
	.attr_set = scl3300_attr_set,
};

#define SCL3300_DEFINE(inst)                                                                       \
	SPI_DT_IODEV_DEFINE(scl3300_spi_iodev_##inst, DT_DRV_INST(inst),                           \
			    SPI_WORD_SET(8) | SPI_TRANSFER_MSB);                                   \
	RTIO_DEFINE_WITH_MEMPOOL(scl3300_rtio_##inst, 2, /* Submission queue depth */              \
				 2,                      /* Completion queue depth */              \
				 2,                      /* Number of memory blocks */             \
				 256,                    /* Memory block size */                   \
				 sizeof(void *)          /* Memory block alignment */              \
	);                                                                                         \
	static struct scl3300_data scl3300_data_##inst = {                                         \
		.rtio = &scl3300_rtio_##inst,                                                      \
	};                                                                                         \
	static const struct scl3300_config scl3300_config_##inst = {                               \
		.spi_iodev = &scl3300_spi_iodev_##inst,                                            \
		.bus_init = scl3300_spi_init,                                                      \
		.init = DT_INST_PROP_OR(inst, init, 0),                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, scl3300_init_device, NULL, &scl3300_data_##inst,               \
			      &scl3300_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,    \
			      &scl3300_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCL3300_DEFINE)
