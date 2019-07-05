/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_

#include <kernel_arch_thread.h>

#define STACK_ALIGN 16
#define STACK_SIZE_ALIGN 16

#ifndef _ASMLANGUAGE

#define Z_ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(STACK_ALIGN) \
		sym[ROUND_UP((size), STACK_SIZE_ALIGN)]

#define Z_ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(STACK_ALIGN) \
		sym[nmemb][ROUND_UP((size), STACK_SIZE_ALIGN)]

#define Z_ARCH_THREAD_STACK_SIZEOF(sym)	sizeof(sym)
#define Z_ARCH_THREAD_STACK_BUFFER(sym) ((char *) sym)

static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	unsigned long key;

	__asm__ volatile (
		"pushfq\n\t"
		"cli\n\t"
		"popq %0\n\t"
		: "=g" (key)
		:
		: "memory"
		);

	return (unsigned int) key;
}

/*
 * Bogus ESF stuff until I figure out what to with it. I suspect
 * this is legacy cruft that we'll want to excise sometime soon, anyway.
 */

struct x86_esf {
};

typedef struct x86_esf z_arch_esf_t;

#endif /* _ASMLANGUAGE */

/*
 * All Intel64 interrupts are dynamically connected.
 */

#define Z_ARCH_IRQ_CONNECT z_arch_irq_connect_dynamic

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_ */
