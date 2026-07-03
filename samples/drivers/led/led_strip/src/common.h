/*
 * Copyright (c) 2026 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SAMPLES_DRIVERS_LED_STRIP_COMMON_H
#define ZEPHYR_SAMPLES_DRIVERS_LED_STRIP_COMMON_H

#include <zephyr/device.h>

#define STRIP_NODE DT_ALIAS(led_strip)

#if DT_NODE_HAS_PROP(STRIP_NODE, chain_length)
#define STRIP_NUM_PIXELS DT_PROP(STRIP_NODE, chain_length)
#else
#error Unable to determine length of LED strip
#endif

#define UPDATE_DELAY K_MSEC(CONFIG_SAMPLE_LED_UPDATE_DELAY)

extern const struct device *const strip;

#endif /* ZEPHYR_SAMPLES_DRIVERS_LED_STRIP_COMMON_H */
