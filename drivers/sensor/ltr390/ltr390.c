/*
 * Copyright (c) 2022 Luke Holt
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT liteon_ltr390

#include "ltr390.h"

#include <errno.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(LTR390, CONFIG_SENSOR_LOG_LEVEL);

int ltr390_read_register(const struct ltr390_config *cfg, const uint8_t addr, uint8_t *value)
{
	return i2c_write_read_dt(&cfg->i2c, &addr, sizeof(addr), value, sizeof(*value));
}

int ltr390_write_register(const struct ltr390_config *cfg, const uint8_t addr, const uint8_t value)
{
	uint8_t buf[] = {addr, value};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

/**
 * @brief Sleep until measurement is ready. See section 6.2 of the datasheet.
 * Different measurement resolutions require different integration times.
 *
 * @param res Selected sensor measurement resolution
 */
static inline void ltr390_wait_for_measurement(enum ltr390_resolution res)
{
	switch (res) {
	case LTR390_RESOLUTION_20BIT:
		k_sleep(K_MSEC(400));
		break;
	case LTR390_RESOLUTION_19BIT:
		k_sleep(K_MSEC(200));
		break;
	case LTR390_RESOLUTION_18BIT:
		k_sleep(K_MSEC(100));
		break;
	case LTR390_RESOLUTION_17BIT:
		k_sleep(K_MSEC(50));
		break;
	case LTR390_RESOLUTION_16BIT:
		k_sleep(K_MSEC(25));
		break;
	case LTR390_RESOLUTION_13BIT:
		/* 12.5ms rounded up to nearest int */
		k_sleep(K_MSEC(13));
		break;
	}

	/*
	 * Sometimes, sensor takes longer than the defined time to
	 * integrate data. To avoid old data, wait another 10ms. Now, timeouts
	 * (or old data flags) are rare.
	 */
	k_sleep(K_MSEC(10));
}

/**
 * @brief Send enable command for selected mode (either light or uvi),
 *        and read data registers once measurement is ready.
 *
 * @param cfg Device config structure
 * @param mode Selected mode of operation. Either ambient light or uvi.
 * @param buf Buffer to write the 3 data bytes into.
 * @return int errno
 */
static int ltr390_trigger_and_read(const struct ltr390_config *cfg, enum ltr390_mode mode,
				   void *buf)
{
	int rc;
	uint8_t mode_registers[3];
	uint8_t selected_mode;
	uint8_t *read = (uint8_t *)buf;

	switch (mode) {
	case LTR390_MODE_ALS:
		selected_mode = LTR390_MC_ALS_MODE;
		mode_registers[0] = LTR390_ALS_DATA_0;
		mode_registers[1] = LTR390_ALS_DATA_1;
		mode_registers[2] = LTR390_ALS_DATA_2;
		break;
	case LTR390_MODE_UVS:
		selected_mode = LTR390_MC_UVS_MODE;
		mode_registers[0] = LTR390_UVS_DATA_0;
		mode_registers[1] = LTR390_UVS_DATA_1;
		mode_registers[2] = LTR390_UVS_DATA_2;
		break;
	default:
		return -ENOTSUP;
	}

	rc = ltr390_write_register(cfg, LTR390_MAIN_CTRL, LTR390_MC_ACTIVE | selected_mode);
	if (rc < 0) {
		return rc;
	}

	ltr390_wait_for_measurement(cfg->resolution);

	rc = ltr390_read_register(cfg, mode_registers[0], &read[0]);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_read_register(cfg, mode_registers[1], &read[1]);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_read_register(cfg, mode_registers[2], &read[2]);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr390_fetch_measurement_data(const struct ltr390_config *cfg, struct ltr390_data *data)
{
	int rc;
	uint8_t als_buf[3], uvs_buf[3];

	rc = ltr390_trigger_and_read(cfg, LTR390_MODE_ALS, &als_buf);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_trigger_and_read(cfg, LTR390_MODE_UVS, &uvs_buf);
	if (rc < 0) {
		return rc;
	}

	data->light = sys_get_le24(als_buf) & 0x0FFFFF;
	data->uv_index = sys_get_le24(uvs_buf) & 0x0FFFFF;

	return 0;
}

static int ltr390_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct ltr390_config *cfg = dev->config;
	struct ltr390_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_ALL:
		__fallthrough;
	case SENSOR_CHAN_LIGHT:
		return ltr390_fetch_measurement_data(cfg, data);
	case SENSOR_CHAN_UVI:
		return ltr390_fetch_measurement_data(cfg, data);
	default:
		return -ENOTSUP;
	}
}

