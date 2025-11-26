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

#include "qmi8658c.h"

LOG_MODULE_REGISTER(QMI8658C, CONFIG_SENSOR_LOG_LEVEL);

/* Read register */
static int qmi8658c_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct qmi8658c_config *config = dev->config;
	int ret;

	ret = i2c_write_read_dt(&config->i2c, &reg, 1, data, len);
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

/* Write register */
static int qmi8658c_reg_write_byte(const struct device *dev, uint8_t reg, uint8_t data)
{
	const struct qmi8658c_config *config = dev->config;
	uint8_t write_buf[2] = {reg, data};
	int ret;

	ret = i2c_write_dt(&config->i2c, write_buf, sizeof(write_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write register 0x%02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

/* Sample fetch implementation */
static int qmi8658c_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct qmi8658c_data *drv_data = dev->data;
	uint8_t status;
	int16_t raw_data[6];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Read status register */
	ret = qmi8658c_reg_read(dev, QMI8658C_REG_STATUS0, &status, 1);
	if (ret < 0) {
		return ret;
	}

	/* Check if data is ready */
	if (!(status & (QMI8658C_STATUS0_ACC_DRDY | QMI8658C_STATUS0_GYR_DRDY))) {
		return -EBUSY;
	}

	/* Read acceleration and gyroscope data (12 bytes)
	 * Reference code directly reads to int16_t array:
	 * qmi8658c_register_read(QMI8658C_AX_L, (uint8_t *)buf, 12);
	 * where buf is int16_t[6]
	 * This works because ESP32-C3 is little-endian and I2C data is little-endian
	 */
	ret = qmi8658c_reg_read(dev, QMI8658C_REG_AX_L, (uint8_t *)raw_data, 12);
	if (ret < 0) {
		return ret;
	}

	/* Direct assignment - exactly as reference code */
	drv_data->acc_x = raw_data[0];
	drv_data->acc_y = raw_data[1];
	drv_data->acc_z = raw_data[2];
	drv_data->gyr_x = raw_data[3];
	drv_data->gyr_y = raw_data[4];
	drv_data->gyr_z = raw_data[5];

	return 0;
}

/* Channel get implementation - directly return raw int16_t values
 * Exactly matching reference code: p->acc_x = buf[0]; etc.
 * No unit conversion, just return raw values
 */
static int qmi8658c_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct qmi8658c_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		/* Return raw int16_t value directly - exactly as reference code */
		val->val1 = (int32_t)drv_data->acc_x;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_ACCEL_Y:
		val->val1 = (int32_t)drv_data->acc_y;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_ACCEL_Z:
		val->val1 = (int32_t)drv_data->acc_z;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_ACCEL_XYZ:
		val[0].val1 = (int32_t)drv_data->acc_x;
		val[0].val2 = 0;
		val[1].val1 = (int32_t)drv_data->acc_y;
		val[1].val2 = 0;
		val[2].val1 = (int32_t)drv_data->acc_z;
		val[2].val2 = 0;
		break;

	case SENSOR_CHAN_GYRO_X:
		/* Return raw int16_t value directly - exactly as reference code */
		val->val1 = (int32_t)drv_data->gyr_x;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GYRO_Y:
		val->val1 = (int32_t)drv_data->gyr_y;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GYRO_Z:
		val->val1 = (int32_t)drv_data->gyr_z;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_GYRO_XYZ:
		val[0].val1 = (int32_t)drv_data->gyr_x;
		val[0].val2 = 0;
		val[1].val1 = (int32_t)drv_data->gyr_y;
		val[1].val2 = 0;
		val[2].val1 = (int32_t)drv_data->gyr_z;
		val[2].val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api qmi8658c_driver_api = {
	.sample_fetch = qmi8658c_sample_fetch,
	.channel_get = qmi8658c_channel_get,
};

/* Initialize sensor */
static int qmi8658c_init(const struct device *dev)
{
	const struct qmi8658c_config *config = dev->config;
	uint8_t id = 0;
	int ret;
	int retry_count = 5;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Small delay to ensure I2C bus is stable */
	k_sleep(K_MSEC(100));

	/* Read WHO_AM_I register to verify communication */
	for (int i = 0; i < retry_count; i++) {
		ret = qmi8658c_reg_read(dev, QMI8658C_REG_WHO_AM_I, &id, 1);
		if (ret == 0 && id == QMI8658C_WHO_AM_I_VAL) {
			break;
		}
		k_sleep(K_MSEC(100));
	}

	if (id != QMI8658C_WHO_AM_I_VAL) {
		LOG_ERR("QMI8658C not found, expected WHO_AM_I: 0x%02x, got: 0x%02x",
			QMI8658C_WHO_AM_I_VAL, id);
		return -ENODEV;
	}

	/* Reset sensor */
	ret = qmi8658c_reg_write_byte(dev, QMI8658C_REG_RESET, QMI8658C_RESET_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to reset sensor");
		return ret;
	}
	k_sleep(K_MSEC(10));

	/* Configure CTRL1: Auto increment address */
	ret = qmi8658c_reg_write_byte(dev, QMI8658C_REG_CTRL1, QMI8658C_CTRL1_AUTO_INC);
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL1");
		return ret;
	}

	/* Configure CTRL7: Enable accelerometer and gyroscope */
	ret = qmi8658c_reg_write_byte(dev, QMI8658C_REG_CTRL7,
				      QMI8658C_CTRL7_ACC_EN | QMI8658C_CTRL7_GYR_EN);
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL7");
		return ret;
	}

	/* Configure CTRL2: Accelerometer 4g range, 250Hz ODR */
	ret = qmi8658c_reg_write_byte(dev, QMI8658C_REG_CTRL2, QMI8658C_CTRL2_ACC_4G_250HZ);
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL2");
		return ret;
	}

	/* Configure CTRL3: Gyroscope 512dps range, 250Hz ODR */
	ret = qmi8658c_reg_write_byte(dev, QMI8658C_REG_CTRL3, QMI8658C_CTRL3_GYR_512DPS_250HZ);
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL3");
		return ret;
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

