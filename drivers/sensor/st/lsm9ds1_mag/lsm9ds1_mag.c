/*
 * Copyright (c) 2024 Bootlin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm9ds1_mag

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "lsm9ds1_mag.h"
#include <stmemsc.h>

LOG_MODULE_REGISTER(LSM9DS1_MAG, CONFIG_SENSOR_LOG_LEVEL);

/* Sensitivity of the magnetometer, indexed by the raw full scale value. Unit is µGauss / LSB */
static const uint16_t lsm9ds1_mag_fs_sens[] = {140, 290, 430, 580};
/*
 * Values of the different sampling frequencies of the magnetometer, indexed by the raw odr value,
 * that the sensor can understand. Unit : Hz.
 * Warning : the real values of the sampling frequencies are often not an integer.
 * For instance, the "0" on this array is not 0 Hz but 0.625 Hz
 */
static const uint16_t lsm9ds1_mag_odr_map[] = {0, 1, 2, 5, 10, 20, 40, 80};
/*
 * Values of the sampling frequencies of the magnetometer while in "fast odr" mode, indexed by the
 * raw odr value. Unit : Hz.
 */
static const uint16_t lsm9ds1_mag_fast_odr_map[] = {1000, 560, 300, 155};

static int lsm9ds1_mag_reboot(const struct device *dev)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	lsm9ds1_ctrl_reg2_m_t ctrl_reg2;
	int ret;

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG2_M, (uint8_t *)&ctrl_reg2, 1);
	if (ret < 0) {
		return ret;
	}

	ctrl_reg2.reboot = 1;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG2_M, (uint8_t *)&ctrl_reg2, 1);
	if (ret < 0) {
		return ret;
	}

	k_busy_wait(USEC_PER_MSEC * 50U);

	return 0;
}

static int lsm9ds1_mag_range_to_fs_val(int32_t range)
{
	switch (range) {
	case 4:
		return LSM9DS1_4Ga;
	case 8:
		return LSM9DS1_8Ga;
	case 12:
		return LSM9DS1_12Ga;
	case 16:
		return LSM9DS1_16Ga;
	default:
		return -EINVAL;
	}
}

static int lsm9ds1_mag_range_set(const struct device *dev, int32_t range)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_mag_data *data = dev->data;
	int fs, ret;

	fs = lsm9ds1_mag_range_to_fs_val(range);

	ret = lsm9ds1_mag_full_scale_set(ctx, fs);
	if (ret < 0) {
		return ret;
	}

	data->mag_gain = lsm9ds1_mag_fs_sens[fs];

	return 0;
}

