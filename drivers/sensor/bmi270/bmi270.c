/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi270

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

struct bmi270_config {
	union bmi270_bus bus;
	const struct bmi270_bus_io *bus_io;
};

static inline int bmi270_bus_check(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmi270_bus_init(const struct device *dev)
{
	const struct bmi270_config *cfg = dev->config;

	return cfg->bus_io->init(&cfg->bus);
}

static int bmi270_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t length)
{
	const struct bmi270_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, data, length);
}

static int bmi270_reg_write(const struct device *dev, uint8_t reg,
			    const uint8_t *data, uint16_t length)
{
	const struct bmi270_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, data, length);
}

static int bmi270_reg_write_with_delay(const struct device *dev,
				       uint8_t reg,
				       const uint8_t *data,
				       uint16_t length,
				       uint32_t delay_us)
{
	int ret = 0;

	ret = bmi270_reg_write(dev, reg, data, length);
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

static int set_accel_odr_osr(const struct device *dev, const struct sensor_value *odr,
			     const struct sensor_value *osr)
{
	struct bmi270_data *data = dev->data;
	uint8_t acc_conf, odr_bits, pwr_ctrl, osr_bits;
	int ret = 0;

	if (odr || osr) {
		ret = bmi270_reg_read(dev, BMI270_REG_ACC_CONF, &acc_conf, 1);
		if (ret != 0) {
			return ret;
		}

		ret = bmi270_reg_read(dev, BMI270_REG_PWR_CTRL, &pwr_ctrl, 1);
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

		data->acc_odr = odr_bits;
	}

	if (osr) {
		if (data->acc_odr >= BMI270_ACC_ODR_100_HZ) {
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
		ret = bmi270_reg_write(dev, BMI270_REG_ACC_CONF, &acc_conf, 1);
		if (ret != 0) {
			return ret;
		}

		/* Assuming we have advance power save enabled */
		k_usleep(BMI270_TRANSC_DELAY_SUSPEND);

		pwr_ctrl &= BMI270_PWR_CTRL_MSK;
		ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CTRL,
						  &pwr_ctrl, 1,
						  BMI270_INTER_WRITE_DELAY_US);
	}

	return ret;
}

static int set_accel_range(const struct device *dev, const struct sensor_value *range)
{
	struct bmi270_data *data = dev->data;
	int ret = 0;
	uint8_t acc_range, reg;

	ret = bmi270_reg_read(dev, BMI270_REG_ACC_RANGE, &acc_range, 1);
	if (ret != 0) {
		return ret;
	}

	/* range->val2 is unused */
	switch (range->val1) {
	case 2:
		reg = BMI270_ACC_RANGE_2G;
		data->acc_range = 2;
		break;
	case 4:
		reg = BMI270_ACC_RANGE_4G;
		data->acc_range = 4;
		break;
	case 8:
		reg = BMI270_ACC_RANGE_8G;
		data->acc_range = 8;
		break;
	case 16:
		reg = BMI270_ACC_RANGE_16G;
		data->acc_range = 16;
		break;
	default:
		return -ENOTSUP;
	}

	acc_range = BMI270_SET_BITS_POS_0(acc_range, BMI270_ACC_RANGE,
					  reg);
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_ACC_RANGE, &acc_range,
					  1, BMI270_INTER_WRITE_DELAY_US);

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

static int set_gyro_odr_osr(const struct device *dev, const struct sensor_value *odr,
			    const struct sensor_value *osr)
{
	struct bmi270_data *data = dev->data;
	uint8_t gyr_conf, odr_bits, pwr_ctrl, osr_bits;
	int ret = 0;

	if (odr || osr) {
		ret = bmi270_reg_read(dev, BMI270_REG_GYR_CONF, &gyr_conf, 1);
		if (ret != 0) {
			return ret;
		}

		ret = bmi270_reg_read(dev, BMI270_REG_PWR_CTRL, &pwr_ctrl, 1);
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

		data->gyr_odr = odr_bits;
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
		ret = bmi270_reg_write(dev, BMI270_REG_GYR_CONF, &gyr_conf, 1);
		if (ret != 0) {
			return ret;
		}

		/* Assuming we have advance power save enabled */
		k_usleep(BMI270_TRANSC_DELAY_SUSPEND);

		pwr_ctrl &= BMI270_PWR_CTRL_MSK;
		ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CTRL,
						  &pwr_ctrl, 1,
						  BMI270_INTER_WRITE_DELAY_US);
	}

	return ret;
}

