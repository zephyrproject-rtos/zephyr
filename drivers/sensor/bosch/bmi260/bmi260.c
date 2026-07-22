/*
 * Copyright (c) 2021 Bosch Sensortec GmbH
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmi260

#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "bmi260.h"
#include "bmi260_config_file.h"

LOG_MODULE_REGISTER(bmi260, CONFIG_SENSOR_LOG_LEVEL);

static inline int bmi260_bus_check(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->check(&cfg->bus);
}

static inline int bmi260_bus_init(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->init(&cfg->bus);
}

int bmi260_reg_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t length)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->read(&cfg->bus, reg, data, length);
}

int bmi260_reg_write(const struct device *dev, uint8_t reg,
		     const uint8_t *data, uint16_t length)
{
	const struct bmi260_config *cfg = dev->config;

	return cfg->bus_io->write(&cfg->bus, reg, data, length);
}

int bmi260_reg_write_with_delay(const struct device *dev,
				uint8_t reg,
				const uint8_t *data,
				uint16_t length,
				uint32_t delay_us)
{
	int ret = 0;

	ret = bmi260_reg_write(dev, reg, data, length);
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
}

static uint8_t acc_odr_to_reg(const struct sensor_value *val)
{
	double odr = sensor_value_to_double((const struct sensor_value *) val);
	uint8_t reg;

	if ((odr >= 0.78125) && (odr < 1.5625)) {
		reg = BMI260_ACC_ODR_25D32_HZ;
	} else if ((odr >= 1.5625) && (odr < 3.125)) {
		reg = BMI260_ACC_ODR_25D16_HZ;
	} else if ((odr >= 3.125) && (odr < 6.25)) {
		reg = BMI260_ACC_ODR_25D8_HZ;
	} else if ((odr >= 6.25) && (odr < 12.5)) {
		reg = BMI260_ACC_ODR_25D4_HZ;
	} else if ((odr >= 12.5) && (odr < 25.0)) {
		reg = BMI260_ACC_ODR_25D2_HZ;
	} else if ((odr >= 25.0) && (odr < 50.0)) {
		reg = BMI260_ACC_ODR_25_HZ;
	} else if ((odr >= 50.0) && (odr < 100.0)) {
		reg = BMI260_ACC_ODR_50_HZ;
	} else if ((odr >= 100.0) && (odr < 200.0)) {
		reg = BMI260_ACC_ODR_100_HZ;
	} else if ((odr >= 200.0) && (odr < 400.0)) {
		reg = BMI260_ACC_ODR_200_HZ;
	} else if ((odr >= 400.0) && (odr < 800.0)) {
		reg = BMI260_ACC_ODR_400_HZ;
	} else if ((odr >= 800.0) && (odr < 1600.0)) {
		reg = BMI260_ACC_ODR_800_HZ;
	} else if (odr >= 1600.0) {
		reg = BMI260_ACC_ODR_1600_HZ;
	} else {
		reg = 0;
	}
	return reg;
}

static int set_accel_odr_osr(const struct device *dev, const struct sensor_value *odr,
			     const struct sensor_value *osr)
{
	struct bmi260_data *data = dev->data;
	uint8_t acc_conf, odr_bits, pwr_ctrl, osr_bits;
	int ret;

	if (!odr && !osr){
		return ret;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_ACC_CONF, &acc_conf, 1);
	if (ret != 0) {
		return ret;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, 1);
	if (ret != 0) {
		return ret;
	}

	if (odr) {
		odr_bits = acc_odr_to_reg(odr);
		acc_conf = BMI260_SET_BITS_POS_0(acc_conf, BMI260_ACC_ODR,
						 odr_bits);

		/* If odr_bits is 0, implies that the sampling frequency is 0Hz
		 * or invalid too.
		 */
		if (odr_bits) {
			pwr_ctrl |= BMI260_PWR_CTRL_ACC_EN;
		} else {
			pwr_ctrl &= ~BMI260_PWR_CTRL_ACC_EN;
		}

		/* If the Sampling frequency (odr) >= 100Hz, enter performance
		 * mode else, power optimized. This also has a consequence
		 * for the OSR
		 */
		if (odr_bits >= BMI260_ACC_ODR_100_HZ) {
			acc_conf = BMI260_SET_BITS(acc_conf, BMI260_ACC_FILT,
						   BMI260_ACC_FILT_PERF_OPT);
		} else {
			acc_conf = BMI260_SET_BITS(acc_conf, BMI260_ACC_FILT,
						   BMI260_ACC_FILT_PWR_OPT);
		}

		data->acc_odr = odr_bits;
	}

	if (osr) {
		if (data->acc_odr >= BMI260_ACC_ODR_100_HZ) {
			/* Performance mode */
			/* osr->val2 should be unused */
			switch (osr->val1) {
			case 4:
				osr_bits = BMI260_ACC_BWP_OSR4_AVG1;
				break;
			case 2:
				osr_bits = BMI260_ACC_BWP_OSR2_AVG2;
				break;
			case 1:
				osr_bits = BMI260_ACC_BWP_NORM_AVG4;
				break;
			default:
				osr_bits = BMI260_ACC_BWP_CIC_AVG8;
				break;
			}
		} else {
			/* Power optimized mode */
			/* osr->val2 should be unused */
			switch (osr->val1) {
			case 1:
				osr_bits = BMI260_ACC_BWP_OSR4_AVG1;
				break;
			case 2:
				osr_bits = BMI260_ACC_BWP_OSR2_AVG2;
				break;
			case 4:
				osr_bits = BMI260_ACC_BWP_NORM_AVG4;
				break;
			case 8:
				osr_bits = BMI260_ACC_BWP_CIC_AVG8;
				break;
			case 16:
				osr_bits = BMI260_ACC_BWP_RES_AVG16;
				break;
			case 32:
				osr_bits = BMI260_ACC_BWP_RES_AVG32;
				break;
			case 64:
				osr_bits = BMI260_ACC_BWP_RES_AVG64;
				break;
			case 128:
				osr_bits = BMI260_ACC_BWP_RES_AVG128;
				break;
			default:
				return -ENOTSUP;
			}
		}

		acc_conf = BMI260_SET_BITS(acc_conf, BMI260_ACC_BWP,
					   osr_bits);
	}

	ret = bmi260_reg_write(dev, BMI260_REG_ACC_CONF, &acc_conf, 1);
	if (ret != 0) {
		return ret;
	}

	/* Assuming we have advance power save enabled */
	k_usleep(BMI260_TRANSC_DELAY_SUSPEND);

	pwr_ctrl &= BMI260_PWR_CTRL_MSK;
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CTRL,
						&pwr_ctrl, 1,
				BMI260_INTER_WRITE_DELAY_US);

	return ret;
}

