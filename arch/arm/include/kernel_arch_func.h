/*
 * Copyright (c) 2013-2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private kernel definitions (ARM)
 *
 * This file contains private kernel function definitions and various
 * other definitions for the ARM Cortex-M processor architecture family.
 *
 * This file is also included by assembly language files which must #define
 * _ASMLANGUAGE before including this header file.  Note that kernel
 * assembly source files obtains structure offset values via "absolute symbols"
 * in the offsets.o module.
 */

/* this file is only meant to be included by kernel_structs.h */

#ifndef ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _ASMLANGUAGE
extern void _FaultInit(void);
extern void _CpuIdleInit(void);
#ifdef CONFIG_ARM_MPU
extern void _arch_configure_static_mpu_regions(void);
extern void _arch_configure_dynamic_mpu_regions(struct k_thread *thread);
#endif /* CONFIG_ARM_MPU */

static ALWAYS_INLINE void kernel_arch_init(void)
{
	_InterruptStackSetup();
	_ExcSetup();
	_FaultInit();
	_CpuIdleInit();
}

static ALWAYS_INLINE void unlock_interrupts(void)
{
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	__enable_irq();
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	__enable_irq();
	__enable_fault_irq();
	__set_BASEPRI(0);
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
}

static ALWAYS_INLINE void
_arch_switch_to_main_thread(struct k_thread *main_thread,
			    k_thread_stack_t *main_stack,
			    size_t main_stack_size, k_thread_entry_t _main)
{
#ifdef CONFIG_ARM_MPU
	/* Configure static memory map. This will program MPU regions,
	 * to set up access permissions for fixed memory sections, such
	 * as Application Memory or No-Cacheable SRAM area.
	 *
	 * This function is invoked once, upon system initialization.
	 */
	_arch_configure_static_mpu_regions();
#endif

	/* get high address of the stack, i.e. its start (stack grows down) */
	char *start_of_main_stack;

#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) && \
	defined(CONFIG_USERSPACE)
	start_of_main_stack =
		K_THREAD_STACK_BUFFER(main_stack) + main_stack_size -
		MPU_GUARD_ALIGN_AND_SIZE;
#else
	start_of_main_stack =
		K_THREAD_STACK_BUFFER(main_stack) + main_stack_size;
#endif
	start_of_main_stack = (void *)STACK_ROUND_DOWN(start_of_main_stack);

#ifdef CONFIG_TRACING
	z_sys_trace_thread_switched_out();
#endif
	_current = main_thread;
#ifdef CONFIG_TRACING
	z_sys_trace_thread_switched_in();
#endif

	/* the ready queue cache already contains the main thread */

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* Set PSPLIM register for built-in stack guarding of main thread. */
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM((u32_t)main_stack);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	__set_PSP((u32_t)start_of_main_stack);
	unlock_interrupts();
#ifdef CONFIG_ARM_MPU
	/*
	 * If stack protection is enabled, make sure to set it
	 * before jumping to thread entry function
	 */
	_arch_configure_dynamic_mpu_regions(main_thread);
#endif
	_thread_entry(_main, 0, 0, 0);
	CODE_UNREACHABLE;
}

static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

extern void k_cpu_atomic_idle(unsigned int key);

#define _is_in_isr() _IsInIsr()

extern FUNC_NORETURN void _arm_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_ */
