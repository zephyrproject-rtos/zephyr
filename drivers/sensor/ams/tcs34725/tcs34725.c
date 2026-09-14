/*
 * Copyright (c) 2026 Dotcom IoT LLP
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Darshan Maru <darshan.maru@dnkmail.in>
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
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register address (5-bit register offset).
 * @param val Value to write to the register.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code from the I2C API on failure.
 */
int tcs34725_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct tcs34725_config *cfg = dev->config;
	uint8_t cmd = TCS34725_CMD_BIT | TCS34725_CMD_TYPE_REPEAT | (reg & 0x1F);

	return i2c_reg_write_byte_dt(&cfg->i2c, cmd, val);
}

/**
 * @brief Read a single byte from a TCS34725 register.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Register address (5-bit register offset).
 * @param[out] val Pointer to store the byte read from the register.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code from the I2C API on failure.
 */
int tcs34725_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct tcs34725_config *cfg = dev->config;
	uint8_t cmd = TCS34725_CMD_BIT | TCS34725_CMD_TYPE_REPEAT | (reg & 0x1F);

	return i2c_reg_read_byte_dt(&cfg->i2c, cmd, val);
}

/**
 * @brief Read a 16-bit (little-endian) value starting at a TCS34725 register.
 *
 * Uses auto-increment addressing to read two consecutive registers
 * (low byte then high byte) in a single I2C transaction.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param reg Starting register address (5-bit register offset).
 * @param[out] val Pointer to store the assembled 16-bit value.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code from the I2C API on failure.
 */
int tcs34725_reg_read16(const struct device *dev, uint8_t reg, uint16_t *val)
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
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code on I2C failure.
 */
int tcs34725_enable(const struct device *dev)
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
 * @brief Disable the TCS34725 (power down).
 *
 * Clears the ENABLE register, powering down the ADC and oscillator.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success.
 * @retval -errno Negative error code on I2C failure.
 */
int tcs34725_disable(const struct device *dev)
{
	return tcs34725_reg_write(dev, TCS34725_REG_ENABLE, 0x00);
}

/**
 * @brief Convert the ATIME/gain register field to its analog gain multiplier.
 *
 * @param gain_reg Raw gain register value (only bits [1:0] are used).
 *
 * @return Analog gain multiplier (1.0, 4.0, 16.0, or 60.0).
 */
static float tcs34725_again_value(uint8_t gain_reg)
{
	switch (gain_reg & 0x03) {
	case 0:
		return 1.0f;
	case 1:
		return 4.0f;
	case 2:
		return 16.0f;
	case 3:
		return 60.0f;
	default:
		return 1.0f;
	}
}

/**
 * @brief Compute the RGBC saturation threshold for a given integration time.
 *
 * @param atime Raw ATIME register value.
 *
 * @return Saturation count threshold; the clear channel is considered
 *         saturated at or above this value (capped at 65535).
 */
static uint16_t tcs34725_saturation(uint8_t atime)
{
	uint16_t cycles = 256 - atime;

	if (cycles > 63) {
		/* Digital saturation dominates past ~154ms integration */
		return 65535;
	}
	return (uint16_t)(1024 * cycles);
}

/**
 * @brief Calculate illuminance (lux) and correlated color temperature (CT).
 *
 * Implements the ams DN40 lux/CT algorithm using the raw RGBC channel
 * counts and the current integration time/gain settings.
 *
 * @param r Raw red channel count.
 * @param g Raw green channel count.
 * @param b Raw blue channel count.
 * @param c Raw clear channel count.
 * @param atime Raw ATIME register value used for the sample.
 * @param gain_reg Raw gain register value used for the sample.
 * @param[out] lux_out Pointer to store the calculated lux value (0 if invalid).
 * @param[out] ct_out Pointer to store the calculated color temperature in
 *                     Kelvin (0 if invalid).
 *
 * @retval 0 on success.
 * @retval -EAGAIN Clear channel is saturated; result not usable.
 * @retval -ERANGE Signal too low or geometrically degenerate for the
 *                  DN40 formula (e.g. negative lux/CT, near-zero red).
 */
static int tcs34725_calculate_lux_ct(uint16_t r, uint16_t g, uint16_t b, uint16_t c, uint8_t atime,
				     uint8_t gain_reg, uint16_t *lux_out, uint16_t *ct_out)
{
	float ir, rp, gp, bp;
	float cpl, again, atime_ms;
	float lux, ct;
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

	/* DN40 3.1: IR rejection */
	ir = ((float)r + (float)g + (float)b - (float)c) / 2.0f;
	rp = (float)r - ir;
	gp = (float)g - ir;
	bp = (float)b - ir;

	if (rp <= 0.0f) {
		/* R' is the CCT denominator; can't produce a valid ratio */
		return -ERANGE;
	}

	/* DN40 3.2: CPL, with GA = 1 (no glass/cover) */
	again = tcs34725_again_value(gain_reg);
	atime_ms = 2.4f * (256 - atime);
	cpl = (atime_ms * again) / TCS34725_DN40_DGF;

	if (cpl <= 0.0f) {
		return -ERANGE;
	}

	/* DN40 3.2: Lux */
	lux = (TCS34725_DN40_R_COEF * rp) + (TCS34725_DN40_G_COEF * gp) +
	      (TCS34725_DN40_B_COEF * bp);
	lux = lux / cpl;

	if (lux < 0.0f) {
		return -ERANGE;
	}

	/* DN40 3.4: Color temperature */
	ct = (TCS34725_DN40_CT_COEF * (bp / rp)) + TCS34725_DN40_CT_OFFSET;
	if (ct < 0.0f) {
		return -ERANGE;
	}

	*lux_out = (uint16_t)lux;
	*ct_out = (uint16_t)ct;

	return 0;
}

