/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RH850 ORed peripheral interrupt (INTIF ORED) header file
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RH850_INTIF_ORED_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RH850_INTIF_ORED_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>
#include "bsp_api.h"

/**
 * Decode channel index from a virtual ORED IRQ number.
 * Encoding rule:
 * channel == 0:
 * irq = eic
 *
 * channel > 0:
 * irq = BSP_MAX_NUMBER_OF_IRQ +
 * channel +
 * (eic - BSP_ORED_MSPI_EIC_START) *
 * BSP_ORED_MAX_OFFSET_CHANNEL
 *
 * For physical IRQs (channel 0), return 0.
 * For virtual IRQs, extract the channel from the encoded offset.
 */
#define BSP_ORED_IRQ_TO_CHANNEL(irq) \
		(((irq) < BSP_MAX_NUMBER_OF_IRQ) ? \
		0U : \
		(((irq) - BSP_MAX_NUMBER_OF_IRQ) % \
		BSP_ORED_MAX_OFFSET_CHANNEL))

/**
 * Decode the original EIC number from a physical or virtual ORED IRQ.
 *
 * Decoding rule:
 * eic =
 * BSP_ORED_MSPI_EIC_START +
 * ((irq - BSP_MAX_NUMBER_OF_IRQ) /
 * BSP_ORED_MAX_OFFSET_CHANNEL)
 *
 * Examples:
 * irq = 92 -> eic = 92
 * irq = 698 -> eic = 92
 * irq = 699 -> eic = 92
 * irq = 700 -> eic = 92
 */
#define BSP_ORED_IRQ_TO_EIC(irq) \
	(((irq) < BSP_MAX_NUMBER_OF_IRQ) ? \
	(irq) : \
	(BSP_ORED_MSPI_EIC_START + \
	(((irq) - BSP_MAX_NUMBER_OF_IRQ) / \
	BSP_ORED_MAX_OFFSET_CHANNEL)))

/** RH850 INTIF callback */
typedef void (*intc_rh850_intif_ored_callback_t)(void *arg);

/**
 * @brief Enables or disables Peripheral Interrupt for a given INTIF source.
 *
 * Immediately after calling this function with 'enable' set to true, the INTIF is considered
 * enabled and ready to trigger interrupts for the specified source (even already pending).
 *
 * The user must take care of installing a proper callback using
 * intc_rh850_intif_ored_set_callback() before enabling it.
 *
 * Enabling an INTIF interrupt for which there is no installed callback is considered
 * undefined behavior. In general, the INTIF driver will handle INTIF interrupts with no installed
 * callback by masking the interrupt bit inside INTIF and logging an error.
 *
 * @param dev Pointer to INTIF device.
 * @param source The INTIF source (bsp_imf_irqn_t) to be controlled.
 * @param enabled A boolean indicating enable or disable.
 *
 * @retval 0       On success.
 * @retval -EINVAL If the operation fails.
 */
int intc_rh850_intif_ored_set_enabled(const struct device *dev, bsp_ored_t source,
					uint16_t priority, bool enabled);

/**
 * @brief Registers a callback for a given INTIF source.
 *
 * @param dev Pointer to INTIF device.
 * @param source the INTIF source for which to register the callback
 * @param cb callback to set
 * @param arg user data passed to callback
 *
 * @retval 0       On success.
 * @retval -EINVAL If the operation fails.
 */
int intc_rh850_intif_ored_set_callback(const struct device *dev, bsp_ored_t source,
				   intc_rh850_intif_ored_callback_t cb, void *arg);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RH850_INTIF_ORED_H_ */
