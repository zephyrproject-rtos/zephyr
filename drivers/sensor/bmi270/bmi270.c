/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi270

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "bmi270.h"
#include "bmi270_config_file.h"

LOG_MODULE_REGISTER(bmi270, CONFIG_SENSOR_LOG_LEVEL);

#define BMI270_WR_LEN                           256
#define BMI270_CONFIG_FILE_RETRIES              15
#define BMI270_CONFIG_FILE_POLL_PERIOD_US       10000
#define BMI270_INTER_WRITE_DELAY_US             1000

static int reg_read(uint8_t reg, uint8_t *data, uint16_t length,
		    struct bmi270_data *dev)
{
	return i2c_burst_read(dev->i2c, dev->i2c_addr, reg, data, length);
}

static int reg_write(uint8_t reg, const uint8_t *data, uint16_t length,
		     struct bmi270_data *dev)
{
	return i2c_burst_write(dev->i2c, dev->i2c_addr, reg, data, length);
}

static int reg_write_with_delay(uint8_t reg, const uint8_t *data, uint16_t length,
		     struct bmi270_data *dev, uint32_t delay_us)
{
	int ret = 0;

	ret = reg_write(reg, data, length, dev);
	if (ret == 0) {
		k_usleep(delay_us);
	}
	return ret;
}

