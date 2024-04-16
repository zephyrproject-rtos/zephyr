/* bmc150_magn.c - Driver for Bosch BMC150 magnetometer sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * This code is based on bmm050.c from:
 * https://github.com/BoschSensortec/BMM050_driver
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmc150_magn

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "bmc150_magn.h"

LOG_MODULE_REGISTER(BMC150_MAGN, CONFIG_SENSOR_LOG_LEVEL);

static const struct {
	int freq;
	uint8_t reg_val;
} bmc150_magn_samp_freq_table[] = { {2, 0x01},
				    {6, 0x02},
				    {8, 0x03},
				    {10, 0x00},
				    {15, 0x04},
				    {20, 0x05},
				    {25, 0x06},
				    {30, 0x07} };

static const struct bmc150_magn_preset {
	uint8_t rep_xy;
	uint8_t rep_z;
	uint8_t odr;
} bmc150_magn_presets_table[] = {
	[LOW_POWER_PRESET] = {3, 3, 10},
	[REGULAR_PRESET] = {9, 15, 10},
	[ENHANCED_REGULAR_PRESET] = {15, 27, 10},
	[HIGH_ACCURACY_PRESET] = {47, 83, 20}
};

static int bmc150_magn_set_power_mode(const struct device *dev,
				      enum bmc150_magn_power_modes mode,
				      int state)
{
	const struct bmc150_magn_config *config = dev->config;

	switch (mode) {
	case BMC150_MAGN_POWER_MODE_SUSPEND:
		if (i2c_reg_update_byte_dt(&config->i2c,
					   BMC150_MAGN_REG_POWER,
					   BMC150_MAGN_MASK_POWER_CTL,
					   !state) < 0) {
			return -EIO;
		}
		k_busy_wait(USEC_PER_MSEC * 5U);

		return 0;
	case BMC150_MAGN_POWER_MODE_SLEEP:
		return i2c_reg_update_byte_dt(&config->i2c,
					      BMC150_MAGN_REG_OPMODE_ODR,
					      BMC150_MAGN_MASK_OPMODE,
					      BMC150_MAGN_MODE_SLEEP <<
					      BMC150_MAGN_SHIFT_OPMODE);
		break;
	case BMC150_MAGN_POWER_MODE_NORMAL:
		return i2c_reg_update_byte_dt(&config->i2c,
					      BMC150_MAGN_REG_OPMODE_ODR,
					      BMC150_MAGN_MASK_OPMODE,
					      BMC150_MAGN_MODE_NORMAL <<
					      BMC150_MAGN_SHIFT_OPMODE);
		break;
	}

	return -ENOTSUP;
}

static int bmc150_magn_set_odr(const struct device *dev, uint8_t val)
{
	const struct bmc150_magn_config *config = dev->config;
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(bmc150_magn_samp_freq_table); ++i) {
		if (val <= bmc150_magn_samp_freq_table[i].freq) {
			return i2c_reg_update_byte_dt(&config->i2c,
						      BMC150_MAGN_REG_OPMODE_ODR,
						      BMC150_MAGN_MASK_ODR,
						      bmc150_magn_samp_freq_table[i].reg_val
						      << BMC150_MAGN_SHIFT_ODR);
		}
	}

	return -ENOTSUP;
}

#if defined(BMC150_MAGN_SET_ATTR)
static int bmc150_magn_read_rep_xy(const struct device *dev)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;
	uint8_t reg_val;

	if (i2c_reg_read_byte_dt(&config->i2c,
				 BMC150_MAGN_REG_REP_XY, &reg_val) < 0) {
		return -EIO;
	}

	data->rep_xy = BMC150_MAGN_REGVAL_TO_REPXY((int)(reg_val));

	return 0;
}

static int bmc150_magn_read_rep_z(const struct device *dev)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;
	uint8_t reg_val;

	if (i2c_reg_read_byte_dt(&config->i2c,
				 BMC150_MAGN_REG_REP_Z, &reg_val) < 0) {
		return -EIO;
	}

	data->rep_z = BMC150_MAGN_REGVAL_TO_REPZ((int)(reg_val));

	return 0;
}

static int bmc150_magn_compute_max_odr(const struct device *dev, int rep_xy,
				       int rep_z, int *max_odr)
{
	struct bmc150_magn_data *data = dev->data;

	if (rep_xy == 0) {
		if (data->rep_xy <= 0) {
			if (bmc150_magn_read_rep_xy(dev) < 0) {
				return -EIO;
			}
		}
		rep_xy = data->rep_xy;
	}

	if (rep_z == 0) {
		if (data->rep_z <= 0) {
			if (bmc150_magn_read_rep_z(dev) < 0) {
				return -EIO;
			}
		}
		rep_z = data->rep_z;
	}

	*max_odr = 1000000 / (145 * rep_xy + 500 * rep_z + 980);

	return 0;
}
#endif

#if defined(BMC150_MAGN_SET_ATTR_REP)
static int bmc150_magn_read_odr(const struct device *dev)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;
	uint8_t i, odr_val, reg_val;

	if (i2c_reg_read_byte_dt(&config->i2c,
				 BMC150_MAGN_REG_OPMODE_ODR, &reg_val) < 0) {
		return -EIO;
	}

	odr_val = (reg_val & BMC150_MAGN_MASK_ODR) >> BMC150_MAGN_SHIFT_ODR;

	for (i = 0U; i < ARRAY_SIZE(bmc150_magn_samp_freq_table); ++i) {
		if (bmc150_magn_samp_freq_table[i].reg_val == odr_val) {
			data->odr = bmc150_magn_samp_freq_table[i].freq;
			return 0;
		}
	}

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_BMC150_MAGN_SAMPLING_REP_XY)
static int bmc150_magn_write_rep_xy(const struct device *dev, int val)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;

	if (i2c_reg_update_byte_dt(&config->i2c,
				   BMC150_MAGN_REG_REP_XY,
				   BMC150_MAGN_REG_REP_DATAMASK,
				   BMC150_MAGN_REPXY_TO_REGVAL(val)) < 0) {
		return -EIO;
	}

	data->rep_xy = val;

	return 0;
}
#endif

#if defined(CONFIG_BMC150_MAGN_SAMPLING_REP_Z)
static int bmc150_magn_write_rep_z(const struct device *dev, int val)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;

	if (i2c_reg_update_byte_dt(&config->i2c,
				   BMC150_MAGN_REG_REP_Z,
				   BMC150_MAGN_REG_REP_DATAMASK,
				   BMC150_MAGN_REPZ_TO_REGVAL(val)) < 0) {
		return -EIO;
	}

	data->rep_z = val;

	return 0;
}
#endif

/*
 * Datasheet part 4.3.4, provided by Bosch here:
 * https://github.com/BoschSensortec/BMM050_driver
 */
