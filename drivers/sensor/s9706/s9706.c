/*
 * Copyright (c) 2026 Carl Zeiss AG
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hamamatsu_s9706

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/drivers/sensor/s9706.h>

LOG_MODULE_REGISTER(S9706, CONFIG_SENSOR_LOG_LEVEL);

#define S9706_NUM_BITS 12

enum s9706_channel {
	RED,
	GREEN,
	BLUE,
	NUM_OF_COLOR_CHANNELS
};

struct s9706_data {
	uint16_t samples[NUM_OF_COLOR_CHANNELS];

	uint32_t frequency;        /* Readout frequency */
	uint32_t integration_time; /* unit: us */
	bool high_range;

	struct k_timer integration_timer;
};

struct s9706_dev_config {
	struct gpio_dt_spec range;
	struct gpio_dt_spec gate;
	struct gpio_dt_spec data;
	struct gpio_dt_spec clock;
};

static int s9706_convert_channel_to_index(enum sensor_channel chan)
{
	switch (chan) {
	case SENSOR_CHAN_RED:
		return RED;
	case SENSOR_CHAN_GREEN:
		return GREEN;
	case SENSOR_CHAN_BLUE:
		return BLUE;
	default:
		return -ENOTSUP;
	}
}

static int s9706_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct s9706_dev_config *config = dev->config;
	struct s9706_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("S9706 can only change attributes for all channels together");
		return -ENOTSUP;
	}

	if (val->val2 != 0) {
		LOG_ERR("S9706 does not support fractional values in attributes");
		return -ENOTSUP;
	}

	if (val->val1 <= 0) {
		LOG_ERR("S9706 does not support non-positive values in attributes");
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_S9706_CLOCK || attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		/* Alias attribute supported for easier use on the sensor shell */
		if (val->val1 > MHZ(2)) {
			LOG_WRN_ONCE("Attempting to use a clock frequency of %d, which is higher "
				     "than the 2MHz recommended by the datasheet",
				     val->val1);
		}
		data->frequency = val->val1;
	} else if (attr == SENSOR_ATTR_S9706_INTEGRATION_TIME ||
		   attr == SENSOR_ATTR_BATCH_DURATION) {
		/* Alias attribute supported for easier use on the sensor shell */
		data->integration_time = val->val1;
	} else if (attr == SENSOR_ATTR_RESOLUTION) {
		if (!device_is_ready(config->range.port)) {
			LOG_ERR("RANGE IO port not configured, can't change range");
			return -ENODEV;
		}

		if (val->val1 == 0) {
			/* Low range mode */
			gpio_pin_set_dt(&config->range, 0);
		} else if (val->val1 == 1) {
			/* High range mode */
			gpio_pin_set_dt(&config->range, 1);
		} else {
			LOG_ERR("Only 0 and 1 are supported as resolutions for low and high range");
		}
	} else {
		LOG_ERR("S9706 does not support attribute %d", attr);
		return -ENOTSUP;
	}

	return 0;
}

static int s9706_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	struct s9706_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("S9706 can only get attributes for all channels together");
		return -ENOTSUP;
	}

	if (attr == SENSOR_ATTR_S9706_CLOCK || attr == SENSOR_ATTR_SAMPLING_FREQUENCY) {
		val->val1 = data->frequency;
		val->val2 = 0;
	} else if (attr == SENSOR_ATTR_S9706_INTEGRATION_TIME ||
		   attr == SENSOR_ATTR_BATCH_DURATION) {
		val->val1 = data->integration_time;
		val->val2 = 0;
	} else if (attr == SENSOR_ATTR_RESOLUTION) {
		val->val1 = data->high_range ? 1 : 0;
		val->val2 = 0;
	} else {
		LOG_ERR("S9706 does not support attribute %d", attr);
		return -ENOTSUP;
	}

	return 0;
}

static int s9706_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct s9706_dev_config *config = dev->config;
	struct s9706_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("S9706 can only fetch all channels together");
		return -ENOTSUP;
	}

	ret = gpio_pin_set_dt(&config->gate, 1);
	if (ret < 0) {
		LOG_ERR("Failed to enable gate (%d)", ret);
		return ret;
	}

	k_timer_start(&data->integration_timer, K_USEC(data->integration_time), K_FOREVER);
	k_timer_status_sync(&data->integration_timer);

	ret = gpio_pin_set_dt(&config->gate, 0);
	if (ret < 0) {
		LOG_ERR("Failed to disable gate (%d)", ret);
		return ret;
	}

	/* datasheet specifies >4us between end of integration and readout (t1) */
	k_busy_wait(4);

	uint32_t cycle_wait_us = MAX((1000000 / data->frequency) / 2, 1);

	/* outer loop, for each channel */
	for (int i = 0; i < NUM_OF_COLOR_CHANNELS; i++) {
		uint16_t rdata = 0;

		/* inner loop, for each bit */
		for (int j = 0; j < S9706_NUM_BITS; j++) {
			gpio_pin_set_dt(&config->clock, 1);
			k_busy_wait(cycle_wait_us);

			gpio_pin_set_dt(&config->clock, 0);

			int bit = gpio_pin_get_dt(&config->data);

			if (bit < 0) {
				LOG_ERR("Failed to read data bit (%d)", bit);
				return bit;
			}

			rdata |= bit << j;

			k_busy_wait(cycle_wait_us);
		}

		data->samples[i] = rdata;

		/* datasheet specifies >3us delay between channels (t2) */
		k_busy_wait(3);
	}

	/* datasheet delay between clock to gate high not needed, covered by t2 above */

	return 0;
}

