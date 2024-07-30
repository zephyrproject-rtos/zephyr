/*
 * Copyright (c) 2019 Derek Hageman <hageman@inthat.cloud>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SAM0_EIC_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SAM0_EIC_H_

#include <zephyr/types.h>

/* callback for EIC interrupt */
typedef void (*sam0_eic_callback_t)(uint32_t pins, void *data);

/**
 * @brief EIC trigger condition
 */
enum sam0_eic_trigger {
	/* Rising edge */
	SAM0_EIC_RISING,
	/* Falling edge */
	SAM0_EIC_FALLING,
	/* Both edges */
	SAM0_EIC_BOTH,
	/* High level detection */
	SAM0_EIC_HIGH,
	/* Low level detection */
	SAM0_EIC_LOW,
};

/**
 * @brief Acquire an EIC interrupt for specific port and pin combination
 *
 * This acquires the EIC interrupt for a specific port and pin combination,
 * or returns an error if the required line is not available.  Only a single
 * callback per port is supported and supplying a different one will
 * change it for all lines on that port.
 *
 * @param port port index (A=0, etc)
 * @param pin pin in the port
 * @param trigger trigger condition
 * @param filter enable filter
 * @param cb interrupt callback
 * @param data parameter to the interrupt callback
 */
int sam0_eic_acquire(int port, int pin, enum sam0_eic_trigger trigger,
		     bool filter, sam0_eic_callback_t cb, void *data);

/**
 * @brief Release the EIC interrupt for a specific port and pin combination
 *
 * Release the EIC configuration for a specific port and pin combination.
 * No effect if that combination does not currently hold the associated
 * EIC line.
 *
 * @param port port index (A=0, etc)
 * @param pin pin in the port
 */
int sam0_eic_release(int port, int pin);

/**
 * @brief Enable the EIC interrupt for a specific port and pin combination
 *
 * @param port port index (A=0, etc)
 * @param pin pin in the port
 */
int sam0_eic_enable_interrupt(int port, int pin);

/**
 * @brief Disable the EIC interrupt for a specific port and pin combination
 *
 * @param port port index (A=0, etc)
 * @param pin pin in the port
 */
int sam0_eic_disable_interrupt(int port, int pin);

/**
 * @brief Test if there is an EIC interrupt pending for a port
 *
 * @param port port index (A=0, etc)
 */
uint32_t sam0_eic_interrupt_pending(int port);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_SAM0_EIC_H_ */
