/*
 * Copyright (c) 2026 Mahendra Sondagar <mahendra@dotcom.co.in>
 * Copyright (c) 2026 Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief STHS34PF80 infrared presence and motion sensor driver.
 *
 * Exposes ambient and object temperature, the presence, motion and
 * temperature-shock algorithm outputs, and the matching detection flags.
 */

#include "sths34pf80.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT st_sths34pf80

LOG_MODULE_REGISTER(sths34pf80, CONFIG_SENSOR_LOG_LEVEL);

static int sths34pf80_reg_read(const struct sths34pf80_config *config, uint8_t reg, uint8_t *buf,
			       size_t len)
{
	return i2c_burst_read_dt(&config->i2c, reg, buf, len);
}

static int sths34pf80_reg_write(const struct sths34pf80_config *config, uint8_t reg, uint8_t value)
{
	return i2c_reg_write_byte_dt(&config->i2c, reg, value);
}

static int sths34pf80_reg_update(const struct sths34pf80_config *config, uint8_t reg, uint8_t mask,
				 uint8_t value)
{
	uint8_t data;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->i2c, reg, &data);
	if (ret < 0) {
		return ret;
	}

	data = (data & ~mask) | (value & mask);

	return i2c_reg_write_byte_dt(&config->i2c, reg, data);
}

static int sths34pf80_read_i16(const struct sths34pf80_config *config, uint8_t reg, int16_t *value)
{
	uint8_t buf[sizeof(int16_t)];
	int ret;

	ret = sths34pf80_reg_read(config, reg, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}

	*value = (int16_t)sys_get_le16(buf);

	return 0;
}

/* Write one embedded function register */
static int sths34pf80_emb_func_write(const struct sths34pf80_config *config, uint8_t addr,
				     uint8_t value)
{
	int ret;
	int cleanup_ret;

	ret = sths34pf80_reg_update(config, STHS34PF80_REG_CTRL2, STHS34PF80_CTRL2_FUNC_CFG_ACCESS,
				    STHS34PF80_CTRL2_FUNC_CFG_ACCESS);
	if (ret < 0) {
		return ret;
	}

	ret = sths34pf80_reg_update(config, STHS34PF80_REG_PAGE_RW,
				    STHS34PF80_PAGE_RW_FUNC_CFG_WRITE,
				    STHS34PF80_PAGE_RW_FUNC_CFG_WRITE);
	if (ret == 0) {
		ret = sths34pf80_reg_write(config, STHS34PF80_REG_FUNC_CFG_ADDR, addr);
	}

	if (ret == 0) {
		ret = sths34pf80_reg_write(config, STHS34PF80_REG_FUNC_CFG_DATA, value);
	}

	cleanup_ret = sths34pf80_reg_update(config, STHS34PF80_REG_PAGE_RW,
					    STHS34PF80_PAGE_RW_FUNC_CFG_WRITE, 0U);
	if (ret == 0) {
		ret = cleanup_ret;
	}

	cleanup_ret = sths34pf80_reg_update(config, STHS34PF80_REG_CTRL2,
					    STHS34PF80_CTRL2_FUNC_CFG_ACCESS, 0U);
	if (ret == 0) {
		ret = cleanup_ret;
	}

	return ret;
}

/* Reload the trimming parameters from OTP memory. */
static int sths34pf80_reboot_otp(const struct sths34pf80_config *config)
{
	int ret;

	ret = sths34pf80_reg_update(config, STHS34PF80_REG_CTRL2, STHS34PF80_CTRL2_BOOT,
				    STHS34PF80_CTRL2_BOOT);
	if (ret < 0) {
		LOG_ERR("Failed to reboot OTP memory (%d)", ret);
		return ret;
	}

	k_msleep(STHS34PF80_OTP_REBOOT_MS);

	return 0;
}

/* Pulse CTRL3 RESET_ALG so the embedded algorithms restart from a clean state. */
static int sths34pf80_reset_algorithms(const struct sths34pf80_config *config)
{
	int ret;

	ret = sths34pf80_reg_update(config, STHS34PF80_REG_CTRL3, STHS34PF80_CTRL3_RESET_ALG,
				    STHS34PF80_CTRL3_RESET_ALG);
	if (ret < 0) {
		LOG_ERR("Failed to set RESET_ALG bit (%d)", ret);
		return ret;
	}

	k_msleep(STHS34PF80_RESET_ALG_MS);

	ret = sths34pf80_reg_update(config, STHS34PF80_REG_CTRL3, STHS34PF80_CTRL3_RESET_ALG, 0U);
	if (ret < 0) {
		LOG_ERR("Failed to clear RESET_ALG bit (%d)", ret);
		return ret;
	}

	k_msleep(STHS34PF80_RESET_ALG_MS);

	return 0;
}