static void channel_accel_convert(struct sensor_value *val, int64_t raw_val,
				  uint8_t range)
{
	/* 16 bit accelerometer. 2^15 bits represent the range in G */
	/* Converting from G to m/s^2 */
	raw_val = (raw_val * SENSOR_G * (int64_t) range) / INT16_MAX;

	val->val1 = raw_val / 1000000LL;
	val->val2 = raw_val % 1000000LL;

	/* Normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1LL;
		val->val2 += 1000000LL;
	}
}

static void channel_gyro_convert(struct sensor_value *val, int64_t raw_val,
				 uint16_t range)
{
	/* 16 bit gyroscope. 2^15 bits represent the range in degrees/s */
	/* Converting from degrees/s to radians/s */

	val->val1 = ((raw_val * (int64_t) range * SENSOR_PI)
		     / (180LL * INT16_MAX)) / 1000000LL;
	val->val2 = ((raw_val * (int64_t) range * SENSOR_PI)
		     / (180LL * INT16_MAX)) % 1000000LL;

	/* Normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1LL;
		val->val2 += 1000000LL;
	}
}

static uint8_t acc_odr_to_reg(const struct sensor_value *val)
{
	double odr = sensor_value_to_double((struct sensor_value *) val);
	uint8_t reg = 0;

	if ((odr >= 0.78125) && (odr < 1.5625)) {
		reg = BMI270_ACC_ODR_25D32_HZ;
	} else if ((odr >= 1.5625) && (odr < 3.125)) {
		reg = BMI270_ACC_ODR_25D16_HZ;
	} else if ((odr >= 3.125) && (odr < 6.25)) {
		reg = BMI270_ACC_ODR_25D8_HZ;
	} else if ((odr >= 6.25) && (odr < 12.5)) {
		reg = BMI270_ACC_ODR_25D4_HZ;
	} else if ((odr >= 12.5) && (odr < 25.0)) {
		reg = BMI270_ACC_ODR_25D2_HZ;
	} else if ((odr >= 25.0) && (odr < 50.0)) {
		reg = BMI270_ACC_ODR_25_HZ;
	} else if ((odr >= 50.0) && (odr < 100.0)) {
		reg = BMI270_ACC_ODR_50_HZ;
	} else if ((odr >= 100.0) && (odr < 200.0)) {
		reg = BMI270_ACC_ODR_100_HZ;
	} else if ((odr >= 200.0) && (odr < 400.0)) {
		reg = BMI270_ACC_ODR_200_HZ;
	} else if ((odr >= 400.0) && (odr < 800.0)) {
		reg = BMI270_ACC_ODR_400_HZ;
	} else if ((odr >= 800.0) && (odr < 1600.0)) {
		reg = BMI270_ACC_ODR_800_HZ;
	} else if (odr >= 1600.0) {
		reg = BMI270_ACC_ODR_1600_HZ;
	}
	return reg;
}

static int set_accel_odr_osr(const struct sensor_value *odr,
			     const struct sensor_value *osr, struct bmi270_data *dev)
{
	uint8_t acc_conf, odr_bits, pwr_ctrl, osr_bits;
	int ret = 0;

	if (odr || osr) {
		ret = reg_read(BMI270_REG_ACC_CONF, &acc_conf, 1, dev);
		if (ret != 0) {
			return ret;
		}

		ret = reg_read(BMI270_REG_PWR_CTRL, &pwr_ctrl, 1, dev);
		if (ret != 0) {
			return ret;
		}
	}

	if (odr) {
		odr_bits = acc_odr_to_reg(odr);
		acc_conf = BMI270_SET_BITS_POS_0(acc_conf, BMI270_ACC_ODR,
						 odr_bits);

		/* If odr_bits is 0, implies that the sampling frequency is 0Hz
		 * or invalid too.
		 */
		if (odr_bits) {
			pwr_ctrl |= BMI270_PWR_CTRL_ACC_EN;
		} else {
			pwr_ctrl &= ~BMI270_PWR_CTRL_ACC_EN;
		}

		/* If the Sampling frequency (odr) >= 100Hz, enter performance
		 * mode else, power optimized. This also has a consequence
		 * for the OSR
		 */
		if (odr_bits >= BMI270_ACC_ODR_100_HZ) {
			acc_conf = BMI270_SET_BITS(acc_conf, BMI270_ACC_FILT,
						   BMI270_ACC_FILT_PERF_OPT);
		} else {
			acc_conf = BMI270_SET_BITS(acc_conf, BMI270_ACC_FILT,
						   BMI270_ACC_FILT_PWR_OPT);
		}

		dev->acc_odr = odr_bits;
	}

	if (osr) {
		if (dev->acc_odr >= BMI270_ACC_ODR_100_HZ) {
			/* Performance mode */
			/* osr->val2 should be unused */
			switch (osr->val1) {
			case 4:
				osr_bits = BMI270_ACC_BWP_OSR4_AVG1;
				break;
			case 2:
				osr_bits = BMI270_ACC_BWP_OSR2_AVG2;
				break;
			case 1:
				osr_bits = BMI270_ACC_BWP_NORM_AVG4;
				break;
			default:
				osr_bits = BMI270_ACC_BWP_CIC_AVG8;
				break;
			}
		} else {
			/* Power optimized mode */
			/* osr->val2 should be unused */
			switch (osr->val1) {
			case 1:
				osr_bits = BMI270_ACC_BWP_OSR4_AVG1;
				break;
			case 2:
				osr_bits = BMI270_ACC_BWP_OSR2_AVG2;
				break;
			case 4:
				osr_bits = BMI270_ACC_BWP_NORM_AVG4;
				break;
			case 8:
				osr_bits = BMI270_ACC_BWP_CIC_AVG8;
				break;
			case 16:
				osr_bits = BMI270_ACC_BWP_RES_AVG16;
				break;
			case 32:
				osr_bits = BMI270_ACC_BWP_RES_AVG32;
				break;
			case 64:
				osr_bits = BMI270_ACC_BWP_RES_AVG64;
				break;
			case 128:
				osr_bits = BMI270_ACC_BWP_RES_AVG128;
				break;
			default:
				return -ENOTSUP;
			}
		}

		acc_conf = BMI270_SET_BITS(acc_conf, BMI270_ACC_BWP,
					   osr_bits);
	}

	if (odr || osr) {
		ret = reg_write(BMI270_REG_ACC_CONF, &acc_conf, 1, dev);
		if (ret != 0) {
			return ret;
		}

		/* Assuming we have advance power save enabled */
		k_usleep(BMI270_TRANSC_DELAY_SUSPEND);

		pwr_ctrl &= BMI270_PWR_CTRL_MSK;
		ret = reg_write_with_delay(BMI270_REG_PWR_CTRL, &pwr_ctrl, 1, dev,
			BMI270_INTER_WRITE_DELAY_US);
	}

	return ret;
}

