/* bmc150_magn.c - Driver for Bosch BMC150 magnetometer sensor */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * This code is based on bmm050.c from:
 * https://github.com/BoschSensortec/BMM050_driver
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <misc/byteorder.h>
#include <misc/__assert.h>

#include <gpio.h>

#include "bmc150_magn.h"

static const struct {
	int freq;
	u8_t reg_val;
} bmc150_magn_samp_freq_table[] = { {2, 0x01},
				    {6, 0x02},
				    {8, 0x03},
				    {10, 0x00},
				    {15, 0x04},
				    {20, 0x05},
				    {25, 0x06},
				    {30, 0x07} };

static const struct bmc150_magn_preset {
	u8_t rep_xy;
	u8_t rep_z;
	u8_t odr;
} bmc150_magn_presets_table[] = {
	[LOW_POWER_PRESET] = {3, 3, 10},
	[REGULAR_PRESET] = {9, 15, 10},
	[ENHANCED_REGULAR_PRESET] = {15, 27, 10},
	[HIGH_ACCURACY_PRESET] = {47, 83, 20}
};

static int bmc150_magn_set_power_mode(struct device *dev,
				      enum bmc150_magn_power_modes mode,
				      int state)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;

	switch (mode) {
	case BMC150_MAGN_POWER_MODE_SUSPEND:
		if (i2c_reg_update_byte(data->i2c_master,
					config->i2c_slave_addr,
					BMC150_MAGN_REG_POWER,
					BMC150_MAGN_MASK_POWER_CTL,
					!state) < 0) {
			return -EIO;
		}
		k_busy_wait(5 * USEC_PER_MSEC);

		return 0;
	case BMC150_MAGN_POWER_MODE_SLEEP:
		return i2c_reg_update_byte(data->i2c_master,
					   config->i2c_slave_addr,
					   BMC150_MAGN_REG_OPMODE_ODR,
					   BMC150_MAGN_MASK_OPMODE,
					   BMC150_MAGN_MODE_SLEEP <<
					   BMC150_MAGN_SHIFT_OPMODE);
		break;
	case BMC150_MAGN_POWER_MODE_NORMAL:
		return i2c_reg_update_byte(data->i2c_master,
					   config->i2c_slave_addr,
					   BMC150_MAGN_REG_OPMODE_ODR,
					   BMC150_MAGN_MASK_OPMODE,
					   BMC150_MAGN_MODE_NORMAL <<
					   BMC150_MAGN_SHIFT_OPMODE);
		break;
	}

	return -ENOTSUP;
}

static int bmc150_magn_set_odr(struct device *dev, u8_t val)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;
	u8_t i;

	for (i = 0; i < ARRAY_SIZE(bmc150_magn_samp_freq_table); ++i) {
		if (val <= bmc150_magn_samp_freq_table[i].freq) {
			return i2c_reg_update_byte(data->i2c_master,
						config->i2c_slave_addr,
						BMC150_MAGN_REG_OPMODE_ODR,
						BMC150_MAGN_MASK_ODR,
						bmc150_magn_samp_freq_table[i].
						reg_val <<
						BMC150_MAGN_SHIFT_ODR);
		}
	}

	return -ENOTSUP;
}

#if defined(BMC150_MAGN_SET_ATTR)
static int bmc150_magn_read_rep_xy(struct device *dev)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;
	u8_t reg_val;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      BMC150_MAGN_REG_REP_XY, &reg_val) < 0) {
		return -EIO;
	}

	data->rep_xy = BMC150_MAGN_REGVAL_TO_REPXY((int)(reg_val));

	return 0;
}

static int bmc150_magn_read_rep_z(struct device *dev)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;
	u8_t reg_val;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      BMC150_MAGN_REG_REP_Z, &reg_val) < 0) {
		return -EIO;
	}

	data->rep_z = BMC150_MAGN_REGVAL_TO_REPZ((int)(reg_val));

	return 0;
}

