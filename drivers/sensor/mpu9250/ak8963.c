/*
 * Copyright (c) 2021, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "mpu9250.h"
#include "ak8963.h"

LOG_MODULE_DECLARE(MPU9250, CONFIG_SENSOR_LOG_LEVEL);


#define I2C_READ_FLAG			BIT(7)

#define AK8963_I2C_ADDR			0x0C

#define AK8963_REG_ID			0x00
#define AK8963_REG_ID_VAL		0x48

#define AK8963_REG_DATA			0x03

#define AK8963_ST2_OVRFL_BIT		BIT(3)

#define AK8963_REG_CNTL1			0x0A
#define AK8963_REG_CNTL1_POWERDOWN_VAL		0x00
#define AK8963_REG_CNTL1_FUSE_ROM_VAL		0x0F
#define AK8963_REG_CNTL1_16BIT_100HZ_VAL	0x16
#define AK8963_SET_MODE_DELAY_MS		1

#define AK8963_REG_CNTL2			0x0B
#define AK8963_REG_CNTL2_RESET_VAL		0x01
#define AK8963_RESET_DELAY_MS			1

#define AK8963_REG_ADJ_DATA_X			0x10
#define AK8963_REG_ADJ_DATA_Y			0x11
#define AK8963_REG_ADJ_DATA_Z			0x12

#define AK9863_SCALE_TO_UG			1499

#define MPU9250_REG_I2C_MST_CTRL			0x24
#define MPU9250_REG_I2C_MST_CTRL_WAIT_MAG_400KHZ_VAL	0x4D

#define MPU9250_REG_I2C_SLV0_ADDR			0x25
#define MPU9250_REG_I2C_SLV0_REG			0x26
#define MPU9250_REG_I2C_SLV0_CTRL			0x27
#define MPU9250_REG_I2C_SLV0_DATA0			0x63
#define MPU9250_REG_READOUT_CTRL_VAL			(BIT(7) | 0x07)

#define MPU9250_REG_USER_CTRL				0x6A
#define MPU9250_REG_USER_CTRL_I2C_MASTERMODE_VAL	0x20

#define MPU9250_REG_EXT_DATA00				0x49

#define MPU9250_REG_I2C_SLV4_ADDR			0x31
#define MPU9250_REG_I2C_SLV4_REG			0x32
#define MPU9250_REG_I2C_SLV4_DO				0x33
#define MPU9250_REG_I2C_SLV4_CTRL			0x34
#define MPU9250_REG_I2C_SLV4_CTRL_VAL			0x80
#define MPU9250_REG_I2C_SLV4_DI				0x35

#define MPU9250_I2C_MST_STS				0x36
#define MPU9250_I2C_MST_STS_SLV4_DONE			BIT(6)


int ak8963_convert_magn(struct sensor_value *val, int16_t raw_val,
			 int16_t scale, uint8_t st2)
{
	/* The sensor device returns 10^-9 Teslas after scaling.
	 * Scale adjusts for calibration data and units
	 * So sensor instance returns Gauss units
	 */

	/* If overflow happens then value is invalid */
	if ((st2 & AK8963_ST2_OVRFL_BIT) != 0) {
		LOG_INF("Magnetometer value overflow.");
		return -EOVERFLOW;
	}

	int32_t scaled_val = (int32_t)raw_val * (int32_t)scale;

	val->val1 = scaled_val / 1000000;
	val->val2 = scaled_val % 1000000;
	return 0;
}


