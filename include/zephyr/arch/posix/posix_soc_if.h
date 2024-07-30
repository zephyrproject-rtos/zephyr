/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ARCH_POSIX_POSIX_SOC_IF_H_
#define ZEPHYR_INCLUDE_ARCH_POSIX_POSIX_SOC_IF_H_

/*
 * This file lists the functions the POSIX architecture core expects the
 * SOC or board will provide
 *
 * All functions listed here must be provided by the implementation of the SOC
 * or all its boards
 */

#include <zephyr/arch/posix/posix_trace.h>
#include "soc_irq.h" /* Must exist and define _ARCH_IRQ/ISR_* macros */

#ifdef __cplusplus
extern "C" {
#endif

void posix_halt_cpu(void);
void posix_atomic_halt_cpu(unsigned int imask);

void posix_irq_enable(unsigned int irq);
void posix_irq_disable(unsigned int irq);
int  posix_irq_is_enabled(unsigned int irq);
unsigned int posix_irq_lock(void);
void posix_irq_unlock(unsigned int key);
void posix_irq_full_unlock(void);
int  posix_get_current_irq(void);
void posix_sw_set_pending_IRQ(unsigned int IRQn);
void posix_sw_clear_pending_IRQ(unsigned int IRQn);
#ifdef CONFIG_IRQ_OFFLOAD
void posix_irq_offload(void (*routine)(const void *), const void *parameter);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ARCH_POSIX_POSIX_SOC_IF_H_ */
