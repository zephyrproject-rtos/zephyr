/*
 * Copyright (c) 2026 Dimitri Varpusvuori
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief M68K interrupt controller devicetree bindings
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_M68K_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_M68K_H_

/** Base vector number for autovectored interrupts. */
#define M68K_IRQ_AUTOVECTOR_BASE 24

/**
 * @brief Convert an interrupt level to its autovector number.
 *
 * @param level Interrupt level.
 */
#define M68K_IRQ_AUTOVECTOR(level) (M68K_IRQ_AUTOVECTOR_BASE + (level))

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_INTERRUPT_CONTROLLER_M68K_H_ */