static int set_accel_range(const struct sensor_value *range,
			   struct bmi270_data *dev)
{
	int ret = 0;
	uint8_t acc_range, reg;

	ret = reg_read(BMI270_REG_ACC_RANGE, &acc_range, 1, dev);
	if (ret != 0) {
		return ret;
	}

	/* range->val2 is unused */
	switch (range->val1) {
	case 2:
		reg = BMI270_ACC_RANGE_2G;
		dev->acc_range = 2;
		break;
	case 4:
		reg = BMI270_ACC_RANGE_4G;
		dev->acc_range = 4;
		break;
	case 8:
		reg = BMI270_ACC_RANGE_8G;
		dev->acc_range = 8;
		break;
	case 16:
		reg = BMI270_ACC_RANGE_16G;
		dev->acc_range = 16;
		break;
	default:
		return -ENOTSUP;
	}

	acc_range = BMI270_SET_BITS_POS_0(acc_range, BMI270_ACC_RANGE,
					  reg);
	ret = reg_write_with_delay(BMI270_REG_ACC_RANGE, &acc_range, 1, dev,
		BMI270_INTER_WRITE_DELAY_US);

	return ret;
}

static uint8_t gyr_odr_to_reg(const struct sensor_value *val)
{
	double odr = sensor_value_to_double((struct sensor_value *) val);
	uint8_t reg = 0;

	if ((odr >= 25.0) && (odr < 50.0)) {
		reg = BMI270_GYR_ODR_25_HZ;
	} else if ((odr >= 50.0) && (odr < 100.0)) {
		reg = BMI270_GYR_ODR_50_HZ;
	} else if ((odr >= 100.0) && (odr < 200.0)) {
		reg = BMI270_GYR_ODR_100_HZ;
	} else if ((odr >= 200.0) && (odr < 400.0)) {
		reg = BMI270_GYR_ODR_200_HZ;
	} else if ((odr >= 400.0) && (odr < 800.0)) {
		reg = BMI270_GYR_ODR_400_HZ;
	} else if ((odr >= 800.0) && (odr < 1600.0)) {
		reg = BMI270_GYR_ODR_800_HZ;
	} else if ((odr >= 1600.0) && (odr < 3200.0)) {
		reg = BMI270_GYR_ODR_1600_HZ;
	} else if (odr >= 3200.0) {
		reg = BMI270_GYR_ODR_3200_HZ;
	}

	return reg;
}

static int set_gyro_odr_osr(const struct sensor_value *odr,
			    const struct sensor_value *osr, struct bmi270_data *dev)
{
	uint8_t gyr_conf, odr_bits, pwr_ctrl, osr_bits;
	int ret = 0;

	if (odr || osr) {
		ret = reg_read(BMI270_REG_GYR_CONF, &gyr_conf, 1, dev);
		if (ret != 0) {
			return ret;
		}

		ret = reg_read(BMI270_REG_PWR_CTRL, &pwr_ctrl, 1, dev);
		if (ret != 0) {
			return ret;
		}
	}

	if (odr) {
		odr_bits = gyr_odr_to_reg(odr);
		gyr_conf = BMI270_SET_BITS_POS_0(gyr_conf, BMI270_GYR_ODR,
						 odr_bits);

		/* If odr_bits is 0, implies that the sampling frequency is
		 * 0Hz or invalid too.
		 */
		if (odr_bits) {
			pwr_ctrl |= BMI270_PWR_CTRL_GYR_EN;
		} else {
			pwr_ctrl &= ~BMI270_PWR_CTRL_GYR_EN;
		}

		/* If the Sampling frequency (odr) >= 100Hz, enter performance
		 * mode else, power optimized. This also has a consequence for
		 * the OSR
		 */
		if (odr_bits >= BMI270_GYR_ODR_100_HZ) {
			gyr_conf = BMI270_SET_BITS(gyr_conf,
						   BMI270_GYR_FILT,
						   BMI270_GYR_FILT_PERF_OPT);
			gyr_conf = BMI270_SET_BITS(gyr_conf,
						   BMI270_GYR_FILT_NOISE,
						   BMI270_GYR_FILT_NOISE_PERF);
		} else {
			gyr_conf = BMI270_SET_BITS(gyr_conf,
						   BMI270_GYR_FILT,
						   BMI270_GYR_FILT_PWR_OPT);
			gyr_conf = BMI270_SET_BITS(gyr_conf,
						   BMI270_GYR_FILT_NOISE,
						   BMI270_GYR_FILT_NOISE_PWR);
		}

		dev->gyr_odr = odr_bits;
	}

	if (osr) {
		/* osr->val2 should be unused */
		switch (osr->val1) {
		case 4:
			osr_bits = BMI270_GYR_BWP_OSR4;
			break;
		case 2:
			osr_bits = BMI270_GYR_BWP_OSR2;
			break;
		default:
			osr_bits = BMI270_GYR_BWP_NORM;
			break;
		}

		gyr_conf = BMI270_SET_BITS(gyr_conf, BMI270_GYR_BWP,
					   osr_bits);
	}

	if (odr || osr) {
		ret = reg_write(BMI270_REG_GYR_CONF, &gyr_conf, 1, dev);
		if (ret != 0) {
			return ret;
		}

		/* Assuming we have advance power save enabled */
		k_usleep(BMI270_TRANSC_DELAY_SUSPEND);

		pwr_ctrl &= BMI270_PWR_CTRL_MSK;
		ret = reg_write_with_delay(BMI270_REG_PWR_CTRL, &pwr_ctrl, 1, dev,
			BMI270_INTER_WRITE_DELAY_US);
	}

	return ret;
}

