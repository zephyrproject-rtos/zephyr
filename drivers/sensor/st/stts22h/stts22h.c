/* ST Microelectronics STTS22H temperature sensor
 *
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/stts22h.pdf
 */

#define DT_DRV_COMPAT st_stts22h

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "stts22h.h"

LOG_MODULE_REGISTER(STTS22H, CONFIG_SENSOR_LOG_LEVEL);

static inline int stts22h_set_odr_raw(const struct device *dev, stts22h_odr_temp_t odr)
{
	const struct stts22h_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

	return stts22h_temp_data_rate_set(ctx, odr);
}

static int stts22h_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct stts22h_data *data = dev->data;
	const struct stts22h_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	int16_t raw_temp;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel: %d", chan);
		return -ENOTSUP;
	}

	if (stts22h_temperature_raw_get(ctx, &raw_temp) < 0) {
		LOG_ERR("Failed to read sample");
		return -EIO;
	}

	data->sample_temp = raw_temp;

	return 0;
}

static inline void stts22h_temp_convert(struct sensor_value *val, int16_t raw_val)
{
	val->val1 = raw_val / 100;
	val->val2 = ((int32_t)raw_val % 100) * 10000;
}

static int stts22h_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct stts22h_data *data = dev->data;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel: %d", chan);
		return -ENOTSUP;
	}

	stts22h_temp_convert(val, data->sample_temp);

	return 0;
}

static const uint8_t stts22h_map[6] = {0, 1, 25, 50, 100, 200};

static int stts22h_odr_set(const struct device *dev,
			   const struct sensor_value *val)
{
	int odr;

	for (odr = 0; odr < ARRAY_SIZE(stts22h_map); odr++) {
		if (val->val1 <= stts22h_map[odr]) {
			break;
		}
	}

	switch (odr) {
	case 0:
		return stts22h_set_odr_raw(dev, STTS22H_POWER_DOWN);
	case 1:
		return stts22h_set_odr_raw(dev, STTS22H_1Hz);
	case 2:
		return stts22h_set_odr_raw(dev, STTS22H_25Hz);
	case 3:
		return stts22h_set_odr_raw(dev, STTS22H_50Hz);
	case 4:
		return stts22h_set_odr_raw(dev, STTS22H_100Hz);
	case 5:
		return stts22h_set_odr_raw(dev, STTS22H_200Hz);
	default:
		LOG_ERR("bad frequency: %d (odr = %d)", val->val1, odr);
		return -EINVAL;
	}
}

static int stts22h_attr_set(const struct device *dev,
			    enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		LOG_ERR("Invalid channel: %d", chan);
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return stts22h_odr_set(dev, val);
	default:
		LOG_ERR("Attribute %d not supported.", attr);
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(sensor, stts22h_api_funcs) = {
	.attr_set = stts22h_attr_set,
	.sample_fetch = stts22h_sample_fetch,
	.channel_get = stts22h_channel_get,
#if CONFIG_STTS22H_TRIGGER
	.trigger_set = stts22h_trigger_set,
#endif
};

static int stts22h_init_chip(const struct device *dev)
{
	const struct stts22h_config *cfg = dev->config;
	stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;
	uint8_t chip_id, odr;

	if (stts22h_dev_id_get(ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	LOG_INF("chip id 0x%02x", chip_id);

	if (stts22h_auto_increment_set(ctx, 1) < 0) {
		LOG_ERR("Failed to set autoincr");
		return -EIO;
	}

	/* set odr from DT */
	odr = cfg->odr;
	LOG_INF("sensor odr is %d", odr);
	if (stts22h_set_odr_raw(dev, odr) < 0) {
		LOG_ERR("Failed to set sampling rate");
		return -EIO;
	}

	return 0;
}

static int stts22h_init(const struct device *dev)
{
	struct stts22h_data *data = dev->data;
#ifdef CONFIG_STTS22H_TRIGGER
	const struct stts22h_config *cfg = dev->config;
#endif

	LOG_INF("Initialize device %s", dev->name);
	data->dev = dev;

	if (stts22h_init_chip(dev) < 0) {
		LOG_ERR("Failed to initialize chip");
		return -EIO;
	}

#ifdef CONFIG_STTS22H_TRIGGER
	if (cfg->int_gpio.port) {
		if (stts22h_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
	}
#endif

	return 0;
}

#define STTS22H_DEFINE(inst)									\
	static struct stts22h_data stts22h_data_##inst;						\
												\
	static const struct stts22h_config stts22h_config_##inst = {				\
		STMEMSC_CTX_I2C(&stts22h_config_##inst.i2c),					\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		.temp_hi = DT_INST_PROP(inst, temperature_hi_threshold),			\
		.temp_lo = DT_INST_PROP(inst, temperature_lo_threshold),			\
		.odr = DT_INST_PROP(inst, sampling_rate),					\
		IF_ENABLED(CONFIG_STTS22H_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stts22h_init, NULL,					\
			      &stts22h_data_##inst, &stts22h_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &stts22h_api_funcs);			\

DT_INST_FOREACH_STATUS_OKAY(STTS22H_DEFINE)