static int s9706_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	const struct s9706_data *data = dev->data;

	const int index = s9706_convert_channel_to_index(chan);

	if (index < 0) {
		return index;
	}

	uint32_t raw_value = data->samples[index];

	/* TODO: scale to lux */

	val->val1 = raw_value;
	val->val2 = 0;

	return 0;
}

static int s9706_sensor_init(const struct device *dev)
{
	const struct s9706_dev_config *config = dev->config;
	struct s9706_data *data = dev->data;
	int ret;

	LOG_DBG("Initializing S9706 sensor");

	k_timer_init(&data->integration_timer, NULL, NULL);

	if (!device_is_ready(config->range.port)) {
		LOG_DBG("S9706 RANGE GPIO not ready, range configuration will be disabled");
	}

	if (!device_is_ready(config->gate.port)) {
		LOG_ERR("S9706 GATE GPIO not ready!");
		return -ENODEV;
	}

	if (!device_is_ready(config->data.port)) {
		LOG_ERR("S9706 DATA GPIO not ready!");
		return -ENODEV;
	}

	if (!device_is_ready(config->clock.port)) {
		LOG_ERR("S9706 CLOCK GPIO not ready!");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->gate, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure gate GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->clock, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure clock GPIO (%d)", ret);
		return ret;
	}

	ret = gpio_pin_configure_dt(&config->data, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure data GPIO (%d)", ret);
		return ret;
	}

	/* Only try to configure range if we actually got a phandle */
	if (device_is_ready(config->range.port)) {
		ret = gpio_pin_configure_dt(&config->range, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure range GPIO (%d)", ret);
			return ret;
		}

#ifdef CONFIG_S9706_DEFAULT_RANGE
		if (gpio_pin_set_dt(&config->range, 1) < 0) {
			LOG_ERR("Could not set gate GPIO (%d)", ret);
			return ret;
		}
		data->high_range = true;
#else  /* !CONFIG_S9706_DEFAULT_RANGE */
		if (gpio_pin_set_dt(&config->range, 0) < 0) {
			LOG_ERR("Could not set gate GPIO (%d)", ret);
			return ret;
		}
		data->high_range = false;
#endif /* !CONFIG_S9706_DEFAULT_RANGE */
	}

	if (gpio_pin_set_dt(&config->gate, 0) < 0) {
		LOG_ERR("Could not set gate GPIO (%d)", ret);
		return ret;
	}

	if (gpio_pin_set_dt(&config->clock, 0) < 0) {
		LOG_ERR("Could not set clock GPIO (%d)", ret);
		return ret;
	}

	LOG_DBG("S9706 initialized.");

	return 0;
}

static DEVICE_API(sensor, s9706_api) = {
	.attr_set = s9706_attr_set,
	.attr_get = s9706_attr_get,
	.sample_fetch = s9706_sample_fetch,
	.channel_get = s9706_channel_get,
};

#define S9706_SENSOR_INIT(i)									\
	static struct s9706_data s9706_sensor_data_##i = {					\
		.frequency = DT_INST_PROP(i, max_frequency),					\
		.integration_time = DT_INST_PROP(i, integration_time)				\
	};											\
												\
	static const struct s9706_dev_config s9706_sensor_config_##i = {			\
		.range = GPIO_DT_SPEC_INST_GET(i, range_gpios),					\
		.gate = GPIO_DT_SPEC_INST_GET(i, gate_gpios),					\
		.data = GPIO_DT_SPEC_INST_GET(i, data_gpios),					\
		.clock = GPIO_DT_SPEC_INST_GET(i, clk_gpios),					\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(i, s9706_sensor_init, NULL, &s9706_sensor_data_##i,	\
				     &s9706_sensor_config_##i, POST_KERNEL,			\
				     CONFIG_SENSOR_INIT_PRIORITY, &s9706_api);

DT_INST_FOREACH_STATUS_OKAY(S9706_SENSOR_INIT)