static int set_gyro_range(const struct sensor_value *range,
			  struct bmi270_data *dev)
{
	int ret = 0;
	uint8_t gyr_range, reg;

	ret = reg_read(BMI270_REG_GYR_RANGE, &gyr_range, 1, dev);
	if (ret != 0) {
		return ret;
	}

	/* range->val2 is unused */
	switch (range->val1) {
	case 125:
		reg = BMI270_GYR_RANGE_125DPS;
		dev->gyr_range = 125;
		break;
	case 250:
		reg = BMI270_GYR_RANGE_250DPS;
		dev->gyr_range = 250;
		break;
	case 500:
		reg = BMI270_GYR_RANGE_500DPS;
		dev->gyr_range = 500;
		break;
	case 1000:
		reg = BMI270_GYR_RANGE_1000DPS;
		dev->gyr_range = 1000;
		break;
	case 2000:
		reg = BMI270_GYR_RANGE_2000DPS;
		dev->gyr_range = 2000;
		break;
	default:
		return -ENOTSUP;
	}

	gyr_range = BMI270_SET_BITS_POS_0(gyr_range, BMI270_GYR_RANGE, reg);
	ret = reg_write_with_delay(BMI270_REG_GYR_RANGE, &gyr_range, 1, dev,
		BMI270_INTER_WRITE_DELAY_US);

	return ret;
}

static int8_t write_config_file(struct bmi270_data *dev)
{
	int8_t ret = 0;
	uint16_t index = 0;
	uint8_t addr_array[2] = { 0 };

	/* Disable loading of the configuration */
	for (index = 0; index < sizeof(bmi270_config_file);
	     index += BMI270_WR_LEN) {
		/* Store 0 to 3 bits of address in first byte */
		addr_array[0] = (uint8_t)((index / 2) & 0x0F);

		/* Store 4 to 11 bits of address in the second byte */
		addr_array[1] = (uint8_t)((index / 2) >> 4);

		ret = reg_write_with_delay(BMI270_REG_INIT_ADDR_0, addr_array, 2, dev,
			BMI270_INTER_WRITE_DELAY_US);

		if (ret == 0) {
			ret = reg_write_with_delay(BMI270_REG_INIT_DATA,
				(bmi270_config_file + index),
				BMI270_WR_LEN, dev, BMI270_INTER_WRITE_DELAY_US);
		}
	}

	return ret;
}

