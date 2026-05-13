/*
 * Copyright (c) 2026 Siemens
 * Copyright (c) 2026 Stefan Gloor
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file led_trigger_timer.h
 *
 * LED Trigger "timer" implementation providing software blink
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_LED_LED_TRIGGER_TIMER_H_
#define ZEPHYR_INCLUDE_DRIVERS_LED_LED_TRIGGER_TIMER_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

/** Per-channel data owned by the timer trigger. */
struct led_trigger_timer_data {
	struct k_work_delayable work; /**< Workqueue item */
	uint32_t delay_on;  /**< On delay time */
	uint32_t delay_off; /**< Off delay time */
	uint8_t brightness; /**< Brightness in the "ON" state */
	bool on_phase;      /**< Momentary state of the LED */
};

/**
 * @brief Start or update periodic blinking via the timer trigger.
 *
 * Allocates a trigger channel for the given (device, led) pair if
 * needed, attaches the timer trigger, and begins toggling the LED
 * between the configured brightness and off.
 *
 * Passing zero for either delay parameter stops the blink and turns
 * the LED on (if only delay_off is 0) or off (if delay_on is 0).
 *
 * @param dev       LED device.
 * @param led       LED index on the device.
 * @param delay_on  ON-phase duration in milliseconds.
 * @param delay_off OFF-phase duration in milliseconds.
 * @return 0 on success, -ENOMEM if the channel pool is exhausted.
 */
int led_trigger_timer_start(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off);

#endif /* ZEPHYR_INCLUDE_DRIVERS_LED_LED_TRIGGER_TIMER_H_ */
