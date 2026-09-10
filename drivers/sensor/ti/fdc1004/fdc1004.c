/*
 * Copyright (c) 2026 Zephyr Project Developers
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Driver for the Texas Instruments FDC1004 4-channel capacitance-to-digital
 * converter.
 *
 * Channels are exposed as FDC1004_CHAN_CIN1 .. FDC1004_CHAN_CIN4 via the
 * private sensor channel range (SENSOR_CHAN_PRIV_START). Include <fdc1004.h>
 * in application code to access these channel identifiers.
 *
 * All four channels are always measured in one-shot mode when
 * sensor_sample_fetch() is called. sensor_channel_get() then retrieves the
 * result for the requested channel.
 *
 * Reported value units: picofarads (pF).
 *   val->val1 = integer pF
 *   val->val2 = fractional pF × 10^6  (... so femtofarads)
 */

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include "fdc1004.h"

LOG_MODULE_REGISTER(fdc1004, CONFIG_SENSOR_LOG_LEVEL);

struct fdc1004_data {
	/*
	 * Raw 16-bit signed measurement for each channel (CIN1..CIN4),
	 * sign-extended to int32_t.
	 */
	int32_t raw[4];
};

struct fdc1004_config {
	struct i2c_dt_spec i2c;
	/* CAPDAC offset per channel (0..31, each step = 3.125 pF) */
	uint8_t capdac[4];
	/* FDC_CONF RATE bits, pre-computed from DT measure-rate property */
	uint16_t rate_bits;
};


/* All FDC1004 registers are 16-bit, big-endian. */

/*
 * @brief Read a 16-bit value from an FDC1004 register
 * @param i2c Pointer to the I2C device structure
 * @param reg Register address
 * @param val Pointer to the value to read
 * @return 0 if successful, otherwise a negative error code
 */
static int reg_read(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t *val)
{
	uint8_t buf[2];
	int ret = i2c_burst_read_dt(i2c, reg, buf, sizeof(buf));

	if (ret == 0) {
		*val = sys_get_be16(buf);
	}
	return ret;
}

/*
 * @brief Write a 16-bit value to an FDC1004 register
 * @param i2c Pointer to the I2C device structure
 * @param reg Register address
 * @param val Value to write
 * @return 0 if successful, otherwise a negative error code
 */
static int reg_write(const struct i2c_dt_spec *i2c, uint8_t reg, uint16_t val)
{
	uint8_t buf[2] = {val >> 8, val & 0xFF};

	return i2c_burst_write_dt(i2c, reg, buf, sizeof(buf));
}


/*
 * @brief Fetch a sensor sample from the FDC1004
 * @param dev Pointer to the device structure
 * @param chan Sensor channel to fetch samples for
 * @return 0 if successful, otherwise a negative error code
 */
static int fdc1004_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	const struct fdc1004_config *cfg = dev->config;
	struct fdc1004_data *data = dev->data;
	uint16_t conf_meas;
	uint16_t fdc_conf;
	uint16_t msb, lsb;
	int ret;

	/*
	 * In one-shot mode (REPEAT=0) the FDC1004 only supports one active
	 * measurement at a time. Enabling multiple MEAS_EN bits simultaneously
	 * is only valid in repeat mode. Trigger each channel individually,
	 * wait for its DONE bit, then read its result before moving on.
	 *
	 * At 100 S/s one measurement takes ~10 ms; poll every 5 ms with a
	 * 100 ms timeout per channel.
	 */
	for (int ch = 0; ch < 4; ch++) {
		/* Configure this channel: CINx single-ended against CAPDAC */
		conf_meas = ((uint16_t)ch                      << FDC1004_MEAS_CHA_SHIFT) |
			    ((uint16_t)FDC1004_MEAS_CHB_CAPDAC << FDC1004_MEAS_CHB_SHIFT) |
			    ((uint16_t)cfg->capdac[ch]          << FDC1004_MEAS_CAPDAC_SHIFT);

		ret = reg_write(&cfg->i2c, FDC1004_REG_CONF_MEAS1 + ch, conf_meas);
		if (ret != 0) {
			LOG_ERR("Failed to write CONF_MEAS%d: %d", ch + 1, ret);
			return ret;
		}

		/* Trigger one-shot for this channel only */
		fdc_conf = cfg->rate_bits | (FDC1004_CONF_MEAS1_EN >> ch);
		ret = reg_write(&cfg->i2c, FDC1004_REG_FDC_CONF, fdc_conf);
		if (ret != 0) {
			LOG_ERR("Failed to trigger measurement %d: %d", ch + 1, ret);
			return ret;
		}

		/* Poll until this channel's DONE bit is set */
		uint16_t done_bit = FDC1004_CONF_DONE1 >> ch;
		int64_t deadline = k_uptime_get() + 100;

		do {
			k_sleep(K_MSEC(5));
			ret = reg_read(&cfg->i2c, FDC1004_REG_FDC_CONF, &fdc_conf);
			if (ret != 0) {
				LOG_ERR("Failed to read FDC_CONF: %d", ret);
				return ret;
			}
			if (k_uptime_get() > deadline) {
				LOG_ERR("Measurement %d timeout (FDC_CONF=0x%04X)",
					ch + 1, fdc_conf);
				return -ETIMEDOUT;
			}
		} while (!(fdc_conf & done_bit));

		/* Read the 24-bit result for this channel */
		ret = reg_read(&cfg->i2c, FDC1004_REG_MEAS1_MSB + ch * 2, &msb);
		if (ret != 0) {
			LOG_ERR("Failed to read MEAS%d_MSB: %d", ch + 1, ret);
			return ret;
		}
		ret = reg_read(&cfg->i2c, FDC1004_REG_MEAS1_LSB + ch * 2, &lsb);
		if (ret != 0) {
			LOG_ERR("Failed to read MEAS%d_LSB: %d", ch + 1, ret);
			return ret;
		}

		/*
		 * The 24-bit result is spread across two 16-bit registers:
		 *   MEAS_MSB[15:0]  → bits [23:8] of the result
		 *   MEAS_LSB[15:8]  → bits [ 7:0] of the result
		 *   MEAS_LSB[ 7:0]  → reserved, always 0
		 */
		int32_t raw24 = ((int32_t)msb << 8) | (lsb >> 8);

		/* Sign-extend from 24 bits to 32 bits */
		data->raw[ch] = (raw24 << 8) >> 8;
	}

	return 0;
}

