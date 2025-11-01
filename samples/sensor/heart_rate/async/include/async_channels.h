/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/drivers/sensor.h>

#define FUNCTION_INDICES(i, APPLY, _chan, ...) \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_##_chan##_##i, (APPLY(__VA_ARGS__, _chan, i)))

#define SUPPORTED_INDICES(APPLY, _chan, ...) LISTIFY(CONFIG_SAMPLE_CHANNEL_MAX_INDEX, FUNCTION_INDICES, (), APPLY, _chan, __VA_ARGS__)

#define SENSOR_AVAILABLE_CHANNEL(APPLY, ...)                                                                        \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_RED, (SUPPORTED_INDICES(APPLY, RED,__VA_ARGS__)))                                                                           \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_IR, (SUPPORTED_INDICES(APPLY, IR, __VA_ARGS__)))                                                                             \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_GREEN, (SUPPORTED_INDICES(APPLY, GREEN, __VA_ARGS__)))                                                                       \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_DIE_TEMP, (SUPPORTED_INDICES(APPLY, DIE_TEMP, __VA_ARGS__)))

#define CHAN_X(_, _chan, _idx) (struct sensor_chan_spec){SENSOR_CHAN_##_chan, _idx},
const struct sensor_chan_spec chan_list[] = {SENSOR_AVAILABLE_CHANNEL(CHAN_X)};
#undef CHAN_X

#define CHAN_X(_0, _chan, _1) #_chan,
const char *channel_names[] = {SENSOR_AVAILABLE_CHANNEL(CHAN_X)};
#undef CHAN_X

#define CHAN_X(_0, _chan, _1) sizeof(#_chan),
const size_t channel_widths[] = {SENSOR_AVAILABLE_CHANNEL(CHAN_X)};
#undef CHAN_X