static int set_gyro_range(const struct device *dev, const struct sensor_value *range)
{
	struct bmi270_data *data = dev->data;
	int ret = 0;
	uint8_t gyr_range, reg;

	ret = bmi270_reg_read(dev, BMI270_REG_GYR_RANGE, &gyr_range, 1);
	if (ret != 0) {
		return ret;
	}

	/* range->val2 is unused */
	switch (range->val1) {
	case 125:
		reg = BMI270_GYR_RANGE_125DPS;
		data->gyr_range = 125;
		break;
	case 250:
		reg = BMI270_GYR_RANGE_250DPS;
		data->gyr_range = 250;
		break;
	case 500:
		reg = BMI270_GYR_RANGE_500DPS;
		data->gyr_range = 500;
		break;
	case 1000:
		reg = BMI270_GYR_RANGE_1000DPS;
		data->gyr_range = 1000;
		break;
	case 2000:
		reg = BMI270_GYR_RANGE_2000DPS;
		data->gyr_range = 2000;
		break;
	default:
		return -ENOTSUP;
	}

	gyr_range = BMI270_SET_BITS_POS_0(gyr_range, BMI270_GYR_RANGE, reg);
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_GYR_RANGE, &gyr_range,
					  1, BMI270_INTER_WRITE_DELAY_US);

	return ret;
}

static int8_t write_config_file(const struct device *dev)
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

		ret = bmi270_reg_write_with_delay(dev, BMI270_REG_INIT_ADDR_0,
						  addr_array, 2,
						  BMI270_INTER_WRITE_DELAY_US);

		if (ret == 0) {
			ret = bmi270_reg_write_with_delay(dev,
						BMI270_REG_INIT_DATA,
						(bmi270_config_file + index),
						BMI270_WR_LEN,
						BMI270_INTER_WRITE_DELAY_US);
		}
	}

	return ret;
}

static int bmi270_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmi270_data *data = dev->data;
	uint8_t buf[12];
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	ret = bmi270_reg_read(dev, BMI270_REG_ACC_X_LSB, buf, 12);
	if (ret == 0) {
		data->ax = (int16_t)sys_get_le16(&buf[0]);
		data->ay = (int16_t)sys_get_le16(&buf[2]);
		data->az = (int16_t)sys_get_le16(&buf[4]);
		data->gx = (int16_t)sys_get_le16(&buf[6]);
		data->gy = (int16_t)sys_get_le16(&buf[8]);
		data->gz = (int16_t)sys_get_le16(&buf[10]);
	} else {
		data->ax = 0;
		data->ay = 0;
		data->az = 0;
		data->gx = 0;
		data->gy = 0;
		data->gz = 0;
	}

	return ret;
}

