/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Driver for Pin interrupt and pattern match engine in NXP MCUs
 *
 * The Pin interrupt and pattern match engine (PINT) supports
 * sourcing inputs from any pins on GPIO ports 0 and 1 of NXP MCUs
 * featuring the module, and generating interrupts based on these inputs.
 * Pin inputs can generate separate interrupts to the NVIC, or be combined
 * using the PINT's boolean logic based pattern match engine.
 * This driver currently only supports the pin interrupt feature of
 * the PINT.
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_PINT_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_PINT_H_

#include <fsl_pint.h>

/**
 * @brief Pin interrupt sources
 *
 * Pin interrupt sources available for use.
 */
enum nxp_pint_trigger {
	/* Do not generate Pin Interrupt */
	NXP_PINT_NONE = kPINT_PinIntEnableNone,
	/* Generate Pin Interrupt on rising edge */
	NXP_PINT_RISING  = kPINT_PinIntEnableRiseEdge,
	/* Generate Pin Interrupt on falling edge */
	NXP_PINT_FALLING  = kPINT_PinIntEnableFallEdge,
	/* Generate Pin Interrupt on both edges */
	NXP_PINT_BOTH = kPINT_PinIntEnableBothEdges,
	/* Generate Pin Interrupt on low level */
	NXP_PINT_LOW  = kPINT_PinIntEnableLowLevel,
	/* Generate Pin Interrupt on high level */
	NXP_PINT_HIGH = kPINT_PinIntEnableHighLevel
};

/* Callback for NXP PINT interrupt */
typedef void (*nxp_pint_cb_t) (uint8_t pin, void *user);

/**
 * @brief Enable PINT interrupt source.
 *
 * @param pin: pin to use as interrupt source
 *     0-64, corresponding to GPIO0 pin 1 - GPIO1 pin 31)
 * @param trigger: one of nxp_pint_trigger flags
 */
int nxp_pint_pin_enable(uint8_t pin, enum nxp_pint_trigger trigger);


/**
 * @brief disable PINT interrupt source.
 *
 * @param pin: pin interrupt source to disable
 */
void nxp_pint_pin_disable(uint8_t pin);

/**
 * @brief Install PINT callback
 *
 * @param pin: interrupt source to install callback for
 * @param cb: callback to install
 * @param data: user data to include in callback
 * @return 0 on success, or negative value on error
 */
int nxp_pint_pin_set_callback(uint8_t pin, nxp_pint_cb_t cb, void *data);

/**
 * @brief Remove PINT callback
 *
 * @param pin: interrupt source to remove callback for
 */
void nxp_pint_pin_unset_callback(uint8_t pin);


#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_NXP_PINT_H_ */
