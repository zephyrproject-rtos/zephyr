/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ams_as7341.c
 * @brief AS7341 Spectral Sensor Module driver.
 *
 * Two-pass SMUX configuration is used to read all 8 spectral filters
 * plus the clear and NIR channels over I2C.
 */

#include "ams_as7341.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT ams_as7341

LOG_MODULE_REGISTER(ams_as7341, CONFIG_SENSOR_LOG_LEVEL);

/* Poll STATUS2 AVALID until spectral data is ready (~500 ms timeout) */
static int as7341_wait_data_ready(const struct device *dev)
{
	const struct as7341_config *cfg = dev->config;
	uint8_t status;
	int timeout = 250;

	while (timeout--) {
		if (i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_STATUS2, &status) < 0) {
			LOG_ERR("Failed to set time");
			return -EIO;
		}

		if (status & AS7341_STATUS2_AVALID) {
			return 0;
		}

		k_msleep(AS7341_POLL_INTERVAL_MS);
	}

	return -ETIMEDOUT;
}

static int as7341_enable_spectral(const struct device *dev, bool en)
{
	const struct as7341_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, AS7341_ENABLE_SP_EN,
				      en ? AS7341_ENABLE_SP_EN : 0);
}

static int as7341_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct as7341_config *cfg = dev->config;
	struct as7341_data *data = dev->data;
	int ret;

	switch (attr) {
	case SENSOR_ATTR_GAIN:
		data->gain = (uint8_t)val->val1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_CFG1, data->gain);
		if (ret < 0) {
			LOG_ERR("Failed to set gain");
			return ret;
		}
		return 0;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		data->atime = (uint16_t)val->val1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ATIME, (uint8_t)data->atime);
		if (ret < 0) {
			LOG_ERR("Failed to set ATIME");
			return ret;
		}
		return 0;

	case SENSOR_ATTR_RESOLUTION:
		data->astep = (uint16_t)val->val1;
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_L,
					    (uint8_t)(data->astep & 0xFF));
		if (ret < 0) {
			LOG_ERR("Failed to set ASTEP LSB");
			return ret;
		}
		ret = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_H,
					    (uint8_t)(data->astep >> 8));
		if (ret < 0) {
			LOG_ERR("Failed to set ASTEP MSB");
			return ret;
		}
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int as7341_attr_get(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, struct sensor_value *val)
{
	struct as7341_data *data = dev->data;

	switch (attr) {
	case SENSOR_ATTR_GAIN:
		val->val1 = data->gain;
		val->val2 = 0;
		return 0;

	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		val->val1 = data->atime;
		val->val2 = 0;
		return 0;

	case SENSOR_ATTR_RESOLUTION:
		val->val1 = data->astep;
		val->val2 = 0;
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int as7341_smux_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct as7341_config *cfg = dev->config;

	return i2c_reg_write_byte_dt(&cfg->i2c, reg, val);
}

/* Configure SMUX routing for spectral channels F1–F4. */
static int as7341_smux_config_f1f4(const struct device *dev)
{
	static const uint8_t smux_cfg[] = {
		0x30, 0x01, 0x00, 0x00, 0x00, 0x42, 0x00, 0x00, 0x50, 0x00,
		0x00, 0x00, 0x20, 0x04, 0x00, 0x30, 0x01, 0x50, 0x00, 0x06,
	};
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(smux_cfg); i++) {
		err = as7341_smux_write(dev, i, smux_cfg[i]);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

/* Configure SMUX routing for spectral channels F5–F8. */
static int as7341_smux_config_f5f8(const struct device *dev)
{
	static const uint8_t smux_cfg[] = {
		0x00, 0x00, 0x00, 0x40, 0x02, 0x00, 0x10, 0x03, 0x50, 0x10,
		0x03, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x50, 0x00, 0x06,
	};
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(smux_cfg); i++) {
		err = as7341_smux_write(dev, i, smux_cfg[i]);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int as7341_smux_config_flicker(const struct device *dev)
{
	/* Flicker Detection SMUX config (Flicker -> ADC5) */
	static const uint8_t smux_cfg[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60,
	};
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(smux_cfg); i++) {
		err = as7341_smux_write(dev, i, smux_cfg[i]);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int as7341_set_smux_command(const struct device *dev, uint8_t cmd)
{
	const struct as7341_config *cfg = dev->config;

	return i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_CFG6, AS7341_CFG6_SMUX_CMD_MASK,
				      (cmd & AS7341_SMUX_CMD_MASK) << AS7341_SMUX_CMD_SHIFT);
}

/* Start SMUX load and wait for SMUXEN to clear (~1 s timeout) */
static int as7341_enable_smux(const struct device *dev)
{
	const struct as7341_config *cfg = dev->config;
	int ret = i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, AS7341_ENABLE_SMUXEN,
					 AS7341_ENABLE_SMUXEN);

	if (ret < 0) {
		return ret;
	}

	uint8_t reg;
	int timeout = 1000;

	while (timeout--) {
		ret = i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, &reg);
		if (ret < 0) {
			return ret;
		}
		if (!(reg & AS7341_ENABLE_SMUXEN)) {
			return 0;
		}
		k_msleep(AS7341_SMUX_POLL_INTERVAL_MS);
	}
	return -ETIMEDOUT;
}

static int as7341_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct as7341_data *data = dev->data;
	const struct as7341_config *cfg = dev->config;
	uint8_t buf[2];
	int err;

	if ((enum sensor_channel_as7341)chan == SENSOR_CHAN_AS7341_FLICKER) {
		return as7341_detect_flicker(dev, &data->flicker_hz);
	}

	if (chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	LOG_DBG("AS7341 sample fetch started");

	/* Low Channels (F1-F4 + CLEAR_0 + NIR_0) */

	/* Disable measurement and configure SMUX for low channels */
	err = as7341_enable_spectral(dev, false);
	if (err < 0) {
		return err;
	}

	err = as7341_set_smux_command(dev, AS7341_SMUX_CMD_WRITE);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_config_f1f4(dev);
	if (err < 0) {
		LOG_ERR("SMUX F1-F4 config failed");
		return err;
	}

	/* Enable SMUX */
	err = as7341_enable_smux(dev);
	if (err < 0) {
		return err;
	}

	/* Start spectral measurement */
	err = as7341_enable_spectral(dev, true);
	if (err < 0) {
		return err;
	}

	/* Wait for data ready */
	err = as7341_wait_data_ready(dev);
	if (err < 0) {
		LOG_ERR("Data ready timeout - Bank 1 (F1-F4)");
		return err;
	}

	/* Read first 6 channels */
	for (int i = 0; i < 6; i++) {
		err = i2c_burst_read_dt(&cfg->i2c, AS7341_REG_CH0_DATA_L_1 + (i * 2), buf, 2);
		if (err < 0) {
			return err;
		}
		data->ch_data[i] = ((uint16_t)buf[1] << 8) | buf[0];
	}

	/* High Channels (F5-F8 + CLEAR + NIR) */

	/* Disable measurement and configure SMUX for high channels */
	err = as7341_enable_spectral(dev, false);
	if (err < 0) {
		return err;
	}

	err = as7341_set_smux_command(dev, AS7341_SMUX_CMD_WRITE);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_config_f5f8(dev);
	if (err < 0) {
		LOG_ERR("SMUX F5-F8 config failed");
		return err;
	}

	/* Enable SMUX */
	err = as7341_enable_smux(dev);
	if (err < 0) {
		return err;
	}

	/* Start spectral measurement */
	err = as7341_enable_spectral(dev, true);
	if (err < 0) {
		return err;
	}

	/* Wait for data ready */
	err = as7341_wait_data_ready(dev);
	if (err < 0) {
		LOG_ERR("Data ready timeout - Bank 2 (F5-F8)");
		return err;
	}

	/* Read next 6 channels */
	for (int i = 0; i < 6; i++) {
		err = i2c_burst_read_dt(&cfg->i2c, AS7341_REG_CH0_DATA_L_1 + (i * 2), buf, 2);
		if (err < 0) {
			return err;
		}

		data->ch_data[i + 6] = ((uint16_t)buf[1] << 8) | buf[0];
	}

	LOG_DBG("AS7341 sample fetch completed successfully");
	return 0;
}

static int as7341_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct as7341_data *data = dev->data;

	switch ((enum sensor_channel_as7341)chan) {
	case SENSOR_CHAN_AS7341_415NM_F1:
		val->val1 = data->ch_data[0];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_445NM_F2:
		val->val1 = data->ch_data[1];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_480NM_F3:
		val->val1 = data->ch_data[2];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_515NM_F4:
		val->val1 = data->ch_data[3];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_CLEAR_0:
		val->val1 = data->ch_data[4];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_NIR_0:
		val->val1 = data->ch_data[5];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_555NM_F5:
		val->val1 = data->ch_data[6];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_590NM_F6:
		val->val1 = data->ch_data[7];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_630NM_F7:
		val->val1 = data->ch_data[8];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_680NM_F8:
		val->val1 = data->ch_data[9];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_CLEAR:
		val->val1 = data->ch_data[10];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_NIR:
		val->val1 = data->ch_data[11];
		val->val2 = 0;
		break;

	case SENSOR_CHAN_AS7341_FLICKER:
		val->val1 = data->flicker_hz;
		val->val2 = 0;
		break;

	default:
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief Detect flicker frequency from AS7341 sensor.
 *
 * Configures sensor for flicker detection, measures, and decodes FD_STATUS
 * into Hz (0 = none, 1 = unknown, 100, or 120).
 *
 * @param dev AS7341 device instance.
 * @param[out] flicker_hz Pointer to store detected frequency in Hz.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL @p flicker_hz is NULL.
 */
int as7341_detect_flicker(const struct device *dev, uint16_t *flicker_hz)
{
	const struct as7341_config *cfg = dev->config;
	uint8_t status;
	int err;

	if (!flicker_hz) {
		return -EINVAL;
	}
	*flicker_hz = 0;

	LOG_DBG("Starting AS7341 Flicker Detection");

	/* Power on only */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, AS7341_ENABLE_PON);
	if (err < 0) {
		return err;
	}

	k_msleep(AS7341_PON_WARMUP_MS);

	/* SMUX config */
	err = as7341_set_smux_command(dev, AS7341_SMUX_CMD_WRITE);
	if (err < 0) {
		return err;
	}

	err = as7341_smux_config_flicker(dev);
	if (err < 0) {
		return err;
	}

	err = as7341_enable_smux(dev);
	if (err < 0) {
		return err;
	}

	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ENABLE,
				    AS7341_ENABLE_PON | AS7341_ENABLE_FDEN);
	if (err < 0) {
		return err;
	}

	/* Wait for measurement */
	k_msleep(AS7341_FD_WAIT_MS);

	/* Read FD_STATUS (0xDB) */
	err = i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_FD_STATUS, &status);
	if (err < 0) {
		return err;
	}

	LOG_DBG("FD_STATUS = 0x%02X", status);

	switch (status) {
	case AS7341_FD_STATUS_120HZ:
		*flicker_hz = 120;
		LOG_DBG("120 Hz flicker detected");
		break;

	case AS7341_FD_STATUS_UNKNOWN:
		*flicker_hz = 1;
		LOG_DBG("Unknown flicker detected");
		break;

	case AS7341_FD_STATUS_100HZ:
		*flicker_hz = 100;
		LOG_DBG("100 Hz flicker detected");
		break;

	default:
		*flicker_hz = 0;
		LOG_DBG("No flicker detected");
		break;
	}

	/* Clean shutdown */
	i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_ENABLE,
			       AS7341_ENABLE_FDEN | AS7341_ENABLE_SP_EN, 0);

	return 0;
}

