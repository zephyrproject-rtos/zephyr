/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE

#ifdef __cplusplus
extern "C" {
#endif

/* stack alignment related macros: STACK_ALIGN_SIZE is defined above */

#define STACK_ROUND_UP(x) ROUND_UP(x, STACK_ALIGN_SIZE)
#define STACK_ROUND_DOWN(x) ROUND_DOWN(x, STACK_ALIGN_SIZE)

extern K_THREAD_STACK_DEFINE(_interrupt_stack, CONFIG_ISR_STACK_SIZE);

/**
 *
 * @brief Performs architecture-specific initialization
 *
 * This routine performs architecture-specific initialization of the kernel.
 * Trivial stuff is done inline; more complex initialization is done via
 * function calls.
 *
 * @return N/A
 */
static inline void kernel_arch_init(void)
{
	_kernel.nested = 0;
	_kernel.irq_stack = K_THREAD_STACK_BUFFER(_interrupt_stack) +
				CONFIG_ISR_STACK_SIZE;
#if CONFIG_X86_STACK_PROTECTION
	_x86_mmu_set_flags(_interrupt_stack, MMU_PAGE_SIZE,
			   MMU_ENTRY_NOT_PRESENT, MMU_PTE_P_MASK);
#endif
}

/**
 *
 * @brief Set the return value for the specified thread (inline)
 *
 * @param thread pointer to thread
 * @param value value to set as return value
 *
 * The register used to store the return value from a function call invocation
 * is set to @a value.  It is assumed that the specified @a thread is pending, and
 * thus the threads context is stored in its TCS.
 *
 * @return N/A
 */
static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	/* write into 'eax' slot created in _Swap() entry */

	*(unsigned int *)(thread->callee_saved.esp) = value;
}

extern void k_cpu_atomic_idle(unsigned int imask);

/**
 * @brief Write to a model specific register (MSR)
 *
 * This function is used to write to an MSR.
 *
 * The definitions of the so-called  "Architectural MSRs" are contained
 * in kernel_structs.h and have the format: IA32_XXX_MSR
 *
 * @return N/A
 */
static inline void _x86_msr_write(unsigned int msr, u64_t data)
{
	u32_t high = data >> 32;
	u32_t low = data & 0xFFFFFFFF;

	__asm__ volatile ("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

/**
 * @brief Read from a model specific register (MSR)
 *
 * This function is used to read from an MSR.
 *
 * The definitions of the so-called  "Architectural MSRs" are contained
 * in kernel_structs.h and have the format: IA32_XXX_MSR
 *
 * @return N/A
 */
static inline u64_t _x86_msr_read(unsigned int msr)
{
	u64_t ret;

	__asm__ volatile("rdmsr" : "=A" (ret) : "c" (msr));

	return ret;
}

#ifdef CONFIG_JAILHOUSE_X2APIC
#define MSR_X2APIC_BASE 0x00000800

static inline u32_t read_x2apic(unsigned int reg)
{
	return _x86_msr_read(MSR_X2APIC_BASE + reg);
}

static inline void write_x2apic(unsigned int reg, u32_t val)
{
	_x86_msr_write(MSR_X2APIC_BASE + reg, val);
}
#endif

/*
 * _IntLibInit() is called from the non-arch specific function,
 * prepare_multithreading(). The IA-32 kernel does not require any special
 * initialization of the interrupt subsystem. However, we still need to
 * provide an _IntLibInit() of some sort to prevent build errors.
 */
static inline void _IntLibInit(void)
{
}

/* the _idt_base_address symbol is generated via a linker script */
extern unsigned char _idt_base_address[];

extern FUNC_NORETURN void _x86_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

#include <stddef.h> /* For size_t */

#ifdef __cplusplus
}
#endif

#define _is_in_isr() (_kernel.nested != 0)

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_KERNEL_ARCH_FUNC_H_ */
