/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_HEXAGON_INCLUDE_IRQ_H_
#define ZEPHYR_ARCH_HEXAGON_INCLUDE_IRQ_H_

#include <zephyr/irq.h>
#include <zephyr/sw_isr_table.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Hexagon supports up to 64 interrupts in the VM model */
#define ARCH_IRQ_COUNT 64

/* Event numbers */
#define HEXAGON_EVENT_MACHINE_CHECK     1
#define HEXAGON_EVENT_GENERAL_EXCEPTION 2
#define HEXAGON_EVENT_DEBUG             3
#define HEXAGON_EVENT_TRAP0             5
#define HEXAGON_EVENT_INTERRUPT         7

/* Interrupt priority levels (0-63, 0 is highest) */
#define HEXAGON_IRQ_PRIORITY_HIGHEST 0
#define HEXAGON_IRQ_PRIORITY_LOWEST  63

/* Special IRQ numbers */
#define HEXAGON_NO_PENDING_IRQ 0xFFFFFFFF

#ifndef _ASMLANGUAGE

/*
 * Interrupt lock/unlock via HVM trap1 calls (vmsetie/vmgetie).
 *
 * vmsetie (trap1 #3) sets the interrupt-enable flag and returns the
 * previous value in r0.  vmgetie (trap1 #4) returns the current value.
 * With CCR.GRE set these are fast virtual instructions handled inline
 * by the CPU.
 *
 * Following the same pattern as the Linux kernel:
 *   arch_irq_lock   -> vmsetie(VM_INT_DISABLE)  returns old IE
 *   arch_irq_unlock -> vmsetie(key)              restores old IE
 */
static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	return hexagon_vm_setie(VM_INT_DISABLE);
}

static ALWAYS_INLINE void arch_irq_unlock(unsigned int key)
{
	hexagon_vm_setie(key);
}

static ALWAYS_INLINE bool arch_irq_unlocked(unsigned int key)
{
	return key != 0;
}

/* Enable specific IRQ */
void arch_irq_enable(unsigned int irq);

/* Disable specific IRQ */
void arch_irq_disable(unsigned int irq);

/* Check if IRQ is enabled */
int arch_irq_is_enabled(unsigned int irq);

/* Connect IRQ at runtime */
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags);

/* Hexagon-specific interrupt operations */
void hexagon_irq_priority_set(unsigned int irq, unsigned int priority);
int hexagon_irq_trigger(unsigned int irq);

/* Initialize interrupt handling */
void z_hexagon_irq_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_HEXAGON_INCLUDE_IRQ_H_ */
