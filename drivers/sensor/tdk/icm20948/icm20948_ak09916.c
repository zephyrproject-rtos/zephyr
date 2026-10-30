/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "icm20948.h"
#include "icm20948_ak09916.h"
#include "icm20948_reg.h"

LOG_MODULE_DECLARE(ICM20948, CONFIG_SENSOR_LOG_LEVEL);

/* Address the magnetometer answers to on the auxiliary I2C bus */
#define AK09916_I2C_ADDR 0x0C

#define AK09916_REG_WIA2  0x01
#define AK09916_REG_ST1   0x10
#define AK09916_REG_CNTL2 0x31
#define AK09916_REG_CNTL3 0x32

/* WIA2 */
#define AK09916_DEVICE_ID 0x09

/* ST2 */
#define BIT_AK09916_HOFL BIT(3)

/* CNTL2, only a continuous mode is of use to a periodic readout */
#define AK09916_MODE_CONT_100HZ 0x08

/* CNTL3 */
#define BIT_AK09916_SRST BIT(0)

/*
 * Single register transfers only happen while the magnetometer is brought
 * up, so waiting for them with a generous bound costs nothing at runtime.
 */
#define AK09916_XFER_RETRIES 100
#define AK09916_XFER_POLL_US 100

#define AK09916_RESET_DELAY_MS 1

/*
 * A firmware restart does not power cycle the part, so the magnetometer can
 * still be holding the auxiliary bus from a transfer that was cut short.
 * Resetting the master frees it, but the magnetometer may need a few tries
 * to be walked out of the transfer it was left in.
 */
#define AK09916_ID_ATTEMPTS 5

/* 0.15 uT per LSB, expressed in the micro Gauss the sensor API asks for */
#define AK09916_SCALE_TO_UG 1500

/*
 * Runs one single byte transfer on the auxiliary bus through slave 4, which
 * is the only slave that reports completion instead of feeding the periodic
 * readout. Callers place the value to write in I2C_SLV4_DO beforehand and
 * pick a read result up from I2C_SLV4_DI afterwards.
 */
static int ak09916_xfer(const struct device *dev, uint8_t reg, bool read)
{
	uint8_t status;
	int ret;

	/*
	 * The status register clears itself as it is read. Empty it first, so
	 * that a completion left behind by the previous transfer cannot be
	 * mistaken for this one finishing before it has even started.
	 */
	ret = icm20948_read(dev, REG_I2C_MST_STATUS, &status, sizeof(status));
	if (ret < 0) {
		return ret;
	}

	ret = icm20948_write(dev, REG_I2C_SLV4_ADDR,
			     AK09916_I2C_ADDR | (read ? BIT_I2C_SLV_RNW : 0));
	if (ret < 0) {
		return ret;
	}

	ret = icm20948_write(dev, REG_I2C_SLV4_REG, reg);
	if (ret < 0) {
		return ret;
	}

	/* Writing the control register is what starts the transfer */
	ret = icm20948_write(dev, REG_I2C_SLV4_CTRL, BIT_I2C_SLV_EN);
	if (ret < 0) {
		return ret;
	}

	for (int i = 0; i < AK09916_XFER_RETRIES; i++) {
		k_usleep(AK09916_XFER_POLL_US);

		/* One read has to catch both the completion and the failure */
		ret = icm20948_read(dev, REG_I2C_MST_STATUS, &status, sizeof(status));
		if (ret < 0) {
			return ret;
		}

		if (status & BIT_I2C_SLV4_DONE) {
			if (status & BIT_I2C_SLV4_NACK) {
				LOG_ERR("register 0x%02x was not acknowledged", reg);
				return -EIO;
			}

			return 0;
		}
	}

	LOG_ERR("auxiliary transfer to register 0x%02x timed out", reg);

	/* Leaving the transfer armed would have it retried on every cycle */
	(void)icm20948_write(dev, REG_I2C_SLV4_CTRL, 0);

	return -ETIMEDOUT;
}

static int ak09916_read_reg(const struct device *dev, uint8_t reg, uint8_t *val)
{
	int ret;

	ret = ak09916_xfer(dev, reg, true);
	if (ret < 0) {
		return ret;
	}

	return icm20948_read(dev, REG_I2C_SLV4_DI, val, sizeof(*val));
}

static int ak09916_write_reg(const struct device *dev, uint8_t reg, uint8_t val)
{
	int ret;

	ret = icm20948_write(dev, REG_I2C_SLV4_DO, val);
	if (ret < 0) {
		return ret;
	}

	return ak09916_xfer(dev, reg, false);
}

/*
 * Resets the state machine of the auxiliary master, which is what frees a
 * bus that a transfer interrupted half way through has left held low.
 */
static int ak09916_reset_master(const struct device *dev)
{
	int ret;

	/* The bit clears itself once the reset has been carried out */
	ret = icm20948_update(dev, REG_USER_CTRL, BIT_I2C_MST_RST, 1);
	if (ret < 0) {
		return ret;
	}

	k_msleep(AK09916_RESET_DELAY_MS);

	return 0;
}

