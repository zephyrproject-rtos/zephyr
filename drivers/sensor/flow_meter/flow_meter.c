/*
 * Copyright (c) 2026 Nicolas Belin <nbelin@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_flow_meter

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flow_meter, CONFIG_SENSOR_LOG_LEVEL);

struct flow_meter_config {
	struct gpio_dt_spec pulse_gpio;
	struct gpio_dt_spec power_gpio; /* optional: port == NULL if absent */
	uint32_t pulses_per_liter;
};

struct flow_meter_data {
	const struct device *dev;

	/* GPIO callback registered at init, fired on each pulse. */
	struct gpio_callback pulse_cb;

	/* Runtime calibration — mirrors DT default, overridable via attr_set. */
	uint32_t pulses_per_liter;

	/* 64-bit pulse counter: low word incremented in ISR, high word
	 * incremented in ISR when low wraps to 0.
	 */
	atomic_t pulses;
	atomic_t pulses_high;
	uint64_t pulses_total;

	/* Volume threshold for SENSOR_TRIG_THRESHOLD on SENSOR_CHAN_VOLUME.
	 * Stored as a delta in pulses from volume_threshold_start so the
	 * ISR comparison is wraparound-safe: (pulses_low - start) >= delta.
	 * delta == 0 means disabled.
	 */
	atomic_t volume_threshold_start;
	atomic_t volume_threshold_delta;

	/* Trigger handler, spec, and deferred work item. */
	sensor_trigger_handler_t volume_handler;
	const struct sensor_trigger *volume_trig;
	struct k_work volume_work;

	/* flow rate computed in sample_fetch, consumed by channel_get. */
	int32_t rate_val1;
	int32_t rate_val2;

	/* State carried across sample_fetch calls for rate calculation. */
	uint64_t last_fetch_pulses;
	int64_t last_fetch_uptime_ms;
};

static void pulses_to_liters(struct sensor_value *val, uint64_t pulses, uint32_t pulses_per_liter)
{
	uint64_t uL = pulses * 1000000 / pulses_per_liter;

	val->val1 = (int32_t)(uL / 1000000);
	val->val2 = (int32_t)(uL - (uint64_t)val->val1 * 1000000);
}

static void volume_work_handler(struct k_work *work)
{
	struct flow_meter_data *data = CONTAINER_OF(work, struct flow_meter_data, volume_work);

	if (data->volume_handler) {
		data->volume_handler(data->dev, data->volume_trig);
	}
}

static void pulse_gpio_callback(const struct device *gpio_dev, struct gpio_callback *cb,
				uint32_t pins)
{
	struct flow_meter_data *data = CONTAINER_OF(cb, struct flow_meter_data, pulse_cb);

	uint32_t pulses_low = (uint32_t)(atomic_inc(&data->pulses) + 1);

	if (pulses_low == 0) {
		atomic_inc(&data->pulses_high);
	}

	atomic_val_t delta = atomic_get(&data->volume_threshold_delta);

	if (delta > 0 && data->volume_handler) {
		uint32_t start = (uint32_t)atomic_get(&data->volume_threshold_start);

		/* Unsigned subtraction gives the number of pulses since the
		 * threshold was armed. This also is correct across a 32-bit wraparound
		 * of pulses_low.
		 */
		if ((pulses_low - start) >= (uint32_t)delta) {
			atomic_set(&data->volume_threshold_delta, 0);
			k_work_submit(&data->volume_work);
		}
	}
}

/*
 * Safely read a 64-bit counter maintained as two 32-bit atomics.
 *
 * The ISR increments *high only after *low wraps to 0. Reading *low before
 * *high risks reading a stale high: an ISR wrap between the two reads would
 * combine the old high with the new low, producing a value 2^32 too small.
 * Fix: read high, read low, verify high unchanged — retry if it changed.
 */
static uint64_t read_counter_64(const atomic_t *low, const atomic_t *high)
{
	uint32_t lo, hi;

	do {
		hi = (uint32_t)atomic_get(high);
		lo = (uint32_t)atomic_get(low);
	} while ((uint32_t)atomic_get(high) != hi);

	return ((uint64_t)hi << 32) | lo;
}

static int flow_meter_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct flow_meter_data *data = dev->data;
	int64_t now_ms = k_uptime_get();

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_FLOW_RATE &&
	    chan != SENSOR_CHAN_VOLUME) {
		return -ENOTSUP;
	}

	data->pulses_total = read_counter_64(&data->pulses, &data->pulses_high);

	uint64_t delta_pulses = data->pulses_total - data->last_fetch_pulses;
	int64_t delta_ms = now_ms - data->last_fetch_uptime_ms;

	data->last_fetch_pulses = data->pulses_total;
	data->last_fetch_uptime_ms = now_ms;

	if (delta_ms > 0 && delta_pulses > 0) {
		/* delta_pulses × 60000 × 10^6 / ppl / delta_ms = L/min scaled by 10^6 */
		uint64_t rate_1M = delta_pulses * 60000 * 1000000 / data->pulses_per_liter /
				   (uint64_t)delta_ms;

		data->rate_val1 = (int32_t)(rate_1M / 1000000);
		data->rate_val2 = (int32_t)(rate_1M - (uint64_t)data->rate_val1 * 1000000);
	} else {
		data->rate_val1 = 0;
		data->rate_val2 = 0;
	}

	return 0;
}