static int bmi270_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmi270_data *data = dev->data;

	if (chan == SENSOR_CHAN_ACCEL_X) {
		channel_accel_convert(val, data->ax, data->acc_range);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		channel_accel_convert(val, data->ay, data->acc_range);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		channel_accel_convert(val, data->az, data->acc_range);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		channel_accel_convert(&val[0], data->ax,
				      data->acc_range);
		channel_accel_convert(&val[1], data->ay,
				      data->acc_range);
		channel_accel_convert(&val[2], data->az,
				      data->acc_range);
	} else if (chan == SENSOR_CHAN_GYRO_X) {
		channel_gyro_convert(val, data->gx, data->gyr_range);
	} else if (chan == SENSOR_CHAN_GYRO_Y) {
		channel_gyro_convert(val, data->gy, data->gyr_range);
	} else if (chan == SENSOR_CHAN_GYRO_Z) {
		channel_gyro_convert(val, data->gz, data->gyr_range);
	} else if (chan == SENSOR_CHAN_GYRO_XYZ) {
		channel_gyro_convert(&val[0], data->gx,
				     data->gyr_range);
		channel_gyro_convert(&val[1], data->gy,
				     data->gyr_range);
		channel_gyro_convert(&val[2], data->gz,
				     data->gyr_range);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int bmi270_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	int ret = -ENOTSUP;

	if ((chan == SENSOR_CHAN_ACCEL_X) || (chan == SENSOR_CHAN_ACCEL_Y)
	    || (chan == SENSOR_CHAN_ACCEL_Z)
	    || (chan == SENSOR_CHAN_ACCEL_XYZ)) {
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = set_accel_odr_osr(dev, val, NULL);
			break;
		case SENSOR_ATTR_OVERSAMPLING:
			ret = set_accel_odr_osr(dev, NULL, val);
			break;
		case SENSOR_ATTR_FULL_SCALE:
			ret = set_accel_range(dev, val);
			break;
		default:
			ret = -ENOTSUP;
		}
	} else if ((chan == SENSOR_CHAN_GYRO_X) || (chan == SENSOR_CHAN_GYRO_Y)
		   || (chan == SENSOR_CHAN_GYRO_Z)
		   || (chan == SENSOR_CHAN_GYRO_XYZ)) {
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			ret = set_gyro_odr_osr(dev, val, NULL);
			break;
		case SENSOR_ATTR_OVERSAMPLING:
			ret = set_gyro_odr_osr(dev, NULL, val);
			break;
		case SENSOR_ATTR_FULL_SCALE:
			ret = set_gyro_range(dev, val);
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
	struct bmi270_data *data = dev->data;
	uint8_t chip_id;
	uint8_t soft_reset_cmd;
	uint8_t init_ctrl;
	uint8_t msg;
	uint8_t tries;
	uint8_t adv_pwr_save;

	ret = bmi270_bus_check(dev);
	if (ret < 0) {
		LOG_ERR("Could not initialize bus");
		return ret;
	}

	data->acc_odr = BMI270_ACC_ODR_100_HZ;
	data->acc_range = 8;
	data->gyr_odr = BMI270_GYR_ODR_200_HZ;
	data->gyr_range = 2000;

	k_usleep(BMI270_POWER_ON_TIME);

	ret = bmi270_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Could not initiate bus communication");
		return ret;
	}

	ret = bmi270_reg_read(dev, BMI270_REG_CHIP_ID, &chip_id, 1);
	if (ret != 0) {
		return ret;
	}

	if (chip_id != BMI270_CHIP_ID) {
		LOG_ERR("Unexpected chip id (%x). Expected (%x)",
			chip_id, BMI270_CHIP_ID);
		return -EIO;
	}

	soft_reset_cmd = BMI270_CMD_SOFT_RESET;
	ret = bmi270_reg_write(dev, BMI270_REG_CMD, &soft_reset_cmd, 1);
	if (ret != 0) {
		return ret;
	}

	k_usleep(BMI270_SOFT_RESET_TIME);

	ret = bmi270_reg_read(dev, BMI270_REG_PWR_CONF, &adv_pwr_save, 1);
	if (ret != 0) {
		return ret;
	}

	adv_pwr_save = BMI270_SET_BITS_POS_0(adv_pwr_save,
					     BMI270_PWR_CONF_ADV_PWR_SAVE,
					     BMI270_PWR_CONF_ADV_PWR_SAVE_DIS);
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CONF,
					  &adv_pwr_save, 1,
					  BMI270_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		return ret;
	}

	init_ctrl = BMI270_PREPARE_CONFIG_LOAD;
	ret = bmi270_reg_write(dev, BMI270_REG_INIT_CTRL, &init_ctrl, 1);
	if (ret != 0) {
		return ret;
	}

	ret = write_config_file(dev);

	if (ret != 0) {
		return ret;
	}

	init_ctrl = BMI270_COMPLETE_CONFIG_LOAD;
	ret = bmi270_reg_write(dev, BMI270_REG_INIT_CTRL, &init_ctrl, 1);
	if (ret != 0) {
		return ret;
	}

	/* Timeout after BMI270_CONFIG_FILE_RETRIES x
	 * BMI270_CONFIG_FILE_POLL_PERIOD_US microseconds.
	 * If tries is BMI270_CONFIG_FILE_RETRIES by the end of the loop,
	 * report an error
	 */
	for (tries = 0; tries <= BMI270_CONFIG_FILE_RETRIES; tries++) {
		ret = bmi270_reg_read(dev, BMI270_REG_INTERNAL_STATUS, &msg, 1);
		if (ret != 0) {
			return ret;
		}

		msg &= BMI270_INST_MESSAGE_MSK;
		if (msg == BMI270_INST_MESSAGE_INIT_OK) {
			break;
		}

		k_usleep(BMI270_CONFIG_FILE_POLL_PERIOD_US);
	}

	if (tries == BMI270_CONFIG_FILE_RETRIES) {
		return -EIO;
	}

	adv_pwr_save = BMI270_SET_BITS_POS_0(adv_pwr_save,
					     BMI270_PWR_CONF_ADV_PWR_SAVE,
					     BMI270_PWR_CONF_ADV_PWR_SAVE_EN);
	ret = bmi270_reg_write_with_delay(dev, BMI270_REG_PWR_CONF,
					  &adv_pwr_save, 1,
					  BMI270_INTER_WRITE_DELAY_US);

	return ret;
}

static const struct sensor_driver_api bmi270_driver_api = {
	.sample_fetch = bmi270_sample_fetch,
	.channel_get = bmi270_channel_get,
	.attr_set = bmi270_attr_set
};

/* Initializes a struct bmi270_config for an instance on a SPI bus. */
#define BMI270_CONFIG_SPI(inst)				\
	{						\
		.bus.spi = SPI_DT_SPEC_INST_GET(	\
			inst, BMI270_SPI_OPERATION, 0),	\
		.bus_io = &bmi270_bus_io_spi,		\
	}

/* Initializes a struct bmi270_config for an instance on an I2C bus. */
#define BMI270_CONFIG_I2C(inst)			       \
	{					       \
		.bus.i2c = I2C_DT_SPEC_INST_GET(inst), \
		.bus_io = &bmi270_bus_io_i2c,	       \
	}


#define BMI270_CREATE_INST(inst)					\
									\
	static struct bmi270_data bmi270_drv_##inst;			\
									\
	static const struct bmi270_config bmi270_config_##inst =	\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
			    (BMI270_CONFIG_SPI(inst)),			\
			    (BMI270_CONFIG_I2C(inst)));			\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			      bmi270_init,				\
			      NULL,					\
			      &bmi270_drv_##inst,			\
			      &bmi270_config_##inst,			\
			      POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY,		\
			      &bmi270_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMI270_CREATE_INST)
