/*
 * Copyright (c) 2025 LCKFB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qstcorp_qmi8658c

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "qmi8658c.h"

LOG_MODULE_REGISTER(QMI8658C, CONFIG_SENSOR_LOG_LEVEL);

/* Convert raw accelerometer value to m/s²
 * QMI8658C: 4g range, raw value range is -32768 to 32767
 * Conversion: (raw / 32767) * 4g * 9.80665 m/s²
 */
static void qmi8658c_convert_accel(struct sensor_value *val, int16_t raw_val)
{
	int64_t conv_val;

	/* Convert to m/s²: raw * 4 * SENSOR_G / 32767 */
	conv_val = ((int64_t)raw_val * 4LL * SENSOR_G) / 32767LL;
	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

/* Convert raw gyroscope value to rad/s
 * QMI8658C: 512dps range, raw value range is -32768 to 32767
 * Conversion: (raw / 32767) * 512dps * (π/180) rad/s
 */
static void qmi8658c_convert_gyro(struct sensor_value *val, int16_t raw_val)
{
	int64_t conv_val;

	/* Convert to rad/s: raw * 512 * SENSOR_PI / (32767 * 180) */
	conv_val = ((int64_t)raw_val * 512LL * SENSOR_PI) / (32767LL * 180LL);
	val->val1 = conv_val / 1000000LL;
	val->val2 = conv_val % 1000000LL;
}

/* Sample fetch implementation */
static int qmi8658c_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct qmi8658c_data *drv_data = dev->data;
	const struct qmi8658c_config *config = dev->config;
	uint8_t status;
	uint8_t buf[12];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Read status register */
	if (i2c_reg_read_byte_dt(&config->i2c, QMI8658C_REG_STATUS0, &status) < 0) {
		LOG_ERR("Failed to read status register");
		return -EIO;
	}

	/* Check if data is ready */
	if (!(status & (QMI8658C_STATUS0_ACC_DRDY | QMI8658C_STATUS0_GYR_DRDY))) {
		return -EBUSY;
	}

	/* Read acceleration and gyroscope data (12 bytes: 6 x int16_t) */
	if (i2c_burst_read_dt(&config->i2c, QMI8658C_REG_AX_L, buf, sizeof(buf)) < 0) {
		LOG_ERR("Failed to read sensor data");
		return -EIO;
	}

	/* Convert from little-endian bytes to int16_t values */
	drv_data->acc_x = (int16_t)sys_get_le16(&buf[0]);
	drv_data->acc_y = (int16_t)sys_get_le16(&buf[2]);
	drv_data->acc_z = (int16_t)sys_get_le16(&buf[4]);
	drv_data->gyr_x = (int16_t)sys_get_le16(&buf[6]);
	drv_data->gyr_y = (int16_t)sys_get_le16(&buf[8]);
	drv_data->gyr_z = (int16_t)sys_get_le16(&buf[10]);

	return 0;
}

/* Channel get implementation */
static int qmi8658c_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct qmi8658c_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_XYZ:
		qmi8658c_convert_accel(val, drv_data->acc_x);
		qmi8658c_convert_accel(val + 1, drv_data->acc_y);
		qmi8658c_convert_accel(val + 2, drv_data->acc_z);
		break;
	case SENSOR_CHAN_ACCEL_X:
		qmi8658c_convert_accel(val, drv_data->acc_x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		qmi8658c_convert_accel(val, drv_data->acc_y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		qmi8658c_convert_accel(val, drv_data->acc_z);
		break;
	case SENSOR_CHAN_GYRO_XYZ:
		qmi8658c_convert_gyro(val, drv_data->gyr_x);
		qmi8658c_convert_gyro(val + 1, drv_data->gyr_y);
		qmi8658c_convert_gyro(val + 2, drv_data->gyr_z);
		break;
	case SENSOR_CHAN_GYRO_X:
		qmi8658c_convert_gyro(val, drv_data->gyr_x);
		break;
	case SENSOR_CHAN_GYRO_Y:
		qmi8658c_convert_gyro(val, drv_data->gyr_y);
		break;
	case SENSOR_CHAN_GYRO_Z:
		qmi8658c_convert_gyro(val, drv_data->gyr_z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, qmi8658c_driver_api) = {
	.sample_fetch = qmi8658c_sample_fetch,
	.channel_get = qmi8658c_channel_get,
};

/* Initialize sensor */
static int qmi8658c_init(const struct device *dev)
{
	const struct qmi8658c_config *config = dev->config;
	uint8_t id = 0;
	int retry_count = 5;
	int i;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Small delay to ensure I2C bus is stable */
	k_sleep(K_MSEC(100));

	/* Read WHO_AM_I register to verify communication */
	for (i = 0; i < retry_count; i++) {
		if (i2c_reg_read_byte_dt(&config->i2c, QMI8658C_REG_WHO_AM_I, &id) == 0 &&
		    id == QMI8658C_WHO_AM_I_VAL) {
			break;
		}
		k_sleep(K_MSEC(100));
	}

	if (id != QMI8658C_WHO_AM_I_VAL) {
		LOG_ERR("QMI8658C not found, expected WHO_AM_I: 0x%02x, got: 0x%02x",
			QMI8658C_WHO_AM_I_VAL, id);
		return -ENODEV;
	}

	LOG_DBG("QMI8658C detected");

	/* Reset sensor */
	if (i2c_reg_write_byte_dt(&config->i2c, QMI8658C_REG_RESET, QMI8658C_RESET_VAL) < 0) {
		LOG_ERR("Failed to reset sensor");
		return -EIO;
	}
	k_sleep(K_MSEC(10));

	/* Configure CTRL1: Auto increment address */
	if (i2c_reg_write_byte_dt(&config->i2c, QMI8658C_REG_CTRL1, QMI8658C_CTRL1_AUTO_INC) < 0) {
		LOG_ERR("Failed to configure CTRL1");
		return -EIO;
	}

	/* Configure CTRL7: Enable accelerometer and gyroscope */
	if (i2c_reg_write_byte_dt(&config->i2c, QMI8658C_REG_CTRL7,
				  QMI8658C_CTRL7_ACC_EN | QMI8658C_CTRL7_GYR_EN) < 0) {
		LOG_ERR("Failed to configure CTRL7");
		return -EIO;
	}

	/* Configure CTRL2: Accelerometer 4g range, 250Hz ODR */
	if (i2c_reg_write_byte_dt(&config->i2c, QMI8658C_REG_CTRL2, 0x95) < 0) {
		LOG_ERR("Failed to configure CTRL2");
		return -EIO;
	}

	/* Configure CTRL3: Gyroscope 512dps range, 250Hz ODR */
	if (i2c_reg_write_byte_dt(&config->i2c, QMI8658C_REG_CTRL3, 0xd5) < 0) {
		LOG_ERR("Failed to configure CTRL3");
		return -EIO;
	}

	/* Wait for first data to be ready (250Hz ODR = 4ms period) */
	k_sleep(K_MSEC(10));

	return 0;
}

#define QMI8658C_DEFINE(inst)							\
	static struct qmi8658c_data qmi8658c_data_##inst;			\
										\
	static const struct qmi8658c_config qmi8658c_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, qmi8658c_init, NULL,		\
				      &qmi8658c_data_##inst,			\
				      &qmi8658c_config_##inst, POST_KERNEL,	\
				      CONFIG_SENSOR_INIT_PRIORITY,		\
				      &qmi8658c_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(QMI8658C_DEFINE)