/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Lexmark International, Inc.
 * Copyright 2025 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARM Cortex-A and Cortex-R
 *
 * Core thread related primitives for the ARM Cortex-A and
 * Cortex-R processor architecture.
 */

#include <zephyr/kernel.h>
#include <kernel_internal.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/sys/barrier.h>
#include <stdbool.h>
#include <cmsis_core.h>

#ifdef CONFIG_USERSPACE
static void setup_priv_stack(struct k_thread *thread)
{
	/* Set up privileged stack before entering user mode */
	thread->arch.priv_stack_start = (uint32_t)z_priv_stack_find(thread->stack_obj);

	/* CONFIG_PRIVILEGED_STACK_SIZE does not account for MPU_GUARD_ALIGN_AND_SIZE.
	 * Therefore, we must compute priv_stack_end here before adjusting priv_stack_start
	 * for the mpu guard alignment
	 */
	thread->arch.priv_stack_end = thread->arch.priv_stack_start + CONFIG_PRIVILEGED_STACK_SIZE;

#if defined(CONFIG_MPU_STACK_GUARD)
	/* Stack guard area reserved at the bottom of the thread's
	 * privileged stack. Adjust the available (writable) stack
	 * buffer area accordingly.
	 */
	thread->arch.priv_stack_start += MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_MPU_STACK_GUARD */
}
#endif

/* An initial context is put at the other end of the stack, and thus reusable
 * by the stack when not needed anymore. It is an exception stack frame (ESF):
 * the first switch to the thread returns to z_arm_cortex_ar_exit_exc(), whose
 * exception return pops it.
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct __basic_sf *iframe;

#if defined(CONFIG_FP_HARDABI) || defined(CONFIG_FP_SOFTABI)
	/*
	 * Both CONFIG_FP_HARDABI and CONFIG_FP_SOFTABI allow the compiler
	 * to generate FP instructions--even without explicit use of FP types.
	 * All threads must thus be considered as using FP registers and
	 * tagged with K_FP_REGS.
	 */
	thread->base.user_options |= K_FP_REGS;
#endif

#ifdef CONFIG_MPU_STACK_GUARD
#if defined(CONFIG_USERSPACE)
	if (z_stack_is_user_capable(stack)) {
		/* Guard area is carved-out of the buffer instead of reserved
		 * for stacks that can host user threads
		 */
		thread->stack_info.start += MPU_GUARD_ALIGN_AND_SIZE;
		thread->stack_info.size -= MPU_GUARD_ALIGN_AND_SIZE;
	}
#endif /* CONFIG_USERSPACE */
#endif /* CONFIG_MPU_STACK_GUARD */

	iframe = Z_STACK_PTR_TO_FRAME(struct __basic_sf, stack_ptr);
#if defined(CONFIG_USERSPACE)
	thread->arch.priv_stack_start = 0;
	if (k_thread_is_user_thread(thread)) {
		setup_priv_stack(thread);
		iframe = Z_STACK_PTR_TO_FRAME(struct __basic_sf, thread->arch.priv_stack_end);
		iframe->pc = (uint32_t)arch_user_mode_enter;
	} else {
		iframe->pc = (uint32_t)z_thread_entry;
	}
#else
	iframe->pc = (uint32_t)z_thread_entry;
#endif

	iframe->a1 = (uint32_t)entry;
	iframe->a2 = (uint32_t)p1;
	iframe->a3 = (uint32_t)p2;
	iframe->a4 = (uint32_t)p3;

	iframe->xpsr = A_BIT | MODE_SYS;
#if defined(CONFIG_BIG_ENDIAN)
	iframe->xpsr |= E_BIT;
#endif /* CONFIG_BIG_ENDIAN */

#if defined(CONFIG_COMPILER_ISA_THUMB2)
	iframe->xpsr |= T_BIT;
#endif /* CONFIG_COMPILER_ISA_THUMB2 */

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	thread->arch.fp_context_empty = true;
#endif

	/*
	 * The exception exit loads the frame address from SP, see
	 * z_arm_cortex_ar_enter_exc(). iframe is 8-byte aligned.
	 */
	uint32_t *frame_addr = (uint32_t *)iframe - 2;

#if defined(CONFIG_EXTRA_EXCEPTION_INFO)
	frame_addr -= Z_ARM_EXC_EXTRA_SIZE / sizeof(uint32_t);
#endif
	*frame_addr = (uint32_t)iframe;
	thread->callee_saved.psp = (uint32_t)frame_addr;

	/*
	 * initial values in all other registers/thread entries are
	 * irrelevant.
	 */
	extern void z_arm_cortex_ar_exit_exc(void);
	thread->switch_handle = thread;
	/* thread birth happens through the exception return path */
	thread->arch.exception_depth = 1;
	thread->callee_saved.lr = (uint32_t)z_arm_cortex_ar_exit_exc;
}

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	uint32_t sp_is_priv = 1;

#if defined(CONFIG_MPU_STACK_GUARD)
#if defined(CONFIG_THREAD_STACK_INFO)
	/* We're dropping to user mode which means the guard area is no
	 * longer used here, it instead is moved to the privilege stack
	 * to catch stack overflows there. Un-do the calculations done
	 * which accounted for memory borrowed from the thread stack.
	 */
	_current->stack_info.start -= MPU_GUARD_ALIGN_AND_SIZE;
	_current->stack_info.size += MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_THREAD_STACK_INFO */
