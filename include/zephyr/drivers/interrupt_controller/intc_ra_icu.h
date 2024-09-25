/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/dt-bindings/interrupt-controller/renesas-ra-icu.h>

#ifndef ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RA_ICU_H_
#define ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RA_ICU_H_

#define RA_ICU_FLAG_EVENT_OFFSET  8
#define RA_ICU_FLAG_EVENT_MASK    (BIT_MASK(8) << RA_ICU_FLAG_EVENT_OFFSET)
#define RA_ICU_FLAG_INTCFG_OFFSET 16
#define RA_ICU_FLAG_INTCFG_MASK   (BIT_MASK(8) << RA_ICU_FLAG_INTCFG_OFFSET)

enum icu_irq_mode {
	ICU_FALLING,
	ICU_RISING,
	ICU_BOTH_EDGE,
	ICU_LOW_LEVEL,
};

typedef void (*ra_isr_handler)(const void *);

extern void ra_icu_clear_int_flag(unsigned int irqn);

extern int ra_icu_query_available_irq(uint32_t event);
extern int ra_icu_query_exists_irq(uint32_t event);

extern void ra_icu_query_irq_config(unsigned int irq, uint32_t *intcfg, ra_isr_handler *pisr,
				    const void **cbarg);

extern int ra_icu_irq_connect_dynamic(unsigned int irq, unsigned int priority,
				      void (*routine)(const void *parameter), const void *parameter,
				      uint32_t flags);

extern int ra_icu_irq_disconnect_dynamic(unsigned int irq, unsigned int priority,
					 void (*routine)(const void *parameter),
					 const void *parameter, uint32_t flags);

#endif /* ZEPHYR_DRIVERS_INTERRUPT_CONTROLLER_INTC_RA_ICU_H_ */
