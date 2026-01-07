/*
 * Copyright (c) 2024 Navimatix GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_pico_pit_channel

#include <hardware/pwm.h>
#include <hardware/structs/pwm.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include "counter_rpi_pico_pit.h"
#include <stdint.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_rpi_pico_pit_channel, CONFIG_COUNTER_LOG_LEVEL);

struct counter_rpi_pico_pit_channel_data {
	pwm_config config_pwm;
	uint16_t top_value;
	struct rpi_pico_pit_callback callback_struct;
	uint32_t frequency;
};

struct counter_rpi_pico_pit_channel_config {
	struct counter_config_info info;
	int32_t slice;
	/* Reference to the pico pit instance that is the channels parent*/
	const struct device *controller;
};

static int counter_rpi_pico_pit_channel_start(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;

	pwm_set_enabled(config->slice, true);

	return 0;
}

static int counter_rpi_pico_pit_channel_stop(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;

	pwm_set_enabled(config->slice, false);

	return 0;
}

static uint32_t counter_rpi_pico_pit_channel_get_top_value(const struct device *dev)
{
	struct counter_rpi_pico_pit_channel_data *data = dev->data;

	return data->top_value;
}

static int counter_rpi_pico_pit_channel_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;

	*ticks = pwm_get_counter(config->slice);
	return 0;
}

static int counter_rpi_pico_pit_channel_set_top_value(const struct device *dev,
						      const struct counter_top_cfg *cfg)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;
	struct counter_rpi_pico_pit_channel_data *data = dev->data;
	uint32_t counter_value = 0;

	if (cfg->ticks == 0 || cfg->ticks > UINT16_MAX) {
		LOG_ERR("%s: Top value should be greater than 0 and have a maximum value of %u",
			dev->name, UINT16_MAX);
		return -EINVAL;
	}
	pwm_set_enabled(config->slice, false);
	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		counter_value = pwm_get_counter(config->slice);

		if (counter_value >= cfg->ticks) {
			pwm_set_enabled(config->slice, true);
			return -ETIME;
		}
	}
	pwm_set_chan_level(config->slice, 1, 0);
	pwm_set_chan_level(config->slice, 0, 0);

	data->config_pwm = pwm_get_default_config();
	pwm_config_set_wrap(&data->config_pwm, cfg->ticks);
	data->top_value = cfg->ticks;
	data->callback_struct.callback = cfg->callback;
	data->callback_struct.top_user_data = cfg->user_data;

	bool callback_set = cfg->callback ? true : false;

	counter_rpi_pico_pit_manage_callback(config->controller, &data->callback_struct,
					     callback_set);

	pwm_init(config->slice, &data->config_pwm, true);
	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		pwm_set_counter(config->slice, counter_value);
	}
	pwm_set_clkdiv_int_frac(config->slice, 0, 0);

	return 0;
}

static uint32_t counter_rpi_pico_pit_channel_get_pending_int(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;

	return counter_rpi_pico_pit_get_pending_int(config->controller, config->slice);
}

static uint32_t counter_rpi_pico_pit_channel_get_frequency(const struct device *dev)
{
	struct counter_rpi_pico_pit_channel_data *data = dev->data;

	return data->frequency;
}

static int counter_rpi_pico_pit_channel_init(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;
	struct counter_rpi_pico_pit_channel_data *data = dev->data;

	data->callback_struct.slice = config->slice;

	/* Use maximum clock division*/
	counter_rpi_pico_pit_get_base_frequency(config->controller, &(data->frequency));
	pwm_set_clkdiv_int_frac(config->slice, 0, 0);
	data->frequency = data->frequency / 256;

	/* Disable slice channels to prevent side effects on their pins*/
	pwm_set_chan_level(config->slice, 1, 0);
	pwm_set_chan_level(config->slice, 0, 0);
	pwm_set_wrap(config->slice, UINT16_MAX);

	pwm_set_enabled(config->slice, true);

	return 0;
}

static DEVICE_API(counter, counter_rpi_pico_pit_channel_api) = {
	.start = counter_rpi_pico_pit_channel_start,
	.stop = counter_rpi_pico_pit_channel_stop,
	.get_value = counter_rpi_pico_pit_channel_get_value,
	.set_top_value = counter_rpi_pico_pit_channel_set_top_value,
	.get_pending_int = counter_rpi_pico_pit_channel_get_pending_int,
	.get_top_value = counter_rpi_pico_pit_channel_get_top_value,
	.get_freq = counter_rpi_pico_pit_channel_get_frequency,
};

#define COUNTER_RPI_PICO_PIT_CHANNEL(inst)                                                         \
	static const struct counter_rpi_pico_pit_channel_config counter_##inst##_config = {        \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = UINT16_MAX,                                       \
				.freq = 0,                                                         \
				.flags = COUNTER_CONFIG_INFO_COUNT_UP,                             \
				.channels = 0,                                                     \
			},                                                                         \
		.slice = DT_INST_PROP_BY_IDX(inst, reg, 0),                                        \
		.controller = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                 \
	};                                                                                         \
	static struct counter_rpi_pico_pit_channel_data counter_##inst##_data = {                  \
		.top_value = UINT16_MAX,                                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, counter_rpi_pico_pit_channel_init, NULL,                       \
			      &counter_##inst##_data, &counter_##inst##_config, POST_KERNEL,       \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rpi_pico_pit_channel_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RPI_PICO_PIT_CHANNEL)