/* Power the sensor down and reset the embedded algorithm block. */
static int sths34pf80_algorithm_reset(const struct sths34pf80_config *config)
{
	int ret;

	ret = sths34pf80_reg_write(config, STHS34PF80_REG_CTRL1, STHS34PF80_ODR_POWER_DOWN);
	if (ret < 0) {
		LOG_ERR("Failed to enter power-down (%d)", ret);
		return ret;
	}

	ret = sths34pf80_emb_func_write(config, STHS34PF80_EMB_RESET_ALGO,
					STHS34PF80_ALGO_ENABLE_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to reset embedded algorithms (%d)", ret);
		return ret;
	}

	return 0;
}

/* Apply default averaging, enable BDU, set the ODR. */
static int sths34pf80_configure_defaults(const struct sths34pf80_config *config)
{
	int ret;

	ret = sths34pf80_reg_update(
		config, STHS34PF80_REG_AVG_TRIM,
		STHS34PF80_AVG_TRIM_AVG_T_MASK | STHS34PF80_AVG_TRIM_AVG_TMOS_MASK,
		(STHS34PF80_AVG_T_8 << STHS34PF80_AVG_TRIM_AVG_T_SHIFT) |
			(STHS34PF80_AVG_TMOS_32 << STHS34PF80_AVG_TRIM_AVG_TMOS_SHIFT));
	if (ret < 0) {
		LOG_ERR("Failed to configure averaging (%d)", ret);
		return ret;
	}

	ret = sths34pf80_reg_update(
		config, STHS34PF80_REG_CTRL1, STHS34PF80_CTRL1_BDU | STHS34PF80_CTRL1_ODR_MASK,
		STHS34PF80_CTRL1_BDU | (config->odr << STHS34PF80_CTRL1_ODR_SHIFT));
	if (ret < 0) {
		LOG_ERR("Failed to configure CTRL1 (%d)", ret);
		return ret;
	}

	return 0;
}

static int sths34pf80_wait_drdy(const struct sths34pf80_config *config)
{
	uint8_t status;
	int ret;

	for (uint32_t poll = 0U; poll < STHS34PF80_DRDY_MAX_POLLS; poll++) {
		ret = i2c_reg_read_byte_dt(&config->i2c, STHS34PF80_REG_STATUS, &status);
		if (ret < 0) {
			return ret;
		}

		if ((status & STHS34PF80_STATUS_DRDY) != 0U) {
			return 0;
		}

		k_msleep(STHS34PF80_DRDY_POLL_MS);
	}

	return -EBUSY;
}

/* Latch a detection flag and keep it set until the hold time has expired. */
static void sths34pf80_update_held_flag(bool detected, int64_t now, bool *flag,
					int64_t *last_detected_ms)
{
	if (detected) {
		*flag = true;
		*last_detected_ms = now;
	} else if (*flag && ((now - *last_detected_ms) > STHS34PF80_FLAG_TIMEOUT_MS)) {
		*flag = false;
	}
}

static bool sths34pf80_held_flag_active(bool flag, int64_t last_detected_ms)
{
	return flag && (k_uptime_get() - last_detected_ms) <= STHS34PF80_FLAG_TIMEOUT_MS;
}

static void sths34pf80_raw_to_sensor_value(int16_t raw, int32_t lsb_per_c, struct sensor_value *val)
{
	val->val1 = raw / lsb_per_c;
	val->val2 = (raw % lsb_per_c) * (USEC_PER_SEC / lsb_per_c);
}

static int sths34pf80_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sths34pf80_data *data = dev->data;
	const struct sths34pf80_config *config = dev->config;
	const struct {
		uint8_t reg;
		int16_t *dst;
	} outputs[] = {
		{STHS34PF80_REG_TOBJECT_L, &data->object_temp_raw},
		{STHS34PF80_REG_TAMBIENT_L, &data->ambient_temp_raw},
		{STHS34PF80_REG_TOBJ_COMP_L, &data->object_temp_comp_raw},
		{STHS34PF80_REG_TPRESENCE_L, &data->presence_raw},
		{STHS34PF80_REG_TMOTION_L, &data->motion_raw},
		{STHS34PF80_REG_TAMB_SHOCK_L, &data->temp_shock_raw},
	};
	uint8_t func_status;
	int64_t now;
	int ret;

	ARG_UNUSED(chan);

	ret = sths34pf80_wait_drdy(config);
	if (ret < 0) {
		return ret;
	}

	for (size_t i = 0; i < ARRAY_SIZE(outputs); i++) {
		ret = sths34pf80_read_i16(config, outputs[i].reg, outputs[i].dst);
		if (ret < 0) {
			LOG_ERR("Failed to read output[%zu] (%d)", i, ret);
			return ret;
		}

		LOG_DBG("output[%zu] raw = %d", i, *outputs[i].dst);
	}

	ret = i2c_reg_read_byte_dt(&config->i2c, STHS34PF80_REG_FUNC_STATUS, &func_status);
	if (ret < 0) {
		LOG_ERR("Failed to read function status (%d)", ret);
		return ret;
	}

	LOG_DBG("FUNC_STATUS = 0x%02x", func_status);

	now = k_uptime_get();

	sths34pf80_update_held_flag((func_status & STHS34PF80_FUNC_STATUS_PRES_FLAG) != 0U, now,
				    &data->presence_flag, &data->presence_last_detected_ms);
	sths34pf80_update_held_flag((func_status & STHS34PF80_FUNC_STATUS_MOT_FLAG) != 0U, now,
				    &data->motion_flag, &data->motion_last_detected_ms);

	data->temp_shock_flag = (func_status & STHS34PF80_FUNC_STATUS_TAMB_SHOCK_FLAG) != 0U;

	return 0;
}

