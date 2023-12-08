/*
 * Copyright (c) 2017, 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <kernel_internal.h>

#include <xtensa_asm2_context.h>
#include <xtensa_internal.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

#ifdef CONFIG_USERSPACE

#ifdef CONFIG_THREAD_LOCAL_STORAGE
/*
 * Per-thread (TLS) variable indicating whether execution is in user mode.
 */
__thread uint32_t is_user_mode;
#endif

#endif /* CONFIG_USERSPACE */

/**
 * Initializes a stack area such that it can be "restored" later and
 * begin running with the specified function and three arguments.  The
 * entry function takes three arguments to match the signature of
 * Zephyr's k_thread_entry_t.  Thread will start with EXCM clear and
 * INTLEVEL set to zero (i.e. it's a user thread, we don't start with
 * anything masked, so don't assume that!).
 */
static void *init_stack(struct k_thread *thread, int *stack_top,
			void (*entry)(void *, void *, void *),
			void *arg1, void *arg2, void *arg3)
{
	void *ret;
	_xtensa_irq_stack_frame_a11_t *frame;
#ifdef CONFIG_USERSPACE
	struct xtensa_thread_stack_header *header =
		(struct xtensa_thread_stack_header *)thread->stack_obj;

	thread->arch.psp = header->privilege_stack +
		sizeof(header->privilege_stack);
#endif

	/* Not-a-cpu ID Ensures that the first time this is run, the
	 * stack will be invalidated.  That covers the edge case of
	 * restarting a thread on a stack that had previously been run
	 * on one CPU, but then initialized on this one, and
	 * potentially run THERE and not HERE.
	 */
	thread->arch.last_cpu = -1;

	/* We cheat and shave 16 bytes off, the top four words are the
	 * A0-A3 spill area for the caller of the entry function,
	 * which doesn't exist.  It will never be touched, so we
	 * arrange to enter the function with a CALLINC of 1 and a
	 * stack pointer 16 bytes above the top, so its ENTRY at the
	 * start will decrement the stack pointer by 16.
	 */
	const int bsasz = sizeof(*frame) - 16;

	frame = (void *)(((char *) stack_top) - bsasz);

	(void)memset(frame, 0, bsasz);

	frame->bsa.ps = PS_WOE | PS_UM | PS_CALLINC(1);
#ifdef CONFIG_USERSPACE
	if ((thread->base.user_options & K_USER) == K_USER) {
		frame->bsa.pc = (uintptr_t)arch_user_mode_enter;
	} else {
		frame->bsa.pc = (uintptr_t)z_thread_entry;
	}
#else
	frame->bsa.pc = (uintptr_t)z_thread_entry;
#endif

#if XCHAL_HAVE_THREADPTR
#ifdef CONFIG_THREAD_LOCAL_STORAGE
	frame->bsa.threadptr = thread->tls;
#elif CONFIG_USERSPACE
	frame->bsa.threadptr = (uintptr_t)((thread->base.user_options & K_USER) ? thread : NULL);
#endif
#endif

	/* Arguments to z_thread_entry().  Remember these start at A6,
	 * which will be rotated into A2 by the ENTRY instruction that
	 * begins the C function.  And A4-A7 and A8-A11 are optional
	 * quads that live below the BSA!
	 */
	frame->a7 = (uintptr_t)arg1;  /* a7 */
	frame->a6 = (uintptr_t)entry; /* a6 */
	frame->a5 = 0;                /* a5 */
	frame->a4 = 0;                /* a4 */

	frame->a11 = 0;                /* a11 */
	frame->a10 = 0;                /* a10 */
	frame->a9  = (uintptr_t)arg3;  /* a9 */
	frame->a8  = (uintptr_t)arg2;  /* a8 */

	/* Finally push the BSA pointer and return the stack pointer
	 * as the handle
	 */
	frame->ptr_to_bsa = (void *)&frame->bsa;
	ret = &frame->ptr_to_bsa;

	return ret;
}

void arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		     char *stack_ptr, k_thread_entry_t entry,
		     void *p1, void *p2, void *p3)
{
	thread->switch_handle = init_stack(thread, (int *)stack_ptr, entry,
					   p1, p2, p3);
#ifdef CONFIG_KERNEL_COHERENCE
	__ASSERT((((size_t)stack) % XCHAL_DCACHE_LINESIZE) == 0, "");
	__ASSERT((((size_t)stack_ptr) % XCHAL_DCACHE_LINESIZE) == 0, "");
	sys_cache_data_flush_and_invd_range(stack, (char *)stack_ptr - (char *)stack);
#endif
}

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
int arch_float_disable(struct k_thread *thread)
{
	/* xtensa always has FPU enabled so cannot be disabled */
	return -ENOTSUP;
}

int arch_float_enable(struct k_thread *thread, unsigned int options)
{
	/* xtensa always has FPU enabled so nothing to do here */
	return 0;
}
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

#ifdef CONFIG_USERSPACE
FUNC_NORETURN void arch_user_mode_enter(k_thread_entry_t user_entry,
					void *p1, void *p2, void *p3)
{
	struct k_thread *current = _current;
	size_t stack_end;

	/* Transition will reset stack pointer to initial, discarding
	 * any old context since this is a one-way operation
	 */
	stack_end = Z_STACK_PTR_ALIGN(current->stack_info.start +
				      current->stack_info.size -
				      current->stack_info.delta);

	xtensa_userspace_enter(user_entry, p1, p2, p3,
			       stack_end, current->stack_info.start);

	CODE_UNREACHABLE;
}
#endif /* CONFIG_USERSPACE */
