/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RZ Interrupt Controller API
 *
 * This is used for Renesas RZ Interrupt Controller Unit supporting selectable interrupts
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_ICU_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_ICU_H_

#include <bsp_api.h>

/**
 * @brief Connect the interrupt ID to the event number
 *
 * @param irq: Interrupt ID
 * @param event: Event number
 * @return 0 on success, or negative value on error
 */
int icu_connect_irq_event(int irq, elc_event_t event);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RZ_ICU_H_ */
