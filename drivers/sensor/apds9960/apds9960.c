/*
 *
 *Copyright (c) 2017 Intel Corporation
 *
 *SPDX-License-Identifier: Apache-2.0
 *
 */

/* @file
 * @brief driver for APDS9960 ALS/RGB/gesture/proximity sensor
 */

#include <device.h>
#include <sensor.h>
#include <i2c.h>
#include <misc/__assert.h>
#include <misc/byteorder.h>
#include <init.h>
#include <kernel.h>
#include <string.h>

#include "apds9960.h"

static int apds9960_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct apds9960_data *data = dev->driver_data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* Read CRGB registers */
	if (i2c_burst_read(data->i2c, APDS9960_I2C_ADDRESS,
			   APDS9960_CDATAL_REG,
			   (u8_t *)&data->sample_crgb,
			   sizeof(data->sample_crgb))) {
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			      APDS9960_PDATA_REG, &data->pdata)) {
		return -EIO;
	}

	return 0;
}

static int apds9960_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct apds9960_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[0]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_RED:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[1]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[2]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		val->val1 = sys_le16_to_cpu(data->sample_crgb[3]);
		val->val2 = 0;
		break;
	case SENSOR_CHAN_PROX:
		val->val1 = data->pdata;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int apds9960_proxy_setup(struct device *dev, int gain)
{
	struct apds9960_data *data = dev->driver_data;

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_POFFSET_UR_REG,
			       APDS9960_DEFAULT_POFFSET_UR)) {
		SYS_LOG_ERR("Default offset UR not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_POFFSET_DL_REG,
			       APDS9960_DEFAULT_POFFSET_DL)) {
		SYS_LOG_ERR("Default offset DL not set ");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_PPULSE_REG,
			       APDS9960_DEFAULT_PROX_PPULSE)) {
		SYS_LOG_ERR("Default pulse count not set ");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_CONTROL_REG,
				APDS9960_CONTROL_LDRIVE,
				APDS9960_DEFAULT_LDRIVE)) {
		SYS_LOG_ERR("LED Drive Strength not set");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_CONTROL_REG, APDS9960_CONTROL_PGAIN,
				(gain & APDS9960_PGAIN_8X))) {
		SYS_LOG_ERR("Gain is not set");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_PILT_REG, APDS9960_DEFAULT_PILT)) {
		SYS_LOG_ERR("Low threshold not set");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_PIHT_REG, APDS9960_DEFAULT_PIHT)) {
		SYS_LOG_ERR("High threshold not set");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_ENABLE_REG, APDS9960_ENABLE_PEN,
				APDS9960_ENABLE_PEN)) {
		SYS_LOG_ERR("Proximity mode is not enabled");
		return -EIO;
	}

	return 0;
}

static int apds9960_ambient_setup(struct device *dev, int gain)
{
	struct apds9960_data *data = dev->driver_data;
	u16_t th;

	/* ADC value */
	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_ATIME_REG, APDS9960_DEFAULT_ATIME)) {
		SYS_LOG_ERR("Default integration time not set for ADC");
		return -EIO;
	}

	/* ALS Gain */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_CONTROL_REG,
				APDS9960_CONTROL_AGAIN,
				(gain & APDS9960_AGAIN_64X))) {
		SYS_LOG_ERR("Ambient Gain is not set");
		return -EIO;
	}

	th = sys_cpu_to_le16(APDS9960_DEFAULT_AILT);
	if (i2c_burst_write(data->i2c, APDS9960_I2C_ADDRESS,
			    APDS9960_INT_AILTL_REG,
			    (u8_t *)&th, sizeof(th))) {
		SYS_LOG_ERR("ALS low threshold not set");
		return -EIO;
	}

	th = sys_cpu_to_le16(APDS9960_DEFAULT_AIHT);
	if (i2c_burst_write(data->i2c, APDS9960_I2C_ADDRESS,
			    APDS9960_INT_AIHTL_REG,
			    (u8_t *)&th, sizeof(th))) {
		SYS_LOG_ERR("ALS low threshold not set");
		return -EIO;
	}

	/* Enable ALS */
	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_ENABLE_REG, APDS9960_ENABLE_AEN,
				APDS9960_ENABLE_AEN)) {
		SYS_LOG_ERR("ALS is not enabled");
		return -EIO;
	}

	return 0;
}

static int apds9960_sensor_setup(struct device *dev)
{
	struct apds9960_data *data = dev->driver_data;
	u8_t chip_id;

	if (i2c_reg_read_byte(data->i2c, APDS9960_I2C_ADDRESS,
			      APDS9960_ID_REG, &chip_id)) {
		SYS_LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	if (!((chip_id == APDS9960_ID_1) || (chip_id == APDS9960_ID_2))) {
		SYS_LOG_ERR("Invalid chip id 0x%x", chip_id);
		return -EIO;
	}

	/* Disable all functions and interrupts */
	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_ENABLE_REG, 0)) {
		SYS_LOG_ERR("ENABLE register is not cleared");
		return -EIO;
	}

	/* Disable gesture interrupt */
	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_GCONFIG4_REG, 0)) {
		SYS_LOG_ERR("GCONFIG4 register is not cleared");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_WTIME_REG, APDS9960_DEFAULT_WTIME)) {
		SYS_LOG_ERR("Default wait time not set");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_CONFIG1_REG,
			       APDS9960_DEFAULT_CONFIG1)) {
		SYS_LOG_ERR("Default WLONG not set");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_CONFIG2_REG,
			       APDS9960_DEFAULT_CONFIG2)) {
		SYS_LOG_ERR("Configuration Register Two not set");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, APDS9960_I2C_ADDRESS,
			       APDS9960_PERS_REG,
			       APDS9960_DEFAULT_PERS)) {
		SYS_LOG_ERR("Interrupt persistence not set");
		return -EIO;
	}

	if (apds9960_proxy_setup(dev, APDS9960_DEFAULT_PGAIN)) {
		SYS_LOG_ERR("Failed to setup proximity functionality");
		return -EIO;
	}

	if (apds9960_ambient_setup(dev, APDS9960_DEFAULT_AGAIN)) {
		SYS_LOG_ERR("Failed to setup ambient light functionality");
		return -EIO;
	}

	if (i2c_reg_update_byte(data->i2c, APDS9960_I2C_ADDRESS,
				APDS9960_ENABLE_REG,
				APDS9960_ENABLE_PON, APDS9960_ENABLE_PON)) {
		SYS_LOG_ERR("Power on bit not set");
		return -EIO;
	}

	return 0;
}

static const struct sensor_driver_api apds9960_driver_api = {
	.sample_fetch = &apds9960_sample_fetch,
	.channel_get = &apds9960_channel_get,
};

static int apds9960_init(struct device *dev)
{
	struct apds9960_data *data = dev->driver_data;

	data->i2c = device_get_binding(CONFIG_APDS9960_I2C_DEV_NAME);

	if (data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
		CONFIG_APDS9960_I2C_DEV_NAME);
		return -EINVAL;
	}

	(void)memset(data->sample_crgb, 0, sizeof(data->sample_crgb));
	data->pdata = 0;

	apds9960_sensor_setup(dev);

	return 0;
}

static struct apds9960_data apds9960_data;

DEVICE_AND_API_INIT(apds9960, CONFIG_APDS9960_DRV_NAME, &apds9960_init,
		&apds9960_data, NULL, POST_KERNEL,
		CONFIG_SENSOR_INIT_PRIORITY, &apds9960_driver_api);