static int flow_meter_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	const struct flow_meter_data *data = dev->data;

	switch ((int)chan) {
	case SENSOR_CHAN_FLOW_RATE:
		val->val1 = data->rate_val1;
		val->val2 = data->rate_val2;
		return 0;

	case SENSOR_CHAN_VOLUME:
		pulses_to_liters(val, data->pulses_total, data->pulses_per_liter);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int flow_meter_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	struct flow_meter_data *data = dev->data;

	switch ((int)attr) {
	case SENSOR_ATTR_CALIBRATION:
		if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_FLOW_RATE &&
		    chan != SENSOR_CHAN_VOLUME) {
			return -ENOTSUP;
		}
		if (val->val1 <= 0 || val->val2 != 0) {
			return -EINVAL;
		}
		data->pulses_per_liter = (uint32_t)val->val1;
		return 0;

	case SENSOR_ATTR_UPPER_THRESH: {
		if (chan != SENSOR_CHAN_VOLUME) {
			return -ENOTSUP;
		}
		if (val->val1 < 0 || val->val2 < 0 || val->val2 >= 1000000) {
			return -EINVAL;
		}
		uint64_t delta_uL = (uint64_t)val->val1 * 1000000 + (uint32_t)val->val2;

		if (delta_uL > UINT64_MAX / data->pulses_per_liter) {
			return -EINVAL;
		}

		uint64_t delta_pulses = delta_uL * data->pulses_per_liter / 1000000;

		/* The ISR stores the delta in an atomic_val_t (32-bit on most
		 * targets) and treats positive values as armed, so the delta
		 * must fit in INT32_MAX.
		 */
		if (delta_pulses > INT32_MAX) {
			return -EINVAL;
		}

		/* Drop pending work from a previously fired threshold so the
		 * handler is not called for an already-consumed event.
		 */
		k_work_cancel(&data->volume_work);

		/* Disarm before moving the start point so an ISR landing
		 * between the two writes cannot compare the new start against
		 * a stale delta.
		 */
		atomic_set(&data->volume_threshold_delta, 0);
		atomic_set(&data->volume_threshold_start, (atomic_val_t)atomic_get(&data->pulses));
		atomic_set(&data->volume_threshold_delta,
			   (delta_uL > 0 && delta_pulses == 0) ? 1 : (atomic_val_t)delta_pulses);
		return 0;
	}

	default:
		return -ENOTSUP;
	}
}

static int flow_meter_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	const struct flow_meter_data *data = dev->data;

	switch ((int)attr) {
	case SENSOR_ATTR_CALIBRATION:
		if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_FLOW_RATE &&
		    chan != SENSOR_CHAN_VOLUME) {
			return -ENOTSUP;
		}
		val->val1 = (int32_t)data->pulses_per_liter;
		val->val2 = 0;
		return 0;

	case SENSOR_ATTR_UPPER_THRESH:
		if (chan != SENSOR_CHAN_VOLUME) {
			return -ENOTSUP;
		}
		pulses_to_liters(val, (uint64_t)(uint32_t)atomic_get(&data->volume_threshold_delta),
				 data->pulses_per_liter);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int flow_meter_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	struct flow_meter_data *data = dev->data;

	if (trig->type == SENSOR_TRIG_THRESHOLD && trig->chan == SENSOR_CHAN_VOLUME) {
		if (!handler) {
			atomic_set(&data->volume_threshold_delta, 0);
		}
		/* Drop pending work from a previously fired threshold so the
		 * new handler is not called for an already-consumed event.
		 */
		k_work_cancel(&data->volume_work);
		data->volume_trig = trig;
		data->volume_handler = handler;
		return 0;
	}

	return -ENOTSUP;
}

static int flow_meter_init(const struct device *dev)
{
	const struct flow_meter_config *cfg = dev->config;
	struct flow_meter_data *data = dev->data;
	int ret;

	data->pulses_per_liter = cfg->pulses_per_liter;

	k_work_init(&data->volume_work, volume_work_handler);

	if (cfg->power_gpio.port) {
		if (!gpio_is_ready_dt(&cfg->power_gpio)) {
			LOG_ERR_DEVICE_NOT_READY(cfg->power_gpio.port);
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&cfg->power_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure power GPIO: %d", ret);
			return ret;
		}
	}

	if (!gpio_is_ready_dt(&cfg->pulse_gpio)) {
		LOG_ERR_DEVICE_NOT_READY(cfg->pulse_gpio.port);
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&cfg->pulse_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure pulse GPIO: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->pulse_cb, pulse_gpio_callback, BIT(cfg->pulse_gpio.pin));
	ret = gpio_add_callback(cfg->pulse_gpio.port, &data->pulse_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add pulse GPIO callback: %d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&cfg->pulse_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to configure pulse GPIO interrupt: %d", ret);
		gpio_remove_callback(cfg->pulse_gpio.port, &data->pulse_cb);
		return ret;
	}

	data->last_fetch_uptime_ms = k_uptime_get();

	return 0;
}

static DEVICE_API(sensor, flow_meter_driver_api) = {
	.sample_fetch = flow_meter_sample_fetch,
	.channel_get = flow_meter_channel_get,
	.attr_set = flow_meter_attr_set,
	.attr_get = flow_meter_attr_get,
	.trigger_set = flow_meter_trigger_set,
};

#define FLOW_METER_INIT(index)                                                                     \
	static struct flow_meter_data flow_meter_data_##index = {                                  \
		.dev = DEVICE_DT_INST_GET(index),                                                  \
	};                                                                                         \
	static const struct flow_meter_config flow_meter_config_##index = {                        \
		.pulse_gpio = GPIO_DT_SPEC_INST_GET(index, pulse_gpios),                           \
		.power_gpio = GPIO_DT_SPEC_INST_GET_OR(index, power_gpios, {0}),                   \
		.pulses_per_liter = DT_INST_PROP(index, pulses_per_liter),                         \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(index, &flow_meter_init, NULL, &flow_meter_data_##index,      \
				     &flow_meter_config_##index, POST_KERNEL,                      \
				     CONFIG_SENSOR_INIT_PRIORITY, &flow_meter_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FLOW_METER_INIT)