static int bmi270_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmi270_data *drv_dev = dev->data;
	uint8_t data[12];
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	ret = reg_read(BMI270_REG_ACC_X_LSB, data, 12, drv_dev);
	if (ret == 0) {
		drv_dev->ax = (int16_t)sys_get_le16(&data[0]);
		drv_dev->ay = (int16_t)sys_get_le16(&data[2]);
		drv_dev->az = (int16_t)sys_get_le16(&data[4]);
		drv_dev->gx = (int16_t)sys_get_le16(&data[6]);
		drv_dev->gy = (int16_t)sys_get_le16(&data[8]);
		drv_dev->gz = (int16_t)sys_get_le16(&data[10]);
	} else {
		drv_dev->ax = 0;
		drv_dev->ay = 0;
		drv_dev->az = 0;
		drv_dev->gx = 0;
		drv_dev->gy = 0;
		drv_dev->gz = 0;
	}

	return ret;
}

static int bmi270_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmi270_data *drv_dev = dev->data;

	if (chan == SENSOR_CHAN_ACCEL_X) {
		channel_accel_convert(val, drv_dev->ax, drv_dev->acc_range);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		channel_accel_convert(val, drv_dev->ay, drv_dev->acc_range);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		channel_accel_convert(val, drv_dev->az, drv_dev->acc_range);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		channel_accel_convert(&val[0], drv_dev->ax,
				      drv_dev->acc_range);
		channel_accel_convert(&val[1], drv_dev->ay,
				      drv_dev->acc_range);
		channel_accel_convert(&val[2], drv_dev->az,
				      drv_dev->acc_range);
	} else if (chan == SENSOR_CHAN_GYRO_X) {
		channel_gyro_convert(val, drv_dev->gx, drv_dev->gyr_range);
	} else if (chan == SENSOR_CHAN_GYRO_Y) {
		channel_gyro_convert(val, drv_dev->gy, drv_dev->gyr_range);
	} else if (chan == SENSOR_CHAN_GYRO_Z) {
		channel_gyro_convert(val, drv_dev->gz, drv_dev->gyr_range);
	} else if (chan == SENSOR_CHAN_GYRO_XYZ) {
		channel_gyro_convert(&val[0], drv_dev->gx,
				     drv_dev->gyr_range);
		channel_gyro_convert(&val[1], drv_dev->gy,
				     drv_dev->gyr_range);
		channel_gyro_convert(&val[2], drv_dev->gz,
				     drv_dev->gyr_range);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int bmi270_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	struct bmi270_data *drv_dev = dev->data;
	int ret = -ENOTSUP;

	if ((chan == SENSOR_CHAN_ACCEL_X) || (chan == SENSOR_CHAN_ACCEL_Y)
	    || (chan == SENSOR_CHAN_ACCEL_Z)
	    || (chan == SENSOR_CHAN_ACCEL_XYZ)) {
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = set_accel_odr_osr(val, NULL, drv_dev);
			break;
		case SENSOR_ATTR_OVERSAMPLING:
			ret = set_accel_odr_osr(NULL, val, drv_dev);
			break;
		case SENSOR_ATTR_FULL_SCALE:
			ret = set_accel_range(val, drv_dev);
			break;
		default:
			ret = -ENOTSUP;
		}
	} else if ((chan == SENSOR_CHAN_GYRO_X) || (chan == SENSOR_CHAN_GYRO_Y)
		   || (chan == SENSOR_CHAN_GYRO_Z)
		   || (chan == SENSOR_CHAN_GYRO_XYZ)) {
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = set_gyro_odr_osr(val, NULL, drv_dev);
			break;
		case SENSOR_ATTR_OVERSAMPLING:
			ret = set_gyro_odr_osr(NULL, val, drv_dev);
			break;
		case SENSOR_ATTR_FULL_SCALE:
			ret = set_gyro_range(val, drv_dev);
			break;
		default:
			ret = -ENOTSUP;
		}

	} else {
		ret = -ENOTSUP;
	}

	return ret;
}

