/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr port of the CPC atomic primitives (see port/silabs/inc/cpc_atomic.h).
 */

#ifndef CPC_ATOMIC_H
#define CPC_ATOMIC_H

#include <zephyr/irq.h>

#define MCU_DECLARE_IRQ_STATE unsigned int __cpc_irq_key
#define MCU_ENTER_CRITICAL()  (__cpc_irq_key = irq_lock())
#define MCU_EXIT_CRITICAL()   irq_unlock(__cpc_irq_key)
#define MCU_ENTER_ATOMIC()    MCU_ENTER_CRITICAL()
#define MCU_EXIT_ATOMIC()     MCU_EXIT_CRITICAL()

/* Same semantic as sl_atomic_load()/sl_atomic_store(): plain word accesses */
#define MCU_ATOMIC_LOAD(dest, source)  ((dest) = (source))
#define MCU_ATOMIC_STORE(dest, source) ((dest) = (source))

#endif /* CPC_ATOMIC_H */