static int set_accel_range(const struct device *dev, const struct sensor_value *range)
{
	struct bmi260_data *data = dev->data;
	int ret = 0;
	uint8_t acc_range, reg;

	ret = bmi260_reg_read(dev, BMI260_REG_ACC_RANGE, &acc_range, 1);
	if (ret != 0) {
		return ret;
	}

	/* range->val2 is unused */
	switch (range->val1) {
	case 2:
		reg = BMI260_ACC_RANGE_2G;
		data->acc_range = 2;
		break;
	case 4:
		reg = BMI260_ACC_RANGE_4G;
		data->acc_range = 4;
		break;
	case 8:
		reg = BMI260_ACC_RANGE_8G;
		data->acc_range = 8;
		break;
	case 16:
		reg = BMI260_ACC_RANGE_16G;
		data->acc_range = 16;
		break;
	default:
		return -ENOTSUP;
	}

	acc_range = BMI260_SET_BITS_POS_0(acc_range, BMI260_ACC_RANGE,
					  reg);
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_ACC_RANGE, &acc_range,
					  1, BMI260_INTER_WRITE_DELAY_US);

	return ret;
}

static uint8_t gyr_odr_to_reg(const struct sensor_value *val)
{
	double odr = sensor_value_to_double((const struct sensor_value *) val);
	uint8_t reg;

	if ((odr >= 25.0) && (odr < 50.0)) {
		reg = BMI260_GYR_ODR_25_HZ;
	} else if ((odr >= 50.0) && (odr < 100.0)) {
		reg = BMI260_GYR_ODR_50_HZ;
	} else if ((odr >= 100.0) && (odr < 200.0)) {
		reg = BMI260_GYR_ODR_100_HZ;
	} else if ((odr >= 200.0) && (odr < 400.0)) {
		reg = BMI260_GYR_ODR_200_HZ;
	} else if ((odr >= 400.0) && (odr < 800.0)) {
		reg = BMI260_GYR_ODR_400_HZ;
	} else if ((odr >= 800.0) && (odr < 1600.0)) {
		reg = BMI260_GYR_ODR_800_HZ;
	} else if ((odr >= 1600.0) && (odr < 3200.0)) {
		reg = BMI260_GYR_ODR_1600_HZ;
	} else if (odr >= 3200.0) {
		reg = BMI260_GYR_ODR_3200_HZ;
	}
	else {
		reg = 0;
	}

	return reg;
}

