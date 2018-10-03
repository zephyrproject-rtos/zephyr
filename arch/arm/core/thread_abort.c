/*
 * Copyright (c) 2016 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M k_thread_abort() routine
 *
 * The ARM Cortex-M architecture provides its own k_thread_abort() to deal
 * with different CPU modes (handler vs thread) when a thread aborts. When its
 * entry point returns or when it aborts itself, the CPU is in thread mode and
 * must call _Swap() (which triggers a service call), but when in handler
 * mode, the CPU must exit handler mode to cause the context switch, and thus
 * must queue the PendSV exception.
 */

#include <kernel.h>
#include <kernel_structs.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <ksched.h>
#include <kswap.h>
#include <wait_q.h>
#include <misc/__assert.h>

extern void _k_thread_single_abort(struct k_thread *thread);

void _impl_k_thread_abort(k_tid_t thread)
{
	unsigned int key;

	key = irq_lock();

	__ASSERT(!(thread->base.user_options & K_ESSENTIAL),
		 "essential thread aborted");

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	if (_current == thread) {
		if ((SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) == 0) {
			(void)_Swap(key);
			CODE_UNREACHABLE;
		} else {
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}

	/* The abort handler might have altered the ready queue. */
	_reschedule(key);
}
