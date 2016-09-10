/*
 * Copyright (c) 2015 Intel corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Public interface for configuring interrupts
 */
#ifndef _IRQ_H_
#define _IRQ_H_

/* Pull in the arch-specific implementations */
#include <arch/cpu.h>

#ifndef _ASMLANGUAGE
#include <toolchain/gcc.h>

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Configure a static interrupt.
 *
 * All arguments must be computable by the compiler at build time.
 *
 * @param irq_p IRQ line number
 * @param priority_p Interrupt priority
 * @param isr_p Interrupt service routine
 * @param isr_param_p ISR parameter
 * @param flags_p Arch-specific IRQ configuration flags
 *
 * @return The vector assigned to this interrupt
 */
#define IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

/**
 * @brief Disable all interrupts on the CPU (inline)
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock() to re-enable interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent unsigned int lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 */
#define irq_lock() _arch_irq_lock()

/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter
 * is an architecture-dependent lock-out key that is returned by a previous
 * invocation of irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level
 *
 * @param key architecture-dependent lock-out key
 *
 * @return N/A
 */
#define irq_unlock(key) _arch_irq_unlock(key)

/**
 * @brief Enable a specific IRQ
 *
 * @param irq IRQ line
 * @return N/A
 */
#define irq_enable(irq) _arch_irq_enable(irq)

/**
 * @brief Disable a specific IRQ
 *
 * @param irq IRQ line
 * @return N/A
 */
#define irq_disable(irq) _arch_irq_disable(irq)

/**
 * @brief Return IRQ enable state
 *
 * @param irq IRQ line
 * @return interrupt enable state, true or false
 */
#define irq_is_enabled(irq) _arch_irq_is_enabled(irq)

#ifdef __cplusplus
}
#endif

#endif /* ASMLANGUAGE */
#endif /* _IRQ_H_ */