static int set_gyro_odr_osr(const struct device *dev, const struct sensor_value *odr,
			    const struct sensor_value *osr)
{
	struct bmi260_data *data = dev->data;
	uint8_t gyr_conf, odr_bits, pwr_ctrl, osr_bits;
	int ret = 0;

	if (odr || osr) {
		ret = bmi260_reg_read(dev, BMI260_REG_GYR_CONF, &gyr_conf, 1);
		if (ret != 0) {
			return ret;
		}

		ret = bmi260_reg_read(dev, BMI260_REG_PWR_CTRL, &pwr_ctrl, 1);
		if (ret != 0) {
			return ret;
		}
	}

	if (odr) {
		odr_bits = gyr_odr_to_reg(odr);
		gyr_conf = BMI260_SET_BITS_POS_0(gyr_conf, BMI260_GYR_ODR,
						 odr_bits);

		/* If odr_bits is 0, implies that the sampling frequency is
		 * 0Hz or invalid too.
		 */
		if (odr_bits) {
			pwr_ctrl |= BMI260_PWR_CTRL_GYR_EN;
		} else {
			pwr_ctrl &= ~BMI260_PWR_CTRL_GYR_EN;
		}

		/* If the Sampling frequency (odr) >= 100Hz, enter performance
		 * mode else, power optimized. This also has a consequence for
		 * the OSR
		 */
		if (odr_bits >= BMI260_GYR_ODR_100_HZ) {
			gyr_conf = BMI260_SET_BITS(gyr_conf,
						   BMI260_GYR_FILT,
			      BMI260_GYR_FILT_PERF_OPT);
			gyr_conf = BMI260_SET_BITS(gyr_conf,
						   BMI260_GYR_FILT_NOISE,
			      BMI260_GYR_FILT_NOISE_PERF);
		} else {
			gyr_conf = BMI260_SET_BITS(gyr_conf,
						   BMI260_GYR_FILT,
			      BMI260_GYR_FILT_PWR_OPT);
			gyr_conf = BMI260_SET_BITS(gyr_conf,
						   BMI260_GYR_FILT_NOISE,
			      BMI260_GYR_FILT_NOISE_PWR);
		}

		data->gyr_odr = odr_bits;
	}

	if (osr) {
		/* osr->val2 should be unused */
		switch (osr->val1) {
		case 4:
			osr_bits = BMI260_GYR_BWP_OSR4;
			break;
		case 2:
			osr_bits = BMI260_GYR_BWP_OSR2;
			break;
		default:
			osr_bits = BMI260_GYR_BWP_NORM;
			break;
		}

		gyr_conf = BMI260_SET_BITS(gyr_conf, BMI260_GYR_BWP,
					   osr_bits);
	}

	if (odr || osr) {
		ret = bmi260_reg_write(dev, BMI260_REG_GYR_CONF, &gyr_conf, 1);
		if (ret != 0) {
			return ret;
		}

		/* Assuming we have advance power save enabled */
		k_usleep(BMI260_TRANSC_DELAY_SUSPEND);

		pwr_ctrl &= BMI260_PWR_CTRL_MSK;
		ret = bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CTRL,
						  &pwr_ctrl, 1,
				    BMI260_INTER_WRITE_DELAY_US);
	}

	return ret;
}

