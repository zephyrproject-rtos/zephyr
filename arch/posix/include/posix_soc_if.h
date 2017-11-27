/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _POSIX_CORE_SOC_PROVIDED_IF_H
#define _POSIX_CORE_SOC_PROVIDED_IF_H

/*
 * This file lists the functions the posix architecture core
 * expect the the SOC or board to provide
 *
 * All functions listed here must be provided by the implementation of the SOC
 * or all its boards
 */

#ifdef __cplusplus
extern "C" {
#endif

void ps_print_error_and_exit(const char *format, ...);
void ps_print_warning(const char *format, ...);
void ps_print_trace(const char *format, ...);

void ps_halt_cpu(void);
void ps_atomic_halt_cpu(unsigned int imask);


#include "soc_irq.h" /*Must exist and define _ARCH_IRQ/ISR_* macros*/

void _arch_irq_enable(unsigned int irq);
void _arch_irq_disable(unsigned int irq);
int  _arch_irq_is_enabled(unsigned int irq);
unsigned int ps_irq_lock(void);
void ps_irq_unlock(unsigned int key);
void ps_irq_full_unlock(void);
int  ps_get_current_irq(void);
/*irq_offload() from irq_offload.h must also be defined by the SOC or board*/

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_CORE_SOC_PROVIDED_IF_H */
