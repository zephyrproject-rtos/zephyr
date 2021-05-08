/* bmm150.c - Driver for Bosch BMM150 Geomagnetic Sensor */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bmm150

#include <logging/log.h>
#include "bmm150.h"

LOG_MODULE_REGISTER(BMM150, CONFIG_SENSOR_LOG_LEVEL);

static const struct {
	int freq;
	uint8_t reg_val;
} bmm150_samp_freq_table[] = { { 2, 0x01 },
			       { 6, 0x02 },
			       { 8, 0x03 },
			       { 10, 0x00 },
			       { 15, 0x04 },
			       { 20, 0x05 },
			       { 25, 0x06 },
			       { 30, 0x07 } };

static const struct bmm150_preset {
	uint8_t rep_xy;
	uint8_t rep_z;
	uint8_t odr;
} bmm150_presets_table[] = {
	[BMM150_LOW_POWER_PRESET] = { 3, 3, 10 },
	[BMM150_REGULAR_PRESET] = { 9, 15, 10 },
	[BMM150_ENHANCED_REGULAR_PRESET] = { 15, 27, 10 },
	[BMM150_HIGH_ACCURACY_PRESET] = { 47, 83, 20 }
};

static int bmm150_set_power_mode(const struct device *dev,
				 enum bmm150_power_modes mode,
				 int state)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;

	switch (mode) {
	case BMM150_POWER_MODE_SUSPEND:
		if (i2c_reg_update_byte(data->i2c,
					config->i2c_slave_addr,
					BMM150_REG_POWER,
					BMM150_MASK_POWER_CTL,
					!state) < 0) {
			return -EIO;
		}
		k_busy_wait(USEC_PER_MSEC * 5U);

		return 0;
	case BMM150_POWER_MODE_SLEEP:
		return i2c_reg_update_byte(data->i2c,
					   config->i2c_slave_addr,
					   BMM150_REG_OPMODE_ODR,
					   BMM150_MASK_OPMODE,
					   BMM150_MODE_SLEEP <<
					   BMM150_SHIFT_OPMODE);
		break;
	case BMM150_POWER_MODE_NORMAL:
		return i2c_reg_update_byte(data->i2c,
					   config->i2c_slave_addr,
					   BMM150_REG_OPMODE_ODR,
					   BMM150_MASK_OPMODE,
					   BMM150_MODE_NORMAL <<
					   BMM150_SHIFT_OPMODE);
		break;
	}

	return -ENOTSUP;

}

static int bmm150_set_odr(const struct device *dev, uint8_t val)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;
	uint8_t i;

	for (i = 0U; i < ARRAY_SIZE(bmm150_samp_freq_table); ++i) {
		if (val <= bmm150_samp_freq_table[i].freq) {
			return i2c_reg_update_byte(data->i2c,
						   config->i2c_slave_addr,
						   BMM150_REG_OPMODE_ODR,
						   BMM150_MASK_ODR,
						   (bmm150_samp_freq_table[i].
						    reg_val <<
						    BMM150_SHIFT_ODR));
		}
	}
	return -ENOTSUP;
}

#if defined(BMM150_SET_ATTR)
static int bmm150_read_rep_xy(const struct device *dev)
{
	struct bmm150_data *data = dev->driver->data;
	const struct bmm150_config *config = dev->config;
	uint8_t reg_val;

	if (i2c_reg_read_byte(data->i2c, config->i2c_slave_addr,
			      BMM150_REG_REP_XY, &reg_val) < 0) {
		return -EIO;
	}

	data->rep_xy = BMM150_REGVAL_TO_REPXY((uint8_t)(reg_val));

	return 0;
}

static int bmm150_read_rep_z(const struct device *dev)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;
	uint8_t reg_val;

	if (i2c_reg_read_byte(data->i2c, config->i2c_slave_addr,
			      BMM150_REG_REP_Z, &reg_val) < 0) {
		return -EIO;
	}

	data->rep_z = BMM150_REGVAL_TO_REPZ((int)(reg_val));

	return 0;
}

