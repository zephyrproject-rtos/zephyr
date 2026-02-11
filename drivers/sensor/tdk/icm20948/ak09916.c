/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm20948.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(icm20948, CONFIG_SENSOR_LOG_LEVEL);

#define I2C_READ_FLAG  BIT(7)
#define I2C_WRITE_FLAG 0x00

#define AK09916_I2C_ADDR 0x0C

/**
 * @brief Device ID (Who Am I) Register
 * @details Fixed value of 0x09
 */
#define AK09916_REG_WIA 0x01
#define AK09916_WIA_VAL 0x09

/**
 * @brief STATUS 1 Register
 */
#define AK09916_REG_ST1 0x10

/**
 * @brief Self-test mode. It returns to “0” when any one of ST2 register or measurement data
 * register (HXL to TMPS) is read.
 */
#define AK09916_ST1_DRDY_MASK BIT(0)

/**
 * @brief STATUS 1 Register
 * @details DOR bit turns to "1" when data has been skipped in Continuous measurement
 * mode 1, 2, 3, 4. It returns to "0" when any one of ST2 register or
 * measurement data register (HXL to TMPS) is read.
 */
#define AK09916_ST1_DOR_MASK BIT(1)

/**
 * @brief HXL to HZH Measurement Data Registers
 * @details Measurement data is stored in two’s complement and Little Endian format. Measurement
 * range of each axis is from --32752 to 32752 in 16-bit output. LSB = 0.15 uT.
 */
#define AK09916_REG_HXL 0x11 /**< X-axis measurement data lower 8bit */
#define AK09916_REG_HXH 0x12 /**< X-axis measurement data higher 8bit */
#define AK09916_REG_HYL 0x13 /**< Y-axis measurement data lower 8bit */
#define AK09916_REG_HYH 0x14 /**< Y-axis measurement data higher 8bit */
#define AK09916_REG_HZL 0x15 /**< Z-axis measurement data lower 8bit */
#define AK09916_REG_HZH 0x16 /**< Z-axis measurement data higher 8bit */

#define AK09916_REG_DATA_START AK09916_REG_HXL
#define AK09916_REG_DATA_BYTES 6

/**
 * @brief STATUS 2 Register
 * @details In Single measurement mode, Continuous measurement mode 1, 2, 3, 4, and
 * Self-test mode, magnetic sensor may overflow even though measurement data
 * register is not saturated. In this case, measurement data is not correct
 * and HOFL bit turns to "1". When measurement data register is updated, HOFL
 * bit is updated. ST2 register has a role as data reading end register, also.
 * When any of measurement data register (HXL to TMPS) is read in Continuous
 * measurement mode 1, 2, 3, 4, it means data reading start and taken as data
 * reading until ST2 register is read. Therefore, when any of measurement data
 * is read, be sure to read ST2 register at the end.
 */
#define AK09916_REG_ST2        0x18
#define AK09916_ST2_HOFL_MASK  BIT(3) /**< Magnetic sensor overflow flag. */
#define AK09916_ST2_RSV28_MASK BIT(4) /**< Reserved. */
#define AK09916_ST2_RSV29_MASK BIT(5) /**< Reserved. */
#define AK09916_ST2_RSV30_MASK BIT(6) /**< Reserved. */

/*
 * We want to automatically include the HOFL status bit in REG 0x18, so we need
 * 2 more bytes on each data block read.
 */
#define AK09916_REG_DATA_PLUS_STATUS_BYTES (AK09916_REG_DATA_BYTES + 2)

/**
 * @brief CONTROL 1 Register
 */
#define AK09916_REG_CNTL1 0x30

/**
 * @brief CONTROL 2 Register
 */
#define AK09916_REG_CNTL2            0x31
#define AK09916_CNTL2_MODE_MASK      (BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define AK09916_CNTL2_MODE_SELF_TEST 0x10

/**
 * @brief CONTROL 3 Register
 */
#define AK09916_REG_CNTL3 0x32

/*
 * When "1" is set, all registers are initialized. After reset, SRST bit turns
 * to "0" automatically.
 */
#define AK09916_CNTL3_SRST_MASK BIT(0)

#define AK09916_SCALE_TO_UG 1499

int ak09916_convert_magn(struct sensor_value *val, int16_t raw_val, int16_t scale, uint8_t st2)
{
	/* The sensor device returns 10^-9 Teslas after scaling.
	 * Scale adjusts for calibration data and units
	 * So sensor instance returns Gauss units
	 */

	/* If overflow happens then value is invalid */
	if ((st2 & AK09916_ST2_HOFL_MASK) != 0) {
		LOG_INF("Magnetometer value overflow.");
		return -EOVERFLOW;
	}

	int32_t scaled_val = (int32_t)raw_val * (int32_t)scale;

	val->val1 = scaled_val / SENSOR_VALUE_SCALE;
	val->val2 = scaled_val % SENSOR_VALUE_SCALE;
	return 0;
}

