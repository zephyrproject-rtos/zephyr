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
 * must call z_swap() (which triggers a service call), but when in handler
 * mode, the CPU must exit handler mode to cause the context switch, and thus
 * must queue the PendSV exception.
 */

#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <ksched.h>
#include <kswap.h>
#include <wait_q.h>
#include <sys/__assert.h>

void z_impl_k_thread_abort(k_tid_t thread)
{
	z_thread_single_abort(thread);

	if (_current == thread) {
		if (arch_is_in_isr()) {
			/* ARM is unlike most arches in that this is true
			 * even for non-peripheral interrupts, even though
			 * for these types of faults there is not an implicit
			 * reschedule on the way out. See #21923.
			 *
			 * We have to reschedule since the current thread
			 * should no longer run after we return, so
			 * Trigger PendSV, in case we are in one of the
			 * situations where the isr check is true but there
			 * is not an implicit scheduler invocation.
			 */
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		} else {
			z_swap_unlocked();
		}
	}

	/* The abort handler might have altered the ready queue. */
	z_reschedule_unlocked();
}