static int bmm150_compute_max_odr(const struct device *dev, int rep_xy,
				  int rep_z, int *max_odr)
{
	struct bmm150_data *data = dev->data;

	if (rep_xy == 0) {
		if (data->rep_xy <= 0) {
			if (bmm150_read_rep_xy(dev) < 0) {
				return -EIO;
			}
		}
		rep_xy = data->rep_xy;
	}

	if (rep_z == 0) {
		if (data->rep_z <= 0) {
			if (bmm150_read_rep_z(dev) < 0) {
				return -EIO;
			}
		}
		rep_z = data->rep_z;
	}

	/* Equation reference Datasheet 4.2.4 */
	*max_odr = 1000000 / (145 * rep_xy + 500 * rep_z + 980);

	return 0;
}
#endif

#if defined(BMM150_SET_ATTR_REP)
static int bmm150_read_odr(const struct device *dev)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;
	uint8_t i, odr_val, reg_val;

	if (i2c_reg_read_byte(data->i2c, config->i2c_slave_addr,
			      BMM150_REG_OPMODE_ODR, &reg_val) < 0) {
		return -EIO;
	}

	odr_val = (reg_val & BMM150_MASK_ODR) >> BMM150_SHIFT_ODR;

	for (i = 0U; i < ARRAY_SIZE(bmm150_samp_freq_table); ++i) {
		if (bmm150_samp_freq_table[i].reg_val == odr_val) {
			data->odr = bmm150_samp_freq_table[i].freq;
			return 0;
		}
	}

	return -ENOTSUP;
}
#endif

#if defined(CONFIG_BMM150_SAMPLING_REP_XY)
static int bmm150_write_rep_xy(const struct device *dev, int val)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c, config->i2c_slave_addr,
				BMM150_REG_REP_XY,
				BMM150_REG_REP_DATAMASK,
				BMM150_REPXY_TO_REGVAL(val)) < 0) {
		return -EIO;
	}

	data->rep_xy = val;

	return 0;
}
#endif

#if defined(CONFIG_BMM150_SAMPLING_REP_Z)
static int bmm150_write_rep_z(const struct device *dev, int val)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;

	if (i2c_reg_update_byte(data->i2c, config->i2c_slave_addr,
				BMM150_REG_REP_Z,
				BMM150_REG_REP_DATAMASK,
				BMM150_REPZ_TO_REGVAL(val)) < 0) {
		return -EIO;
	}

	data->rep_z = val;

	return 0;
}
#endif

/* Reference Datasheet 4.3.2 */
static int32_t bmm150_compensate_xy(struct bmm150_trim_regs *tregs,
				  int16_t xy, uint16_t rhall, bool is_x)
{
	int8_t txy1, txy2;
	int16_t val;
	uint16_t prevalue;
	int32_t temp1, temp2, temp3;

	if (xy == BMM150_XY_OVERFLOW_VAL) {
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

	prevalue = (uint16_t)((((int32_t)tregs->xyz1) << 14) / rhall);

	val = (int16_t)((prevalue) - ((uint16_t)0x4000));

	temp1 = (((int32_t)tregs->xy2) * ((((int32_t)val) * ((int32_t)val)) >> 7));

	temp2 = ((int32_t)val) * ((int32_t)(((int16_t)tregs->xy1) << 7));

	temp3 = (((((temp1 + temp2) >> 9) +
		((int32_t)0x100000)) * ((int32_t)(((int16_t)txy2) +
		((int16_t)0xA0)))) >> 12);

	val = ((int16_t)((((int32_t)xy) * temp3) >> 13)) + (((int16_t)txy1) << 3);

	return (int32_t)val;
}

static int32_t bmm150_compensate_z(struct bmm150_trim_regs *tregs,
				 int16_t z, uint16_t rhall)
{
	int32_t val, temp1, temp2;
	int16_t temp3;

	if (z == BMM150_Z_OVERFLOW_VAL) {
		return INT32_MIN;
	}

	temp1 = (((int32_t)(z - tregs->z4)) << 15);

	temp2 = ((((int32_t)tregs->z3) *
		((int32_t)(((int16_t)rhall) - ((int16_t)tregs->xyz1)))) >> 2);

	temp3 = ((int16_t)(((((int32_t)tregs->z1) *
		((((int16_t)rhall) << 1))) + (1 << 15)) >> 16));

	val = ((temp1 - temp2) / (tregs->z2 + temp3));

	return val;
}

static int bmm150_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{

	struct bmm150_data *drv_data = dev->data;
	const struct bmm150_config *config = dev->config;
	uint16_t values[BMM150_AXIS_XYZR_MAX];
	int16_t raw_x, raw_y, raw_z;
	uint16_t rhall;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL ||
			chan == SENSOR_CHAN_MAGN_XYZ);

	if (i2c_burst_read(drv_data->i2c, config->i2c_slave_addr,
			   BMM150_REG_X_L, (uint8_t *)values,
			   sizeof(values)) < 0) {
		LOG_ERR("failed to read sample");
		return -EIO;
	}

	raw_x = (int16_t)sys_le16_to_cpu(values[BMM150_AXIS_X]) >>
		BMM150_SHIFT_XY_L;
	raw_y = (int16_t)sys_le16_to_cpu(values[BMM150_AXIS_Y]) >>
		BMM150_SHIFT_XY_L;
	raw_z = (int16_t)sys_le16_to_cpu(values[BMM150_AXIS_Z]) >>
		BMM150_SHIFT_Z_L;

	rhall = sys_le16_to_cpu(values[BMM150_RHALL]) >>
		BMM150_SHIFT_RHALL_L;

	drv_data->sample_x = bmm150_compensate_xy(&drv_data->tregs,
							raw_x, rhall, true);
	drv_data->sample_y = bmm150_compensate_xy(&drv_data->tregs,
							raw_y, rhall, false);
	drv_data->sample_z = bmm150_compensate_z(&drv_data->tregs,
							raw_z, rhall);

	return 0;
}

