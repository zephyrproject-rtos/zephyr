/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief New thread creation for ARCv2
 *
 * Core thread related primitives for the ARCv2 processor architecture.
 */

#include <kernel.h>
#include <ksched.h>
#include <offsets_short.h>
#include <wait_q.h>

#ifdef CONFIG_USERSPACE
#include <arch/arc/v2/mpu/arc_core_mpu.h>
#endif

/*  initial stack frame */
struct init_stack_frame {
	u32_t pc;
#ifdef CONFIG_ARC_HAS_SECURE
	u32_t sec_stat;
#endif
	u32_t status32;
	u32_t r3;
	u32_t r2;
	u32_t r1;
	u32_t r0;
};

/*
 * @brief Initialize a new thread from its stack space
 *
 * The thread control structure is put at the lower address of the stack. An
 * initial context, to be "restored" by __return_from_coop(), is put at
 * the other end of the stack, and thus reusable by the stack when not
 * needed anymore.
 *
 * The initial context is a basic stack frame that contains arguments for
 * z_thread_entry() return address, that points at z_thread_entry()
 * and status register.
 *
 * <options> is currently unused.
 *
 * @param pStackmem the pointer to aligned stack memory
 * @param stackSize the stack size in bytes
 * @param pEntry thread entry point routine
 * @param parameter1 first param to entry point
 * @param parameter2 second param to entry point
 * @param parameter3 third param to entry point
 * @param priority thread priority
 * @param options thread options: K_ESSENTIAL
 *
 * @return N/A
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     size_t stackSize, k_thread_entry_t pEntry,
		     void *parameter1, void *parameter2, void *parameter3,
		     int priority, unsigned int options)
{
	char *pStackMem = Z_THREAD_STACK_BUFFER(stack);

	char *stackEnd;
	char *priv_stack_end;
	struct init_stack_frame *pInitCtx;

#ifdef CONFIG_USERSPACE

	size_t stackAdjSize;
	size_t offset = 0;


	stackAdjSize = Z_ARC_MPU_SIZE_ALIGN(stackSize);

	stackEnd = pStackMem + stackAdjSize;

#ifdef CONFIG_STACK_POINTER_RANDOM
	offset = stackAdjSize - stackSize;
#endif

	if (options & K_USER) {
#ifdef CONFIG_GEN_PRIV_STACKS
		thread->arch.priv_stack_start =
			(u32_t)z_priv_stack_find(thread->stack_obj);
#else
		thread->arch.priv_stack_start =
			(u32_t)(stackEnd + STACK_GUARD_SIZE);
#endif

		priv_stack_end = (char *)Z_STACK_PTR_ALIGN(
				thread->arch.priv_stack_start +
				CONFIG_PRIVILEGED_STACK_SIZE);

		/* reserve 4 bytes for the start of user sp */
		priv_stack_end -= 4;
		(*(u32_t *)priv_stack_end) = Z_STACK_PTR_ALIGN(
			(u32_t)stackEnd - offset);

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
		/* reserve stack space for the userspace local data struct */
		thread->userspace_local_data =
			(struct _thread_userspace_local_data *)
			Z_STACK_PTR_ALIGN(stackEnd -
			sizeof(*thread->userspace_local_data) - offset);
		/* update the start of user sp */
		(*(u32_t *)priv_stack_end) =
			(u32_t) thread->userspace_local_data;
#endif

	} else {
		pStackMem += STACK_GUARD_SIZE;
		stackEnd += STACK_GUARD_SIZE;

		thread->arch.priv_stack_start = 0;

#ifdef CONFIG_THREAD_USERSPACE_LOCAL_DATA
		/* reserve stack space for the userspace local data struct */
		priv_stack_end = (char *)Z_STACK_PTR_ALIGN(stackEnd
			- sizeof(*thread->userspace_local_data) - offset);
		thread->userspace_local_data =
			(struct _thread_userspace_local_data *)priv_stack_end;
#else
		priv_stack_end = (char *)Z_STACK_PTR_ALIGN(stackEnd - offset);
#endif
	}

	z_new_thread_init(thread, pStackMem, stackAdjSize);

	/* carve the thread entry struct from the "base" of
		the privileged stack */
	pInitCtx = (struct init_stack_frame *)(
		priv_stack_end - sizeof(struct init_stack_frame));

	/* fill init context */
	pInitCtx->status32 = 0U;
	if (options & K_USER) {
		pInitCtx->pc = ((u32_t)z_user_thread_entry_wrapper);
	} else {
		pInitCtx->pc = ((u32_t)z_thread_entry_wrapper);
	}

	/*
	 * enable US bit, US is read as zero in user mode. This will allow use
	 * mode sleep instructions, and it enables a form of denial-of-service
	 * attack by putting the processor in sleep mode, but since interrupt
	 * level/mask can't be set from user space that's not worse than
	 * executing a loop without yielding.
	 */
	pInitCtx->status32 |= _ARC_V2_STATUS32_US;
