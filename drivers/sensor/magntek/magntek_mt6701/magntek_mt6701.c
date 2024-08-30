/*
 * Copyright (c) 2024, Vinicius Miguel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT magntek_mt6701

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <math.h>
#include <zephyr/drivers/sensor/magntek_mt6701.h>

LOG_MODULE_REGISTER(magntek_mt6701, CONFIG_SENSOR_LOG_LEVEL);

#define MT6701_SPI_CONFIG     SPI_OP_MODE_MASTER | SPI_MODE_CPHA | SPI_WORD_SET(8)
#define MT6701_FULL_ANGLE     360
#define MT6701_PULSES_PER_REV 4096
#define MT6701_MILLION_UNIT   1000000

struct mt6701_dev_cfg {
	struct spi_dt_spec spi_port;
};

struct mt6701_dev_data {
	uint32_t position;
	uint8_t field_status;
	int8_t push_status;
	uint8_t loss_status;
};

static uint8_t table_crc[64] = {0x00, 0x03, 0x06, 0x05, 0x0C, 0x0F, 0x0A, 0x09, 0x18, 0x1B, 0x1E,
				0x1D, 0x14, 0x17, 0x12, 0x11, 0x30, 0x33, 0x36, 0x35, 0x3C, 0x3F,
				0x3A, 0x39, 0x28, 0x2B, 0x2E, 0x2D, 0x24, 0x27, 0x22, 0x21, 0x23,
				0x20, 0x25, 0x26, 0x2F, 0x2C, 0x29, 0x2A, 0x3B, 0x38, 0x3D, 0x3E,
				0x37, 0x34, 0x31, 0x32, 0x13, 0x10, 0x15, 0x16, 0x1F, 0x1C, 0x19,
				0x1A, 0x0B, 0x08, 0x0D, 0x0E, 0x07, 0x04, 0x01, 0x02};

static uint8_t mt6701_crc_check(uint32_t input)
{
	uint8_t index = 0;
	uint8_t crc = 0;

	index = (uint8_t)(((uint32_t)input >> 12u) & 0x0000003Fu);

	crc = (uint8_t)(((uint32_t)input >> 6u) & 0x0000003Fu);
	index = crc ^ table_crc[index];

	crc = (uint8_t)((uint32_t)input & 0x0000003Fu);
	index = crc ^ table_crc[index];

	crc = table_crc[index];

	return crc;
}

static int mt6701_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct mt6701_dev_data *dev_data = dev->data;
	const struct mt6701_dev_cfg *dev_cfg = dev->config;

	uint32_t spi_32 = 0;
	uint32_t angle_spi = 0;
	uint8_t received_crc = 0;

	uint8_t read_data[3] = {0, 0, 0};

	const struct spi_buf rx_buf[] = {{
		.buf = read_data,
		.len = ARRAY_SIZE(read_data),
	}};

	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = ARRAY_SIZE(read_data),
	};

	int err = spi_read_dt(&dev_cfg->spi_port, &rx);

	/* an invalid reading preserves the last good value */
	if (!err) {

		/*
		 * [0:13] Angle data
		 * [14:17] Magnetic field status
		 * [18:23] CRC
		 */

		spi_32 = (read_data[0] << 16) | (read_data[1] << 8) | read_data[2];
		angle_spi = spi_32 >> 10;
		received_crc = spi_32 & 0x3F;

		if (received_crc == mt6701_crc_check(spi_32 >> 6)) {
			dev_data->field_status = (spi_32 >> 6) & 0x3;
			dev_data->push_status = (spi_32 >> 8) & 0x1;
			dev_data->loss_status = (spi_32 >> 9) & 0x1;
			dev_data->position = angle_spi;
		} else {
			err = -EILSEQ;
		}
	}

	return err;
}

static int mt6701_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct mt6701_dev_data *dev_data = dev->data;
	float new_angle = 0;
	double integer = 0;

	switch ((int16_t)chan) {
	case SENSOR_CHAN_ROTATION:
		new_angle = ((float)dev_data->position / MT6701_PULSES_PER_REV) * MT6701_FULL_ANGLE;
		val->val2 = (int32_t)(modf(new_angle, &integer) * MT6701_MILLION_UNIT);
		val->val1 = (int32_t)integer;
		break;
	case SENSOR_CHAN_FIELD_STATUS:
		val->val1 = dev_data->field_status;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_PUSH_STATUS:
		val->val1 = dev_data->push_status;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_LOSS_STATUS:
		val->val1 = dev_data->loss_status;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int mt6701_initialize(const struct device *dev)
{
	struct mt6701_dev_data *const dev_data = dev->data;

	dev_data->position = 0;

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static const struct sensor_driver_api mt6701_driver_api = {
	.sample_fetch = mt6701_fetch,
	.channel_get = mt6701_get,
};

#define MT6701_INIT(n)                                                                             \
	static struct mt6701_dev_data mt6701_data##n;                                              \
	static const struct mt6701_dev_cfg mt6701_cfg##n = {                                       \
		.spi_port = SPI_DT_SPEC_INST_GET(n, MT6701_SPI_CONFIG, 0),                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(n, mt6701_initialize, NULL, &mt6701_data##n, &mt6701_cfg##n,  \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &mt6701_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MT6701_INIT)
