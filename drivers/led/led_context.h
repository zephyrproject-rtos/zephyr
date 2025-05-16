/*
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private LED driver APIs
 */

#ifndef ZEPHYR_DRIVERS_LED_LED_CONTEXT_H_
#define ZEPHYR_DRIVERS_LED_LED_CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal driver specific representation of an LED device
 */

struct led_data {
	/* Minimum acceptable LED blinking time period (in ms) */
	uint32_t min_period;
	/* Maximum acceptable LED blinking time period (in ms) */
	uint32_t max_period;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_LED_LED_CONTEXT_H_ */