#endif /* CONFIG_MPU_STACK_GUARD */

	/* 2 ways how arch_user_mode_enter is called:
	 * - as the entry point of a thread created with K_USER, in this case privileged stack is
	 *   already setup and stack pointer points to privileged stack.
	 * - called directly from k_thread_user_mode_enter, in this case privileged stack is not
	 *   setup and stack pointer points to user stack.
	 *
	 * When called from k_thread_user_mode_enter, we need to check and setup the privileged
	 * stack and then instruct z_arm_userspace_enter to change the PSP to the privileged stack.
	 * Note that we do not change the PSP in this function to avoid any conflict with compiler's
	 * sequence which has already pushed stuff on the user stack.
	 */
	if (0 == _current->arch.priv_stack_start) {
		setup_priv_stack(_current);
		sp_is_priv = 0;
	}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	z_arm_fpu_user_mode_enter();
#endif

	z_arm_userspace_enter(user_entry, p1, p2, p3,
			     (uint32_t)_current->stack_info.start,
			     _current->stack_info.size -
			     _current->stack_info.delta,
			     sp_is_priv);
	CODE_UNREACHABLE;
}

bool z_arm_thread_is_in_user_mode(void)
{
	uint32_t value;

	/*
	 * For Cortex-R, the mode (lower 5) bits will be 0x10 for user mode.
	 */
	value = __get_CPSR();
	return ((value & CPSR_M_Msk) == CPSR_M_USR);
}
EXPORT_SYMBOL(z_arm_thread_is_in_user_mode);
#endif

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)

#define IS_MPU_GUARD_VIOLATION(guard_start, guard_len, fault_addr, stack_ptr) \
	((fault_addr != -EINVAL) ? \
	((fault_addr >= guard_start) && \
	(fault_addr < (guard_start + guard_len)) && \
	(stack_ptr < (guard_start + guard_len))) \
	: \
	(stack_ptr < (guard_start + guard_len)))

/**
 * @brief Assess occurrence of current thread's stack corruption
 *
 * This function performs an assessment whether a memory fault (on a
 * given memory address) is the result of stack memory corruption of
 * the current thread.
 *
 * Thread stack corruption for supervisor threads or user threads in
 * privilege mode (when User Space is supported) is reported upon an
 * attempt to access the stack guard area (if MPU Stack Guard feature
 * is supported). Additionally the current PSP (process stack pointer)
 * must be pointing inside or below the guard area.
 *
 * Thread stack corruption for user threads in user mode is reported,
 * if the current PSP is pointing below the start of the current
 * thread's stack.
 *
 * Notes:
 * - we assume a fully descending stack,
 * - we assume a stacking error has occurred,
 * - the function shall be called when handling MemManage and Bus fault,
 *   and only if a Stacking error has been reported.
 *
 * If stack corruption is detected, the function returns the lowest
 * allowed address where the Stack Pointer can safely point to, to
 * prevent from errors when un-stacking the corrupted stack frame
 * upon exception return.
 *
 * @param fault_addr memory address on which memory access violation
 *                   has been reported. It can be invalid (-EINVAL),
 *                   if only Stacking error has been reported.
 * @param psp        current address the PSP points to
 *
 * @return The lowest allowed stack frame pointer, if error is a
 *         thread stack corruption, otherwise return 0.
 */
uint32_t z_check_thread_stack_fail(const uint32_t fault_addr, const uint32_t psp)
{
#if defined(CONFIG_MULTITHREADING)
	const struct k_thread *thread = _current;

	if (thread == NULL) {
		return 0;
	}
#endif

	/* If MPU_STACK_GUARD is not enabled, the guard length is
	 * effectively zero. Stack overflows may be detected only
	 * for user threads in nPRIV mode.
	 */
	uint32_t guard_len = MPU_GUARD_ALIGN_AND_SIZE;

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) {
		/* User thread */
		if (z_arm_thread_is_in_user_mode() == false) {
			/* User thread in privilege mode */
			if (IS_MPU_GUARD_VIOLATION(
				thread->arch.priv_stack_start - guard_len,
					guard_len,
				fault_addr, psp)) {
				/* Thread's privilege stack corruption */
				return thread->arch.priv_stack_start;
			}
		} else {
			if (psp < (uint32_t)thread->stack_obj) {
				/* Thread's user stack corruption */
				return (uint32_t)thread->stack_obj;
			}
		}
	} else {
		/* Supervisor thread */
		if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start -
				guard_len,
				guard_len,
				fault_addr, psp)) {
			/* Supervisor thread stack corruption */
			return thread->stack_info.start;
		}
	}
#else /* CONFIG_USERSPACE */
#if defined(CONFIG_MULTITHREADING)
	if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start - guard_len,
			guard_len,
			fault_addr, psp)) {
		/* Thread stack corruption */
		return thread->stack_info.start;
	}
#else
	if (IS_MPU_GUARD_VIOLATION((uint32_t)z_main_stack,
			guard_len,
			fault_addr, psp)) {
		/* Thread stack corruption */
		return (uint32_t)K_THREAD_STACK_BUFFER(z_main_stack);
	}
#endif
#endif /* CONFIG_USERSPACE */

	return 0;
}
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */

#if !defined(CONFIG_FPU_SHARING)
int arch_coprocessors_disable(struct k_thread *thread)
{
	ARG_UNUSED(thread);

	return -ENOTSUP;
}
#endif