static int ak09916_init_master(const struct device *dev)
{
	int ret;

	ret = icm20948_update(dev, REG_USER_CTRL, BIT_I2C_MST_EN, 1);
	if (ret < 0) {
		return ret;
	}

	/*
	 * The master only runs a transfer on a cycle of its own clock. Leave
	 * it duty cycled, as it is out of reset, but take the divider off so
	 * that the cycle runs at the full base rate rather than at the much
	 * slower output data rate.
	 */
	ret = icm20948_update(dev, REG_LP_CONFIG, BIT_I2C_MST_CYCLE, 1);
	if (ret < 0) {
		return ret;
	}

	ret = icm20948_write(dev, REG_I2C_MST_ODR_CFG,
			     FIELD_PREP(MASK_I2C_MST_ODR, I2C_MST_ODR_MAX));
	if (ret < 0) {
		return ret;
	}

	return icm20948_write(dev, REG_I2C_MST_CTRL,
			      BIT_I2C_MST_P_NSR | FIELD_PREP(MASK_I2C_MST_CLK, I2C_MST_CLK_345K));
}

static int ak09916_init_readout(const struct device *dev)
{
	int ret;

	ret = icm20948_write(dev, REG_I2C_SLV0_ADDR, AK09916_I2C_ADDR | BIT_I2C_SLV_RNW);
	if (ret < 0) {
		return ret;
	}

	/*
	 * Starting at ST1 makes the burst end on ST2, which is what tells the
	 * magnetometer that the sample was taken and the next one may start.
	 */
	ret = icm20948_write(dev, REG_I2C_SLV0_REG, AK09916_REG_ST1);
	if (ret < 0) {
		return ret;
	}

	return icm20948_write(dev, REG_I2C_SLV0_CTRL,
			      BIT_I2C_SLV_EN | FIELD_PREP(MASK_I2C_SLV_LENG, ICM20948_MAGN_LEN));
}

int ak09916_init(const struct device *dev)
{
	uint8_t id = 0;
	int ret;

	ret = ak09916_init_master(dev);
	if (ret < 0) {
		LOG_ERR("failed to bring up the auxiliary I2C master: %d", ret);
		return ret;
	}

	for (int i = 0; i < AK09916_ID_ATTEMPTS; i++) {
		/*
		 * Neither of these is worth giving up over. A bus that is
		 * still held makes them fail, and freeing it is exactly what
		 * the reset below is there for.
		 */
		(void)ak09916_write_reg(dev, AK09916_REG_CNTL3, BIT_AK09916_SRST);
		k_msleep(AK09916_RESET_DELAY_MS);
		(void)ak09916_read_reg(dev, AK09916_REG_WIA2, &id);

		if (id == AK09916_DEVICE_ID) {
			break;
		}

		ret = ak09916_reset_master(dev);
		if (ret < 0) {
			LOG_ERR("failed to reset the auxiliary I2C master: %d", ret);
			return ret;
		}
	}

	if (id != AK09916_DEVICE_ID) {
		LOG_ERR("unexpected magnetometer device id 0x%02x", id);
		return -ENODEV;
	}

	ret = ak09916_write_reg(dev, AK09916_REG_CNTL2, AK09916_MODE_CONT_100HZ);
	if (ret < 0) {
		LOG_ERR("failed to start the magnetometer: %d", ret);
		return ret;
	}

	ret = ak09916_init_readout(dev);
	if (ret < 0) {
		LOG_ERR("failed to arm the magnetometer readout: %d", ret);
		return ret;
	}

	return 0;
}

void ak09916_parse_magn(struct icm20948_data *data, const uint8_t *ext)
{
	/*
	 * ST1 is not looked at. The magnetometer holds its output registers
	 * at the last completed measurement until they are read, and the
	 * auxiliary master reads them an order of magnitude more often than
	 * they change, so the mirror always carries the current measurement
	 * whether or not it is one that has been seen before.
	 *
	 * The magnetometer die is mounted turned half a turn about X with
	 * respect to the rest of the part, which the vendor driver carries
	 * as the compass mounting matrix diag(1, -1, -1). It is also little
	 * endian, unlike the accelerometer and the gyroscope.
	 */
	data->magn[0] = (int16_t)sys_get_le16(&ext[1]);
	data->magn[1] = -(int16_t)sys_get_le16(&ext[3]);
	data->magn[2] = -(int16_t)sys_get_le16(&ext[5]);
	data->magn_st2 = ext[8];
	data->magn_valid = true;
}

int ak09916_convert_magn(struct sensor_value *val, int16_t raw, uint8_t st2)
{
	int64_t conv;

	if (st2 & BIT_AK09916_HOFL) {
		return -EOVERFLOW;
	}

	conv = (int64_t)raw * AK09916_SCALE_TO_UG;

	val->val1 = conv / 1000000;
	val->val2 = conv % 1000000;

	return 0;
}