#else /* For no USERSPACE feature */
	pStackMem += STACK_GUARD_SIZE;
	stackEnd = pStackMem + stackSize;

	z_new_thread_init(thread, pStackMem, stackSize);

	priv_stack_end = stackEnd;

	pInitCtx = (struct init_stack_frame *)(
		Z_STACK_PTR_ALIGN(priv_stack_end) -
		sizeof(struct init_stack_frame));

	pInitCtx->status32 = 0U;
	pInitCtx->pc = ((u32_t)z_thread_entry_wrapper);
#endif

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	pInitCtx->sec_stat = z_arc_v2_aux_reg_read(_ARC_V2_SEC_STAT);
#endif

	pInitCtx->r0 = (u32_t)pEntry;
	pInitCtx->r1 = (u32_t)parameter1;
	pInitCtx->r2 = (u32_t)parameter2;
	pInitCtx->r3 = (u32_t)parameter3;

/* stack check configuration */
#ifdef CONFIG_ARC_STACK_CHECKING
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	pInitCtx->sec_stat |= _ARC_V2_SEC_STAT_SSC;
#else
	pInitCtx->status32 |= _ARC_V2_STATUS32_SC;
#endif
#ifdef CONFIG_USERSPACE
	if (options & K_USER) {
		thread->arch.u_stack_top = (u32_t)pStackMem;
		thread->arch.u_stack_base = (u32_t)stackEnd;
		thread->arch.k_stack_top =
			 (u32_t)(thread->arch.priv_stack_start);
		thread->arch.k_stack_base = (u32_t)
		(thread->arch.priv_stack_start + CONFIG_PRIVILEGED_STACK_SIZE);
	} else {
		thread->arch.k_stack_top = (u32_t)pStackMem;
		thread->arch.k_stack_base = (u32_t)stackEnd;
		thread->arch.u_stack_top = 0;
		thread->arch.u_stack_base = 0;
	}
#else
	thread->arch.k_stack_top = (u32_t) pStackMem;
	thread->arch.k_stack_base = (u32_t) stackEnd;
#endif
#endif

#ifdef CONFIG_ARC_USE_UNALIGNED_MEM_ACCESS
	pInitCtx->status32 |= _ARC_V2_STATUS32_AD;
#endif

	thread->switch_handle = thread;
	thread->arch.relinquish_cause = _CAUSE_COOP;
	thread->callee_saved.sp =
		(u32_t)pInitCtx - ___callee_saved_stack_t_SIZEOF;

	/* initial values in all other regs/k_thread entries are irrelevant */
}

void *z_arch_get_next_switch_handle(struct k_thread **old_thread)
{
	*old_thread =  _current;

	return z_get_next_switch_handle(*old_thread);
}

#ifdef CONFIG_USERSPACE

FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{


	_current->stack_info.start = (u32_t)_current->stack_obj;
#ifdef CONFIG_GEN_PRIV_STACKS
	_current->arch.priv_stack_start =
			(u32_t)z_priv_stack_find(_current->stack_obj);
#else
	_current->arch.priv_stack_start =
			(u32_t)(_current->stack_info.start +
				_current->stack_info.size + STACK_GUARD_SIZE);
#endif


#ifdef CONFIG_ARC_STACK_CHECKING
	_current->arch.k_stack_top = _current->arch.priv_stack_start;
	_current->arch.k_stack_base = _current->arch.priv_stack_start +
				CONFIG_PRIVILEGED_STACK_SIZE;
	_current->arch.u_stack_top = _current->stack_info.start;
	_current->arch.u_stack_base = _current->stack_info.start +
				_current->stack_info.size;
#endif

	/* possible optimizaiton: no need to load mem domain anymore */
	/* need to lock cpu here ? */
	configure_mpu_thread(_current);

	z_arc_userspace_enter(user_entry, p1, p2, p3,
			      (u32_t)_current->stack_obj,
			      _current->stack_info.size, _current);
	CODE_UNREACHABLE;
}

#endif

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	unsigned int key;

	/* Ensure a preemptive context switch does not occur */

	key = irq_lock();

	/* Disable all floating point capabilities for the thread */
	thread->base.user_options &= ~K_FP_REGS;

	irq_unlock(key);

	return 0;
}


int arch_float_enable(struct k_thread *thread)
{
	unsigned int key;

	/* Ensure a preemptive context switch does not occur */

	key = irq_lock();

	/* Enable all floating point capabilities for the thread */
	thread->base.user_options |= K_FP_REGS;

	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */
