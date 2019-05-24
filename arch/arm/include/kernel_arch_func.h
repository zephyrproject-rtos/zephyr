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
extern void z_FaultInit(void);
extern void z_CpuIdleInit(void);
#ifdef CONFIG_ARM_MPU
extern void z_arch_configure_static_mpu_regions(void);
extern void z_arch_configure_dynamic_mpu_regions(struct k_thread *thread);
#endif /* CONFIG_ARM_MPU */

static ALWAYS_INLINE void kernel_arch_init(void)
{
	z_InterruptStackSetup();
	z_ExcSetup();
	z_FaultInit();
	z_CpuIdleInit();
	z_clearfaults();
}

static ALWAYS_INLINE void
z_arch_switch_to_main_thread(struct k_thread *main_thread,
			    k_thread_stack_t *main_stack,
			    size_t main_stack_size, k_thread_entry_t _main)
{
#if defined(CONFIG_FLOAT)
	/* Initialize the Floating Point Status and Control Register when in
	 * Unshared FP Registers mode (In Shared FP Registers mode, FPSCR is
	 * initialized at thread creation for threads that make use of the FP).
	 */
	__set_FPSCR(0);
#if defined(CONFIG_FP_SHARING)
	/* In Sharing mode clearing FPSCR may set the CONTROL.FPCA flag. */
	__set_CONTROL(__get_CONTROL() & (~(CONTROL_FPCA_Msk)));
	__ISB();
#endif /* CONFIG_FP_SHARING */
#endif /* CONFIG_FLOAT */

#ifdef CONFIG_ARM_MPU
	/* Configure static memory map. This will program MPU regions,
	 * to set up access permissions for fixed memory sections, such
	 * as Application Memory or No-Cacheable SRAM area.
	 *
	 * This function is invoked once, upon system initialization.
	 */
	z_arch_configure_static_mpu_regions();
#endif

	/* get high address of the stack, i.e. its start (stack grows down) */
	char *start_of_main_stack;

#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) && \
	defined(CONFIG_USERSPACE)
	start_of_main_stack =
		Z_THREAD_STACK_BUFFER(main_stack) + main_stack_size -
		MPU_GUARD_ALIGN_AND_SIZE;
#else
	start_of_main_stack =
		Z_THREAD_STACK_BUFFER(main_stack) + main_stack_size;
#endif
	start_of_main_stack = (char *)STACK_ROUND_DOWN(start_of_main_stack);

#ifdef CONFIG_TRACING
	z_sys_trace_thread_switched_out();
#endif
	_current = main_thread;
#ifdef CONFIG_TRACING
	z_sys_trace_thread_switched_in();
#endif

	/* the ready queue cache already contains the main thread */

#ifdef CONFIG_ARM_MPU
	/*
	 * If stack protection is enabled, make sure to set it
	 * before jumping to thread entry function
	 */
	z_arch_configure_dynamic_mpu_regions(main_thread);
#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
	/* Set PSPLIM register for built-in stack guarding of main thread. */
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM((u32_t)main_stack);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
#endif /* CONFIG_BUILTIN_STACK_GUARD */

	/*
	 * Set PSP to the highest address of the main stack
	 * before enabling interrupts and jumping to main.
	 */
	__asm__ volatile (
	"mov   r0,  %0     \n\t"   /* Store _main in R0 */
	"msr   PSP, %1     \n\t"   /* __set_PSP(start_of_main_stack) */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	"cpsie i           \n\t"   /* __enable_irq() */
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	"cpsie if          \n\t"   /* __enable_irq(); __enable_fault_irq() */
	"mov   r1,  #0     \n\t"
	"msr   BASEPRI, r1 \n\t"   /* __set_BASEPRI(0) */
#else
#error Unknown ARM architecture
#endif /* CONFIG_ARMV6_M_ARMV8_M_BASELINE */
	"isb               \n\t"
	"movs r1, #0       \n\t"
	"movs r2, #0       \n\t"
	"movs r3, #0       \n\t"
	"bl z_thread_entry \n\t"   /* z_thread_entry(_main, 0, 0, 0); */
	:
	: "r" (_main), "r" (start_of_main_stack)
	);

	CODE_UNREACHABLE;
}

static ALWAYS_INLINE void
z_set_thread_return_value(struct k_thread *thread, unsigned int value)
{
	thread->arch.swap_return_value = value;
}

extern void k_cpu_atomic_idle(unsigned int key);

#define z_is_in_isr() z_IsInIsr()

extern FUNC_NORETURN void z_arm_userspace_enter(k_thread_entry_t user_entry,
					       void *p1, void *p2, void *p3,
					       u32_t stack_end,
					       u32_t stack_start);

#endif /* _ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_ARM_INCLUDE_KERNEL_ARCH_FUNC_H_ */