/**
 * @brief Set a runtime-configurable sensor attribute.
 *
 * Supports setting the analog gain (SENSOR_ATTR_GAIN) and the RGBC
 * integration time (SENSOR_ATTR_SAMPLING_FREQUENCY, mapped to ATIME).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan Channel the attribute applies to (SENSOR_CHAN_ALL or
 *             SENSOR_CHAN_LIGHT).
 * @param attr Attribute to set.
 * @param val Pointer to the new attribute value.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP Unsupported channel or attribute.
 * @retval -EINVAL Value out of the accepted range.
 * @retval -errno Negative error code on I2C failure.
 */
static int tcs34725_attr_set(const struct device *dev, enum sensor_channel chan,
			     enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_LIGHT) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_GAIN:
		if (val->val1 < 0 || val->val1 > 3) {
			return -EINVAL;
		}
		return tcs34725_reg_write(dev, TCS34725_REG_CONTROL, val->val1 & 0x03);

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		if (val->val1 < 0 || val->val1 > 255) {
			return -EINVAL;
		}
		return tcs34725_reg_write(dev, TCS34725_REG_ATIME, (uint8_t)val->val1);

	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Block until the RGBC data is valid or a timeout elapses.
 *
 * Polls the STATUS register for the AVALID bit, sleeping briefly
 * between reads.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 once AVALID is set.
 * @retval -ETIMEDOUT If AVALID is not set within ~1000 ms.
 * @retval -errno Negative error code on I2C failure.
 */
static int tcs34725_wait_valid(const struct device *dev)
{
	uint8_t status;
	int ret;
	int64_t timeout = k_uptime_get() + 1000;

	do {
		ret = tcs34725_reg_read(dev, TCS34725_REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}
		if (status & TCS34725_STATUS_AVALID) {
			return 0;
		}
		k_sleep(K_MSEC(3));
	} while (k_uptime_get() < timeout);

	LOG_WRN("Timed out waiting for AVALID");
	return -ETIMEDOUT;
}

/**
 * @brief Fetch a new sample from the TCS34725 into internal driver data.
 *
 * Waits for a valid RGBC conversion, reads the clear/red/green/blue
 * channels, and computes lux/color temperature via the DN40 algorithm.
 * The validity of the lux/CT result is recorded in data->lux_valid and
 * surfaced through tcs34725_channel_get().
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan Channel to fetch (SENSOR_CHAN_ALL or one of the supported
 *             RGBC/lux/color-temp channels).
 *
 * @retval 0 on success (even if lux/CT computation was invalid; the RGBC
 *         data itself was still fetched).
 * @retval -ENOTSUP Unsupported channel.
 * @retval -errno Negative error code on I2C failure.
 */
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

/**
 * @brief Get a sensor channel value from the last fetched sample.
 *
 * For SENSOR_CHAN_TCS34725_LUX and SENSOR_CHAN_TCS34725_COLOR_TEMP,
 * val->val2 doubles as a validity flag: 1 if the last DN40 lux/CT
 * calculation succeeded, 0 if it was saturated or below the minimum
 * usable clear-channel threshold (in which case val->val1 reads as 0,
 * not a genuine zero-light measurement). See tcs34725_sample_fetch()
 * and tcs34725_calculate_lux_ct() for the underlying error conditions.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param chan Channel to read.
 * @param[out] val Pointer to store the result.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP Unsupported channel.
 */
static int tcs34725_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct tcs34725_data *data = dev->data;
	uint16_t raw;

	switch ((int)chan) {
	case SENSOR_CHAN_RED:
		raw = data->red;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_GREEN:
		raw = data->green;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_BLUE:
		raw = data->blue;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_LIGHT:
		raw = data->clear;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_TCS34725_LUX:
		raw = data->lux;
		val->val2 = data->lux_valid ? 1 : 0;
		break;
	case SENSOR_CHAN_TCS34725_COLOR_TEMP:
		raw = data->color_temp;
		val->val2 = data->lux_valid ? 1 : 0;
		break;
	default:
		return -ENOTSUP;
	}

	val->val1 = raw;

	return 0;
}

/**
 * @brief Initialize the TCS34725 device instance.
 *
 * Verifies the I2C bus is ready, checks the device ID, applies the
 * devicetree-configured ATIME/gain (and wait-time settings if enabled),
 * and powers on the sensor.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success.
 * @retval -ENODEV I2C bus not ready or unexpected device ID.
 * @retval -errno Negative error code on I2C failure.
 */
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

/** @brief TCS34725 sensor driver API implementation. */
static DEVICE_API(sensor, tcs34725_driver_api) = {
	.attr_set = tcs34725_attr_set,
	.sample_fetch = tcs34725_sample_fetch,
	.channel_get = tcs34725_channel_get,
};

/**
 * @brief Instantiate a TCS34725 driver instance from devicetree.
 *
 * @param inst Devicetree instance number.
 */
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
