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
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "stts22h.h"

LOG_MODULE_REGISTER(STTS22H, CONFIG_SENSOR_LOG_LEVEL);

#define ST22H_RANGE_LOWEST_TEMP	-40
#define ST22H_RANGE_HIGHEST_TEMP 127

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
	struct stts22h_data *data = dev->data;
	stts22h_odr_temp_t odr_raw;
	int odr;
	int ret;

	for (odr = 0; odr < ARRAY_SIZE(stts22h_map); odr++) {
		if (val->val1 <= stts22h_map[odr]) {
			break;
		}
	}

	switch (odr) {
	case 0:
		odr_raw = STTS22H_POWER_DOWN;
		break;
	case 1:
		odr_raw = STTS22H_1Hz;
		break;
	case 2:
		odr_raw = STTS22H_25Hz;
		break;
	case 3:
		odr_raw = STTS22H_50Hz;
		break;
	case 4:
		odr_raw = STTS22H_100Hz;
		break;
	case 5:
		odr_raw = STTS22H_200Hz;
		break;
	default:
		LOG_ERR("bad frequency: %d (odr = %d)", val->val1, odr);
		return -EINVAL;
	}

	ret = stts22h_set_odr_raw(dev, odr_raw);
	if (ret < 0) {
		return ret;
	}

	/* Restored on PM resume */
	data->odr = odr_raw;

	return 0;
}
#if CONFIG_STTS22H_TRIGGER
static inline uint32_t stts22h_val_to_threshold(const struct sensor_value *val)
{
	if (val->val1 < ST22H_RANGE_LOWEST_TEMP || val->val1 > ST22H_RANGE_HIGHEST_TEMP) {
		LOG_ERR("Invalid value: %d", val->val1);
		return UINT32_MAX;
	}
	/* The conversion formula is: (degC / 0.64) + 63, we multiply by 100*/
	int32_t temperature_degree_times100 = val->val1 * 100 + val->val2 / 10000;
	uint32_t raw_value = (temperature_degree_times100 / 64) + 63;

	if (raw_value > 0xFF) {
		LOG_ERR("Invalid value: %d", raw_value);
		return UINT32_MAX;
	}

	return raw_value;
}
#endif


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
#if CONFIG_STTS22H_TRIGGER
	case SENSOR_ATTR_UPPER_THRESH: {
		const struct stts22h_config *cfg = dev->config;
		struct stts22h_data *data = dev->data;
		stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

		uint32_t raw_value = stts22h_val_to_threshold(val);

		if (raw_value == UINT32_MAX) {
			return -EINVAL;
		}
		if (stts22h_temp_trshld_high_set(ctx, (uint8_t)raw_value) < 0) {
			LOG_DBG("Could not set high threshold");
			return -EIO;
		}
		/* Restored on PM turn-on when int-gpios is set */
		data->temp_hi = (uint8_t)raw_value;
		return 0;
	}
	case SENSOR_ATTR_LOWER_THRESH: {
		const struct stts22h_config *cfg = dev->config;
		struct stts22h_data *data = dev->data;
		stmdev_ctx_t *ctx = (stmdev_ctx_t *)&cfg->ctx;

		uint32_t raw_value = stts22h_val_to_threshold(val);

		if (raw_value == UINT32_MAX) {
			return -EINVAL;
		}
		if (stts22h_temp_trshld_low_set(ctx, (uint8_t)raw_value) < 0) {
			LOG_DBG("Could not set low threshold ");
			return -EIO;
		}
		/* Restored on PM turn-on when int-gpios is set */
		data->temp_lo = (uint8_t)raw_value;
		return 0;
	}
#endif
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
	uint8_t chip_id;

	if (stts22h_dev_id_get(ctx, &chip_id) < 0) {
		LOG_ERR("Failed reading chip id");
		return -EIO;
	}

	LOG_DBG("chip id 0x%02x", chip_id);

	if (stts22h_auto_increment_set(ctx, 1) < 0) {
		LOG_ERR("Failed to set autoincr");
		return -EIO;
	}

	/* CTRL keeps its mode across an MCU reset that does not cycle the supply */
	if (stts22h_set_odr_raw(dev, STTS22H_POWER_DOWN) < 0) {
		LOG_ERR("Failed to set power-down mode");
		return -EIO;
	}

	return 0;
}

/*
 * The chip leaves TURN_ON in power-down mode with auto-increment set and,
 * with int-gpios, the thresholds restored and the interrupt enabled.
 * RESUME restarts conversions at the last ODR.
 */
static int stts22h_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct stts22h_data *data = dev->data;
#ifdef CONFIG_STTS22H_TRIGGER
	const struct stts22h_config *cfg = dev->config;
#endif
	int ret;

	/*
	 * Every action but TURN_OFF needs I2C. I2C cannot block in the idle
	 * thread (system-managed device PM, which skips -ENOTSUP) or in an ISR
	 * (a power domain that reports power changes from its interrupt). A
	 * TURN_ON that cannot run must be reported as failed, not unsupported.
	 * TURN_OFF handles those contexts in stts22h_config_interrupt().
	 */
	if ((action != PM_DEVICE_ACTION_TURN_OFF) && !k_can_yield()) {
		return (action == PM_DEVICE_ACTION_TURN_ON) ? -EWOULDBLOCK : -ENOTSUP;
	}

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		/* The power domain's startup-delay-us must cover the 12 ms boot time */
		ret = stts22h_init_chip(dev);
		if (ret < 0) {
			return ret;
		}
#ifdef CONFIG_STTS22H_TRIGGER
		if (cfg->int_gpio.port != NULL) {
			return stts22h_config_interrupt(dev, true);
		}
#endif
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		return stts22h_set_odr_raw(dev, data->odr);
	case PM_DEVICE_ACTION_SUSPEND:
		return stts22h_set_odr_raw(dev, STTS22H_POWER_DOWN);
	case PM_DEVICE_ACTION_TURN_OFF:
#ifdef CONFIG_STTS22H_TRIGGER
		/* Without supply the INT line level is undefined */
		if (cfg->int_gpio.port != NULL) {
			return stts22h_config_interrupt(dev, false);
		}
#endif
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int stts22h_init(const struct device *dev)
{
	const struct stts22h_config *cfg = dev->config;
	struct stts22h_data *data = dev->data;

	LOG_INF("Initialize device %s", dev->name);
	data->dev = dev;
	data->odr = cfg->odr;

#ifdef CONFIG_STTS22H_TRIGGER
	data->temp_hi = cfg->temp_hi;
	data->temp_lo = cfg->temp_lo;

	/* A failed init may be retried; the thread and GPIO callback exist once */
	if (cfg->int_gpio.port != NULL && !data->trigger_ready) {
		if (stts22h_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupt.");
			return -EIO;
		}
		data->trigger_ready = true;
	}
#endif

	return pm_device_driver_init(dev, stts22h_pm_action);
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
	PM_DEVICE_DT_INST_DEFINE(inst, stts22h_pm_action);					\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, stts22h_init, PM_DEVICE_DT_INST_GET(inst),		\
			      &stts22h_data_##inst, &stts22h_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &stts22h_api_funcs);			\

DT_INST_FOREACH_STATUS_OKAY(STTS22H_DEFINE)