static int bmc150_magn_compute_max_odr(struct device *dev, int rep_xy,
				       int rep_z, int *max_odr)
{
	struct bmc150_magn_data *data = dev->driver_data;

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
static int bmc150_magn_read_odr(struct device *dev)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;
	u8_t i, odr_val, reg_val;

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      BMC150_MAGN_REG_OPMODE_ODR, &reg_val) < 0) {
		return -EIO;
	}

	odr_val = (reg_val & BMC150_MAGN_MASK_ODR) >> BMC150_MAGN_SHIFT_ODR;

	for (i = 0; i < ARRAY_SIZE(bmc150_magn_samp_freq_table); ++i) {
		if (bmc150_magn_samp_freq_table[i].reg_val == odr_val) {
			data->odr = bmc150_magn_samp_freq_table[i].freq;
			return 0;
		}
	}

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_BMC150_MAGN_SAMPLING_REP_XY)
static int bmc150_magn_write_rep_xy(struct device *dev, int val)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
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
static int bmc150_magn_write_rep_z(struct device *dev, int val)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;

	if (i2c_reg_update_byte(data->i2c_master, config->i2c_slave_addr,
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
static s32_t bmc150_magn_compensate_xy(struct bmc150_magn_trim_regs *tregs,
					s16_t xy, u16_t rhall, bool is_x)
{
	s8_t txy1, txy2;
	s16_t val;

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

	val = ((s16_t)(((u16_t)((((s32_t)tregs->xyz1) << 14) / rhall)) -
	      ((u16_t)0x4000)));
	val = ((s16_t)((((s32_t)xy) * ((((((((s32_t)tregs->xy2) *
	      ((((s32_t)val) * ((s32_t)val)) >> 7)) + (((s32_t)val) *
	      ((s32_t)(((s16_t)tregs->xy1) << 7)))) >> 9) +
	      ((s32_t)0x100000)) * ((s32_t)(((s16_t)txy2) +
	      ((s16_t)0xA0)))) >> 12)) >> 13)) + (((s16_t)txy1) << 3);

	return (s32_t)val;
}

static s32_t bmc150_magn_compensate_z(struct bmc150_magn_trim_regs *tregs,
					s16_t z, u16_t rhall)
{
	s32_t val;

	if (z == BMC150_MAGN_Z_OVERFLOW_VAL) {
		return INT32_MIN;
	}

	val = (((((s32_t)(z - tregs->z4)) << 15) - ((((s32_t)tregs->z3) *
	      ((s32_t)(((s16_t)rhall) - ((s16_t)tregs->xyz1)))) >> 2)) /
	      (tregs->z2 + ((s16_t)(((((s32_t)tregs->z1) *
	      ((((s16_t)rhall) << 1))) + (1 << 15)) >> 16))));

	return val;
}

static int bmc150_magn_sample_fetch(struct device *dev,
				    enum sensor_channel chan)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;
	u16_t values[BMC150_MAGN_AXIS_XYZR_MAX];
	s16_t raw_x, raw_y, raw_z;
	u16_t rhall;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_MAGN_XYZ);

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   BMC150_MAGN_REG_X_L, (u8_t *)values,
			   sizeof(values)) < 0) {
		SYS_LOG_ERR("failed to read sample");
		return -EIO;
	}

	raw_x = (s16_t)sys_le16_to_cpu(values[BMC150_MAGN_AXIS_X]) >>
		BMC150_MAGN_SHIFT_XY_L;
	raw_y = (s16_t)sys_le16_to_cpu(values[BMC150_MAGN_AXIS_Y]) >>
		BMC150_MAGN_SHIFT_XY_L;
	raw_z = (s16_t)sys_le16_to_cpu(values[BMC150_MAGN_AXIS_Z]) >>
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
	val->val2 = ((s32_t)raw_val * (1000000 / 1600)) % 1000000;
}

static int bmc150_magn_channel_get(struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct bmc150_magn_data *data = dev->driver_data;

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
		return -EINVAL;
	}

	return 0;
}

