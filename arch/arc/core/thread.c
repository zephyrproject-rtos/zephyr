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
	uint32_t pc;
#ifdef CONFIG_ARC_HAS_SECURE
	uint32_t sec_stat;
#endif
	uint32_t status32;
	uint32_t r3;
	uint32_t r2;
	uint32_t r1;
	uint32_t r0;
};

#ifdef CONFIG_USERSPACE
struct user_init_stack_frame {
	struct init_stack_frame iframe;
	uint32_t user_sp;
};

static bool is_user(struct k_thread *thread)
{
	return (thread->base.user_options & K_USER) != 0;
}
#endif

/* Set all stack-related architecture variables for the provided thread */
static void setup_stack_vars(struct k_thread *thread)
{
#ifdef CONFIG_USERSPACE
	if (is_user(thread)) {
#ifdef CONFIG_GEN_PRIV_STACKS
		thread->arch.priv_stack_start =
			(uint32_t)z_priv_stack_find(thread->stack_obj);
#else
		thread->arch.priv_stack_start =	(uint32_t)(thread->stack_obj);
#endif /* CONFIG_GEN_PRIV_STACKS */
		thread->arch.priv_stack_start += Z_ARC_STACK_GUARD_SIZE;
	} else {
		thread->arch.priv_stack_start = 0;
	}
#endif /* CONFIG_USERSPACE */

#ifdef CONFIG_ARC_STACK_CHECKING
#ifdef CONFIG_USERSPACE
	if (is_user(thread)) {
		thread->arch.k_stack_top = thread->arch.priv_stack_start;
		thread->arch.k_stack_base = (thread->arch.priv_stack_start +
					     CONFIG_PRIVILEGED_STACK_SIZE);
		thread->arch.u_stack_top = thread->stack_info.start;
		thread->arch.u_stack_base = (thread->stack_info.start +
					     thread->stack_info.size);
	} else
#endif /* CONFIG_USERSPACE */
	{
		thread->arch.k_stack_top = (uint32_t)thread->stack_info.start;
		thread->arch.k_stack_base = (uint32_t)(thread->stack_info.start +
						    thread->stack_info.size);
#ifdef CONFIG_USERSPACE
		thread->arch.u_stack_top = 0;
		thread->arch.u_stack_base = 0;
#endif /* CONFIG_USERSPACE */
	}
#endif /* CONFIG_ARC_STACK_CHECKING */
}

/* Get the initial stack frame pointer from the thread's stack buffer. */
static struct init_stack_frame *get_iframe(struct k_thread *thread,
					   char *stack_ptr)
{
#ifdef CONFIG_USERSPACE
	if (is_user(thread)) {
		/* Initial stack frame for a user thread is slightly larger;
		 * we land in z_user_thread_entry_wrapper on the privilege
		 * stack, and pop off an additional value for the user
		 * stack pointer.
		 */
		struct user_init_stack_frame *uframe;

		uframe = Z_STACK_PTR_TO_FRAME(struct user_init_stack_frame,
					      thread->arch.priv_stack_start +
					      CONFIG_PRIVILEGED_STACK_SIZE);
		uframe->user_sp = (uint32_t)stack_ptr;
		return &uframe->iframe;
	}
#endif
	return Z_STACK_PTR_TO_FRAME(struct init_stack_frame, stack_ptr);
}

/*
 * Pre-populate values in the registers inside _callee_saved_stack struct
 * so these registers have pre-defined values when new thread begins
 * execution. For example, setting up the thread pointer for thread local
 * storage here so the thread starts with thread pointer already set up.
 */
static inline void arch_setup_callee_saved_regs(struct k_thread *thread,
						uintptr_t stack_ptr)
{
	_callee_saved_stack_t *regs = UINT_TO_POINTER(stack_ptr);

	ARG_UNUSED(regs);

#ifdef CONFIG_THREAD_LOCAL_STORAGE
	/* R26 is used for thread pointer */
	regs->r26 = thread->tls;
#endif
}

/*
 * The initial context is a basic stack frame that contains arguments for
 * z_thread_entry() return address, that points at z_thread_entry()
 * and status register.
 */
void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	struct init_stack_frame *iframe;

	setup_stack_vars(thread);

	/* Set up initial stack frame */
	iframe = get_iframe(thread, stack_ptr);

#ifdef CONFIG_USERSPACE
	/* enable US bit, US is read as zero in user mode. This will allow user
	 * mode sleep instructions, and it enables a form of denial-of-service
	 * attack by putting the processor in sleep mode, but since interrupt
	 * level/mask can't be set from user space that's not worse than
	 * executing a loop without yielding.
	 */
	iframe->status32 = _ARC_V2_STATUS32_US;
	if (is_user(thread)) {
		iframe->pc = (uint32_t)z_user_thread_entry_wrapper;
	} else {
		iframe->pc = (uint32_t)z_thread_entry_wrapper;
	}
#else
	iframe->status32 = 0;
	iframe->pc = ((uint32_t)z_thread_entry_wrapper);
#endif /* CONFIG_USERSPACE */
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	iframe->sec_stat = z_arc_v2_aux_reg_read(_ARC_V2_SEC_STAT);
#endif
	iframe->r0 = (uint32_t)entry;
	iframe->r1 = (uint32_t)p1;
	iframe->r2 = (uint32_t)p2;
	iframe->r3 = (uint32_t)p3;

#ifdef CONFIG_ARC_STACK_CHECKING
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	iframe->sec_stat |= _ARC_V2_SEC_STAT_SSC;
#else
	iframe->status32 |= _ARC_V2_STATUS32_SC;
#endif /* CONFIG_ARC_SECURE_FIRMWARE */
#endif /* CONFIG_ARC_STACK_CHECKING */
#ifdef CONFIG_ARC_USE_UNALIGNED_MEM_ACCESS
	iframe->status32 |= _ARC_V2_STATUS32_AD;
#endif
	/* Set required thread members */
	thread->switch_handle = thread;
	thread->arch.relinquish_cause = _CAUSE_COOP;
	thread->callee_saved.sp =
		(uint32_t)iframe - ___callee_saved_stack_t_SIZEOF;

	arch_setup_callee_saved_regs(thread, thread->callee_saved.sp);

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
	setup_stack_vars(_current);

	/* possible optimizaiton: no need to load mem domain anymore */
	/* need to lock cpu here ? */
	configure_mpu_thread(_current);

	z_arc_userspace_enter(user_entry, p1, p2, p3,
			      (uint32_t)_current->stack_info.start,
			      (_current->stack_info.size -
			       _current->stack_info.delta), _current);
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
