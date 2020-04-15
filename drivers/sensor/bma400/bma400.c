/*
 * Copyright (c) 2020 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma400

#include <drivers/i2c.h>
#include <init.h>
#include <drivers/sensor.h>
#include <sys/__assert.h>
#include <logging/log.h>

#include "bma400.h"

LOG_MODULE_REGISTER(bma400, CONFIG_SENSOR_LOG_LEVEL);

static int reg_read(u8_t reg, u8_t *data, u16_t length,
	struct bma400_dev *dev)
{
	int ret = 0;

	if ((dev->i2c != NULL) && (data != NULL)) {
		ret = i2c_burst_read(dev->i2c, dev->dev_id, reg, data, length);
	} else   {
		ret = -EINVAL;
	}

	return ret;
}

static int reg_write(u8_t reg, const u8_t *data, u16_t length,
	struct bma400_dev *dev)
{
	int ret = 0;
	u16_t i;

	if ((dev->i2c != NULL) && (data != NULL)) {
		for (i = 0; i < length; i++) {
			ret = i2c_burst_write(dev->i2c, dev->dev_id, reg + i,
				&data[i], 1);
		}
	} else   {
		ret = -EINVAL;
	}

	return ret;
}

static void channel_accel_convert(struct sensor_value *val, s64_t raw_val,
	u8_t range)
{
	/* 12 bit accelerometer. 2^11 bits represent the range in G */
	raw_val = (raw_val * SENSOR_G * range) / 2048;

	val->val1 = raw_val / 1000000;
	val->val2 = raw_val % 1000000;

	/* Normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}
}

static int set_accel_config(const struct bma400_acc_cfg *cfg,
	struct bma400_dev *dev)
{
	int ret;
	u8_t acc_cfg[3] = { 0 };
	u8_t range[4] = { 2, 4, 8, 16 };

	/* Read the existing configuration to account for register defaults */
	ret = reg_read(BMA400_REG_ACC_CONFIG0, acc_cfg, 3, dev);
	if (ret == 0) {
		acc_cfg[1] = BMA400_SET_BITS_POS_0(acc_cfg[1], BMA400_ODR,
			cfg->odr);
		acc_cfg[1] = BMA400_SET_BITS(acc_cfg[1], BMA400_RANGE,
			cfg->range);
		acc_cfg[2] = BMA400_SET_BITS(acc_cfg[2], BMA400_DATA_SRC,
			cfg->data_src);
		acc_cfg[1] = BMA400_SET_BITS(acc_cfg[1], BMA400_OSR,
			cfg->osr);
		acc_cfg[0] = BMA400_SET_BITS(acc_cfg[0], BMA400_OSR_LP,
			cfg->osr_lp);
		acc_cfg[0] = BMA400_SET_BITS(acc_cfg[0], BMA400_FILT1_BW,
			cfg->filt1_bw);

		ret = reg_write(BMA400_REG_ACC_CONFIG0, acc_cfg, 3, dev);

		dev->range = range[cfg->range];
	}

	return ret;
}

static s16_t convert_acc_data(u8_t lsb, u8_t msb)
{
	s16_t data;

	data = (s16_t) (((u16_t) msb * 256) + lsb);
	if (data > 2047) {
		data = data - 4096;
	}
	return data;
}

static int set_power_mode(u8_t power_mode, struct bma400_dev *dev)
{
	int ret;
	u8_t acc_cfg0;

	ret = reg_read(BMA400_REG_ACC_CONFIG0, &acc_cfg0, 1, dev);
	if (ret == 0) {
		acc_cfg0 = BMA400_SET_BITS_POS_0(acc_cfg0, BMA400_POWER_MODE,
			power_mode);

		ret = reg_write(BMA400_REG_ACC_CONFIG0, &acc_cfg0, 1, dev);
	}

	return ret;
}

static int set_attributes(double odr, double osr, double range,
	struct bma400_dev *dev)
{
	struct bma400_acc_cfg acc_cfg;
	u8_t power_mode = BMA400_POWER_MODE_SLEEP;
	int ret;

	acc_cfg.odr = BMA400_ODR_12_5HZ;

	if (odr >= 12.5f) {
		power_mode = BMA400_POWER_MODE_NORMAL;
		if ((odr >= 12.5f) && (odr < 25.0f)) {
			acc_cfg.odr = BMA400_ODR_12_5HZ;
		} else if ((odr >= 25.0f) && (odr < 50.0f)) {
			power_mode = BMA400_POWER_MODE_LOW_POWER;
			acc_cfg.odr = BMA400_ODR_25HZ;
		} else if ((odr >= 50.0f) && (odr < 100.0f)) {
			acc_cfg.odr = BMA400_ODR_50HZ;
		} else if ((odr >= 100.0f) && (odr < 200.0f)) {
			acc_cfg.odr = BMA400_ODR_100HZ;
		} else if ((odr >= 200.0f) && (odr < 400.0f)) {
			acc_cfg.odr = BMA400_ODR_200HZ;
		} else if ((odr >= 400.0f) && (odr < 800.0f)) {
			acc_cfg.odr = BMA400_ODR_400HZ;
		} else if (odr >= 800.0f) {
			acc_cfg.odr = BMA400_ODR_800HZ;
		}
	}

