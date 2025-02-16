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

#define LOG_LEVEL CONFIG_COUNTER_LOG_LEVEL
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_rpi_pico_pit_channel, LOG_LEVEL);

struct counter_rpi_pico_pit_channel_data {
	pwm_config config_pwm;
	uint16_t top_value;
	struct rpi_pico_pit_callback callback_struct;
};

struct counter_rpi_pico_pit_channel_config {
	struct counter_config_info info;
	int32_t slice;
	/* Reference to the pico pit instance that is the channels parent*/
	const struct device *controller;
	uint8_t divider_integral;
	uint8_t divider_frac;
};

/* Returns the clockdivider of the slice associated with this pit channel as a float*/
static float counter_rpi_pico_pit_channel_get_clkdiv(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;

	/* the divider is a fixed point 8.4 convert to float */
	return (float)config->divider_integral + (float)config->divider_frac / 16.0f;
}

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

static int counter_rpi_pico_pit_channel_set_alarm(const struct device *dev, uint8_t id,
						  const struct counter_alarm_cfg *alarm_cfg)
{
	return -ENOTSUP;
}

static int counter_rpi_pico_pit_channel_cancel_alarm(const struct device *dev, uint8_t id)
{
	return -ENOTSUP;
}

static int counter_rpi_pico_pit_channel_set_top_value(const struct device *dev,
						      const struct counter_top_cfg *cfg)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;
	struct counter_rpi_pico_pit_channel_data *data = dev->data;

	if (cfg->ticks == 0 || cfg->ticks > UINT16_MAX) {
		LOG_ERR("%s: Top value should be greater than 0 and have a maximum value of %u",
			dev->name, UINT16_MAX);
		return -EINVAL;
	}
	pwm_set_enabled(config->slice, false);
	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		pwm_set_counter(config->slice, 0);

	} else {
		uint32_t value = pwm_get_counter(config->slice);

		if (value >= cfg->ticks) {
			pwm_set_enabled(config->slice, true);
			return -ETIME;
		}
	}
	pwm_set_chan_level(config->slice, 1, 0);
	pwm_set_chan_level(config->slice, 0, 0);

	data->config_pwm = pwm_get_default_config();
	pwm_config_set_wrap(&data->config_pwm, cfg->ticks);
	pwm_config_set_clkdiv_int_frac(&data->config_pwm, config->divider_integral,
				       config->divider_frac);
	data->top_value = cfg->ticks;
	data->callback_struct.callback = cfg->callback;
	data->callback_struct.top_user_data = cfg->user_data;

	bool callback_set = cfg->callback ? true : false;

	counter_rpi_pico_pit_manage_callback(config->controller, &data->callback_struct,
					     callback_set);

	pwm_init(config->slice, &data->config_pwm, true);

	return 0;
}

static uint32_t counter_rpi_pico_pit_channel_get_pending_int(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;

	return counter_rpi_pico_pit_get_pending_int(config->controller, config->slice);
}

static uint32_t counter_rpi_pico_pit_channel_get_guard_period(const struct device *dev,
							      uint32_t flags)
{
	return -ENOTSUP;
}

static int counter_rpi_pico_pit_channel_set_guard_period(const struct device *dev, uint32_t guard,
							 uint32_t flags)
{
	return -ENOTSUP;
}

static uint32_t counter_rpi_pico_pit_channel_get_frequency(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;
	uint32_t frequency = 0;

	/* Get clockspeed from controller, as it is the same for all slices, but each slice
	 * has its own divider. No need to check for div by 0 as the clockdivider is
	 * constant
	 */
	counter_rpi_pico_pit_get_base_frequency(config->controller, &frequency);
	return frequency / counter_rpi_pico_pit_channel_get_clkdiv(dev);
}

static int counter_rpi_pico_pit_channel_init(const struct device *dev)
{
	const struct counter_rpi_pico_pit_channel_config *config = dev->config;
	struct counter_rpi_pico_pit_channel_data *data = dev->data;

	data->callback_struct.slice = config->slice;

	/* Disable slice channels to prevent side effects on their pins*/
	pwm_set_chan_level(config->slice, 1, 0);
	pwm_set_chan_level(config->slice, 0, 0);
	pwm_set_wrap(config->slice, UINT16_MAX);

	pwm_set_enabled(config->slice, true);

	return 0;
}

static const struct counter_driver_api counter_rpi_pico_pit_channel_api = {
	.start = counter_rpi_pico_pit_channel_start,
	.stop = counter_rpi_pico_pit_channel_stop,
	.get_value = counter_rpi_pico_pit_channel_get_value,
	.set_alarm = counter_rpi_pico_pit_channel_set_alarm,
	.cancel_alarm = counter_rpi_pico_pit_channel_cancel_alarm,
	.set_top_value = counter_rpi_pico_pit_channel_set_top_value,
	.get_pending_int = counter_rpi_pico_pit_channel_get_pending_int,
	.get_top_value = counter_rpi_pico_pit_channel_get_top_value,
	.get_guard_period = counter_rpi_pico_pit_channel_get_guard_period,
	.set_guard_period = counter_rpi_pico_pit_channel_set_guard_period,
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
		.slice = DT_INST_PROP(inst, pwm_slice),                                            \
		.controller = DEVICE_DT_GET(DT_INST_PARENT(inst)),                                 \
		.divider_integral = 255,                                                           \
		.divider_frac = 15,                                                                \
	};                                                                                         \
	static struct counter_rpi_pico_pit_channel_data counter_##inst##_data = {                  \
		.top_value = UINT16_MAX,                                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, counter_rpi_pico_pit_channel_init, NULL,                       \
			      &counter_##inst##_data, &counter_##inst##_config, POST_KERNEL,       \
			      CONFIG_COUNTER_INIT_PRIORITY, &counter_rpi_pico_pit_channel_api);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_RPI_PICO_PIT_CHANNEL)
