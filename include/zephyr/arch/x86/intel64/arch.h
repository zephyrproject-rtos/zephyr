/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel-64 specific kernel interface header
 *
 * This header contains the Intel-64 portion of the X86 specific kernel
 * interface (see include/zephyr/arch/x86/cpu.h).
 */

#ifndef ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_
#define ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_

#include <zephyr/arch/exception.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/arch/x86/intel64/exception.h>
#include <zephyr/arch/x86/intel64/thread.h>
#include <zephyr/arch/x86/thread_stack.h>
#if defined(CONFIG_PCIE) && !defined(_ASMLANGUAGE)
#include <zephyr/sys/iterable_sections.h>
#endif

#if CONFIG_ISR_STACK_SIZE != (CONFIG_ISR_SUBSTACK_SIZE * CONFIG_ISR_DEPTH)
#error "Check ISR stack configuration (CONFIG_ISR_*)"
#endif

#if CONFIG_ISR_SUBSTACK_SIZE % ARCH_STACK_PTR_ALIGN
#error "CONFIG_ISR_SUBSTACK_SIZE must be a multiple of 16"
#endif

#ifndef _ASMLANGUAGE

static ALWAYS_INLINE void sys_write64(uint64_t data, mm_reg_t addr)
{
	__asm__ volatile("movq %0, %1"
			 :
			 : "r"(data), "m" (*(volatile uint64_t *)
					   (uintptr_t) addr)
			 : "memory");
}

static ALWAYS_INLINE uint64_t sys_read64(mm_reg_t addr)
{
	uint64_t ret;

	__asm__ volatile("movq %1, %0"
			 : "=r"(ret)
			 : "m" (*(volatile uint64_t *)(uintptr_t) addr)
			 : "memory");

	return ret;
}

static ALWAYS_INLINE unsigned int arch_irq_lock(void)
{
	unsigned long key;

	__asm__ volatile ("pushfq; cli; popq %0" : "=g" (key) : : "memory");

	return (unsigned int) key;
}

#define ARCH_EXCEPT(reason_p) do { \
	__asm__ volatile( \
		"movq %[reason], %%rax\n\t" \
		"int $32\n\t" \
		: \
		: [reason] "i" (reason_p) \
		: "memory"); \
	CODE_UNREACHABLE; /* LCOV_EXCL_LINE */ \
} while (false)

#ifdef CONFIG_PCIE
#define X86_RESERVE_IRQ(irq_p, name) \
	static TYPE_SECTION_ITERABLE(uint8_t, name, irq_alloc, name) = irq_p
#else
#define X86_RESERVE_IRQ(irq_p, name)
#endif

#endif /* _ASMLANGUAGE */

/*
 * All Intel64 interrupts are dynamically connected.
 */

#define ARCH_IRQ_CONNECT(irq_p, priority_p, isr_p, isr_param_p, flags_p) \
	X86_RESERVE_IRQ(irq_p, _CONCAT(_irq_alloc_fixed, __COUNTER__)); \
	arch_irq_connect_dynamic(irq_p, priority_p,			\
				 (void (*)(const void *))isr_p,		\
				 isr_param_p, flags_p)

#ifdef CONFIG_PCIE

#define ARCH_PCIE_IRQ_CONNECT(bdf_p, irq_p, priority_p,			\
			      isr_p, isr_param_p, flags_p)		\
	X86_RESERVE_IRQ(irq_p, _CONCAT(_irq_alloc_fixed, __COUNTER__)); \
	pcie_connect_dynamic_irq(bdf_p, irq_p, priority_p,		\
				 (void (*)(const void *))isr_p,		\
				 isr_param_p, flags_p)

#endif /* CONFIG_PCIE */

/*
 * Thread object needs to be 16-byte aligned.
 */
#define ARCH_DYNAMIC_OBJ_K_THREAD_ALIGNMENT	16

#endif /* ZEPHYR_INCLUDE_ARCH_X86_INTEL64_ARCH_H_ */
