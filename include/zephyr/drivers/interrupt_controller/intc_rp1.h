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
 * SoC IRQ glue reaches it through the route operations below.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RP1_H_
#define ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RP1_H_

#include <zephyr/drivers/interrupt_controller/irq_route.h>

/** @brief Route operations for the RP1 controller. */
extern const struct irq_route_ops rp1_intc_route_ops;

#endif /* ZEPHYR_INCLUDE_DRIVERS_INTERRUPT_CONTROLLER_INTC_RP1_H_ */
