/*
 * Copyright (c) 2023 - 2024 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RA_ICU_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RA_ICU_H_

/**
 * The Renesas RA ICU is an interrupt controller that works with the NVIC.
 * In the Renesas RA, interrupt numbers are not assigned to specific functions;
 * instead, the ICU settings associate the numbers and functions.
 *
 * This specification is complex and cannot be supported by the original
 * interrupt * handling mechanism, so we introduce unique solutions for
 * isr_table generation.
 *
 * Specifically, the following points differ from the standard.
 *
 * 1. Interrupt handlers are set in the isr_table in the order they are found
 *    during linking. At the same time, a lookup table is generated, which
 *    associates event numbers with interrupts.
 *
 * 2. All arguments that specify IRQs in IRQ APIs, such as `irq_enable,
 *  ` are interpreted as event numbers. The IRQ APIs implemented in the ICU
 *    resolve this using the lookup table above. In other words, the API does
 *    not use interrupt numbers but specifies all events by number.
 *
 * The interrupt controller uses NVIC, so we provide a wrapper as a driver API that
 * looks up the IRQ number from the event number.
 * Now, we can use it in the same way as a normal interrupt on Zephyr.
 */

enum icu_irq_mode {
	ICU_FALLING,
	ICU_RISING,
	ICU_BOTH_EDGE,
	ICU_LOW_LEVEL,
};

void ra_icu_event_enable(unsigned int event);
void ra_icu_event_disable(unsigned int event);
int ra_icu_event_is_enabled(unsigned int event);
void ra_icu_event_priority_set(unsigned int event, unsigned int prio, uint32_t flags);

void ra_icu_event_clear_flag(unsigned int event);
void ra_icu_event_query_config(unsigned int event, uint32_t *intcfg);
void ra_icu_event_configure(unsigned int event, uint32_t intcfg);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RA_ICU_H_ */
