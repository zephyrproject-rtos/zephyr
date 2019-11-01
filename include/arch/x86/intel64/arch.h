/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_

#include <kernel_arch_thread.h>

#define STACK_ALIGN 16
#define STACK_SIZE_ALIGN 16

#if CONFIG_ISR_STACK_SIZE != (CONFIG_ISR_SUBSTACK_SIZE * CONFIG_ISR_DEPTH)
#error "Check ISR stack configuration (CONFIG_ISR_*)"
#endif

#if CONFIG_ISR_SUBSTACK_SIZE % STACK_ALIGN
#error "CONFIG_ISR_SUBSTACK_SIZE must be a multiple of 16"
#endif

#ifndef _ASMLANGUAGE

#define Z_ARCH_THREAD_STACK_LEN(size) (ROUND_UP((size), STACK_SIZE_ALIGN))

#define Z_ARCH_THREAD_STACK_DEFINE(sym, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(STACK_ALIGN) \
		sym[Z_ARCH_THREAD_STACK_LEN(size)]

#define Z_ARCH_THREAD_STACK_ARRAY_DEFINE(sym, nmemb, size) \
	struct _k_thread_stack_element __noinit \
		__aligned(STACK_ALIGN) \
		sym[nmemb][Z_ARCH_THREAD_STACK_LEN(size)]

#define Z_ARCH_THREAD_STACK_MEMBER(sym, size) \
	struct _k_thread_stack_element __aligned(STACK_ALIGN) \
		sym[Z_ARCH_THREAD_STACK_LEN(size)]

#define Z_ARCH_THREAD_STACK_SIZEOF(sym)	sizeof(sym)
#define Z_ARCH_THREAD_STACK_BUFFER(sym) ((char *) sym)

static ALWAYS_INLINE unsigned int z_arch_irq_lock(void)
{
	unsigned long key;

	__asm__ volatile ("pushfq; cli; popq %0" : "=g" (key) : : "memory");

	return (unsigned int) key;
}

/*
 * the exception stack frame
 */

struct x86_esf {
	unsigned long rax;
	unsigned long rbx;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rbp;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long r8;
	unsigned long r9;
	unsigned long r10;
	unsigned long r11;
	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;
	unsigned long vector;
	unsigned long code;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

typedef struct x86_esf z_arch_esf_t;

#endif /* _ASMLANGUAGE */

/*
 * All Intel64 interrupts are dynamically connected.
 */

#define Z_ARCH_IRQ_CONNECT z_arch_irq_connect_dynamic

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_ */