void sv_div(struct sensor_value *val, uint32_t div)
{
	val->val2 = (val->val1 % div) * 1000000 / div;
	val->val1 /= div;
}

void sv_mult(struct sensor_value *val, uint32_t mult)
{
	val->val2 *= mult;
	val->val1 *= mult;
	if (val->val2 > 999999) {
		val->val1++;
		val->val2 -= 1000000;
	}
}

/**
 * @brief Convert raw bytes into ambient light measurement in lux.
 *        See section 7 of the datasheet.
 *
 * @param cfg Device config structure
 * @param val Ambient light value after conversion
 * @param als_raw Raw measurement bytes obtained from sensor
 */
static inline void ltr390_als_bytes_to_value(const struct ltr390_config *cfg,
					     struct sensor_value *val, uint32_t als_raw)
{
	val->val1 = als_raw;
	val->val2 = 0;

	/* Divide by configured gain */
	switch (cfg->gain) {
	case LTR390_GAIN_1:
		/* nothing */
		break;
	case LTR390_GAIN_3:
		sv_div(val, 3);
		break;
	case LTR390_GAIN_6:
		sv_div(val, 6);
		break;
	case LTR390_GAIN_9:
		sv_div(val, 9);
		break;
	case LTR390_GAIN_18:
		sv_div(val, 18);
		break;
	}

	/* Divide by appropriate integration time according to configured resolution */
	switch (cfg->resolution) {
	case LTR390_RESOLUTION_20BIT:
		sv_div(val, 4);
		break;
	case LTR390_RESOLUTION_19BIT:
		sv_div(val, 2);
		break;
	case LTR390_RESOLUTION_18BIT:
		/* nothing */
		break;
	case LTR390_RESOLUTION_17BIT:
		sv_mult(val, 2); /* divide by 0.5 */
		break;
	case LTR390_RESOLUTION_16BIT:
		sv_mult(val, 4); /* divide by 0.25 */
		break;
	case LTR390_RESOLUTION_13BIT:
		sv_mult(val, 8); /* divide by 0.125 */
		break;
	}

	/* multiply by 0.6 */
	sv_mult(val, 3);
	sv_div(val, 5);
}

/**
 * @brief Convert raw bytes into UV index measurement.
 *        See section 7 of datasheet.
 *
 * @param cfg Device config structure
 * @param val UV index value after conversion
 * @param uvs_raw Raw measurement bytes obtained from sensor
 */
static inline void ltr390_uvs_bytes_to_value(const struct ltr390_config *cfg,
					     struct sensor_value *val, uint32_t uvs_raw)
{
	val->val1 = uvs_raw;
	val->val2 = 0;

	sv_div(val, 2300);
}

static int ltr390_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ltr390_config *cfg = dev->config;
	struct ltr390_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_LIGHT:
		ltr390_als_bytes_to_value(cfg, val, data->light);
		return 0;
	case SENSOR_CHAN_UVI:
		ltr390_uvs_bytes_to_value(cfg, val, data->uv_index);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api ltr390_driver_api = {
	.sample_fetch = ltr390_sample_fetch,
	.channel_get = ltr390_channel_get,
#ifdef CONFIG_LTR390_TRIGGER
	.attr_set = ltr390_attr_set,
	.trigger_set = ltr390_trigger_set,
#endif
};

