/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Darshan Maru <darshan.maru@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver implementation for the AMS TCS34725 RGB sensor.
 *
 * Zephyr driver for the AMS TCS34725 RGB Color Light-to-Digital Converter.
 *
 * Register semantics: ams TCS3472 datasheet [v1-02] 2016-Feb-08.
 * Lux / Color-Temperature algorithm: ams Application Note DN40-Rev 1.0.
 */

#define DT_DRV_COMPAT ams_tcs34725

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#include "tcs34725.h"

LOG_MODULE_REGISTER(TCS34725, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Write a single byte to a TCS34725 register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param reg Register address (5-bit register offset).
 * @param val Value to write.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int tcs34725_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct tcs34725_config *cfg = dev->config;
	uint8_t cmd = TCS34725_CMD_BIT | TCS34725_CMD_TYPE_REPEAT | (reg & 0x1F);

	return i2c_reg_write_byte_dt(&cfg->i2c, cmd, val);
}

/**
 * @brief Read a single byte from a TCS34725 register.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg Register address (5-bit register offset).
 * @param[out] val Pointer to store the byte read from the register.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int tcs34725_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct tcs34725_config *cfg = dev->config;
	uint8_t cmd = TCS34725_CMD_BIT | TCS34725_CMD_TYPE_REPEAT | (reg & 0x1F);

	return i2c_reg_read_byte_dt(&cfg->i2c, cmd, val);
}

/**
 * @brief Read a 16-bit little-endian value starting at a TCS34725 register.
 *
 * Uses auto-increment addressing to read two consecutive registers
 * (low byte then high byte) in a single I2C transaction.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 * @param[in] reg Starting register address (5-bit register offset).
 * @param[out] val Pointer to store the assembled 16-bit value.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int tcs34725_reg_read16(const struct device *dev, uint8_t reg, uint16_t *val)
{
	const struct tcs34725_config *cfg = dev->config;
	uint8_t cmd = TCS34725_CMD_BIT | TCS34725_CMD_TYPE_AUTOINC | (reg & 0x1F);
	uint8_t buf[2];
	int ret;

	ret = i2c_write_read_dt(&cfg->i2c, &cmd, 1, buf, 2);
	if (ret < 0) {
		return ret;
	}

	*val = sys_get_le16(buf);
	return 0;
}

/**
 * @brief Power on and enable the TCS34725 RGBC ADC.
 *
 * Sets the PON bit, waits for the internal oscillator warm-up period,
 * then sets AEN (and WEN if wait time is configured in devicetree) to
 * start RGBC conversions.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success.
 * @retval -errno Negative error code from the I2C API.
 */
static int tcs34725_enable(const struct device *dev)
{
	const struct tcs34725_config *cfg = dev->config;
	int ret;
	uint8_t enable_val = TCS34725_ENABLE_PON;

	ret = tcs34725_reg_write(dev, TCS34725_REG_ENABLE, enable_val);
	if (ret < 0) {
		return ret;
	}
	k_sleep(K_MSEC(TCS34725_PON_WARMUP_MS));

	enable_val |= TCS34725_ENABLE_AEN;
	if (cfg->wait_enable) {
		enable_val |= TCS34725_ENABLE_WEN;
	}

	return tcs34725_reg_write(dev, TCS34725_REG_ENABLE, enable_val);
}

/**
 * @brief Convert the gain register field to its analog gain multiplier.
 *
 * @param gain_reg Raw gain register value (only bits [1:0] are used).
 *
 * @return Analog gain multiplier in x (1x, 4x, 16x, or 60x).
 */
static float tcs34725_again_value(uint8_t gain_reg)
{
	switch (gain_reg & 0x03) {
	case 0:
		return 1;
	case 1:
		return 4;
	case 2:
		return 16;
	case 3:
		return 60;
	default:
		return 1;
	}
}

/**
 * @brief Compute the RGBC saturation threshold for an integration time.
 *
 * @param atime Raw ATIME register value.
 *
 * @return Saturation count threshold (capped at 65535). Clear channel
 *         readings at or above this value are considered saturated.
 */