static int set_gyro_range(const struct device *dev, const struct sensor_value *range)
{
	struct bmi260_data *data = dev->data;
	int ret = 0;
	uint8_t gyr_range, reg;

	ret = bmi260_reg_read(dev, BMI260_REG_GYR_RANGE, &gyr_range, 1);
	if (ret != 0) {
		return ret;
	}

	/* range->val2 is unused */
	switch (range->val1) {
	case 125:
		reg = BMI260_GYR_RANGE_125DPS;
		data->gyr_range = 125;
		break;
	case 250:
		reg = BMI260_GYR_RANGE_250DPS;
		data->gyr_range = 250;
		break;
	case 500:
		reg = BMI260_GYR_RANGE_500DPS;
		data->gyr_range = 500;
		break;
	case 1000:
		reg = BMI260_GYR_RANGE_1000DPS;
		data->gyr_range = 1000;
		break;
	case 2000:
		reg = BMI260_GYR_RANGE_2000DPS;
		data->gyr_range = 2000;
		break;
	default:
		return -ENOTSUP;
	}

	gyr_range = BMI260_SET_BITS_POS_0(gyr_range, BMI260_GYR_RANGE, reg);
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_GYR_RANGE, &gyr_range,
					  1, BMI260_INTER_WRITE_DELAY_US);

	return ret;
}

static int8_t write_config_file(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;
	int ret;
	uint8_t addr_array[2] = { 0 };

	LOG_DBG("writing config file %s", cfg->feature->name);

	/* Disable loading of the configuration */
	for (uint16_t index = 0; index < cfg->feature->config_file_len;
	     index += BMI260_WR_LEN) {
		/* Store 0 to 3 bits of address in first byte */
		addr_array[0] = (uint8_t)((index / 2) & 0x0F);

		/* Store 4 to 11 bits of address in the second byte */
		addr_array[1] = (uint8_t)((index / 2) >> 4);

		ret = bmi260_reg_write_with_delay(dev, BMI260_REG_INIT_ADDR_0,
						  addr_array, 2,
				    BMI260_INTER_WRITE_DELAY_US);

		if (ret == 0) {
			ret = bmi260_reg_write_with_delay(dev,
							  BMI260_REG_INIT_DATA,
				     &cfg->feature->config_file[index],
				     BMI260_WR_LEN,
				     BMI260_INTER_WRITE_DELAY_US);
		}
	}

	return ret;
}

static int bmi260_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct bmi260_data *data = dev->data;
	uint8_t buf[12];
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_ACC_X_LSB, buf, 12);
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

static int bmi260_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmi260_data *data = dev->data;

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

#if defined(CONFIG_BMI260_TRIGGER)

/* ANYMO_1.duration conversion is 20 ms / LSB */
#define ANYMO_1_DURATION_MSEC_TO_LSB(_ms)	\
BMI260_ANYMO_1_DURATION(_ms / 20)

static int bmi260_write_anymo_threshold(const struct device *dev,
					struct sensor_value val)
{
	struct bmi260_data *data = dev->data;

	/* this takes configuration in g. */
	if (val.val1 > 0) {
		LOG_DBG("anymo_threshold set to max");
		val.val2 = 1e6;
	}

	/* max = BIT_MASK(10) = 1g => 0.49 mg/LSB */
	uint16_t lsbs = (val.val2 * BMI260_ANYMO_2_THRESHOLD_MASK) / 1e6;

	if (!lsbs) {
		LOG_ERR("Threshold too low!");
		return -EINVAL;
	}

	uint16_t anymo_2 = BMI260_ANYMO_2_THRESHOLD(lsbs)
	| BMI260_ANYMO_2_OUT_CONF_BIT_6;

