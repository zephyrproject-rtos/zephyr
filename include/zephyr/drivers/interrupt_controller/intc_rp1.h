/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Raspberry Pi RP1 MSI-X interrupt controller.
 *
 * The RP1 is a PCIe endpoint that delivers one MSI-X vector per internal
 * interrupt source. The driver keeps the controller state private; the
 * SoC IRQ glue controls it through the functions below.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RP1_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RP1_H_

#include <zephyr/device.h>

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Enable an RP1 interrupt and its MSI parent channel.
 *
 * @param dev Initialized RP1 interrupt controller.
 * @param line RP1-local interrupt source number.
 */
void rp1_irq_enable(const struct device *dev, unsigned int line);

/**
 * @brief Disable an RP1 interrupt and its MSI parent channel.
 *
 * @param dev Initialized RP1 interrupt controller.
 * @param line RP1-local interrupt source number.
 */
void rp1_irq_disable(const struct device *dev, unsigned int line);

/**
 * @brief Check whether an RP1 interrupt source is enabled.
 *
 * @param dev Initialized RP1 interrupt controller.
 * @param line RP1-local interrupt source number.
 *
 * @retval 1 The interrupt source is enabled.
 * @retval 0 The interrupt source is disabled or invalid, or the controller is not ready.
 */
int rp1_irq_is_enabled(const struct device *dev, unsigned int line);

/**
 * @brief Acknowledge an RP1 interrupt before completing its host interrupt.
 *
 * @param dev Initialized RP1 interrupt controller.
 * @param line RP1-local interrupt source number.
 */
void rp1_irq_eoi(const struct device *dev, unsigned int line);

/** @endcond */

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RP1_H_ */