static int sths34pf80_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	const struct sths34pf80_data *data = dev->data;

	val->val2 = 0;

	switch ((int)chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		sths34pf80_raw_to_sensor_value(data->ambient_temp_raw,
					       STHS34PF80_AMBIENT_TEMP_LSB_PER_C, val);
		return 0;

	case SENSOR_CHAN_STHS34PF80_OBJECT_TEMP:
		sths34pf80_raw_to_sensor_value(data->object_temp_raw,
					       STHS34PF80_OBJECT_TEMP_LSB_PER_C, val);
		return 0;

	case SENSOR_CHAN_STHS34PF80_OBJECT_TEMP_COMP:
		sths34pf80_raw_to_sensor_value(data->object_temp_comp_raw,
					       STHS34PF80_OBJECT_TEMP_LSB_PER_C, val);
		return 0;

	case SENSOR_CHAN_STHS34PF80_PRESENCE:
		val->val1 = data->presence_raw;
		return 0;

	case SENSOR_CHAN_STHS34PF80_MOTION:
		val->val1 = data->motion_raw;
		return 0;

	case SENSOR_CHAN_STHS34PF80_TEMP_SHOCK:
		val->val1 = data->temp_shock_raw;
		return 0;

	case SENSOR_CHAN_STHS34PF80_PRESENCE_FLAG:
		val->val1 = sths34pf80_held_flag_active(data->presence_flag,
							data->presence_last_detected_ms);
		return 0;

	case SENSOR_CHAN_STHS34PF80_MOTION_FLAG:
		val->val1 = sths34pf80_held_flag_active(data->motion_flag,
							data->motion_last_detected_ms);
		return 0;

	case SENSOR_CHAN_STHS34PF80_TEMP_SHOCK_FLAG:
		val->val1 = data->temp_shock_flag;
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int sths34pf80_init(const struct device *dev)
{
	const struct sths34pf80_config *cfg = dev->config;
	uint8_t who_am_i;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->i2c.bus);
		return -ENODEV;
	}

	ret = i2c_reg_read_byte_dt(&cfg->i2c, STHS34PF80_REG_WHO_AM_I, &who_am_i);
	if (ret < 0) {
		LOG_ERR("Failed to read WHO_AM_I (%d)", ret);
		return ret;
	}

	if (who_am_i != STHS34PF80_WHO_AM_I_VALUE) {
		LOG_ERR("Invalid WHO_AM_I: 0x%02x, expected 0x%02x", who_am_i,
			STHS34PF80_WHO_AM_I_VALUE);
		return -ENODEV;
	}

	ret = sths34pf80_reboot_otp(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = sths34pf80_algorithm_reset(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = sths34pf80_configure_defaults(cfg);
	if (ret < 0) {
		return ret;
	}

	ret = sths34pf80_reset_algorithms(cfg);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("STHS34PF80 initialized");

	return 0;
}

static DEVICE_API(sensor, sths34pf80_api) = {
	.sample_fetch = sths34pf80_sample_fetch,
	.channel_get = sths34pf80_channel_get,
};

#define STHS34PF80_DEFINE(inst)                                                                    \
	static struct sths34pf80_data sths34pf80_data_##inst;                                      \
                                                                                                   \
	static const struct sths34pf80_config sths34pf80_config_##inst = {                         \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.odr = STHS34PF80_ODR_1_HZ,                                                        \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, sths34pf80_init, NULL, &sths34pf80_data_##inst,         \
				     &sths34pf80_config_##inst, POST_KERNEL,                       \
				     CONFIG_SENSOR_INIT_PRIORITY, &sths34pf80_api);

DT_INST_FOREACH_STATUS_OKAY(STHS34PF80_DEFINE)