/**
 * @brief Set registers with values in ltr390_config struct. Reset others.
 *
 * @param cfg Device config structure
 * @return int errno
 */
static int ltr390_write_config(const struct ltr390_config *cfg)
{
	int rc;

	/* Reset main control register */
	rc = ltr390_write_register(cfg, LTR390_MAIN_CTRL, LTR390_RESET_MAIN_CTRL);
	if (rc < 0) {
		return rc;
	}

	/* Set measure rate register to the value in the user's board.overlay */
	uint8_t meas_rate = cfg->resolution << 4 | cfg->rate;

	rc = ltr390_write_register(cfg, LTR390_MEAS_RATE, meas_rate);
	if (rc < 0) {
		return rc;
	}

	/* Set gain register to the value in the user's board.overlay */
	rc = ltr390_write_register(cfg, LTR390_GAIN, cfg->gain);
	if (rc < 0) {
		return rc;
	}

	/* Reset interrupt config register */
	rc = ltr390_write_register(cfg, LTR390_INT_CFG, LTR390_RESET_INT_CFG);
	if (rc < 0) {
		return rc;
	}

#ifdef CONFIG_LTR390_TRIGGER
	/*
	 * The int_persist value is taken from DT, which is in range [1, 16].
	 * INT_PST register takes values in range [0, 15].
	 */
	uint8_t int_pst = (cfg->int_persist - 1) << 4;

	rc = ltr390_write_register(cfg, LTR390_INT_PST, int_pst);
#else
	/* Reset register if triggers disabled */
	rc = ltr390_write_register(cfg, LTR390_INT_PST, LTR390_RESET_INT_PST);
#endif
	if (rc < 0) {
		return rc;
	}

	/* Reset interrupt threshold registers */

	rc = ltr390_write_register(cfg, LTR390_THRES_UP_0, LTR390_RESET_THRES_UP_0);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_write_register(cfg, LTR390_THRES_UP_1, LTR390_RESET_THRES_UP_1);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_write_register(cfg, LTR390_THRES_UP_2, LTR390_RESET_THRES_UP_2);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_write_register(cfg, LTR390_THRES_LO_0, LTR390_RESET_THRES_LO_0);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_write_register(cfg, LTR390_THRES_LO_1, LTR390_RESET_THRES_LO_1);
	if (rc < 0) {
		return rc;
	}

	rc = ltr390_write_register(cfg, LTR390_THRES_LO_2, LTR390_RESET_THRES_LO_2);
	if (rc < 0) {
		return rc;
	}

	return 0;
}

static int ltr390_init(const struct device *dev)
{
	const struct ltr390_config *cfg = dev->config;
	int rc;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	rc = ltr390_write_config(cfg);
	if (rc < 0) {
		LOG_ERR("Could not set defaults");
		return rc;
	}

#ifdef CONFIG_LTR390_TRIGGER
	if (cfg->int_gpio.port) {
		rc = ltr390_setup_interrupt(dev);
		if (rc < 0) {
			LOG_ERR("Could not setup interrupt");
			return rc;
		}
	}
#endif

	return 0;
}

#define LTR390_INST(inst)                                                                          \
	static struct ltr390_data ltr390_data_##inst;                                              \
	static const struct ltr390_config ltr390_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		.resolution = DT_INST_PROP(inst, resolution),                                      \
		.rate = DT_INST_PROP(inst, rate),                                                  \
		.gain = DT_INST_PROP(inst, gain),                                                  \
		IF_ENABLED(CONFIG_LTR390_TRIGGER,                                                  \
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),            \
			    .int_persist = DT_INST_PROP(inst, int_persist)))};                     \
	DEVICE_DT_INST_DEFINE(inst, ltr390_init, NULL, &ltr390_data_##inst, &ltr390_config_##inst, \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &ltr390_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTR390_INST)