	data->anymo_2 = anymo_2;
	return 0;
}

static int bmi260_write_anymo_duration(const struct device *dev, uint32_t ms)
{
	struct bmi260_data *data = dev->data;
	uint16_t val = ANYMO_1_DURATION_MSEC_TO_LSB(ms)
	| BMI260_ANYMO_1_SELECT_XYZ;

	data->anymo_1 = val;
	return 0;
}
#endif /* CONFIG_BMI260_TRIGGER */

static int bmi260_attr_set(const struct device *dev, enum sensor_channel chan,
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
			#if defined(CONFIG_BMI260_TRIGGER)
		case SENSOR_ATTR_SLOPE_DUR:
			return bmi260_write_anymo_duration(dev, val->val1);
		case SENSOR_ATTR_SLOPE_TH:
			return bmi260_write_anymo_threshold(dev, *val);
			#endif
		default:
			ret = -ENOTSUP;
		}
		return ret;
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
		return ret;
	} else {
		return ret;
	}
}

static int bmi260_init(const struct device *dev)
{
	int ret;
	struct bmi260_data *data = dev->data;
	uint8_t chip_id;
	uint8_t soft_reset_cmd;
	uint8_t init_ctrl;
	uint8_t msg;
	uint8_t tries;
	uint8_t adv_pwr_save;

	ret = bmi260_bus_check(dev);
	if (ret < 0) {
		LOG_ERR("Could not initialize bus");
		return ret;
	}

	#if CONFIG_BMI260_TRIGGER
	data->dev = dev;
	k_mutex_init(&data->trigger_mutex);
	#endif

	data->acc_odr = BMI260_ACC_ODR_100_HZ;
	data->acc_range = 8;
	data->gyr_odr = BMI260_GYR_ODR_200_HZ;
	data->gyr_range = 2000;

	k_usleep(BMI260_POWER_ON_TIME);

	ret = bmi260_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Could not initiate bus communication");
		return ret;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_CHIP_ID, &chip_id, 1);
	if (ret != 0) {
		return ret;
	}

	if (chip_id != BMI260_CHIP_ID) {
		LOG_ERR("Unexpected chip id (%x). Expected (%x)",
			chip_id, BMI260_CHIP_ID);
		return -EIO;
	}

	soft_reset_cmd = BMI260_CMD_SOFT_RESET;
	ret = bmi260_reg_write(dev, BMI260_REG_CMD, &soft_reset_cmd, 1);
	if (ret != 0) {
		return ret;
	}

	k_usleep(BMI260_SOFT_RESET_TIME);

	/* Initialize bus after soft reset according to BMI260 spec */
	ret = bmi260_bus_init(dev);
	if (ret != 0) {
		LOG_ERR("Could not initiate bus communication");
		return ret;
	}

	ret = bmi260_reg_read(dev, BMI260_REG_PWR_CONF, &adv_pwr_save, 1);
	if (ret != 0) {
		return ret;
	}

	adv_pwr_save = BMI260_SET_BITS_POS_0(adv_pwr_save,
					     BMI260_PWR_CONF_ADV_PWR_SAVE,
				      BMI260_PWR_CONF_ADV_PWR_SAVE_DIS);
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CONF,
					  &adv_pwr_save, 1,
				   BMI260_INTER_WRITE_DELAY_US);
	if (ret != 0) {
		return ret;
	}

	init_ctrl = BMI260_PREPARE_CONFIG_LOAD;
	ret = bmi260_reg_write(dev, BMI260_REG_INIT_CTRL, &init_ctrl, 1);
	if (ret != 0) {
		return ret;
	}

	ret = write_config_file(dev);

	if (ret != 0) {
		return ret;
	}

	init_ctrl = BMI260_COMPLETE_CONFIG_LOAD;
	ret = bmi260_reg_write(dev, BMI260_REG_INIT_CTRL, &init_ctrl, 1);
	if (ret != 0) {
		return ret;
	}

	/* Timeout after BMI260_CONFIG_FILE_RETRIES x
	 * BMI260_CONFIG_FILE_POLL_PERIOD_US microseconds.
	 * If tries is BMI260_CONFIG_FILE_RETRIES by the end of the loop,
	 * report an error
	 */
	for (tries = 0; tries <= BMI260_CONFIG_FILE_RETRIES; tries++) {
		ret = bmi260_reg_read(dev, BMI260_REG_INTERNAL_STATUS, &msg, 1);
		if (ret != 0) {
			return ret;
		}

		msg &= BMI260_INST_MESSAGE_MSK;
		if (msg == BMI260_INST_MESSAGE_INIT_OK) {
			break;
		}

		k_usleep(BMI260_CONFIG_FILE_POLL_PERIOD_US);
	}

	if (tries > BMI260_CONFIG_FILE_RETRIES) {
		return -EIO;
	}

	#if CONFIG_BMI260_TRIGGER
	ret = bmi260_init_interrupts(dev);
	if (ret) {
		LOG_ERR("bmi260_init_interrupts returned %d", ret);
		return ret;
	}
	#endif

	adv_pwr_save = BMI260_SET_BITS_POS_0(adv_pwr_save,
					     BMI260_PWR_CONF_ADV_PWR_SAVE,
				      BMI260_PWR_CONF_ADV_PWR_SAVE_EN);
	ret = bmi260_reg_write_with_delay(dev, BMI260_REG_PWR_CONF,
					  &adv_pwr_save, 1,
				   BMI260_INTER_WRITE_DELAY_US);

	return ret;
}