static int bmi270_init(const struct device *dev)
{
	int ret;
	struct bmi270_data *drv_dev = dev->data;
	const struct bmi270_dev_config *cfg = dev->config;
	uint8_t chip_id;
	uint8_t soft_reset_cmd;
	uint8_t init_ctrl;
	uint8_t msg;
	uint8_t tries;
	uint8_t adv_pwr_save;

	drv_dev->i2c = device_get_binding(cfg->i2c_master_name);
	if (drv_dev->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device",
			cfg->i2c_master_name);
		return -EINVAL;
	}

	drv_dev->i2c_addr = cfg->i2c_addr;
	drv_dev->acc_odr = BMI270_ACC_ODR_100_HZ;
	drv_dev->acc_range = 8;
	drv_dev->gyr_odr = BMI270_GYR_ODR_200_HZ;
	drv_dev->gyr_range = 2000;

	k_usleep(BMI270_POWER_ON_TIME);

	ret = reg_read(BMI270_REG_CHIP_ID, &chip_id, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	if (chip_id != BMI270_CHIP_ID) {
		LOG_ERR("Unexpected chip id (%x). Expected (%x)",
			chip_id, BMI270_CHIP_ID);
		return -EIO;
	}

	soft_reset_cmd = BMI270_CMD_SOFT_RESET;
	ret = reg_write(BMI270_REG_CMD, &soft_reset_cmd, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	k_usleep(BMI270_SOFT_RESET_TIME);

	ret = reg_read(BMI270_REG_PWR_CONF, &adv_pwr_save, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	adv_pwr_save = BMI270_SET_BITS_POS_0(adv_pwr_save,
					     BMI270_PWR_CONF_ADV_PWR_SAVE,
					     BMI270_PWR_CONF_ADV_PWR_SAVE_DIS);
	ret = reg_write(BMI270_REG_PWR_CONF, &adv_pwr_save, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	init_ctrl = BMI270_PREPARE_CONFIG_LOAD;
	ret = reg_write(BMI270_REG_INIT_CTRL, &init_ctrl, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	ret = write_config_file(drv_dev);

	if (ret != 0) {
		return ret;
	}

	init_ctrl = BMI270_COMPLETE_CONFIG_LOAD;
	ret = reg_write(BMI270_REG_INIT_CTRL, &init_ctrl, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	/* Timeout after BMI270_CONFIG_FILE_RETRIES x
	 * BMI270_CONFIG_FILE_POLL_PERIOD_US microseconds.
	 * If tries is 0 by the end of the loop,
	 * report an error
	 */
	for (tries = 0; tries <= BMI270_CONFIG_FILE_RETRIES; tries++) {
		ret = reg_read(BMI270_REG_INTERNAL_STATUS, &msg, 1, drv_dev);
		if (ret != 0) {
			return ret;
		}

		msg &= BMI270_INST_MESSAGE_MSK;
		if (msg == BMI270_INST_MESSAGE_INIT_OK) {
			break;
		}

		k_usleep(BMI270_CONFIG_FILE_POLL_PERIOD_US);
	}

	if (tries == 0) {
		return -EIO;
	}

	adv_pwr_save = BMI270_SET_BITS_POS_0(adv_pwr_save,
					     BMI270_PWR_CONF_ADV_PWR_SAVE,
					     BMI270_PWR_CONF_ADV_PWR_SAVE_EN);
	ret = reg_write_with_delay(BMI270_REG_PWR_CONF, &adv_pwr_save, 1, drv_dev,
		BMI270_INTER_WRITE_DELAY_US);

	return ret;
}

static const struct sensor_driver_api bmi270_driver_api = {
	.sample_fetch = bmi270_sample_fetch,
	.channel_get = bmi270_channel_get,
	.attr_set = bmi270_attr_set
};

#define BMI270_CREATE_INST(inst)				       \
								       \
	static struct bmi270_data bmi270_drv_##inst;		       \
								       \
	static const struct bmi270_dev_config bmi270_config_##inst = { \
		.i2c_master_name = DT_INST_BUS_LABEL(inst),	       \
		.i2c_addr = DT_INST_REG_ADDR(inst),		       \
	};							       \
								       \
	DEVICE_DT_INST_DEFINE(inst,				       \
			      bmi270_init,			       \
			      NULL,				       \
			      &bmi270_drv_##inst,		       \
			      &bmi270_config_##inst,		       \
			      POST_KERNEL,			       \
			      CONFIG_SENSOR_INIT_PRIORITY,	       \
			      &bmi270_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMI270_CREATE_INST)
