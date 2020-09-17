/*
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_icm42605

#include <drivers/spi.h>
#include <init.h>
#include <sys/byteorder.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "icm42605.h"
#include "icm42605_reg.h"
#include "icm42605_setup.h"
#include "icm42605_spi.h"

LOG_MODULE_REGISTER(ICM42605, CONFIG_SENSOR_LOG_LEVEL);

struct spi_cs_control spi_cs;

static struct spi_config spi_cfg = {
	.frequency = DT_INST_PROP(0, spi_max_frequency),
	.operation = (SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
		      SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE |
		      SPI_TRANSFER_MSB),
	.slave = DT_INST_REG_ADDR(0),
	.cs = NULL,
};

/* see "Accelerometer Measurements" section from register map description */
static void icm42605_convert_accel(struct sensor_value *val, int16_t raw_val,
				   uint16_t sensitivity_shift)
{
	int64_t conv_val;

	conv_val = ((int64_t)raw_val * SENSOR_G) >> sensitivity_shift;
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Gyroscope Measurements" section from register map description */
static void icm42605_convert_gyro(struct sensor_value *val, int16_t raw_val,
				  uint16_t sensitivity_x10)
{
	int64_t conv_val;

	conv_val = ((int64_t)raw_val * SENSOR_PI * 10) /
		   (sensitivity_x10 * 180U);
	val->val1 = conv_val / 1000000;
	val->val2 = conv_val % 1000000;
}

/* see "Temperature Measurement" section from register map description */
static inline void icm42605_convert_temp(struct sensor_value *val,
					 int16_t raw_val)
{
	val->val1 = (((int64_t)raw_val * 100) / 207) + 25;
	val->val2 = ((((int64_t)raw_val * 100) % 207) * 1000000) / 207;

	if (val->val2 < 0) {
		val->val1--;
		val->val2 += 1000000;
	} else if (val->val2 >= 1000000) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

static int icm42605_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct icm42605_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		icm42605_convert_accel(val, drv_data->accel_x,
				       drv_data->accel_sensitivity_shift);
		icm42605_convert_accel(val + 1, drv_data->accel_y,
				       drv_data->accel_sensitivity_shift);
		icm42605_convert_accel(val + 2, drv_data->accel_z,
				       drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_X:
		icm42605_convert_accel(val, drv_data->accel_x,
				       drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		icm42605_convert_accel(val, drv_data->accel_y,
				       drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		icm42605_convert_accel(val, drv_data->accel_z,
				       drv_data->accel_sensitivity_shift);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		icm42605_convert_gyro(val, drv_data->gyro_x,
				      drv_data->gyro_sensitivity_x10);
		icm42605_convert_gyro(val + 1, drv_data->gyro_y,
				      drv_data->gyro_sensitivity_x10);
		icm42605_convert_gyro(val + 2, drv_data->gyro_z,
				      drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_X:
		icm42605_convert_gyro(val, drv_data->gyro_x,
				      drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Y:
		icm42605_convert_gyro(val, drv_data->gyro_y,
				      drv_data->gyro_sensitivity_x10);
		break;
	case SENSOR_CHAN_GYRO_Z:
		icm42605_convert_gyro(val, drv_data->gyro_z,
				      drv_data->gyro_sensitivity_x10);
		break;
	default: /* chan == SENSOR_CHAN_DIE_TEMP */
		icm42605_convert_temp(val, drv_data->temp);
	}

	return 0;
}

static int icm42605_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	int result = 0;
	uint16_t fifo_count = 0;
	static uint8_t data[HARDWARE_FIFO_SIZE] = { 0, };
	struct icm42605_data *drv_data = dev->data;

	if (drv_data->tap_en) {
		result = inv_spi_read(REG_INT_STATUS3, data, 1);

		if (data[0] & BIT_INT_STATUS_TAP_DET) {
			LOG_DBG("Tap detected!!!");
			result = inv_spi_read(REG_APEX_DATA4, data, 1);
			if (data[0] & 0x08) {
				LOG_DBG("Single Tap");
			} else if (data[0] & 0x10) {
				LOG_DBG("Double Tap");
			} else {
				LOG_DBG("Not supported");
			}
		}
	}

	/* Read INT_STATUS (0x45) and FIFO_COUNTH(0x46), FIFO_COUNTL(0x47) */
	result = inv_spi_read(REG_INT_STATUS, data, 3);

	if (data[0] & BIT_INT_STATUS_DRDY) {
		fifo_count = ((data[1] & 0xff) << 8) + (data[2] & 0xff);
		result = inv_spi_read(REG_FIFO_DATA, data, fifo_count);

		/* FIFO Data structure
		 * Packet 1 : FIFO Header(1), AccelX(2), AccelY(2),
		 *            AccelZ(2), Temperature(1)
		 * Packet 2 : FIFO Header(1), GyroX(2), GyroY(2),
		 *            GyroZ(2), Temperature(1)
		 * Packet 3 : FIFO Header(1), AccelX(2), AccelY(2), AccelZ(2),
		 *            GyroX(2), GyroY(2), GyroZ(2), Temperature(1)
		 */

		if (data[0] & BIT_FIFO_HEAD_ACCEL) {
			/* Check empty values */
			if (!(data[1] == 0x80 && data[2] == 0x00)) {
				drv_data->accel_x =
					((data[1] & 0xff) << 8)
					+ (data[2] & 0xff);
				drv_data->accel_y =
					((data[3] & 0xff) << 8)
					+ (data[4] & 0xff);
				drv_data->accel_z =
					((data[5] & 0xff) << 8)
					+ (data[6] & 0xff);
			}
			if (!(data[0] & BIT_FIFO_HEAD_GYRO)) {
				drv_data->temp = (int16_t)data[7];
			} else {
				if (!(data[7] == 0x80 && data[8] == 0x00)) {
					drv_data->gyro_x =
						((data[7] & 0xff) << 8)
						+ (data[8] & 0xff);
					drv_data->gyro_y =
						((data[9] & 0xff) << 8)
						+ (data[10] & 0xff);
					drv_data->gyro_z =
						((data[11] & 0xff) << 8)
						+ (data[12] & 0xff);
				}
				drv_data->temp = (int16_t)data[13];
			}
		} else {
			if (data[0] & BIT_FIFO_HEAD_GYRO) {
				if (!(data[1] == 0x80 && data[2] == 0x00)) {
					drv_data->gyro_x =
						((data[1] & 0xff) << 8)
						+ (data[2] & 0xff);
					drv_data->gyro_y =
						((data[3] & 0xff) << 8)
						+ (data[4] & 0xff);
					drv_data->gyro_z =
						((data[5] & 0xff) << 8)
						+ (data[6] & 0xff);
				}
				drv_data->temp = (int16_t)data[7];
			}
		}
	}

	return 0;
}

static int icm42605_attr_set(struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     const struct sensor_value *val)
{
	struct icm42605_data *drv_data = dev->data;

	if (val == NULL) {
		LOG_ERR("Attr set : val is null");
		return -EINVAL;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			if (val->val1 > 1000) {
				drv_data->accel_hz = 1000;
			} else if (val->val1 < 12) {
				drv_data->accel_hz = 12;
			} else {
				drv_data->accel_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			if (val->val1 < 0) {
				drv_data->accel_sf = ACCEL_FS_16G;
			} else if (val->val1 > 3) {
				drv_data->accel_sf = ACCEL_FS_2G;
			} else {
				drv_data->accel_sf = val->val1;
			}
		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}

		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			if (val->val1 > 1000) {
				drv_data->gyro_hz = 1000;
			} else if (val->val1 < 12) {
				drv_data->gyro_hz = 12;
			} else {
				drv_data->gyro_hz = val->val1;
			}
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			if (val->val1 < 0) {
				drv_data->gyro_sf = GYRO_FS_2000DPS;
			}
			if (val->val1 > 7) {
				drv_data->gyro_sf = GYRO_FS_15DPS;
			}
			drv_data->gyro_sf = val->val1;
		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Not support");
		return -EINVAL;
	}

	return 0;
}

static int icm42605_attr_get(struct device *dev,
			     enum sensor_channel chan,
			     enum sensor_attribute attr,
			     struct sensor_value *val)
{
	struct icm42605_data *drv_data = dev->data;

	if (val == NULL) {
		LOG_ERR("Attr set : val is null");
		return -EINVAL;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = drv_data->accel_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = drv_data->accel_sf;
		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}

		break;
	case SENSOR_CHAN_GYRO_X:
	case SENSOR_CHAN_GYRO_Y:
	case SENSOR_CHAN_GYRO_Z:
	case SENSOR_CHAN_GYRO_XYZ:
		if (attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
			val->val1 = drv_data->gyro_hz;
		} else if (attr == SENSOR_ATTR_FULL_SCALE) {
			val->val1 = drv_data->gyro_sf;
		} else {
			LOG_ERR("Not supported ATTR");
			return -EINVAL;
		}

		break;

	default:
		LOG_ERR("Not support");
		return -EINVAL;
	}

	return 0;
}

static int icm42605_data_init(struct icm42605_data *data)
{
	data->accel_x = 0;
	data->accel_y = 0;
	data->accel_z = 0;
	data->temp = 0;
	data->gyro_x = 0;
	data->gyro_y = 0;
	data->gyro_z = 0;
	data->accel_hz = 10;
	data->gyro_hz = 10;

	data->accel_sf = ACCEL_FS_16G;
	data->gyro_sf = GYRO_FS_2000DPS;

	data->tap_en = false;
	data->sensor_started = false;

	return 0;
}


static const struct sensor_driver_api icm42605_driver_api = {
	.trigger_set = icm42605_trigger_set,
	.sample_fetch = icm42605_sample_fetch,
	.channel_get = icm42605_channel_get,
	.attr_set = icm42605_attr_set,
	.attr_get = icm42605_attr_get,
};

int icm42605_init(struct device *dev)
{
	struct icm42605_data *drv_data = dev->data;

	drv_data->spi = device_get_binding(DT_INST_BUS_LABEL(0));

	spi_cs.gpio_dev = device_get_binding(DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	spi_cs.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	spi_cs.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	spi_cs.delay = 0U;
	spi_cfg.cs = &spi_cs;

	icm42605_spi_init(drv_data->spi, &spi_cfg);
	icm42605_data_init(drv_data);

	k_msleep(100);

	icm42605_sensor_init(dev);

	drv_data->accel_sensitivity_shift = 14 - 3;
	drv_data->gyro_sensitivity_x10 = icm42605_gyro_sensitivity_x10[3];

	if (icm42605_init_interrupt(dev) < 0) {
		LOG_ERR("Failed to initialize interrupts.");
		return -EIO;
	}

	LOG_DBG("Initialize interrupt done");

	return 0;
}


static struct icm42605_data icm42605_driver;
static const struct icm42605_config icm42605_cfg = {
	.spi_label = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.spi_addr = DT_INST_REG_ADDR(0),
	.int_label = DT_INST_GPIO_LABEL(0, int_gpios),
	.int_pin =  DT_INST_GPIO_PIN(0, int_gpios),
	.int_flags = DT_INST_GPIO_FLAGS(0, int_gpios),
};

DEVICE_AND_API_INIT(icm42605, DT_INST_LABEL(0),
		    icm42605_init, &icm42605_driver, &icm42605_cfg,
		    POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &icm42605_driver_api);
