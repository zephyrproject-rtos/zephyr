/* bmp280.c - Driver for Bosch BMP280 temperature and pressure sensor */

/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <sensor.h>
#include <init.h>
#include <gpio.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#ifdef DT_TDK_ICM20948_BUS_I2C
#include <i2c.h>
#elif defined DT_TDK_ICM20948_BUS_SPI
#include <spi.h>
#endif

#include <logging/log.h>

#include "icm20948.h"

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(ICM20948);

static inline int icm20948_set_gyro_fs(enum icm20948_gyro_fs gyro_fs)
{
	return 0;
}

static inline int icm20948_set_acc_fs(enum icm20948_gyro_fs gyro_fs)
{
	return 0;
}

static int icm20948_channel_get(struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	//    struct icm20948_data *drv_data = dev->driver_data;
	return 0;
}

static int icm20948_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct icm20948_data *data = dev->driver_data;
	union {
		u8_t raw[6];
		struct {
			s16_t axis[3];
		};
	} buf __aligned(2);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ: {
		if (data->hw_tf->read_data(data, ICM20948_REG_ACCEL_XOUT_H_SH,
					   buf.raw, sizeof(buf))) {
			LOG_DBG("Failed to fetch raw data samples");
			return -EIO;
		}
		data->acc[0] = sys_be16_to_cpu(buf.axis[0]);
		data->acc[1] = sys_be16_to_cpu(buf.axis[0]);
		data->acc[2] = sys_be16_to_cpu(buf.axis[0]);
	} break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ: {
		if (data->hw_tf->read_data(data, ICM20948_REG_ACCEL_XOUT_H_SH,
					   buf.raw, sizeof(buf))) {
			LOG_DBG("Failed to fetch raw data samples");
			return -EIO;
		}
	} break;
	default:
		break;
	}
	return 0;
}

int icm20948_init(struct device *dev)
{
	struct icm20948_data *data = (struct icm20948_data *)dev->driver_data;

#if defined(DT_TDK_ICM20948_BUS_SPI)
	icm20948_spi_init(dev);
#elif defined(DT_TDK_ICM20948_BUS_I2C)
	icm20948_i2c_init(dev);
#else
#error "BUS MACRO NOT DEFINED IN DTS"
#endif

	u8_t tmp;
	if (data->hw_tf->read_reg(data, ICM20948_REG_WHO_AM_I, &tmp)) {
		LOG_ERR("Failed to read chip ID");
		return -EIO;
	}

	if (tmp != ICM20948_WHO_AM_I) {
		LOG_ERR("Invalid Chip ID");
	}

	/* set default fullscale range for acc */
	if (data->hw_tf->update_reg(data, ICM20948_REG_ACCEL_CONFIG,
				    ICM20948_ACCEL_MASK,
				    ICM20948_ACCEL_FS_DEFAULT)) {
		return -EIO;
	}

	/* set default fullscale range for gyro */
	if (data->hw_tf->update_reg(data, ICM20948_REG_GYRO_CONFIG_1,
				    ICM20948_GYRO_MASK,
				    ICM20948_GYRO_FS_DEFAULT)) {
		return -EIO;
	}

	return 0;
}

struct icm20948_data icm20948_data;

static const struct sensor_driver_api icm20498_driver_api = {
	.sample_fetch = icm20948_sample_fetch,
	.channel_get = icm20948_channel_get
};

DEVICE_AND_API_INIT(icm20948, CONFIG_ICM20948_NAME, icm20948_init,
		    &icm20948_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &icm20498_driver_api);
