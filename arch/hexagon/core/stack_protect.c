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

#ifdef CONFIG_HW_STACK_PROTECTION

void z_arch_stack_protection_setup(struct k_thread *thread)
{
	if (thread == NULL) {
		return;
	}

	uintptr_t stack_limit = thread->stack_info.start;

	thread->arch.framelimit = stack_limit;
	hexagon_set_framelimit(stack_limit);
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

#endif /* CONFIG_HW_STACK_PROTECTION */