static uint16_t tcs34725_saturation(uint8_t atime)
{
	uint16_t cycles = 256 - atime;

	if (cycles > 63) {
		return UINT16_MAX;
	}
	return (uint16_t)(1024 * cycles);
}

/**
 * @brief Calculate illuminance and correlated color temperature.
 *
 * Implements the ams DN40 lux/CT algorithm using the raw RGBC channel
 * counts and the current integration time and gain settings. Uses
 * Q16.16 fixed-point arithmetic (no floating-point), so it carries no
 * FPU / CONFIG_FPU_SHARING requirement even when called from an RTIO
 * worker thread (CONFIG_SENSOR_ASYNC_API).
 *
 * @param r Raw red channel count, in ADC counts.
 * @param g Raw green channel count, in ADC counts.
 * @param b Raw blue channel count, in ADC counts.
 * @param c Raw clear channel count, in ADC counts.
 * @param atime Raw ATIME register value used for the sample.
 * @param gain_reg Raw gain register value used for the sample.
 * @param[out] lux_out Calculated illuminance in lux (0 if invalid).
 * @param[out] ct_out Calculated correlated color temperature in Kelvin (0 if invalid).
 *
 * @retval 0 Success.
 * @retval -EAGAIN Clear channel saturated; result not usable.
 * @retval -ERANGE Signal too low, geometrically degenerate for the
 *                 DN40 formula (negative lux/CT, near-zero red channel),
 *                 or the computed lux/CT does not fit in a uint16_t.
 */
static int tcs34725_calculate_lux_ct(uint16_t r, uint16_t g, uint16_t b, uint16_t c, uint8_t atime,
				     uint8_t gain_reg, uint16_t *lux_out, uint16_t *ct_out)
{
	int64_t ir_fx, rp_fx, gp_fx, bp_fx;
	int64_t cpl_fx, atime_ms_fx;
	int64_t lux_fx, ct_fx, ratio_fx;
	int64_t term_r, term_g, term_b, lux_num_fx;
	uint8_t again;
	uint16_t sat;

	*lux_out = 0;
	*ct_out = 0;

	/* DN40 3.5: clear channel saturates first (R+G+B ~= C) */
	sat = tcs34725_saturation(atime);
	if (c >= sat) {
		return -EAGAIN;
	}

	/* DN40 3.14: low counts make the result noisy/unstable */
	if (c < TCS34725_DN40_MIN_CLEAR) {
		return -ERANGE;
	}

	/* DN40 3.1: IR rejection. (r+g+b-c) is exact; the /2 keeps any
	 * half-count fraction exactly representable in Q16.16.
	 */
	ir_fx = ((int64_t)r + g + b - c) * TCS34725_FX_ONE / 2;
	rp_fx = (int64_t)r * TCS34725_FX_ONE - ir_fx;
	gp_fx = (int64_t)g * TCS34725_FX_ONE - ir_fx;
	bp_fx = (int64_t)b * TCS34725_FX_ONE - ir_fx;

	if (rp_fx <= 0) {
		/* R' is the CCT denominator; can't produce a valid ratio */
		return -ERANGE;
	}

	/* DN40 3.2: CPL, with GA = 1 (no glass/cover) */
	again = tcs34725_again_value(gain_reg);
	atime_ms_fx = TCS34725_DN40_ATIME_STEP_FX * (int64_t)(256 - atime);
	cpl_fx = (atime_ms_fx * again * TCS34725_FX_ONE) / TCS34725_DN40_DGF_FX;

	if (cpl_fx <= 0) {
		return -ERANGE;
	}

	/* DN40 3.2: Lux */
	term_r = (TCS34725_DN40_R_COEF_FX * rp_fx) >> TCS34725_FX_SHIFT;
	term_g = (TCS34725_DN40_G_COEF_FX * gp_fx) >> TCS34725_FX_SHIFT;
	term_b = (TCS34725_DN40_B_COEF_FX * bp_fx) >> TCS34725_FX_SHIFT;
	lux_num_fx = term_r + term_g + term_b;

	lux_fx = (lux_num_fx * TCS34725_FX_ONE) / cpl_fx;

	if (lux_fx < 0 || lux_fx > ((int64_t)UINT16_MAX << TCS34725_FX_SHIFT)) {
		return -ERANGE;
	}

	/* DN40 3.4: Color temperature */
	ratio_fx = (bp_fx * TCS34725_FX_ONE) / rp_fx;
	ct_fx = ((TCS34725_DN40_CT_COEF_FX * ratio_fx) >> TCS34725_FX_SHIFT) +
		TCS34725_DN40_CT_OFFSET_FX;

	if (ct_fx < 0 || ct_fx > ((int64_t)UINT16_MAX << TCS34725_FX_SHIFT)) {
		return -ERANGE;
	}

	*lux_out = (uint16_t)((lux_fx + TCS34725_FX_ONE / 2) >> TCS34725_FX_SHIFT);
	*ct_out = (uint16_t)((ct_fx + TCS34725_FX_ONE / 2) >> TCS34725_FX_SHIFT);

	return 0;
}

