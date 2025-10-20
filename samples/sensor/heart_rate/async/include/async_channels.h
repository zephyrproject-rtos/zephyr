/*
 * Copyright (c) 2025, CATIE
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#define SENSOR_AVAILABLE_CHANNEL(APPLY, ...)                                                                        \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_RED, (APPLY(__VA_ARGS__, RED, "RED", 0)))                                                                           \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_IR, (APPLY(__VA_ARGS__, IR, "IR", 0)))                                                                             \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_GREEN, (APPLY(__VA_ARGS__, GREEN, "GREEN", 0)))                                                                       \
	IF_ENABLED(CONFIG_SAMPLE_CHANNEL_DIE_TEMP, (APPLY(__VA_ARGS__, DIE_TEMP, "TEMP", 0)))

const int channel_width = 5;
