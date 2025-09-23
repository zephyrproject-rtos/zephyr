/*
 * Copyright (c) 2025 Michael Hope <michaelh@juju.nz>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_WCH_EXTI_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_WCH_EXTI_H_

#include <stdint.h>

#include <zephyr/sys/util_macro.h>

/* Callback for EXTI interrupt. */
typedef void (*wch_exti_callback_handler_t)(uint8_t line, void *user);

enum wch_exti_trigger {
	/*
	 * Note that this is a flag set and these values can be ORed to trigger on
	 * both edges.
	 */

	/* Trigger on rising edge */
	WCH_EXTI_TRIGGER_RISING_EDGE = BIT(0),
	/* Trigger on falling edge */
	WCH_EXTI_TRIGGER_FALLING_EDGE = BIT(1),
};

/* Enable the EXTI interrupt for `line` */
void wch_exti_enable(uint8_t line);

/* Disable the EXTI interrupt for `line` */
void wch_exti_disable(uint8_t line);

/* Set the trigger mode for `line` */
void wch_exti_set_trigger(uint8_t line, enum wch_exti_trigger trigger);

/* Register a callback for `line` */
int wch_exti_configure(uint8_t line, wch_exti_callback_handler_t callback, void *user);

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_WCH_EXTI_H_ */
