/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARM Cortex-M
 *
 * Core thread related primitives for the ARM Cortex-M processor architecture.
 */

#include <kernel.h>
#include <toolchain.h>
#include <kernel_structs.h>
#include <wait_q.h>

#ifdef CONFIG_USERSPACE
extern u8_t *z_priv_stack_find(void *obj);
#endif

/**
 *
 * @brief Initialize a new thread from its stack space
 *
 * The control structure (thread) is put at the lower address of the stack. An
 * initial context, to be "restored" by __pendsv(), is put at the other end of
 * the stack, and thus reusable by the stack when not needed anymore.
 *
 * The initial context is an exception stack frame (ESF) since exiting the
 * PendSV exception will want to pop an ESF. Interestingly, even if the lsb of
 * an instruction address to jump to must always be set since the CPU always
 * runs in thumb mode, the ESF expects the real address of the instruction,
 * with the lsb *not* set (instructions are always aligned on 16 bit halfwords).
 * Since the compiler automatically sets the lsb of function addresses, we have
 * to unset it manually before storing it in the 'pc' field of the ESF.
 *
 * <options> is currently unused.
 *
 * @param stack      pointer to the aligned stack memory
 * @param stackSize  size of the available stack memory in bytes
 * @param pEntry the entry point
 * @param parameter1 entry point to the first param
 * @param parameter2 entry point to the second param
 * @param parameter3 entry point to the third param
 * @param priority   thread priority
 * @param options    thread options: K_ESSENTIAL, K_FP_REGS
 *
 * @return N/A
 */

void z_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		 size_t stackSize, k_thread_entry_t pEntry,
		 void *parameter1, void *parameter2, void *parameter3,
		 int priority, unsigned int options)
{
	char *pStackMem = Z_THREAD_STACK_BUFFER(stack);
	char *stackEnd;
	/* Offset between the top of stack and the high end of stack area. */
	u32_t top_of_stack_offset = 0U;

	Z_ASSERT_VALID_PRIO(priority, pEntry);

#if defined(CONFIG_USERSPACE)
	/* Truncate the stack size to align with the MPU region granularity.
	 * This is done proactively to account for the case when the thread
	 * switches to user mode (thus, its stack area will need to be MPU-
	 * programmed to be assigned unprivileged RW access permission).
	 */
	stackSize &= ~(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE - 1);

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
	/* Reserve space on top of stack for local data. */
	u32_t p_local_data = STACK_ROUND_DOWN(pStackMem + stackSize
		- sizeof(*thread->userspace_local_data));

	thread->userspace_local_data =
		(struct _thread_userspace_local_data *)(p_local_data);

	/* Top of actual stack must be moved below the user local data. */
	top_of_stack_offset = (u32_t)
		(pStackMem + stackSize - ((char *)p_local_data));

#endif /* CONFIG_THREAD_USERSPACE_LOCAL_DATA */
#endif /* CONFIG_USERSPACE */

#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT) \
	&& defined(CONFIG_USERSPACE)
	/* This is required to work-around the case where the thread
	 * is created without using K_THREAD_STACK_SIZEOF() macro in
	 * k_thread_create(). If K_THREAD_STACK_SIZEOF() is used, the
	 * Guard size has already been take out of stackSize.
	 */
	stackSize -= MPU_GUARD_ALIGN_AND_SIZE;
#endif
	stackEnd = pStackMem + stackSize;

	struct __esf *pInitCtx;

	z_new_thread_init(thread, pStackMem, stackSize, priority,
			 options);

	/* Carve the thread entry struct from the "base" of the stack
	 *
	 * The initial carved stack frame only needs to contain the basic
	 * stack frame (state context), because no FP operations have been
	 * performed yet for this thread.
	 */
	pInitCtx = (struct __esf *)(STACK_ROUND_DOWN(stackEnd -
		(char *)top_of_stack_offset - sizeof(struct __basic_sf)));

#if defined(CONFIG_USERSPACE)
	if ((options & K_USER) != 0) {
		pInitCtx->basic.pc = (u32_t)z_arch_user_mode_enter;
	} else {
		pInitCtx->basic.pc = (u32_t)z_thread_entry;
	}
#else
	pInitCtx->basic.pc = (u32_t)z_thread_entry;
#endif

	/* force ARM mode by clearing LSB of address */
	pInitCtx->basic.pc &= 0xfffffffe;

	pInitCtx->basic.a1 = (u32_t)pEntry;
	pInitCtx->basic.a2 = (u32_t)parameter1;
	pInitCtx->basic.a3 = (u32_t)parameter2;
	pInitCtx->basic.a4 = (u32_t)parameter3;
	pInitCtx->basic.xpsr =
		0x01000000UL; /* clear all, thumb bit is 1, even if RO */

	thread->callee_saved.psp = (u32_t)pInitCtx;
	thread->arch.basepri = 0;

#if defined(CONFIG_USERSPACE) || defined(CONFIG_FP_SHARING)
	thread->arch.mode = 0;
#if defined(CONFIG_USERSPACE)
	thread->arch.priv_stack_start = 0;
#endif
#endif

	/* swap_return_value can contain garbage */

	/*
	 * initial values in all other registers/thread entries are
	 * irrelevant.
	 */
}