static int tcs34725_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_GAIN:
		/* Valid AGAIN values: 0=1x, 1=4x, 2=16x, 3=60x */
		if (val->val1 < 0 || val->val1 > 3) {
			return -EINVAL;
		}
		return tcs34725_reg_write(dev, TCS34725_REG_CONTROL, val->val1 & 0x03);

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		/* ATIME register accepts 0-255 */
		if (val->val1 < 0 || val->val1 > 255) {
			return -EINVAL;
		}
		return tcs34725_reg_write(dev, TCS34725_REG_ATIME, (uint8_t)val->val1);

	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Block until the RGBC data is valid or a timeout expires.
 *
 * Polls the STATUS register for the AVALID bit, sleeping briefly
 * between reads.
 *
 * @param[in] dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 Success, AVALID bit set.
 * @retval -ETIMEDOUT AVALID not set within 1000 ms.
 * @retval -errno Negative error code from the I2C API.
 */
static int tcs34725_wait_valid(const struct device *dev)
{
	uint8_t status;
	int ret;
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(1000));

	do {
		ret = tcs34725_reg_read(dev, TCS34725_REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}
		if (status & TCS34725_STATUS_AVALID) {
			return 0;
		}
		k_sleep(K_MSEC(3));
	} while (!sys_timepoint_expired(end));

	LOG_WRN("Timed out waiting for AVALID");
	return -ETIMEDOUT;
}

static int tcs34725_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tcs34725_config *cfg = dev->config;
	struct tcs34725_data *data = dev->data;
	int ret;
	int lux_ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_RED && chan != SENSOR_CHAN_GREEN &&
	    chan != SENSOR_CHAN_BLUE && chan != SENSOR_CHAN_LIGHT &&
	    chan != (enum sensor_channel)SENSOR_CHAN_TCS34725_LUX &&
	    chan != (enum sensor_channel)SENSOR_CHAN_TCS34725_COLOR_TEMP) {
		return -ENOTSUP;
	}

	ret = tcs34725_wait_valid(dev);
	if (ret < 0) {
		return ret;
	}

	ret = tcs34725_reg_read16(dev, TCS34725_REG_CDATAL, &data->clear);
	if (ret < 0) {
		return ret;
	}
	ret = tcs34725_reg_read16(dev, TCS34725_REG_RDATAL, &data->red);
	if (ret < 0) {
		return ret;
	}
	ret = tcs34725_reg_read16(dev, TCS34725_REG_GDATAL, &data->green);
	if (ret < 0) {
		return ret;
	}
	ret = tcs34725_reg_read16(dev, TCS34725_REG_BDATAL, &data->blue);
	if (ret < 0) {
		return ret;
	}

	/* lux/CT validity is stored in data->lux_valid for channel_get() */
	lux_ret = tcs34725_calculate_lux_ct(data->red, data->green, data->blue, data->clear,
					    cfg->atime, cfg->gain, &data->lux, &data->color_temp);
	data->lux_valid = (lux_ret == 0);

	if (lux_ret == -EAGAIN) {
		LOG_WRN("RGBC saturated (C=%u) - lux/CT invalid, reduce ATIME or gain",
			data->clear);
	} else if (lux_ret == -ERANGE) {
		LOG_DBG("Signal too low/degenerate (C=%u) for reliable lux/CT", data->clear);
	}

	return 0;
}