static int ak09916_execute_rw(const struct device *dev, uint8_t reg, bool write)
{
	/* Use ICM20948's I2C master (SLV4) to access AK09916 */
	uint8_t mode_bit = write ? I2C_WRITE_FLAG : I2C_READ_FLAG;
	uint8_t status;
	int ret;
	int timeout = AK09916_I2C_TIMEOUT_LOOPS;

	/* Set target I2C address (Bank 3) */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV4_ADDR, AK09916_I2C_ADDR | mode_bit);
	if (ret < 0) {
		LOG_ERR("Failed to write I2C target slave address (err %d).", ret);
		return ret;
	}

	/* Set target I2C register (Bank 3) */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV4_REG, reg);
	if (ret < 0) {
		LOG_ERR("Failed to write I2C target slave register (err %d).", ret);
		return ret;
	}

	/* Initiate transfer by enabling SLV4 (Bank 3) */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV4_CTRL, ICM20948_I2C_SLVX_CTRL_EN_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to initiate I2C slave transfer (err %d).", ret);
		return ret;
	}

	/* Wait for transfer to complete (Bank 0) */
	do {
		k_usleep(AK09916_I2C_POLL_DELAY_US);
		ret = icm20948_read_reg(dev, ICM20948_REG_I2C_MST_STATUS, &status);
		if (ret < 0) {
			LOG_ERR("Waiting for slave failed (err %d).", ret);
			return ret;
		}
		if (--timeout == 0) {
			LOG_ERR("I2C master transfer timeout");
			return -ETIMEDOUT;
		}
	} while (!(status & ICM20948_I2C_MST_STATUS_SLV4_DONE_MASK));

	/* Check for NACK */
	if (status & ICM20948_I2C_MST_STATSU_SLV4_NACK_MASK) {
		LOG_ERR("AK09916 NACK received");
		return -EIO;
	}

	return 0;
}

static int ak09916_read_reg(const struct device *dev, uint8_t reg, uint8_t *data)
{
	int ret;

	/* Execute read transfer */
	ret = ak09916_execute_rw(dev, reg, false);
	if (ret < 0) {
		LOG_ERR("Failed to execute read transfer to slave (err %d).", ret);
		return ret;
	}

	/* Read the result from SLV4_DI (Bank 3) */
	ret = icm20948_read_reg(dev, ICM20948_REG_I2C_SLV4_DI, data);
	if (ret < 0) {
		LOG_ERR("Failed to read data from slave (err %d).", ret);
		return ret;
	}

	return 0;
}

static int ak09916_write_reg(const struct device *dev, uint8_t reg, uint8_t data)
{
	int ret;

	/* Set the data to write in SLV4_DO (Bank 3) */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV4_DO, data);
	if (ret < 0) {
		LOG_ERR("Failed to write data to slave (err %d).", ret);
		return ret;
	}

	/* Execute write transfer */
	ret = ak09916_execute_rw(dev, reg, true);
	if (ret < 0) {
		LOG_ERR("Failed to execute write transfer to slave (err %d).", ret);
		return ret;
	}

	return 0;
}

static int ak09916_set_mode(const struct device *dev, uint8_t mode)
{
	int ret;

	ret = ak09916_write_reg(dev, AK09916_REG_CNTL2, mode);
	if (ret < 0) {
		LOG_ERR("Failed to set AK09916 mode (err %d).", ret);
		return ret;
	}

	/* Mode change requires small delay */
	k_usleep(AK09916_MODE_CHANGE_DELAY_US);

	return 0;
}

static void ak09916_set_scale(const struct device *dev)
{
	/* AK09916 (unlike AK8963 in MPU-9250) does NOT have sensitivity adjustment registers.
	 * It has fixed sensitivity of 0.15 µT/LSB.
	 *
	 * Datasheet: 16-bit output, max value 32752 corresponds to 4912 µT
	 * Scale factor: 4912 / 32752 = 0.15 µT/LSB
	 *
	 * Zephyr uses Gauss: 1 T = 10^4 G, so 0.15 µT = 0.15 * 10^-6 T = 1.5 * 10^-6 G = 1.5 µG
	 * Scale to µG: 0.15 * 10^4 = 1500 (we use 1499 for precision)
	 */
	struct icm20948_data *drv_data = dev->data;

	drv_data->magn_scale_x = AK09916_SCALE_TO_UG;
	drv_data->magn_scale_y = AK09916_SCALE_TO_UG;
	drv_data->magn_scale_z = AK09916_SCALE_TO_UG;

	LOG_DBG("Magnetometer scale set to %d µG/LSB", AK09916_SCALE_TO_UG);
}

