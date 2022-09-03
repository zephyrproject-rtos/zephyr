/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for WCH's Programmable Fast Interrupt Controller (PFIC)
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_WCH_PFIC_H_
#define ZEPHYR_INCLUDE_DRIVERS_WCH_PFIC_H_

void wch_pfic_irq_enable(uint32_t irq);
void wch_pfic_irq_disable(uint32_t irq);
int wch_pfic_irq_is_enabled(uint32_t irq);
void wch_pfic_irq_priority_set(uint32_t irq, uint32_t pri, uint32_t flags);

#endif /* ZEPHYR_INCLUDE_DRIVERS_WCH_PFIC_H_ */
