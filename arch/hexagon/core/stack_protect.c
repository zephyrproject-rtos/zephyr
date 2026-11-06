/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Hexagon hardware stack protection via FRAMELIMIT (c16)
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <hexagon_arch.h>
#include <kernel_internal.h>
#include <zephyr/arch/hexagon/thread.h>
#include <event_context.h>

#ifdef CONFIG_HW_STACK_PROTECTION

/*
 * Room below FRAMELIMIT for the event frame EVENT_ENTRY pushes for the
 * stack-limit exception itself: the context plus the saved r31:30 pair.
 * Stack objects reserve it below stack_info.start; priv_stack, a plain
 * array, gives it up from its own bottom.
 */
#define HEXAGON_STACK_LIMIT_HEADROOM (sizeof(struct event_context) + 8U)

BUILD_ASSERT(K_KERNEL_STACK_RESERVED >= HEXAGON_STACK_LIMIT_HEADROOM &&
	     K_THREAD_STACK_RESERVED >= HEXAGON_STACK_LIMIT_HEADROOM,
	     "stack objects must reserve room for the stack-limit event frame");

void z_arch_stack_protection_setup(struct k_thread *thread)
{
	if (thread == NULL) {
		return;
	}

	/* FRAMELIMIT is loaded when the thread is switched in. */
	thread->arch.framelimit = thread->stack_info.start;
	thread->arch.flags |= HEXAGON_THREAD_FLAG_STACK_PROT;
}

/*
 * Called from arch_switch() after the stack has been swapped and _current
 * points to the new thread, to update FRAMELIMIT for it.
 */
void z_arch_stack_protection_switch(struct k_thread *old_thread,
				    struct k_thread *new_thread)
{
	ARG_UNUSED(old_thread);

	if (new_thread != NULL &&
	    (new_thread->arch.flags & HEXAGON_THREAD_FLAG_STACK_PROT)) {
		hexagon_set_framelimit(new_thread->arch.framelimit);
	} else {
		hexagon_set_framelimit(0);
	}
}

/* Called from z_hexagon_thread_start for new threads bypassing arch_switch(). */
void z_hexagon_thread_start_stack_protect(struct k_thread *thread)
{
	z_arch_stack_protection_switch(NULL, thread);
}

/* Called from EVENT_EXIT after a preemptive context switch. */
void z_hexagon_event_exit_stack_protect(void)
{
	z_arch_stack_protection_switch(NULL, _current);
}

#ifdef CONFIG_USERSPACE
/*
 * Called from arch_user_mode_enter() with interrupts locked, right before
 * the drop to user mode. From here on the thread's guest-mode code runs
 * on priv_stack, which need not lie above its user stack, so each mode
 * gets its own limit: EVENT_ENTRY loads framelimit on an event taken
 * from user mode and EVENT_EXIT loads user_framelimit on the way back.
 */
void z_hexagon_stack_protect_user_enter(struct k_thread *thread)
{
	if ((thread->arch.flags & HEXAGON_THREAD_FLAG_STACK_PROT) == 0U) {
		return;
	}

	thread->arch.user_framelimit = thread->stack_info.start;
	thread->arch.framelimit = (uintptr_t)thread->arch.priv_stack + HEXAGON_STACK_LIMIT_HEADROOM;
	hexagon_set_framelimit(thread->arch.user_framelimit);
}
#endif

#endif /* CONFIG_HW_STACK_PROTECTION */