static DEVICE_API(sensor, as7341_api) = {
	.attr_set = as7341_attr_set,
	.attr_get = as7341_attr_get,
	.sample_fetch = as7341_sample_fetch,
	.channel_get = as7341_channel_get,
};

static int as7341_init(const struct device *dev)
{
	const struct as7341_config *cfg = dev->config;
	uint8_t chip_id;
	int err;

	LOG_DBG("as7341_init");

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	/* Verify chip ID */
	err = i2c_reg_read_byte_dt(&cfg->i2c, AS7341_REG_ID, &chip_id);
	if (err < 0) {
		LOG_ERR("Failed to read chip ID");
		return err;
	}

	LOG_DBG("Chip ID read: 0x%02X", chip_id);

	if (chip_id != AS7341_CHIP_ID_VAL && chip_id != AS7341_CHIP_ID_ALT) {
		LOG_ERR("Unexpected chip ID: 0x%02X", chip_id);
		return -ENODEV;
	}

	/* Power ON sensor */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ENABLE, AS7341_ENABLE_PON);
	if (err < 0) {
		LOG_ERR("Failed to power on");
		return err;
	}

	k_msleep(AS7341_PON_WARMUP_MS);

	err = i2c_reg_update_byte_dt(&cfg->i2c, AS7341_REG_CONFIG, AS7341_CONFIG_FLICKER_MODE,
				     AS7341_CONFIG_FLICKER_MODE);
	if (err < 0) {
		return err;
	}

	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_LED, AS7341_LED_BRIGHTNESS_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to set LED brightness");
		return err;
	}

	/* Set ATIME */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ATIME, AS7341_ATIME_DEFAULT);
	if (err < 0) {
		LOG_ERR("Error setting ATIME");
		return err;
	}

	/* Set ASTEP = 999 */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_L, AS7341_ASTEP_999_LSB);
	if (err < 0) {
		LOG_ERR("Error setting ASTEP LSB");
		return err;
	}
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_ASTEP_H, AS7341_ASTEP_999_MSB);
	if (err < 0) {
		LOG_ERR("Error setting ASTEP MSB");
		return err;
	}

	/* Set gain - AS7341_GAIN_256X */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_CFG1, AS7341_GAIN_256X);
	if (err < 0) {
		LOG_ERR("Error setting gain");
		return err;
	}

	/* Configure CFG0 for normal operation, bank 0 */
	err = i2c_reg_write_byte_dt(&cfg->i2c, AS7341_REG_CFG0, AS7341_CFG0_NORMAL_BANK0);
	if (err < 0) {
		LOG_ERR("Error setting CFG0");
		return err;
	}

	LOG_DBG("AS7341 initialized successfully");

	return 0;
}

#define AS7341_DEFINE(inst)                                                                        \
	static struct as7341_data as7341_data_##inst;                                              \
	static const struct as7341_config as7341_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, as7341_init, NULL, &as7341_data_##inst,                 \
				     &as7341_config_##inst, POST_KERNEL,                           \
				     CONFIG_SENSOR_INIT_PRIORITY, &as7341_api);

DT_INST_FOREACH_STATUS_OKAY(AS7341_DEFINE)
