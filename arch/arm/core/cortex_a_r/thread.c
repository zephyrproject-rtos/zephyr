/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Lexmark International, Inc.
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
#include <zephyr/llext/symbol.h>
#include <ksched.h>
#include <zephyr/sys/barrier.h>
#include <stdbool.h>
#include <cmsis_core.h>

#if (MPU_GUARD_ALIGN_AND_SIZE_FLOAT > MPU_GUARD_ALIGN_AND_SIZE)
#define FP_GUARD_EXTRA_SIZE	(MPU_GUARD_ALIGN_AND_SIZE_FLOAT - \
				 MPU_GUARD_ALIGN_AND_SIZE)
#else
#define FP_GUARD_EXTRA_SIZE	0
#endif

#ifndef EXC_RETURN_FTYPE
/* bit [4] allocate stack for floating-point context: 0=done 1=skipped  */
#define EXC_RETURN_FTYPE           (0x00000010UL)
#endif

/* Default last octet of EXC_RETURN, for threads that have not run yet.
 * The full EXC_RETURN value will be e.g. 0xFFFFFFBC.
 */
#define DEFAULT_EXC_RETURN 0xFD;

/* An initial context, to be "restored" by z_arm_pendsv(), is put at the other
 * end of the stack, and thus reusable by the stack when not needed anymore.
 *
 * The initial context is an exception stack frame (ESF) since exiting the
 * PendSV exception will want to pop an ESF. Interestingly, even if the lsb of
 * an instruction address to jump to must always be set since the CPU always
 * runs in thumb mode, the ESF expects the real address of the instruction,
 * with the lsb *not* set (instructions are always aligned on 16 bit
 * halfwords).  Since the compiler automatically sets the lsb of function
 * addresses, we have to unset it manually before storing it in the 'pc' field
 * of the ESF.
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct __basic_sf *iframe;

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
#if FP_GUARD_EXTRA_SIZE > 0
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		/* Larger guard needed due to lazy stacking of FP regs may
		 * overshoot the guard area without writing anything. We
		 * carve it out of the stack buffer as-needed instead of
		 * unconditionally reserving it.
		 */
		thread->stack_info.start += FP_GUARD_EXTRA_SIZE;
		thread->stack_info.size -= FP_GUARD_EXTRA_SIZE;
	}
#endif /* FP_GUARD_EXTRA_SIZE */
#endif /* CONFIG_MPU_STACK_GUARD */

	iframe = Z_STACK_PTR_TO_FRAME(struct __basic_sf, stack_ptr);
#if defined(CONFIG_USERSPACE)
	if ((thread->base.user_options & K_USER) != 0) {
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
	iframe = (struct __basic_sf *)
		((uintptr_t)iframe - sizeof(struct __fpu_sf));
	memset(iframe, 0, sizeof(struct __fpu_sf));
#endif

	thread->callee_saved.psp = (uint32_t)iframe;
	thread->arch.basepri = 0;

#if defined(CONFIG_ARM_STORE_EXC_RETURN) || defined(CONFIG_USERSPACE)
	thread->arch.mode = 0;
#if defined(CONFIG_ARM_STORE_EXC_RETURN)
	thread->arch.mode_exc_return = DEFAULT_EXC_RETURN;
#endif
#if FP_GUARD_EXTRA_SIZE > 0
	if ((thread->base.user_options & K_FP_REGS) != 0) {
		thread->arch.mode |= Z_ARM_MODE_MPU_GUARD_FLOAT_Msk;
	}
#endif
#if defined(CONFIG_USERSPACE)
	thread->arch.priv_stack_start = 0;
#endif
#endif
	/*
	 * initial values in all other registers/thread entries are
	 * irrelevant.
	 */
#if defined(CONFIG_USE_SWITCH)
	extern void z_arm_cortex_ar_exit_exc(void);
	thread->switch_handle = thread;
	/* thread birth happens through the exception return path */
	thread->arch.exception_depth = 1;
	thread->callee_saved.lr = (uint32_t)z_arm_cortex_ar_exit_exc;
#endif
}

#if defined(CONFIG_MPU_STACK_GUARD) && defined(CONFIG_FPU) \
	&& defined(CONFIG_FPU_SHARING)