static int tcs34725_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct tcs34725_data *data = dev->data;
	uint16_t raw;

	val->val2 = 0;

	switch ((int)chan) {
	case SENSOR_CHAN_RED:
		raw = data->red;
		break;
	case SENSOR_CHAN_GREEN:
		raw = data->green;
		break;
	case SENSOR_CHAN_BLUE:
		raw = data->blue;
		break;
	case SENSOR_CHAN_LIGHT:
		raw = data->clear;
		break;
	case SENSOR_CHAN_TCS34725_LUX:
		raw = data->lux;
		/* val2 = 1 if DN40 lux/CT calculation was valid, 0 if saturated or too dim */
		val->val2 = data->lux_valid ? 1 : 0;
		break;
	case SENSOR_CHAN_TCS34725_COLOR_TEMP:
		raw = data->color_temp;
		/* val2 = 1 if DN40 lux/CT calculation was valid, 0 if saturated or too dim */
		val->val2 = data->lux_valid ? 1 : 0;
		break;
	default:
		return -ENOTSUP;
	}

	val->val1 = raw;

	return 0;
}

static int tcs34725_init(const struct device *dev)
{
	const struct tcs34725_config *cfg = dev->config;
	uint8_t id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(dev);
		return -ENODEV;
	}

	ret = tcs34725_reg_read(dev, TCS34725_REG_ID, &id);
	if (ret < 0) {
		LOG_ERR("Failed to read ID register (%d)", ret);
		return ret;
	}

	if (id != TCS34725_ID_34721_34725 && id != TCS34725_ID_34723_34727) {
		LOG_ERR("Unexpected device ID 0x%02x", id);
		return -ENODEV;
	}

	/* Set integration time (ATIME) and gain (CONTROL) before enabling */
	ret = tcs34725_reg_write(dev, TCS34725_REG_ATIME, cfg->atime);
	if (ret < 0) {
		return ret;
	}

	ret = tcs34725_reg_write(dev, TCS34725_REG_CONTROL, cfg->gain & 0x03);
	if (ret < 0) {
		return ret;
	}

	/* Wait-state config, written before enabling WEN */
	if (cfg->wait_enable) {
		ret = tcs34725_reg_write(dev, TCS34725_REG_WTIME, cfg->wtime);
		if (ret < 0) {
			return ret;
		}
		ret = tcs34725_reg_write(dev, TCS34725_REG_CONFIG,
					 cfg->wlong ? TCS34725_CONFIG_WLONG : 0);
		if (ret < 0) {
			return ret;
		}
	}

	/* PON -> warmup -> AEN (+WEN if configured), per datasheet system-state note */
	ret = tcs34725_enable(dev);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("TCS34725 initialized (ID=0x%02x)", id);
	return 0;
}

static DEVICE_API(sensor, tcs34725_driver_api) = {
	.attr_set = tcs34725_attr_set,
	.sample_fetch = tcs34725_sample_fetch,
	.channel_get = tcs34725_channel_get,
};

#define TCS34725_INIT(inst)                                                                        \
	static struct tcs34725_data tcs34725_data_##inst;                                          \
	static const struct tcs34725_config tcs34725_config_##inst = {                             \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.gain = DT_INST_PROP(inst, gain),                                                  \
		.atime = DT_INST_PROP(inst, atime),                                                \
		.wtime = DT_INST_PROP(inst, wtime),                                                \
		.wlong = DT_INST_PROP(inst, wlong),                                                \
		.wait_enable = DT_INST_PROP(inst, wait_enable),                                    \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tcs34725_init, NULL, &tcs34725_data_##inst,             \
				     &tcs34725_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &tcs34725_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TCS34725_INIT)