static int ak09916_reset(const struct device *dev)
{
	int ret;

	/* Reset the chip -> reset all settings */
	ret = ak09916_write_reg(dev, AK09916_REG_CNTL3, AK09916_CNTL3_SRST_MASK);
	if (ret < 0) {
		LOG_ERR("Failed to reset AK09916 (err %d).", ret);
		return ret;
	}

	uint16_t count = 0;
	uint8_t rst = 1;

	do {
		ak09916_read_reg(dev, AK09916_REG_CNTL3, &rst);
		if (count++ > AK09916_RESET_TIMEOUT_LOOPS) {
			LOG_ERR("Timed out waiting for reset bit to clear.");
			return -ETIMEDOUT;
		}
	} while (FIELD_GET(AK09916_CNTL3_SRST_MASK, rst) == 1);

	return 0;
}

static int ak09916_init_master(const struct device *dev)
{
	int ret;

	/* Disable I2C bypass mode - required for I2C master to work */
	ret = icm20948_write_field(dev, ICM20948_REG_INT_PIN_CFG,
				   ICM20948_INT_PIN_CFG_BYPASS_EN_MASK, 0);
	if (ret < 0) {
		LOG_ERR("Failed to disable I2C bypass mode (err %d).", ret);
		return ret;
	}

	/* Enable I2C master mode in USER_CTRL (Bank 0) */
	ret = icm20948_write_field(dev, ICM20948_REG_USER_CTRL, ICM20948_USER_CTRL_I2C_MST_EN_MASK,
				   1);
	if (ret < 0) {
		LOG_ERR("Failed to enable I2C master mode (err %d).", ret);
		return ret;
	}

	/* Set I2C master clock to 400kHz (Bank 3) */
	ret = icm20948_write_field(dev, ICM20948_REG_I2C_MST_CTRL, ICM20948_I2C_MST_CTRL_CLK_MASK,
				   ICM20948_I2C_MST_CTRL_CLK_400KHZ);
	if (ret < 0) {
		LOG_ERR("Failed to set I2C master clock (err %d).", ret);
		return ret;
	}

	/* Small delay for I2C master to stabilize */
	k_msleep(AK09916_I2C_MST_STABILIZE_MS);

	return 0;
}

static int ak09916_init_readout(const struct device *dev)
{
	int ret;

	/* Configure SLV0 to automatically read magnetometer data.
	 * Data will be placed in EXT_SLV_SENS_DATA registers starting at 0x3B.
	 */

	/* Set target I2C address (read mode) - Bank 3 */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV0_ADDR, AK09916_I2C_ADDR | I2C_READ_FLAG);
	if (ret < 0) {
		LOG_ERR("Failed to set AK09916 slave address (err %d).", ret);
		return ret;
	}

	/* Set target register to start reading from (HXL) - Bank 3 */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV0_REG, AK09916_REG_DATA_START);
	if (ret < 0) {
		LOG_ERR("Failed to set AK09916 register address (err %d).", ret);
		return ret;
	}

	/* Enable SLV0 and set to read 8 bytes (6 data + ST2 + dummy for alignment) - Bank 3
	 * Reading ST2 is required to signal end of data read to AK09916.
	 */
	ret = icm20948_write_reg(dev, ICM20948_REG_I2C_SLV0_CTRL,
				 ICM20948_I2C_SLVX_CTRL_EN_MASK |
					 AK09916_REG_DATA_PLUS_STATUS_BYTES);
	if (ret < 0) {
		LOG_ERR("Failed to enable AK09916 readout (err %d).", ret);
		return ret;
	}

	return 0;
}

int ak09916_init(const struct device *dev)
{
	const struct icm20948_config *cfg = (const struct icm20948_config *)dev->config;

	uint8_t buf;
	int ret;

	ret = ak09916_init_master(dev);
	if (ret < 0) {
		LOG_ERR("Initializing I2C master mode failed (err %d).", ret);
		return ret;
	}

	ret = ak09916_reset(dev);
	if (ret < 0) {
		LOG_ERR("Resetting AK09916 failed (err %d).", ret);
		return ret;
	}

	/* First check that the chip says hello */
	ret = ak09916_read_reg(dev, AK09916_REG_WIA, &buf);
	if (ret < 0) {
		LOG_ERR("Failed to read AK09916 chip id (err %d).", ret);
		return ret;
	}

	if (buf != AK09916_WIA_VAL) {
		LOG_ERR("Invalid AK09916 chip id (0x%02X, expected 0x%02X).", buf, AK09916_WIA_VAL);
		return -ENOTSUP;
	}

	LOG_INF("AK09916 magnetometer detected (chip ID: 0x%02X)", buf);

	/* Set fixed scale values (AK09916 has no calibration registers) */
	ak09916_set_scale(dev);

	/* Set AK09916 to continuous measurement mode at 100Hz */
	ret = ak09916_set_mode(dev, cfg->mag_mode);
	if (ret < 0) {
		LOG_ERR("Failed to set AK09916 measurement mode (err %d).", ret);
		return ret;
	}

	/* Configure automatic readout via SLV0 */
	ret = ak09916_init_readout(dev);
	if (ret < 0) {
		LOG_ERR("Initializing AK09916 readout failed (err %d).", ret);
		return ret;
	}

	return 0;
}