static int32_t bmc150_magn_compensate_xy(struct bmc150_magn_trim_regs *tregs,
					int16_t xy, uint16_t rhall, bool is_x)
{
	int8_t txy1, txy2;
	int16_t val;

	if (xy == BMC150_MAGN_XY_OVERFLOW_VAL) {
		return INT32_MIN;
	}

	if (!rhall) {
		rhall = tregs->xyz1;
	}

	if (is_x) {
		txy1 = tregs->x1;
		txy2 = tregs->x2;
	} else {
		txy1 = tregs->y1;
		txy2 = tregs->y2;
	}

	val = ((int16_t)(((uint16_t)((((int32_t)tregs->xyz1) << 14) / rhall)) -
	      ((uint16_t)0x4000)));
	val = ((int16_t)((((int32_t)xy) * ((((((((int32_t)tregs->xy2) *
	      ((((int32_t)val) * ((int32_t)val)) >> 7)) + (((int32_t)val) *
	      ((int32_t)(((int16_t)tregs->xy1) << 7)))) >> 9) +
	      ((int32_t)0x100000)) * ((int32_t)(((int16_t)txy2) +
	      ((int16_t)0xA0)))) >> 12)) >> 13)) + (((int16_t)txy1) << 3);

	return (int32_t)val;
}

static int32_t bmc150_magn_compensate_z(struct bmc150_magn_trim_regs *tregs,
					int16_t z, uint16_t rhall)
{
	int32_t val;

	if (z == BMC150_MAGN_Z_OVERFLOW_VAL) {
		return INT32_MIN;
	}

	val = (((((int32_t)(z - tregs->z4)) << 15) - ((((int32_t)tregs->z3) *
	      ((int32_t)(((int16_t)rhall) - ((int16_t)tregs->xyz1)))) >> 2)) /
	      (tregs->z2 + ((int16_t)(((((int32_t)tregs->z1) *
	      ((((int16_t)rhall) << 1))) + (1 << 15)) >> 16))));

	return val;
}

