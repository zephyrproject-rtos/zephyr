/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * dummies for now
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_

#define Z_ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element sym[size]

#define Z_ARCH_THREAD_STACK_SIZEOF(sym)	sizeof(sym)
#define Z_ARCH_THREAD_STACK_BUFFER(sym) ((char *) sym)

#define Z_IRQ_TO_INTERRUPT_VECTOR(irq)	(0)
#define Z_ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p)

#define z_arch_irq_lock()		(0)
#define z_arch_irq_unlock(k)

#define z_x86_msr_read(a)		(0)

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_ */
