/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIMPLE_PROCESS_IRQ_CTRL_H
#define _SIMPLE_PROCESS_IRQ_CTRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {TIMER, NONE} irq_type_t;

void hw_irq_controller(irq_type_t irq);
void hw_irq_controller_clear_irqs(void);
uint64_t hw_irq_controller_get_irq_status(void);

#ifdef __cplusplus
}
#endif

#endif /* _SIMPLE_PROCESS_IRQ_CTRL_H */