/*
 * Datasheet specify raw units are 16 LSB/uT and this function converts it to
 * Gauss
 */
static void bmm150_convert(struct sensor_value *val, int raw_val)
{
	/* val = raw_val / 1600 */
	val->val1 = raw_val / 1600;
	val->val2 = ((int32_t)raw_val * (1000000 / 1600)) % 1000000;
}

static int bmm150_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bmm150_data *drv_data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		bmm150_convert(val, drv_data->sample_x);
		break;
	case SENSOR_CHAN_MAGN_Y:
		bmm150_convert(val, drv_data->sample_y);
		break;
	case SENSOR_CHAN_MAGN_Z:
		bmm150_convert(val, drv_data->sample_x);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		bmm150_convert(val, drv_data->sample_x);
		bmm150_convert(val + 1, drv_data->sample_y);
		bmm150_convert(val + 2, drv_data->sample_z);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#if defined(BMM150_SET_ATTR_REP)
static inline int bmm150_attr_set_rep(const struct device *dev,
				      enum sensor_channel chan,
				      const struct sensor_value *val)
{
	struct bmm150_data *data = dev->data;
	int max_odr;

	switch (chan) {
#if defined(CONFIG_BMM150_SAMPLING_REP_XY)
	case SENSOR_CHAN_MAGN_X:
	case SENSOR_CHAN_MAGN_Y:
		if (val->val1 < 1 || val->val1 > 511) {
			return -EINVAL;
		}

		if (bmm150_compute_max_odr(dev, val->val1, 0,
					   &max_odr) < 0) {
			return -EIO;
		}

		if (data->odr <= 0) {
			if (bmm150_read_odr(dev) < 0) {
				return -EIO;
			}
		}

		if (data->odr > max_odr) {
			return -EINVAL;
		}

		if (bmm150_write_rep_xy(dev, val->val1) < 0) {
			return -EIO;
		}
		break;
#endif

#if defined(CONFIG_BMM150_SAMPLING_REP_Z)
	case SENSOR_CHAN_MAGN_Z:
		if (val->val1 < 1 || val->val1 > 256) {
			return -EINVAL;
		}

		if (bmm150_compute_max_odr(dev, 0, val->val1,
					   &max_odr) < 0) {
			return -EIO;
		}

		if (data->odr <= 0) {
			if (bmm150_read_odr(dev) < 0) {
				return -EIO;
			}
		}

		if (data->odr > max_odr) {
			return -EINVAL;
		}

		if (bmm150_write_rep_z(dev, val->val1) < 0) {
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

#if defined(BMM150_SET_ATTR)
static int bmm150_attr_set(const struct device *dev,
			   enum sensor_channel chan,
			   enum sensor_attribute attr,
			   const struct sensor_value *val)
{
	struct bmm150_magn_data *data = dev->data;

	switch (attr) {
#if defined(CONFIG_BMM150_SAMPLING_RATE_RUNTIME)
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (data->max_odr <= 0) {
			if (bmm150_compute_max_odr(dev, 0, 0,
						   &data->max_odr) < 0) {
				return -EIO;
			}
		}

		if (data->max_odr < val->val1) {
			LOG_ERR("not supported with current oversampling");
			return -ENOTSUP;
		}

		if (bmm150_set_odr(dev, (uint8_t)(val->val1)) < 0) {
			return -EIO;
		}
		break;
#endif
#if defined(BMM150_SET_ATTR_REP)
	case SENSOR_ATTR_OVERSAMPLING:
		bmm150_attr_set_rep(dev, chan, val);
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}
#endif

static const struct sensor_driver_api bmm150_api_funcs = {
#if defined(BMM150_SET_ATTR)
	.attr_set = bmm150_attr_set,
#endif
	.sample_fetch = bmm150_sample_fetch,
	.channel_get = bmm150_channel_get,
};

static int bmm150_init_chip(const struct device *dev)
{
	struct bmm150_data *data = dev->data;
	const struct bmm150_config *config = dev->config;
	uint8_t chip_id;
	struct bmm150_preset preset;

	if (bmm150_set_power_mode(dev, BMM150_POWER_MODE_NORMAL, 0) < 0) {
		LOG_ERR("failed to bring up device from normal mode");
		return -EIO;
	}

	if (bmm150_set_power_mode(dev, BMM150_POWER_MODE_SUSPEND, 1) < 0) {
		LOG_ERR("failed to bring up device in suspend mode");
		return -EIO;
	}

	if (bmm150_set_power_mode(dev, BMM150_POWER_MODE_SUSPEND, 0)
	    < 0) {
		LOG_ERR("failed to bring up device from suspend mode");
		return -EIO;
	}

	if (i2c_reg_read_byte(data->i2c, config->i2c_slave_addr,
			      BMM150_REG_CHIP_ID, &chip_id) < 0) {
		LOG_ERR("failed reading chip id");
		goto err_poweroff;
	}

	if (chip_id != BMM150_CHIP_ID_VAL) {
		LOG_ERR("invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}

	preset = bmm150_presets_table[BMM150_DEFAULT_PRESET];
	if (bmm150_set_odr(dev, preset.odr) < 0) {
		LOG_ERR("failed to set ODR to %d",
			    preset.odr);
		goto err_poweroff;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_slave_addr,
			       BMM150_REG_REP_XY,
			       BMM150_REPXY_TO_REGVAL(preset.rep_xy))
	    < 0) {
		LOG_ERR("failed to set REP XY to %d",
			    preset.rep_xy);
		goto err_poweroff;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_slave_addr,
			       BMM150_REG_REP_Z,
			       BMM150_REPZ_TO_REGVAL(preset.rep_z)) < 0) {
		LOG_ERR("failed to set REP Z to %d",
			    preset.rep_z);
		goto err_poweroff;
	}

	if (bmm150_set_power_mode(dev, BMM150_POWER_MODE_NORMAL, 1)
	    < 0) {
		LOG_ERR("failed to power on device");
	}

	if (i2c_burst_read(data->i2c, config->i2c_slave_addr,
			   BMM150_REG_TRIM_START, (uint8_t *)&data->tregs,
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
	bmm150_set_power_mode(dev, BMM150_POWER_MODE_NORMAL, 0);
	bmm150_set_power_mode(dev, BMM150_POWER_MODE_SUSPEND, 1);
	return -EIO;
}

static int bmm150_init(const struct device *dev)
{
	const struct bmm150_config *const config =
		dev->config;
	struct bmm150_data *data = dev->data;

	data->i2c = device_get_binding(config->i2c_master_dev_name);
	if (!data->i2c) {
		LOG_ERR("i2c master not found: %s",
			    config->i2c_master_dev_name);
		return -EINVAL;
	}

	if (bmm150_init_chip(dev) < 0) {
		LOG_ERR("failed to initialize chip");
		return -EIO;
	}

	return 0;
}

static const struct bmm150_config bmm150_config = {
	.i2c_master_dev_name = DT_INST_BUS_LABEL(0),
	.i2c_slave_addr = BMM150_I2C_ADDR,
};

static struct bmm150_data bmm150_data;

DEVICE_DT_INST_DEFINE(0, bmm150_init, NULL,
			&bmm150_data, &bmm150_config, POST_KERNEL,
			CONFIG_SENSOR_INIT_PRIORITY, &bmm150_api_funcs);