	acc_cfg.osr = BMA400_OSR_LOWEST;
	acc_cfg.osr_lp = BMA400_OSR_LOWEST;
	if (osr < 1.0f) {
		acc_cfg.osr = BMA400_OSR_LOWEST;
		acc_cfg.osr_lp = BMA400_OSR_LOWEST;
	} else if ((osr >= 1.0f) && (osr < 2.0f)) {
		acc_cfg.osr = BMA400_OSR_LOW;
		acc_cfg.osr_lp = BMA400_OSR_LOW;
	} else if ((osr >= 2.0f) && (osr < 3.0f)) {
		acc_cfg.osr = BMA400_OSR_HIGH;
		acc_cfg.osr_lp = BMA400_OSR_HIGH;
	} else if (osr >= 3.0f) {
		acc_cfg.osr = BMA400_OSR_HIGHEST;
		acc_cfg.osr_lp = BMA400_OSR_HIGHEST;
	}

	acc_cfg.range = BMA400_RANGE_2G;
	if (range < 4.0f) {
		acc_cfg.range = BMA400_RANGE_2G;
	} else if ((range >= 4.0f) && (range < 8.0f)) {
		acc_cfg.range = BMA400_RANGE_4G;
	} else if ((range >= 8.0f) && (range < 16.0f)) {
		acc_cfg.range = BMA400_RANGE_8G;
	} else if (range >= 16.0f) {
		acc_cfg.range = BMA400_RANGE_16G;
	}

	acc_cfg.filt1_bw = BMA400_FILT1_BW_0_48_ODR;
	acc_cfg.data_src = BMA400_DATA_SRC_VAR_ODR;

	ret = set_accel_config(&acc_cfg, dev);
	if (ret != 0) {
		return ret;
	}

	ret = set_power_mode(power_mode, dev);

	return ret;
}

int bma400_init(struct device *dev)
{
	int ret;
	struct bma400_dev *drv_dev = dev->driver_data;
	u8_t chip_id;
	u8_t soft_reset_cmd = BMA400_CMD_SOFT_RESET;

	drv_dev->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (drv_dev->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	drv_dev->dev_id = DT_INST_REG_ADDR(0);

	ret = reg_read(BMA400_REG_CHIP_ID, &chip_id, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	if (chip_id != BMA400_CHIP_ID) {
		LOG_ERR("Unexpected chip id (%x). Expected (%x)", chip_id,
			BMA400_CHIP_ID);
		return -EIO;
	}

	ret = reg_write(BMA400_REG_CMD, &soft_reset_cmd, 1, drv_dev);
	if (ret != 0) {
		return ret;
	}

	k_usleep(BMA400_PERIOD_SOFT_RESET);

	ret = set_power_mode(BMA400_POWER_MODE_SLEEP, drv_dev);
	drv_dev->last_odr = 0.0f;
	drv_dev->last_osr = 0.0f;
	drv_dev->last_range = 4.0f; /* 4G */

	return ret;
}

int bma400_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct bma400_dev *drv_dev = dev->driver_data;
	u8_t data[6];
	int ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	ret = reg_read(BMA400_REG_ACC_X_LSB, data, 6, drv_dev);
	if (ret == 0) {
		drv_dev->data.x = convert_acc_data(data[0], data[1]);
		drv_dev->data.y = convert_acc_data(data[2], data[3]);
		drv_dev->data.z = convert_acc_data(data[4], data[5]);
	} else   {
		drv_dev->data.x = 0;
		drv_dev->data.y = 0;
		drv_dev->data.z = 0;
	}

	return ret;
}

int bma400_channel_get(struct device *dev, enum sensor_channel chan,
	struct sensor_value *val)
{
	struct bma400_dev *drv_dev = dev->driver_data;

	if (chan == SENSOR_CHAN_ACCEL_X) {
		channel_accel_convert(val, drv_dev->data.x, drv_dev->range);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		channel_accel_convert(val, drv_dev->data.y, drv_dev->range);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		channel_accel_convert(val, drv_dev->data.z, drv_dev->range);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		channel_accel_convert(&val[0], drv_dev->data.x, drv_dev->range);
		channel_accel_convert(&val[1], drv_dev->data.y, drv_dev->range);
		channel_accel_convert(&val[2], drv_dev->data.z, drv_dev->range);
	} else   {
		return -ENOTSUP;
	}

	return 0;
}

int bma400_attr_set(struct device *dev, enum sensor_channel chan,
	enum sensor_attribute attr, const struct sensor_value *val)
{
	struct bma400_dev *drv_dev = dev->driver_data;
	int ret;
	double odr = drv_dev->last_odr;
	double osr = drv_dev->last_osr;
	double range = drv_dev->last_range;

	if ((chan == SENSOR_CHAN_ACCEL_X) || (chan == SENSOR_CHAN_ACCEL_Y)
		|| (chan == SENSOR_CHAN_ACCEL_Z)
		|| (chan == SENSOR_CHAN_ACCEL_XYZ)) {
		switch (attr) {
		case SENSOR_ATTR_SAMPLING_FREQUENCY:
			odr = (double) val->val1 + (double) val->val2 / 1000000;
			drv_dev->last_odr = odr;
			break;
		case SENSOR_ATTR_OVERSAMPLING:
			osr = (double) val->val1 + (double) val->val2 / 1000000;
			drv_dev->last_osr = osr;
			break;
		case SENSOR_ATTR_FULL_SCALE:
			range = (double) val->val1 + (double) val->val2
				/ 1000000;
			drv_dev->last_range = range;
			break;
		default:
			return -ENOTSUP;
		}
		ret = set_attributes(odr, osr, range, drv_dev);
	} else   {
		return -ENOTSUP;
	}

	return ret;
}

static struct bma400_dev bma400_drv;
static const struct sensor_driver_api bma400_driver_api = {
	.sample_fetch = bma400_sample_fetch,
	.channel_get = bma400_channel_get,
	.attr_set = bma400_attr_set
};

DEVICE_AND_API_INIT(bma400, DT_INST_LABEL(0), bma400_init, &bma400_drv,
	NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &bma400_driver_api);
