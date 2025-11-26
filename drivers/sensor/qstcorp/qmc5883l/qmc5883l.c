/*
 * Copyright (c) 2025 LCKFB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT qstcorp_qmc5883l

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "qmc5883l.h"

LOG_MODULE_REGISTER(QMC5883L, CONFIG_SENSOR_LOG_LEVEL);

/* Read register */
static int qmc5883l_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, size_t len)
{
	const struct qmc5883l_config *config = dev->config;
	int ret;

	ret = i2c_write_read_dt(&config->i2c, &reg, 1, data, len);
	if (ret < 0) {
		LOG_ERR("Failed to read register 0x%02x: %d", reg, ret);
		return ret;
	}

	return 0;
}

/* Write register */
static int qmc5883l_reg_write_byte(const struct device *dev, uint8_t reg, uint8_t data)
{
	const struct qmc5883l_config *config = dev->config;
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
static int qmc5883l_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct qmc5883l_data *drv_data = dev->data;
	uint8_t status;
	int16_t mag_data[3];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Read status register */
	ret = qmc5883l_reg_read(dev, QMC5883L_REG_STATUS, &status, 1);
	if (ret < 0) {
		return ret;
	}

	/* Check if data is ready */
	if (!(status & QMC5883L_STATUS_DRDY)) {
		return -EBUSY;
	}

	/* Read magnetometer data (6 bytes) - exactly as reference code */
	ret = qmc5883l_reg_read(dev, QMC5883L_REG_XOUT_L, (uint8_t *)mag_data, 6);
	if (ret < 0) {
		return ret;
	}

	/* Direct assignment - exactly as reference code */
	drv_data->mag_x = mag_data[0];
	drv_data->mag_y = mag_data[1];
	drv_data->mag_z = mag_data[2];

	return 0;
}

/* Channel get implementation - directly return raw int16_t values */
static int qmc5883l_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct qmc5883l_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		/* Return raw int16_t value directly */
		val->val1 = (int32_t)drv_data->mag_x;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_MAGN_Y:
		val->val1 = (int32_t)drv_data->mag_y;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_MAGN_Z:
		val->val1 = (int32_t)drv_data->mag_z;
		val->val2 = 0;
		break;

	case SENSOR_CHAN_MAGN_XYZ:
		val[0].val1 = (int32_t)drv_data->mag_x;
		val[0].val2 = 0;
		val[1].val1 = (int32_t)drv_data->mag_y;
		val[1].val2 = 0;
		val[2].val1 = (int32_t)drv_data->mag_z;
		val[2].val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api qmc5883l_driver_api = {
	.sample_fetch = qmc5883l_sample_fetch,
	.channel_get = qmc5883l_channel_get,
};

/* Initialize sensor - exactly matching reference code */
static int qmc5883l_init(const struct device *dev)
{
	const struct qmc5883l_config *config = dev->config;
	uint8_t id = 0;
	int ret;
	int retry_count = 5;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready: %s", config->i2c.bus->name);
		return -ENODEV;
	}

	/* Small delay to ensure I2C bus is stable */
	k_sleep(K_MSEC(100));

	/* Read CHIPID register to verify communication - exactly as reference code */
	for (int i = 0; i < retry_count; i++) {
		ret = qmc5883l_reg_read(dev, QMC5883L_REG_CHIPID, &id, 1);
		if (ret == 0 && id == QMC5883L_CHIPID_VAL) {
			break;
		}
		k_sleep(K_MSEC(100));
	}

	if (id != QMC5883L_CHIPID_VAL) {
		LOG_ERR("QMC5883L not found, expected CHIPID: 0x%02x, got: 0x%02x",
			QMC5883L_CHIPID_VAL, id);
		return -ENODEV;
	}

	LOG_INF("QMC5883L OK!");

	/* Reset sensor - exactly as reference code: CTRL2 = 0x80 */
	ret = qmc5883l_reg_write_byte(dev, QMC5883L_REG_CTRL2, QMC5883L_CTRL2_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to reset sensor");
		return ret;
	}
	k_sleep(K_MSEC(10));

	/* Configure CTRL1: Continuous mode, 50Hz - exactly as reference code: CTRL1 = 0x05 */
	ret = qmc5883l_reg_write_byte(dev, QMC5883L_REG_CTRL1, QMC5883L_CTRL1_CONT_50HZ);
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL1");
		return ret;
	}

	/* Configure CTRL2: 0x00 - exactly as reference code */
	ret = qmc5883l_reg_write_byte(dev, QMC5883L_REG_CTRL2, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL2");
		return ret;
	}

	/* Configure FBR: 0x01 - exactly as reference code */
	ret = qmc5883l_reg_write_byte(dev, QMC5883L_REG_FBR, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to configure FBR");
		return ret;
	}

	return 0;
}

#define QMC5883L_DEFINE(inst)							\
	static struct qmc5883l_data qmc5883l_data_##inst;			\
										\
	static const struct qmc5883l_config qmc5883l_config_##inst = {	\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, qmc5883l_init, NULL,		\
				      &qmc5883l_data_##inst,			\
				      &qmc5883l_config_##inst, POST_KERNEL,	\
				      CONFIG_SENSOR_INIT_PRIORITY,		\
				      &qmc5883l_driver_api);			\
	
DT_INST_FOREACH_STATUS_OKAY(QMC5883L_DEFINE)