#if defined(BMC150_MAGN_SET_ATTR_REP)
static inline int bmc150_magn_attr_set_rep(struct device *dev,
					   enum sensor_channel chan,
					   const struct sensor_value *val)
{
	struct bmc150_magn_data *data = dev->driver_data;
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
static int bmc150_magn_attr_set(struct device *dev,
				enum sensor_channel chan,
				enum sensor_attribute attr,
				const struct sensor_value *val)
{
	struct bmc150_magn_data *data = dev->driver_data;

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
			SYS_LOG_ERR("not supported with current oversampling");
			return -ENOTSUP;
		}

		if (bmc150_magn_set_odr(dev, (u8_t)(val->val1)) < 0) {
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

static int bmc150_magn_init_chip(struct device *dev)
{
	struct bmc150_magn_data *data = dev->driver_data;
	const struct bmc150_magn_config *config = dev->config->config_info;
	u8_t chip_id;
	struct bmc150_magn_preset preset;

	bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_NORMAL, 0);
	bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_SUSPEND, 1);

	if (bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_SUSPEND, 0)
				       < 0) {
		SYS_LOG_ERR("failed to bring up device from suspend mode");
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c_master, config->i2c_slave_addr,
			      BMC150_MAGN_REG_CHIP_ID, &chip_id) < 0) {
		SYS_LOG_ERR("failed reading chip id");
		goto err_poweroff;
	}
	if (chip_id != BMC150_MAGN_CHIP_ID_VAL) {
		SYS_LOG_ERR("invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}
	SYS_LOG_ERR("chip id 0x%x", chip_id);

	preset = bmc150_magn_presets_table[BMC150_MAGN_DEFAULT_PRESET];
	if (bmc150_magn_set_odr(dev, preset.odr) < 0) {
		SYS_LOG_ERR("failed to set ODR to %d",
			    preset.odr);
		goto err_poweroff;
	}

	if (i2c_reg_write_byte(data->i2c_master, config->i2c_slave_addr,
			       BMC150_MAGN_REG_REP_XY,
			       BMC150_MAGN_REPXY_TO_REGVAL(preset.rep_xy))
			       < 0) {
		SYS_LOG_ERR("failed to set REP XY to %d",
			    preset.rep_xy);
		goto err_poweroff;
	}

	if (i2c_reg_write_byte(data->i2c_master, config->i2c_slave_addr,
			       BMC150_MAGN_REG_REP_Z,
			       BMC150_MAGN_REPZ_TO_REGVAL(preset.rep_z)) < 0) {
		SYS_LOG_ERR("failed to set REP Z to %d",
			    preset.rep_z);
		goto err_poweroff;
	}

	if (bmc150_magn_set_power_mode(dev, BMC150_MAGN_POWER_MODE_NORMAL, 1)
				       < 0) {
		SYS_LOG_ERR("failed to power on device");
		goto err_poweroff;
	}

	if (i2c_burst_read(data->i2c_master, config->i2c_slave_addr,
			   BMC150_MAGN_REG_TRIM_START, (u8_t *)&data->tregs,
			   sizeof(data->tregs)) < 0) {
		SYS_LOG_ERR("failed to read trim regs");
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

static int bmc150_magn_init(struct device *dev)
{
	const struct bmc150_magn_config * const config =
					  dev->config->config_info;
	struct bmc150_magn_data *data = dev->driver_data;

	data->i2c_master = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c_master) {
		SYS_LOG_ERR("i2c master not found: %s",
			   config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (bmc150_magn_init_chip(dev) < 0) {
		SYS_LOG_ERR("failed to initialize chip");
		return -EIO;
	}

#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	if (bmc150_magn_init_interrupt(dev) < 0) {
		SYS_LOG_ERR("failed to initialize interrupts");
		return -EINVAL;
	}
#endif
	return 0;
}

static const struct bmc150_magn_config bmc150_magn_config = {
	.i2c_master_dev_name = CONFIG_BMC150_MAGN_I2C_MASTER_DEV_NAME,
	.i2c_slave_addr = BMC150_MAGN_I2C_ADDR,
#if defined(CONFIG_BMC150_MAGN_TRIGGER_DRDY)
	.gpio_drdy_dev_name = CONFIG_BMC150_MAGN_GPIO_DRDY_DEV_NAME,
	.gpio_drdy_int_pin = CONFIG_BMC150_MAGN_GPIO_DRDY_INT_PIN,
#endif
};

static struct bmc150_magn_data bmc150_magn_data;

DEVICE_AND_API_INIT(bmc150_magn, CONFIG_BMC150_MAGN_DEV_NAME, bmc150_magn_init,
	    &bmc150_magn_data, &bmc150_magn_config, POST_KERNEL,
	    CONFIG_SENSOR_INIT_PRIORITY, &bmc150_magn_api_funcs);
