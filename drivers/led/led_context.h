/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private LED driver APIs
 */

#ifndef __LED_CONTEXT_H__
#define __LED_CONTEXT_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal driver specific representation of an LED device
 */

struct led_data {
	/* Minimum acceptable LED blinking time period (in ms) */
	u32_t min_period;
	/* Maximum acceptable LED blinking time period (in ms) */
	u32_t max_period;
	/* Minimum acceptable LED brightness value */
	u16_t min_brightness;
	/* Maximum acceptable LED brightness value */
	u16_t max_brightness;
};

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONTEXT_H__ */