/*
 * @brief Get the value of a sensor channel from the FDC1004
 * @param dev Pointer to the device structure
 * @param chan Sensor channel to get the value for
 * @param val Pointer to the sensor value structure
 * @return 0 if successful, -ENOTSUP for unsupported channels
 */
static int fdc1004_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct fdc1004_config *cfg = dev->config;
	const struct fdc1004_data *data = dev->data;
	enum fdc1004_channel priv_chan = (enum fdc1004_channel)chan;
	int ch;

	if (priv_chan < FDC1004_CHAN_CIN1 || priv_chan > FDC1004_CHAN_CIN4) {
		return -ENOTSUP;
	}
	ch = priv_chan - FDC1004_CHAN_CIN1;

	/*
	 * Convert raw (16+8)-bit signed count to picofarads:
	 *
	 *   C_actual = (raw / 2^23) * 15 pF  +  capdac * 3.125 pF
	 *
	 * Work in units of "millionths of a picofarad" to keep integer
	 * arithmetic throughout, then split into val1 (integer pF) and
	 * val2 (fractional pF * 10^6).
	 */

	int64_t pf_upf = ((int64_t)data->raw[ch] * (int64_t)FDC1004_FULL_SCALE_PF * 1000000LL)
			 >> 23; /* divide by 2^23 */
	pf_upf += (int64_t)cfg->capdac[ch] * FDC1004_CAPDAC_STEP_NPF;

	val->val1 = (int32_t)(pf_upf / 1000000LL);
	val->val2 = (int32_t)(pf_upf % 1000000LL);

	return 0;
}

static DEVICE_API(sensor, fdc1004_api) = {
	.sample_fetch = fdc1004_sample_fetch,
	.channel_get  = fdc1004_channel_get,
};


/*
 * @brief Initialise the FDC1004 sensor, checks that the I2C bus is ready,
 *        reads and verifies the device ID.
 * @param dev Pointer to the device structure
 * @return 0 if successful, otherwise a negative error code
 */
static int fdc1004_init(const struct device *dev)
{
	const struct fdc1004_config *cfg = dev->config;
	uint16_t device_id;
	uint16_t manufacturer_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	ret = reg_read(&cfg->i2c, FDC1004_REG_MANUFACTURER_ID, &manufacturer_id);
	if (ret != 0) {
		LOG_ERR("Failed to read manufacturer ID: %d", ret);
		return ret;
	}

	ret = reg_read(&cfg->i2c, FDC1004_REG_DEVICE_ID, &device_id);
	if (ret != 0) {
		LOG_ERR("Failed to read device ID: %d", ret);
		return ret;
	}

	if (device_id != 0x1004) {
		LOG_ERR("Unexpected device ID 0x%04X at I2C addr 0x%02X "
			"(expected 0x1004)", device_id, cfg->i2c.addr);
		return -ENODEV;
	}

	LOG_INF("FDC1004 @ 0x%02X ready (manufacturer=0x%04X device=0x%04X)",
		cfg->i2c.addr, manufacturer_id, device_id);

	return 0;
}

/* Device tree instantiation */
#define FDC1004_RATE_BITS(inst)                                          \
	(DT_INST_PROP(inst, measure_rate) == 400 ? FDC1004_CONF_RATE_400SPS \
	 : DT_INST_PROP(inst, measure_rate) == 200 ? FDC1004_CONF_RATE_200SPS \
	 : FDC1004_CONF_RATE_100SPS)

#define FDC1004_DEFINE(inst)                                             \
	static struct fdc1004_data fdc1004_data_##inst;                  \
									 \
	static const struct fdc1004_config fdc1004_config_##inst = {     \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                      \
		.capdac = {                                              \
			DT_INST_PROP_BY_IDX(inst, ti_capdac, 0),         \
			DT_INST_PROP_BY_IDX(inst, ti_capdac, 1),         \
			DT_INST_PROP_BY_IDX(inst, ti_capdac, 2),         \
			DT_INST_PROP_BY_IDX(inst, ti_capdac, 3),         \
		},                                                       \
		.rate_bits = FDC1004_RATE_BITS(inst),                    \
	};                                                               \
									 \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,                               \
				     fdc1004_init,                       \
				     NULL,                               \
				     &fdc1004_data_##inst,               \
				     &fdc1004_config_##inst,             \
				     POST_KERNEL,                        \
				     CONFIG_SENSOR_INIT_PRIORITY,        \
				     &fdc1004_api);

#define DT_DRV_COMPAT ti_fdc1004
DT_INST_FOREACH_STATUS_OKAY(FDC1004_DEFINE)
