/*
 * Copyright (c) 2024 Bjarki Arge Andreasen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_IRQ_HANDLER_H_
#define ZEPHYR_INCLUDE_SYS_IRQ_HANDLER_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

/** Log spurious interrupt line */
void sys_irq_log_spurious_intl(uint16_t intc_ord, uint16_t intln);

/** Spurious interrupt handler */
void sys_irq_spurious_handler(void);

/** Dynamic interrupt handler */
int sys_irq_dynamic_handler(uint16_t irqn);

/** Include generated IRQ handlers */
#include <sys_irq_handler_generated.h>

#endif /* ZEPHYR_INCLUDE_SYS_IRQ_HANDLER_H_ */