#ifdef CONFIG_USERSPACE

FUNC_NORETURN void z_arch_user_mode_enter(k_thread_entry_t user_entry,
	void *p1, void *p2, void *p3)
{

	/* Set up privileged stack before entering user mode */
	_current->arch.priv_stack_start =
		(u32_t)z_priv_stack_find(_current->stack_obj);

	z_arm_userspace_enter(user_entry, p1, p2, p3,
			     (u32_t)_current->stack_info.start,
			     _current->stack_info.size);
	CODE_UNREACHABLE;
}

#endif

#if defined(CONFIG_BUILTIN_STACK_GUARD)
/*
 * @brief Configure ARM built-in stack guard
 *
 * This function configures per thread stack guards by reprogramming
 * the built-in Process Stack Pointer Limit Register (PSPLIM).
 * The functionality is meant to be used during context switch.
 *
 * @param thread thread info data structure.
 */
void configure_builtin_stack_guard(struct k_thread *thread)
{
#if defined(CONFIG_USERSPACE)
	if ((thread->arch.mode & CONTROL_nPRIV_Msk) != 0) {
		/* Only configure stack limit for threads in privileged mode
		 * (i.e supervisor threads or user threads doing system call).
		 * User threads executing in user mode do not require a stack
		 * limit protection.
		 */
		return;
	}
	u32_t guard_start = thread->arch.priv_stack_start ?
			    (u32_t)thread->arch.priv_stack_start :
			    (u32_t)thread->stack_obj;

	__ASSERT(thread->stack_info.start == ((u32_t)thread->stack_obj),
		"stack_info.start does not point to the start of the"
		"thread allocated area.");
#else
	u32_t guard_start = thread->stack_info.start;
#endif
#if defined(CONFIG_CPU_CORTEX_M_HAS_SPLIM)
	__set_PSPLIM(guard_start);
#else
#error "Built-in PSP limit checks not supported by HW"
#endif
}
#endif /* CONFIG_BUILTIN_STACK_GUARD */

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)

#define IS_MPU_GUARD_VIOLATION(guard_start, fault_addr, stack_ptr) \
	(fault_addr == -EINVAL) ? \
	((fault_addr >= guard_start) && \
	(fault_addr < (guard_start + MPU_GUARD_ALIGN_AND_SIZE)) && \
	(stack_ptr < (guard_start + MPU_GUARD_ALIGN_AND_SIZE))) \
	: \
	(stack_ptr < (guard_start + MPU_GUARD_ALIGN_AND_SIZE))

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
u32_t z_check_thread_stack_fail(const u32_t fault_addr, const u32_t psp)
{
	const struct k_thread *thread = _current;

	if (!thread) {
		return 0;
	}

#if defined(CONFIG_USERSPACE)
	if (thread->arch.priv_stack_start) {
		/* User thread */
		if ((__get_CONTROL() & CONTROL_nPRIV_Msk) == 0) {
			/* User thread in privilege mode */
			if (IS_MPU_GUARD_VIOLATION(
				thread->arch.priv_stack_start,
				fault_addr, psp)) {
				/* Thread's privilege stack corruption */
				return thread->arch.priv_stack_start +
					MPU_GUARD_ALIGN_AND_SIZE;
			}
		} else {
			if (psp < (u32_t)thread->stack_obj) {
				/* Thread's user stack corruption */
				return (u32_t)thread->stack_obj;
			}
		}
	} else {
		/* Supervisor thread */
		if (IS_MPU_GUARD_VIOLATION((u32_t)thread->stack_obj,
			fault_addr, psp)) {
			/* Supervisor thread stack corruption */
			return (u32_t)thread->stack_obj +
				MPU_GUARD_ALIGN_AND_SIZE;
		}
	}
#else /* CONFIG_USERSPACE */
	if (IS_MPU_GUARD_VIOLATION(thread->stack_info.start,
			fault_addr, psp)) {
		/* Thread stack corruption */
		return thread->stack_info.start +
			MPU_GUARD_ALIGN_AND_SIZE;
	}
#endif /* CONFIG_USERSPACE */

	return 0;
}
#endif /* CONFIG_MPU_STACK_GUARD || CONFIG_USERSPACE */