static int ak8963_execute_rw(const struct device *dev, uint8_t reg, bool write)
{
	/* Instruct the MPU9250 to access over its external i2c bus
	 * given device register with given details
	 */
	const struct mpu9250_config *cfg = dev->config;
	uint8_t mode_bit = 0x00;
	uint8_t status;
	int ret;

	if (!write) {
		mode_bit = I2C_READ_FLAG;
	}

	/* Set target i2c address */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV4_ADDR,
				    AK8963_I2C_ADDR | mode_bit);
	if (ret < 0) {
		LOG_ERR("Failed to write i2c target slave address.");
		return ret;
	}

	/* Set target i2c register */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV4_REG,
				    reg);
	if (ret < 0) {
		LOG_ERR("Failed to write i2c target slave register.");
		return ret;
	}

	/* Initiate transfer  */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV4_CTRL,
				    MPU9250_REG_I2C_SLV4_CTRL_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to initiate i2c slave transfer.");
		return ret;
	}

	/* Wait for a transfer to be ready */
	do {
		ret = i2c_reg_read_byte_dt(&cfg->i2c,
					   MPU9250_I2C_MST_STS, &status);
		if (ret < 0) {
			LOG_ERR("Waiting for slave failed.");
			return ret;
		}
	} while (!(status & MPU9250_I2C_MST_STS_SLV4_DONE));

	return 0;
}

static int ak8963_read_reg(const struct device *dev, uint8_t reg, uint8_t *data)
{
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	/* Execute transfer */
	ret = ak8963_execute_rw(dev, reg, false);
	if (ret < 0) {
		LOG_ERR("Failed to prepare transfer.");
		return ret;
	}

	/* Read the result */
	ret = i2c_reg_read_byte_dt(&cfg->i2c,
				   MPU9250_REG_I2C_SLV4_DI, data);
	if (ret < 0) {
		LOG_ERR("Failed to read data from slave.");
		return ret;
	}

	return 0;
}

static int ak8963_write_reg(const struct device *dev, uint8_t reg, uint8_t data)
{
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	/* Set the data to write */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV4_DO, data);
	if (ret < 0) {
		LOG_ERR("Failed to write data to slave.");
		return ret;
	}

	/* Execute transfer */
	ret = ak8963_execute_rw(dev, reg, true);
	if (ret < 0) {
		LOG_ERR("Failed to transfer write to slave.");
		return ret;
	}

	return 0;
}


static int ak8963_set_mode(const struct device *dev, uint8_t mode)
{
	int ret;

	ret = ak8963_write_reg(dev, AK8963_REG_CNTL1, mode);
	if (ret < 0) {
		LOG_ERR("Failed to set AK8963 mode.");
		return ret;
	}

	/* Wait for mode to change */
	k_msleep(AK8963_SET_MODE_DELAY_MS);
	return 0;
}

static int16_t ak8963_calc_adj(int16_t val)
{

	/** Datasheet says the actual register value is in 16bit output max
	 *  value of 32760 that corresponds to 4912 uT flux, yielding factor
	 *  of 0.149938.
	 *
	 *  Now Zephyr unit is Gauss, and conversion is 1T = 10^4G
	 *  -> 0.1499 * 10^4 = 1499
	 *  So if we multiply with scaling with 1499 the unit is uG.
	 *
	 *  Calculation from MPU-9250 Register Map and Descriptions
	 *  adj = (((val-128)*0.5)/128)+1
	 */
	return ((AK9863_SCALE_TO_UG * (val - 128)) / 256) + AK9863_SCALE_TO_UG;
}

static int ak8963_fetch_adj(const struct device *dev)
{
	/* Read magnetometer adjustment data from the AK8963 chip */
	struct mpu9250_data *drv_data = dev->data;
	uint8_t buf;
	int ret;

	/* Change to FUSE access mode to access adjustment registers */
	ret = ak8963_set_mode(dev, AK8963_REG_CNTL1_FUSE_ROM_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to set chip in fuse access mode.");
		return ret;
	}

	ret = ak8963_read_reg(dev, AK8963_REG_ADJ_DATA_X, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read adjustment data.");
		return ret;
	}
	drv_data->magn_scale_x = ak8963_calc_adj(buf);

	ret = ak8963_read_reg(dev, AK8963_REG_ADJ_DATA_Y, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read adjustment data.");
		return ret;
	}
	drv_data->magn_scale_y = ak8963_calc_adj(buf);

	ret = ak8963_read_reg(dev, AK8963_REG_ADJ_DATA_Z, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read adjustment data.");
		return ret;
	}
	drv_data->magn_scale_z = ak8963_calc_adj(buf);

	/* Change back to the powerdown mode */
	ret = ak8963_set_mode(dev, AK8963_REG_CNTL1_POWERDOWN_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to set chip in power down mode.");
		return ret;
	}

	LOG_DBG("Adjustment values %d %d %d", drv_data->magn_scale_x,
		drv_data->magn_scale_y, drv_data->magn_scale_z);

	return 0;
}

