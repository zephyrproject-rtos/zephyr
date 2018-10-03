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
 * other definitions for the ARM Cortex-M3 processor architecture.
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
static ALWAYS_INLINE void kernel_arch_init(void)
{
	_InterruptStackSetup();
	_ExcSetup();
	_FaultInit();
	_CpuIdleInit();
}

static ALWAYS_INLINE void
_arch_switch_to_main_thread(struct k_thread *main_thread,
			    k_thread_stack_t *main_stack,
			    size_t main_stack_size, k_thread_entry_t _main)
{
	/* get high address of the stack, i.e. its start (stack grows down) */
	char *start_of_main_stack;

#ifdef CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT
	start_of_main_stack =
		K_THREAD_STACK_BUFFER(main_stack) + main_stack_size -
		MPU_GUARD_ALIGN_AND_SIZE;
#else
	start_of_main_stack =
		K_THREAD_STACK_BUFFER(main_stack) + main_stack_size;
#endif
	start_of_main_stack = (void *)STACK_ROUND_DOWN(start_of_main_stack);

	_current = main_thread;

	/* the ready queue cache already contains the main thread */

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* Set PSPLIM register for built-in stack guarding of main thread. */
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM((u32_t)main_stack);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	__asm__ __volatile__(

		/* move to main() thread stack */
		"msr PSP, %0 \t\n"

		/* unlock interrupts */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
		"cpsie i \t\n"
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
		"cpsie if \t\n"
		"movs %%r1, #0 \n\t"
		"msr BASEPRI, %%r1 \n\t"
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */

#ifdef CONFIG_MPU_STACK_GUARD
		/*
		 * if guard is enabled, make sure to set it before jumping to thread
		 * entry function
		*/
		"mov %%r0, %3 \t\n"
		"push {r2, lr} \t\n"
		"blx configure_mpu_stack_guard \t\n"
		"pop {r2, lr} \t\n"
#endif
		/* branch to _thread_entry(_main, 0, 0, 0) */
		"mov %%r0, %1 \n\t"
		"bx %2 \t\n"

		/* never gets here */

		:
		: "r"(start_of_main_stack),
		  "r"(_main), "r"(_thread_entry),
		  "r"(main_thread)

		: "r0", "r1", "sp"
	);

	CODE_UNREACHABLE;
}

static ALWAYS_INLINE void
_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

extern void k_cpu_atomic_idle(unsigned int key);

#define _is_in_isr() _IsInIsr()

extern void _IntLibInit(void);


extern FUNC_NORETURN void _arm_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_ */
