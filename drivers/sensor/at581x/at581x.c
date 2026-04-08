/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT andar_at581x

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(at581x, CONFIG_SENSOR_LOG_LEVEL);

/* Register Definitions */
#define AT581X_REG_RESET        0x00
#define AT581X_REG_THRESHOLD_L  0x10
#define AT581X_REG_THRESHOLD_H  0x11
#define AT581X_REG_WIN_LEN      0x31
#define AT581X_REG_WIN_THR      0x32
#define AT581X_REG_SELF_CK_L    0x38
#define AT581X_REG_SELF_CK_H    0x39
#define AT581X_REG_BASE_TIME_L  0x3D
#define AT581X_REG_BASE_TIME_H  0x3E
#define AT581X_REG_OUT_CTRL     0x41
#define AT581X_REG_KEEP_TIME_L  0x42
#define AT581X_REG_KEEP_TIME_H  0x43
#define AT581X_REG_PROT_TIME_L  0x4E
#define AT581X_REG_PROT_TIME_H  0x4F
#define AT581X_REG_RF_CTRL      0x51
#define AT581X_REG_MAGIC        0x55
#define AT581X_REG_GAIN         0x5C

struct at581x_config {
	struct i2c_dt_spec i2c;
	uint16_t threshold;
	uint8_t gain;
	uint16_t base_time_ms;
	uint16_t keep_time_ms;
};

struct at581x_data {
	uint16_t detection_value;
};

static int at581x_reg_write(const struct device *dev, uint8_t reg, uint8_t val)
{
	const struct at581x_config *cfg = dev->config;
	uint8_t buf[2] = {reg, val};

	return i2c_write_dt(&cfg->i2c, buf, sizeof(buf));
}

static int at581x_reg_read(const struct device *dev, uint8_t reg, uint8_t *val)
{
	const struct at581x_config *cfg = dev->config;

	return i2c_write_read_dt(&cfg->i2c, &reg, 1, val, 1);
}

static int at581x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct at581x_data *data = dev->data;
	uint8_t val_l, val_h;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	/* Read detection threshold registers to get current detection value */
	ret = at581x_reg_read(dev, AT581X_REG_SELF_CK_L, &val_l);
	if (ret < 0) {
		return ret;
	}

	ret = at581x_reg_read(dev, AT581X_REG_SELF_CK_H, &val_h);
	if (ret < 0) {
		return ret;
	}

	data->detection_value = (val_h << 8) | val_l;

	return 0;
}

static int at581x_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct at581x_data *data = dev->data;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	/* Return detection value as sensor reading */
	val->val1 = data->detection_value;
	val->val2 = 0;

	return 0;
}

static const struct sensor_driver_api at581x_driver_api = {
	.sample_fetch = at581x_sample_fetch,
	.channel_get = at581x_channel_get,
};

static int at581x_init(const struct device *dev)
{
	const struct at581x_config *cfg = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c)) {
		LOG_ERR("I2C bus %s not ready", cfg->i2c.bus->name);
		return -ENODEV;
	}

	/* Factory Demo Setup Sequence */
	ret = at581x_reg_write(dev, 0x68, 0x68);
	if (ret < 0) {
		LOG_ERR("Failed to write factory setup register");
		return ret;
	}

	ret = at581x_reg_write(dev, 0x67, 0x8B);
	if (ret < 0) {
		LOG_ERR("Failed to write factory setup register");
		return ret;
	}

	/* Set Detection Threshold */
	ret = at581x_reg_write(dev, AT581X_REG_THRESHOLD_L, cfg->threshold & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write threshold low register");
		return ret;
	}

	ret = at581x_reg_write(dev, AT581X_REG_THRESHOLD_H, (cfg->threshold >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write threshold high register");
		return ret;
	}

	/* Set Gain */
	ret = at581x_reg_write(dev, AT581X_REG_GAIN, cfg->gain);
	if (ret < 0) {
		LOG_ERR("Failed to write gain register");
		return ret;
	}

	/* Set Base Time */
	ret = at581x_reg_write(dev, AT581X_REG_BASE_TIME_L, cfg->base_time_ms & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write base time low register");
		return ret;
	}

	ret = at581x_reg_write(dev, AT581X_REG_BASE_TIME_H, (cfg->base_time_ms >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write base time high register");
		return ret;
	}

	/* Set Output Control */
	ret = at581x_reg_write(dev, AT581X_REG_OUT_CTRL, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to write output control register");
		return ret;
	}

	/* Set Keep Time */
	ret = at581x_reg_write(dev, AT581X_REG_KEEP_TIME_L, cfg->keep_time_ms & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write keep time low register");
		return ret;
	}

	ret = at581x_reg_write(dev, AT581X_REG_KEEP_TIME_H, (cfg->keep_time_ms >> 8) & 0xFF);
	if (ret < 0) {
		LOG_ERR("Failed to write keep time high register");
		return ret;
	}

	/* Magic Register (Required for stability) */
	ret = at581x_reg_write(dev, AT581X_REG_MAGIC, 0x04);
	if (ret < 0) {
		LOG_ERR("Failed to write magic register");
		return ret;
	}

	/* Soft Reset to apply settings */
	ret = at581x_reg_write(dev, AT581X_REG_RESET, 0x00);
	if (ret < 0) {
		LOG_ERR("Failed to reset device");
		return ret;
	}

	k_msleep(10);

	ret = at581x_reg_write(dev, AT581X_REG_RESET, 0x01);
	if (ret < 0) {
		LOG_ERR("Failed to complete reset sequence");
		return ret;
	}

	LOG_INF("AT581X radar sensor initialized");

	return 0;
}

#define AT581X_INIT(inst)							\
	static struct at581x_data at581x_data_##inst;				\
										\
	static const struct at581x_config at581x_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),				\
		.threshold = DT_INST_PROP_OR(inst, threshold, 200),		\
		.gain = DT_INST_PROP_OR(inst, gain, 0x1B),			\
		.base_time_ms = DT_INST_PROP_OR(inst, base_time_ms, 500),	\
		.keep_time_ms = DT_INST_PROP_OR(inst, keep_time_ms, 1500),	\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, at581x_init, NULL,			\
				      &at581x_data_##inst,			\
				      &at581x_config_##inst, POST_KERNEL,	\
				      CONFIG_SENSOR_INIT_PRIORITY,		\
				      &at581x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AT581X_INIT)