static int bmc150_magn_sample_fetch(const struct device *dev,
				    enum sensor_channel chan)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;
	uint16_t values[BMC150_MAGN_AXIS_XYZR_MAX];
	int16_t raw_x, raw_y, raw_z;
	uint16_t rhall;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_MAGN_XYZ);

	if (i2c_burst_read_dt(&config->i2c,
			      BMC150_MAGN_REG_X_L, (uint8_t *)values,
			      sizeof(values)) < 0) {
		LOG_ERR("failed to read sample");
		return -EIO;
	}

	raw_x = (int16_t)sys_le16_to_cpu(values[BMC150_MAGN_AXIS_X]) >>
		BMC150_MAGN_SHIFT_XY_L;
	raw_y = (int16_t)sys_le16_to_cpu(values[BMC150_MAGN_AXIS_Y]) >>
		BMC150_MAGN_SHIFT_XY_L;
	raw_z = (int16_t)sys_le16_to_cpu(values[BMC150_MAGN_AXIS_Z]) >>
		BMC150_MAGN_SHIFT_Z_L;
	rhall = sys_le16_to_cpu(values[BMC150_MAGN_RHALL]) >>
		BMC150_MAGN_SHIFT_RHALL_L;

	data->sample_x = bmc150_magn_compensate_xy(&data->tregs, raw_x, rhall,
						   true);
	data->sample_y = bmc150_magn_compensate_xy(&data->tregs, raw_y, rhall,
						   false);
	data->sample_z = bmc150_magn_compensate_z(&data->tregs, raw_z, rhall);

	return 0;
}

static void bmc150_magn_convert(struct sensor_value *val, int raw_val)
{
	/* val = raw_val / 1600 */
	val->val1 = raw_val / 1600;
	val->val2 = ((int32_t)raw_val * (1000000 / 1600)) % 1000000;
}

