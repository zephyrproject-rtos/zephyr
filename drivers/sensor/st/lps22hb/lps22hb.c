/* lps22hb.c - Driver for LPS22HB and LPS33HW pressure sensors */

/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lps22hb_press

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "lps22hb.h"

LOG_MODULE_REGISTER(LPS22HB, CONFIG_SENSOR_LOG_LEVEL);

static inline int lps22hb_set_odr_raw(const struct device *dev, uint8_t odr)
{
	const struct lps22hb_config *config = dev->config;

	return i2c_reg_update_byte_dt(&config->i2c, LPS22HB_REG_CTRL_REG1,
				      LPS22HB_MASK_CTRL_REG1_ODR,
				      odr << LPS22HB_SHIFT_CTRL_REG1_ODR);
}

static int lps22hb_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct lps22hb_data *data = dev->data;
	const struct lps22hb_config *config = dev->config;
	uint8_t out[5];

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_PRESS &&
	    chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	if (i2c_burst_read_dt(&config->i2c, LPS22HB_REG_PRESS_OUT_XL,
			      out, 5) < 0) {
		LOG_DBG("Failed to read sample");
		return -EIO;
	}

	/* Pressure is a 24-bit two's complement value. */
	data->sample_press = sign_extend(sys_get_le24(out), 23);
	data->sample_temp = (int16_t)sys_get_le16(&out[3]);

	return 0;
}

static int lps22hb_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct lps22hb_data *data = dev->data;

	if (chan == SENSOR_CHAN_PRESS) {
		/* 4096 LSB/hPa. The sensor API reports kilopascals. */
		return sensor_value_from_micro(val,
				(int64_t)data->sample_press * 100000 / 4096);
	} else if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		/* 100 LSB per degree Celsius. */
		return sensor_value_from_micro(val,
				(int64_t)data->sample_temp * 10000);
	}

	return -ENOTSUP;
}

static DEVICE_API(sensor, lps22hb_api_funcs) = {
	.sample_fetch = lps22hb_sample_fetch,
	.channel_get = lps22hb_channel_get,
};

static int lps22hb_init_chip(const struct device *dev)
{
	const struct lps22hb_config *config = dev->config;
	uint8_t chip_id;

	if (i2c_reg_read_byte_dt(&config->i2c, LPS22HB_REG_WHO_AM_I,
				 &chip_id) < 0) {
		LOG_DBG("Failed reading chip id");
		goto err_poweroff;
	}

	if (chip_id != LPS22HB_VAL_WHO_AM_I) {
		LOG_DBG("Invalid chip id 0x%x", chip_id);
		goto err_poweroff;
	}

	if (lps22hb_set_odr_raw(dev, config->odr) < 0) {
		LOG_DBG("Failed to set sampling rate");
		goto err_poweroff;
	}

	if (i2c_reg_update_byte_dt(&config->i2c, LPS22HB_REG_CTRL_REG1,
				   LPS22HB_MASK_CTRL_REG1_BDU,
				   (1 << LPS22HB_SHIFT_CTRL_REG1_BDU)) < 0) {
		LOG_DBG("Failed to set BDU");
		goto err_poweroff;
	}

	return 0;

err_poweroff:
	return -EIO;
}

static int lps22hb_init(const struct device *dev)
{
	const struct lps22hb_config * const config = dev->config;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	if (lps22hb_init_chip(dev) < 0) {
		LOG_DBG("Failed to initialize chip");
		return -EIO;
	}

	return 0;
}

/* Binding odr is in Hz. The register field uses the same encoding on both parts. */
#define LPS22HB_ODR_FROM_HZ(hz)			\
	((hz) == 1 ? 1 :			\
	 (hz) == 10 ? 2 :			\
	 (hz) == 25 ? 3 :			\
	 (hz) == 50 ? 4 :			\
	 (hz) == 75 ? 5 : 0)

#define LPS22HB_INST_ODR(inst)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, odr),				\
		    (LPS22HB_ODR_FROM_HZ(DT_INST_PROP(inst, odr))),		\
		    (LPS22HB_DEFAULT_SAMPLING_RATE))

#define LPS22HB_DEFINE(inst, model)						\
	static struct lps22hb_data model##_data_##inst;			\
										\
	static const struct lps22hb_config model##_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		.odr = LPS22HB_INST_ODR(inst),					\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, lps22hb_init, NULL,			\
			&model##_data_##inst,					\
			&model##_config_##inst,					\
			POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,		\
			&lps22hb_api_funcs);

DT_INST_FOREACH_STATUS_OKAY_VARGS(LPS22HB_DEFINE, lps22hb)
#undef DT_DRV_COMPAT

#define DT_DRV_COMPAT st_lps33hw
DT_INST_FOREACH_STATUS_OKAY_VARGS(LPS22HB_DEFINE, lps33hw)
#undef DT_DRV_COMPAT