static int ak8963_reset(const struct device *dev)
{
	int ret;

	/* Reset the chip -> reset all settings. */
	ret = ak8963_write_reg(dev, AK8963_REG_CNTL2,
			       AK8963_REG_CNTL2_RESET_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to reset AK8963.");
		return ret;
	}

	/* Wait for reset */
	k_msleep(AK8963_RESET_DELAY_MS);

	return 0;
}

static int ak8963_init_master(const struct device *dev)
{
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	/* Instruct MPU9250 to use its external I2C bus as master */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_USER_CTRL,
				    MPU9250_REG_USER_CTRL_I2C_MASTERMODE_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to set MPU9250 master i2c mode.");
		return ret;
	}

	/* Set MPU9250 I2C bus as 400kHz and issue interrupt at data ready. */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_MST_CTRL,
				    MPU9250_REG_I2C_MST_CTRL_WAIT_MAG_400KHZ_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to set MPU9250 master i2c speed.");
		return ret;
	}

	return 0;
}

static int ak8963_init_readout(const struct device *dev)
{
	const struct mpu9250_config *cfg = dev->config;
	int ret;

	/* Set target i2c address */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV0_ADDR,
				    AK8963_I2C_ADDR | I2C_READ_FLAG);
	if (ret < 0) {
		LOG_ERR("Failed to set AK8963 slave address.");
		return ret;
	}

	/* Set target as data registers */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV0_REG, AK8963_REG_DATA);
	if (ret < 0) {
		LOG_ERR("Failed to set AK8963 register address.");
		return ret;
	}

	/* Initiate readout at sample rate */
	ret = i2c_reg_write_byte_dt(&cfg->i2c,
				    MPU9250_REG_I2C_SLV0_CTRL,
				    MPU9250_REG_READOUT_CTRL_VAL);
	if (ret < 0) {
		LOG_ERR("Failed to init AK8963 value readout.");
		return ret;
	}

	return 0;
}

int ak8963_init(const struct device *dev)
{
	uint8_t buf;
	int ret;

	ret = ak8963_init_master(dev);
	if (ret < 0) {
		LOG_ERR("Initializing MPU9250 master mode failed.");
		return ret;
	}

	ret = ak8963_reset(dev);
	if (ret < 0) {
		LOG_ERR("Resetting AK8963 failed.");
		return ret;
	}

	/* First check that the chip says hello */
	ret = ak8963_read_reg(dev, AK8963_REG_ID, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read AK8963 chip id.");
		return ret;
	}

	if (buf != AK8963_REG_ID_VAL) {
		LOG_ERR("Invalid AK8963 chip id (0x%X).", buf);
		return -ENOTSUP;
	}

	/* Fetch calibration data */
	ret = ak8963_fetch_adj(dev);
	if (ret < 0) {
		LOG_ERR("Calibrating AK8963 failed.");
		return ret;
	}

	/* Set AK sample rate and resolution */
	ret = ak8963_set_mode(dev, AK8963_REG_CNTL1_16BIT_100HZ_VAL);
	if (ret < 0) {
		LOG_ERR("Failed set sample rate for AK8963.");
		return ret;
	}

	/* Init constant readouts at sample rate */
	ret = ak8963_init_readout(dev);
	if (ret < 0) {
		LOG_ERR("Initializing AK8963 readout failed.");
		return ret;
	}

	return 0;
}