static int bmc150_magn_channel_get(const struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct bmc150_magn_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		bmc150_magn_convert(val, data->sample_x);
		break;
	case SENSOR_CHAN_MAGN_Y:
		bmc150_magn_convert(val, data->sample_y);
		break;
	case SENSOR_CHAN_MAGN_Z:
		bmc150_magn_convert(val, data->sample_z);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		bmc150_magn_convert(val, data->sample_x);
		bmc150_magn_convert(val + 1, data->sample_y);
		bmc150_magn_convert(val + 2, data->sample_z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#if defined(BMC150_MAGN_SET_ATTR_REP)
static inline int bmc150_magn_attr_set_rep(const struct device *dev,
					   enum sensor_channel chan,
					   const struct sensor_value *val)
{
	struct bmc150_magn_data *data = dev->data;
	int max_odr;

	switch (chan) {
#if defined(CONFIG_BMC150_MAGN_SAMPLING_REP_XY)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
		if (val->val1 < 1 || val->val1 > 511) {
			return -EINVAL;
		}

		if (bmc150_magn_compute_max_odr(dev, val->val1, 0,
						&max_odr) < 0) {
			return -EIO;
		}

		if (data->odr <= 0) {
			if (bmc150_magn_read_odr(dev) < 0) {
				return -EIO;
			}
		}

		if (data->odr > max_odr) {
			return -EINVAL;
		}

		if (bmc150_magn_write_rep_xy(dev, val->val1) < 0) {
			return -EIO;
		}
		break;
#endif
#if defined(CONFIG_BMC150_MAGN_SAMPLING_REP_Z)
	case SENSOR_CHAN_MAGN_Z:
		if (val->val1 < 1 || val->val1 > 256) {
			return -EINVAL;
		}

		if (bmc150_magn_compute_max_odr(dev, 0, val->val1,
						&max_odr) < 0) {
			return -EIO;
		}

		if (data->odr <= 0) {
			if (bmc150_magn_read_odr(dev) < 0) {
				return -EIO;
			}
		}

		if (data->odr > max_odr) {
			return -EINVAL;
		}

		if (bmc150_magn_write_rep_z(dev, val->val1) < 0) {
			return -EIO;
		}
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

#if defined(BMC150_MAGN_SET_ATTR)
static int bmc150_magn_attr_set(const struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct bmc150_magn_data *data = dev->data;

	switch (attr) {
#if defined(CONFIG_BMC150_MAGN_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (data->max_odr <= 0) {
			if (bmc150_magn_compute_max_odr(dev, 0, 0,
							&data->max_odr) < 0) {
				return -EIO;
			}
		}

		if (data->max_odr < val->val1) {
			LOG_ERR("not supported with current oversampling");
			return -ENOTSUP;
		}

		if (bmc150_magn_set_odr(dev, (uint8_t)(val->val1)) < 0) {
			return -EIO;
		}
		break;
#endif
#if defined(BMC150_MAGN_SET_ATTR_REP)
	case SENSOR_ATTR_OVERSAMPLING:
		bmc150_magn_attr_set_rep(dev, chan, val);
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static const struct sensor_driver_api bmc150_magn_api_funcs = {
#if defined(BMC150_MAGN_SET_ATTR)
	.attr_set = bmc150_magn_attr_set,
#endif
	.sample_fetch = bmc150_magn_sample_fetch,
	.channel_get = bmc150_magn_channel_get,
#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	.trigger_set = bmc150_magn_trigger_set,
#endif
};

static int bmc150_magn_init_chip(const struct device *dev)
{
	struct bmc150_magn_data *data = dev->data;
	const struct bmc150_magn_config *config = dev->config;
	uint8_t chip_id;
	struct bmc150_magn_preset preset;

	bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_NORMAL, 0);
	bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_SUSPEND, 1);

	if (bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_SUSPEND, 0)
				       < 0) {
		LOG_ERR("failed to bring up device from suspend mode");
		return -EIO;
	}

	if (i2c_reg_read_byte_dt(&config->i2c,
				 BMC150_MAGN_REG_CHIP_ID, &chip_id) < 0) {
		LOG_ERR("failed reading chip id");
		goto err_poweroff;
	}
	if (chip_id != BMC150_MAGN_CHIP_ID_VAL) {
		LOG_ERR("invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}
	LOG_ERR("chip id 0x%x", chip_id);

	preset = bmc150_magn_presets_table[BMC150_MAGN_DEFAULT_PRESET];
	if (bmc150_magn_set_odr(dev, preset.odr) < 0) {
		LOG_ERR("failed to set ODR to %d",
			    preset.odr);
		goto err_poweroff;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
				  BMC150_MAGN_REG_REP_XY,
				  BMC150_MAGN_REPXY_TO_REGVAL(preset.rep_xy))
				  < 0) {
		LOG_ERR("failed to set REP XY to %d",
			    preset.rep_xy);
		goto err_poweroff;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
				  BMC150_MAGN_REG_REP_Z,
				  BMC150_MAGN_REPZ_TO_REGVAL(preset.rep_z)) < 0) {
		LOG_ERR("failed to set REP Z to %d",
			    preset.rep_z);
		goto err_poweroff;
	}

	if (bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_NORMAL, 1)
				       < 0) {
		LOG_ERR("failed to power on device");
		goto err_poweroff;
	}

	if (i2c_burst_read_dt(&config->i2c,
			      BMC150_MAGN_REG_TRIM_START, (uint8_t *)&data->tregs,
			      sizeof(data->tregs)) < 0) {
		LOG_ERR("failed to read trim regs");
		goto err_poweroff;
	}

	data->rep_xy = 0;
	data->rep_z = 0;
	data->odr = 0;
	data->max_odr = 0;
	data->sample_x = 0;
	data->sample_y = 0;
	data->sample_z = 0;

	data->tregs.xyz1 = sys_le16_to_cpu(data->tregs.xyz1);
	data->tregs.z1 = sys_le16_to_cpu(data->tregs.z1);
	data->tregs.z2 = sys_le16_to_cpu(data->tregs.z2);
	data->tregs.z3 = sys_le16_to_cpu(data->tregs.z3);
	data->tregs.z4 = sys_le16_to_cpu(data->tregs.z4);

	return 0;

err_poweroff:
	bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_NORMAL, 0);
	bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_SUSPEND, 1);
	return -EIO;
}

static int bmc150_magn_init(const struct device *dev)
{
	const struct bmc150_magn_config * const config =
					  dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	if (bmc150_magn_init_chip(dev) < 0) {
		LOG_ERR("failed to initialize chip");
		return -EIO;
	}

#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	if (config->int_gpio.port) {
		if (bmc150_magn_init_interrupt(dev) < 0) {
			LOG_ERR("failed to initialize interrupts");
			return -EINVAL;
		}
	}
#endif
	return 0;
}

#define BMC150_MAGN_DEFINE(inst)								\
	static struct bmc150_magn_data bmc150_magn_data_##inst;					\
												\
	static const struct bmc150_magn_config bmc150_magn_config_##inst = {			\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_BMC150_MAGN_TRIGGER_DRDY,					\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, drdy_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bmc150_magn_init, NULL,				\
		&bmc150_magn_data_##inst, &bmc150_magn_config_##inst, POST_KERNEL,		\
		CONFIG_SENSOR_INIT_PRIORITY, &bmc150_magn_api_funcs);				\

DT_INST_FOREACH_STATUS_OKAY(BMC150_MAGN_DEFINE)