static int lsm9ds1_mag_freq_to_odr_val(uint16_t freq)
{
	for (int i = 0; i < ARRAY_SIZE(lsm9ds1_mag_odr_map); i++) {
		if (freq <= lsm9ds1_mag_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lsm9ds1_mag_freq_to_fast_odr_val(uint16_t freq)
{
	for (int i = ARRAY_SIZE(lsm9ds1_mag_fast_odr_map) - 1; i >= 0; i--) {
		if (freq <= lsm9ds1_mag_fast_odr_map[i]) {
			return i;
		}
	}

	return -EINVAL;
}

static int lsm9ds1_mag_odr_set_normal(const struct device *dev, uint16_t freq)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_mag_data *data = dev->data;
	lsm9ds1_ctrl_reg1_m_t ctrl_reg1_m;
	lsm9ds1_ctrl_reg4_m_t ctrl_reg4_m;
	int odr, ret;

	odr = lsm9ds1_mag_freq_to_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t *)&ctrl_reg1_m, 1);
	if (ret < 0) {
		return ret;
	}

	if (ctrl_reg1_m.fast_odr) { /* restore the operating mode */
		ctrl_reg1_m.om = data->old_om;

		ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4_M, (uint8_t *)&ctrl_reg4_m, 1);
		if (ret < 0) {
			return ret;
		}

		ctrl_reg4_m.omz = data->old_om;

		ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG4_M, (uint8_t *)&ctrl_reg4_m, 1);
		if (ret < 0) {
			return ret;
		}
	}

	ctrl_reg1_m._do = odr;
	ctrl_reg1_m.fast_odr = 0;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t *)&ctrl_reg1_m, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int lsm9ds1_mag_fast_odr_set(const struct device *dev, uint16_t freq)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_mag_data *data = dev->data;
	lsm9ds1_ctrl_reg1_m_t ctrl_reg1_m;
	lsm9ds1_ctrl_reg4_m_t ctrl_reg4_m;
	int odr, ret;

	odr = lsm9ds1_mag_freq_to_fast_odr_val(freq);
	if (odr < 0) {
		return odr;
	}

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t *)&ctrl_reg1_m, 1);
	if (ret < 0) {
		return ret;
	}

	if (!ctrl_reg1_m.fast_odr) { /* preserve the operating mode */
		data->old_om = ctrl_reg1_m.om;
	}

	ctrl_reg1_m._do = 0;
	ctrl_reg1_m.fast_odr = 1;
	ctrl_reg1_m.om = odr;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG1_M, (uint8_t *)&ctrl_reg1_m, 1);
	if (ret < 0) {
		return ret;
	}

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG4_M, (uint8_t *)&ctrl_reg4_m, 1);
	if (ret < 0) {
		return ret;
	}

	ctrl_reg4_m.omz = odr;

	ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG4_M, (uint8_t *)&ctrl_reg4_m, 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int lsm9ds1_mag_odr_set(const struct device *dev, const struct sensor_value *val)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_mag_data *data = dev->data;
	int ret;
	lsm9ds1_ctrl_reg3_m_t ctrl_reg3_m;

	if (val->val1 == 0 && val->val2 == 0) { /* We want to power down the sensor */
		ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_M, (uint8_t *)&ctrl_reg3_m, 1);
		if (ret < 0) {
			return ret;
		}

		ctrl_reg3_m.md = LSM9DS1_MAG_POWER_DOWN_VALUE;

		ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_M, (uint8_t *)&ctrl_reg3_m, 1);
		if (ret < 0) {
			return ret;
		}

		data->powered_down = 1;

		return 0;
	}

	if (data->powered_down) {
		ret = lsm9ds1_read_reg(ctx, LSM9DS1_CTRL_REG3_M, (uint8_t *)&ctrl_reg3_m, 1);
		if (ret < 0) {
			return ret;
		}

		ctrl_reg3_m.md = 0;

		ret = lsm9ds1_write_reg(ctx, LSM9DS1_CTRL_REG3_M, (uint8_t *)&ctrl_reg3_m, 1);
		if (ret < 0) {
			return ret;
		}

		data->powered_down = 0;
	}

	if (val->val1 <= MAX_NORMAL_ODR) {
		return lsm9ds1_mag_odr_set_normal(dev, val->val1);
	}

	return lsm9ds1_mag_fast_odr_set(dev, val->val1);
}

static int lsm9ds1_mag_attr_set(const struct device *dev, enum sensor_channel chan,
				enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_MAGN_XYZ) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_FULL_SCALE:
		return lsm9ds1_mag_range_set(dev, val->val1);
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return lsm9ds1_mag_odr_set(dev, val);
	default:
		return -ENOTSUP;
	}
}