static DEVICE_API(sensor, bmi260_driver_api) = {
	.sample_fetch = bmi260_sample_fetch,
	.channel_get = bmi260_channel_get,
	.attr_set = bmi260_attr_set,
	#if defined(CONFIG_BMI260_TRIGGER)
	.trigger_set = bmi260_trigger_set,
	#endif
};

static const struct bmi260_feature_config bmi260_feature_base = {
	.name = "base",
	.config_file = bmi260_config_file_base,
	.config_file_len = sizeof(bmi260_config_file_base),
	.anymo_1 = &(struct bmi260_feature_reg){ .page = 1, .addr = 0x3C },
	.anymo_2 = &(struct bmi260_feature_reg){ .page = 1, .addr = 0x3E },
};

#define BMI260_FEATURE(inst) &bmi260_feature_base

#if CONFIG_BMI260_TRIGGER
#define BMI260_CONFIG_INT(inst) \
.int1 = GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, irq_gpios, 0, {}),\
.int2 = GPIO_DT_SPEC_INST_GET_BY_IDX_OR(inst, irq_gpios, 1, {}),
#else
#define BMI260_CONFIG_INT(inst)
#endif

/* Initializes a struct bmi260_config for an instance on a SPI bus. */
#define BMI260_CONFIG_SPI(inst)				\
.bus.spi = SPI_DT_SPEC_INST_GET(		\
inst, BMI260_SPI_OPERATION),		\
.bus_io = &bmi260_bus_io_spi,

/* Initializes a struct bmi260_config for an instance on an I2C bus. */
#define BMI260_CONFIG_I2C(inst)				\
.bus.i2c = I2C_DT_SPEC_INST_GET(inst),		\
.bus_io = &bmi260_bus_io_i2c,

#define BMI260_CREATE_INST(inst)					\
\
static struct bmi260_data bmi260_drv_##inst;			\
\
static const struct bmi260_config bmi260_config_##inst = {	\
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),			\
	(BMI260_CONFIG_SPI(inst)),			\
	(BMI260_CONFIG_I2C(inst)))			\
	.feature = BMI260_FEATURE(inst),			\
	BMI260_CONFIG_INT(inst)					\
};								\
\
SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
bmi260_init,				\
NULL,					\
&bmi260_drv_##inst,			\
&bmi260_config_##inst,			\
POST_KERNEL,				\
CONFIG_SENSOR_INIT_PRIORITY,		\
&bmi260_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMI260_CREATE_INST);
