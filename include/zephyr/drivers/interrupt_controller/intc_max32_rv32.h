/*
 * Copyright (c) 2025 Analog Devices, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MAX32_RV32_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MAX32_RV32_H_

/**
 * @brief Clear the pending interrupt line in the controller.
 *
 * @param source: the IRQ number to clear any pending status for.
 */
void intc_max32_rv32_irq_clear_pending(int source);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_MAX32_RV32_H_ */