static inline void z_arm_thread_stack_info_adjust(struct k_thread *thread,
	bool use_large_guard)
{
	if (use_large_guard) {
		/* Switch to use a large MPU guard if not already. */
		if ((thread->arch.mode &
			Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) == 0) {
			/* Default guard size is used. Update required. */
			thread->arch.mode |= Z_ARM_MODE_MPU_GUARD_FLOAT_Msk;
#if defined(CONFIG_USERSPACE)
			if (thread->arch.priv_stack_start) {
				/* User thread */
				thread->arch.priv_stack_start +=
					FP_GUARD_EXTRA_SIZE;
			} else
#endif /* CONFIG_USERSPACE */
			{
				/* Privileged thread */
				thread->stack_info.start +=
					FP_GUARD_EXTRA_SIZE;
				thread->stack_info.size -=
					FP_GUARD_EXTRA_SIZE;
			}
		}
	} else {
		/* Switch to use the default MPU guard size if not already. */
		if ((thread->arch.mode &
			Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) {
			/* Large guard size is used. Update required. */
			thread->arch.mode &= ~Z_ARM_MODE_MPU_GUARD_FLOAT_Msk;
#if defined(CONFIG_USERSPACE)
			if (thread->arch.priv_stack_start) {
				/* User thread */
				thread->arch.priv_stack_start -=
					FP_GUARD_EXTRA_SIZE;
			} else
#endif /* CONFIG_USERSPACE */
			{
				/* Privileged thread */
				thread->stack_info.start -=
					FP_GUARD_EXTRA_SIZE;
				thread->stack_info.size +=
					FP_GUARD_EXTRA_SIZE;
			}
		}
	}
}

#endif

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{

	/* Set up privileged stack before entering user mode */
	arch_current_thread()->arch.priv_stack_start =
		(uint32_t)z_priv_stack_find(arch_current_thread()->stack_obj);
#if defined(CONFIG_MPU_STACK_GUARD)
#if defined(CONFIG_THREAD_STACK_INFO)
	/* We're dropping to user mode which means the guard area is no
	 * longer used here, it instead is moved to the privilege stack
	 * to catch stack overflows there. Un-do the calculations done
	 * which accounted for memory borrowed from the thread stack.
	 */
#if FP_GUARD_EXTRA_SIZE > 0
	if ((arch_current_thread()->arch.mode & Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) {
		arch_current_thread()->stack_info.start -= FP_GUARD_EXTRA_SIZE;
		arch_current_thread()->stack_info.size += FP_GUARD_EXTRA_SIZE;
	}
#endif /* FP_GUARD_EXTRA_SIZE */
	arch_current_thread()->stack_info.start -= MPU_GUARD_ALIGN_AND_SIZE;
	arch_current_thread()->stack_info.size += MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_THREAD_STACK_INFO */

	/* Stack guard area reserved at the bottom of the thread's
	 * privileged stack. Adjust the available (writable) stack
	 * buffer area accordingly.
	 */
#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	arch_current_thread()->arch.priv_stack_start +=
		((arch_current_thread()->arch.mode & Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) ?
		MPU_GUARD_ALIGN_AND_SIZE_FLOAT : MPU_GUARD_ALIGN_AND_SIZE;
#else
	arch_current_thread()->arch.priv_stack_start += MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
#endif /* CONFIG_MPU_STACK_GUARD */

#if defined(CONFIG_CPU_AARCH32_CORTEX_R)
	arch_current_thread()->arch.priv_stack_end =
		arch_current_thread()->arch.priv_stack_start + CONFIG_PRIVILEGED_STACK_SIZE;
#endif

	z_arm_userspace_enter(user_entry, p1, p2, p3,
			     (uint32_t)arch_current_thread()->stack_info.start,
			     arch_current_thread()->stack_info.size -
			     arch_current_thread()->stack_info.delta);
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
	const struct k_thread *thread = arch_current_thread();

	if (thread == NULL) {
		return 0;
	}
#endif

#if (defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)) && \
	defined(CONFIG_MPU_STACK_GUARD)
	uint32_t guard_len =
		((arch_current_thread()->arch.mode & Z_ARM_MODE_MPU_GUARD_FLOAT_Msk) != 0) ?
		MPU_GUARD_ALIGN_AND_SIZE_FLOAT : MPU_GUARD_ALIGN_AND_SIZE;
#else
	/* If MPU_STACK_GUARD is not enabled, the guard length is
	 * effectively zero. Stack overflows may be detected only
	 * for user threads in nPRIV mode.
	 */
	uint32_t guard_len = MPU_GUARD_ALIGN_AND_SIZE;
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

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

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	if (thread != arch_current_thread()) {
		return -EINVAL;
	}

	if (arch_is_in_isr()) {
		return -EINVAL;
	}

	/* Disable all floating point capabilities for the thread */

	/* K_FP_REG flag is used in SWAP and stack check fail. Locking
	 * interrupts here prevents a possible context-switch or MPU
	 * fault to take an outdated thread user_options flag into
	 * account.
	 */
	int key = arch_irq_lock();

	thread->base.user_options &= ~K_FP_REGS;

	__set_FPEXC(0);

	/* No need to add an ISB barrier after setting the CONTROL
	 * register; arch_irq_unlock() already adds one.
	 */

	arch_irq_unlock(key);

	return 0;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	/* This is not supported in Cortex-A and Cortex-R */
	return -ENOTSUP;
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