static int lsm9ds1_mag_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_mag_data *data = dev->data;
	int ret;

	if (chan == SENSOR_CHAN_MAGN_XYZ || chan == SENSOR_CHAN_ALL) {
		ret = lsm9ds1_magnetic_raw_get(ctx, data->mag);
		if (ret < 0) {
			LOG_DBG("failed to read sample");
			return -EIO;
		}

		return 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static inline void lsm9ds1_mag_convert(struct sensor_value *val, int raw_val, uint32_t sensitivity)
{
	int64_t dval;

	/* sensitivity is exposed in µGauss/LSB. */
	dval = (int64_t)(raw_val) * sensitivity;
	val->val1 = (int32_t)(dval / 1000000);
	val->val2 = (int32_t)(dval % 1000000);
}

static int lsm9ds1_mag_channel_get(const struct device *dev, enum sensor_channel chan,
				   struct sensor_value *val)
{
	struct lsm9ds1_mag_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_MAGN_X:
		lsm9ds1_mag_convert(val, data->mag[0], data->mag_gain);
		break;
	case SENSOR_CHAN_MAGN_Y:
		lsm9ds1_mag_convert(val, data->mag[1], data->mag_gain);
		break;
	case SENSOR_CHAN_MAGN_Z:
		lsm9ds1_mag_convert(val, data->mag[2], data->mag_gain);
		break;
	case SENSOR_CHAN_MAGN_XYZ:
		for (int i = 0; i < 3; i++) {
			lsm9ds1_mag_convert(val++, data->mag[i], data->mag_gain);
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, lsm9ds1_mag_api_funcs) = {
	.sample_fetch = lsm9ds1_mag_sample_fetch,
	.channel_get = lsm9ds1_mag_channel_get,
	.attr_set = lsm9ds1_mag_attr_set,
};

static int lsm9ds1_mag_init(const struct device *dev)
{
	const struct lsm9ds1_mag_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	struct lsm9ds1_mag_data *data = dev->data;
	uint8_t chip_id, fs;
	int ret;

	ret = lsm9ds1_mag_reboot(dev);
	if (ret < 0) {
		LOG_DBG("Failed to reboot device");
		return ret;
	}

	ret = lsm9ds1_read_reg(ctx, LSM9DS1_WHO_AM_I_M, &chip_id, 1);
	if (ret < 0) {
		LOG_DBG("failed reading chip id");
		return ret;
	}

	if (chip_id != LSM9DS1_MAG_ID) {
		LOG_DBG("Invalid ID : got 0x%x", chip_id);
		return -EIO;
	}
	LOG_INF("mag chip_id : 0x%x\n", chip_id);

	ret = lsm9ds1_mag_data_rate_set(ctx, cfg->mag_odr);
	if (ret < 0) {
		LOG_ERR("failed to set the odr");
		return ret;
	}

	if (cfg->mag_odr == LSM9DS1_MAG_POWER_DOWN) {
		data->powered_down = 1;
	}

	fs = cfg->mag_range;
	ret = lsm9ds1_mag_full_scale_set(ctx, fs);
	if (ret < 0) {
		LOG_ERR("failed to set magnetometer range");
		return ret;
	}

	data->mag_gain = lsm9ds1_mag_fs_sens[fs];

	return 0;
}

#define LSM9DS1_MAG_CONFIG_COMMON(inst)                                                            \
	.mag_odr = DT_INST_PROP(inst, mag_odr), .mag_range = DT_INST_PROP(inst, mag_range),

/*
 * Instantiation macros used when a device is on an I2C bus.
 */

#define LSM9DS1_MAG_CONFIG_I2C(inst)                                                               \
	{                                                                                          \
		STMEMSC_CTX_I2C(&lsm9ds1_mag_config_##inst.stmemsc_cfg),                           \
		.stmemsc_cfg =                                                                     \
			{                                                                          \
				.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
			},                                                                         \
		LSM9DS1_MAG_CONFIG_COMMON(inst)                                                    \
	}

#define LSM9DS1_MAG_DEFINE(inst)                                                                   \
	static struct lsm9ds1_mag_data lsm9ds1_mag_data_##inst = {.mag_gain = 0,                   \
								  .powered_down = 0};              \
                                                                                                   \
	static struct lsm9ds1_mag_config lsm9ds1_mag_config_##inst = LSM9DS1_MAG_CONFIG_I2C(inst); \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lsm9ds1_mag_init, NULL, &lsm9ds1_mag_data_##inst,       \
				     &lsm9ds1_mag_config_##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &lsm9ds1_mag_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(LSM9DS1_MAG_DEFINE);
