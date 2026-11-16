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
 * Stack growth (down)
 *
 *   Stack object                         priv_stack
 *   +----------------------+             +----------------------+
 *   | usable stack         |             | usable stack         |
 *   +----------------------+             +----------------------+
 *   FRAMELIMIT: stack_info.start         FRAMELIMIT: priv_stack + headroom
 *   | event headroom       |             | event headroom       |
 *   +----------------------+             +----------------------+
 *   stack buffer base                   priv_stack
 *
 * Headroom fits one unchecked EVENT_ENTRY frame. allocframe checks normal
 * growth, including trap0 nesting, before it crosses FRAMELIMIT.
 */
#define HEXAGON_STACK_LIMIT_HEADROOM (sizeof(struct event_context) + 8U)

BUILD_ASSERT(K_KERNEL_STACK_RESERVED >= HEXAGON_STACK_LIMIT_HEADROOM &&
	     K_THREAD_STACK_RESERVED >= HEXAGON_STACK_LIMIT_HEADROOM,
	     "stack objects must reserve room for the stack-limit event frame");
BUILD_ASSERT(HEXAGON_THREAD_FLAG_STACK_PROT == 0x04,
	     "arch/hexagon/core/switch.S hardcodes this bit position "
	     "(tstbit #2) -- update both if this value changes");

void z_arch_stack_protection_setup(struct k_thread *thread)
{
	if (thread == NULL) {
		return;
	}

	/* FRAMELIMIT is loaded when the thread is switched in. */
	thread->arch.framelimit = thread->stack_info.start;
	thread->arch.flags |= HEXAGON_THREAD_FLAG_STACK_PROT;
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
