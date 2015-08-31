/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief Nanokernel fiber support primitives
 *
 * This module provides various nanokernel fiber related primitives,
 * either in the form of an actual function or an alias to a function.
 */

#include <nano_private.h>
#include <nano_internal.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

/**
 *
 * @brief Add a fiber to the list of runnable fibers
 *
 * The list of runnable fibers is maintained via a single linked list
 * in priority order. Numerically lower priorities represent higher priority
 * fibers.
 *
 * Interrupts must already be locked to ensure list cannot change
 * while this routine is executing!
 *
 * @return N/A
 */
void _nano_fiber_schedule(struct tcs *tcs)
{
	struct tcs *pQ = (struct tcs *)&_nanokernel.fiber;

	/*
	 * Search until end of list or until a fiber with numerically
	 * higher priority is located.
	 */

	while (pQ->link && (tcs->prio >= pQ->link->prio)) {
		pQ = pQ->link;
	}

	/* Insert fiber, following any equal priority fibers */

	tcs->link = pQ->link;
	pQ->link = tcs;
}


/* currently the fiber and task implementations are identical */

FUNC_ALIAS(_fiber_start, fiber_fiber_start, void);
FUNC_ALIAS(_fiber_start, task_fiber_start, void);
FUNC_ALIAS(_fiber_start, fiber_start, void);

void _fiber_start(char *pStack,
		unsigned stackSize, /* stack size in bytes */
		nano_fiber_entry_t pEntry,
		int parameter1,
		int parameter2,
		unsigned priority,
		unsigned options)
{
	struct tcs *tcs;
	unsigned int imask;

	tcs = (struct tcs *) pStack;
	_new_thread(pStack,
			stackSize,
			(_thread_entry_t)pEntry,
			(void *)parameter1,
			(void *)parameter2,
			(void *)0,
			priority,
			options);

	/* _new_thread() has already set the flags depending on the 'options'
	 * and 'priority' parameters passed to it */

	/* lock interrupts to prevent corruption of the runnable fiber list */

	imask = irq_lock();

	/* make the newly crafted TCS a runnable fiber */

	_nano_fiber_schedule(tcs);

	/*
	 * Simply return to the caller if the current thread is FIBER,
	 * otherwise swap into the newly created fiber
	 */

	if ((_nanokernel.current->flags & TASK) == TASK) {
		_Swap(imask);
	} else {
		irq_unlock(imask);
	}
}

void fiber_yield(void)
{
	unsigned int imask = irq_lock();

	if ((_nanokernel.fiber != (struct tcs *)NULL) &&
	    (_nanokernel.current->prio >= _nanokernel.fiber->prio)) {
		/*
		 * Reinsert current thread into the list of runnable threads,
		 * and
		 * then swap to the thread at the head of the fiber list.
		 */

		_nano_fiber_schedule(_nanokernel.current);
		_Swap(imask);
	} else {
		irq_unlock(imask);
	}
}

/**
 *
 * @brief Pass control from the currently executing fiber
 *
 * This routine is used when a fiber voluntarily gives up control of the CPU.
 *
 * This routine can only be called from a fiber.
 *
 * @return This function never returns
 */
FUNC_NORETURN void _nano_fiber_swap(void)
{
	unsigned int imask;

	/*
	 * Since the currently running fiber is not queued onto the runnable
	 * fiber list, simply performing a _Swap() shall initiate a context
	 * switch to the highest priority fiber, or the highest priority task
	 * if there are no runnable fibers.
	 */

	imask = irq_lock();
	_Swap(imask);

	/*
	 * Compiler can't know that _Swap() won't return and will issue a
	 * warning
	 * unless we explicitly tell it that control never gets this far.
	 */

	CODE_UNREACHABLE;
}

#ifndef CONFIG_ARCH_HAS_NANO_FIBER_ABORT
FUNC_NORETURN void fiber_abort(void)
{
	/* Do normal thread exit cleanup, then give up CPU control */

	_thread_exit(_nanokernel.current);
	_nano_fiber_swap();
}
#endif

#ifdef CONFIG_NANO_TIMEOUTS

#include <wait_q.h>

void fiber_sleep(int32_t timeout_in_ticks)
{
	int key;

	if (TICKS_NONE == timeout_in_ticks) {
		fiber_yield();
		return;
	}

	key = irq_lock();
	_nano_timeout_add(_nanokernel.current, NULL, timeout_in_ticks);
	_Swap(key);
}

FUNC_ALIAS(fiber_delayed_start, fiber_fiber_delayed_start, void *);
FUNC_ALIAS(fiber_delayed_start, task_fiber_delayed_start, void *);

void *fiber_delayed_start(char *stack, unsigned int stack_size_in_bytes,
							nano_fiber_entry_t entry_point, int param1,
							int param2, unsigned int priority,
							unsigned int options, int32_t timeout_in_ticks)
{
	unsigned int key;
	struct tcs *tcs;

	tcs = (struct tcs *)stack;
	_new_thread(stack, stack_size_in_bytes, (_thread_entry_t)entry_point,
				(void *)param1, (void *)param2, (void *)0, priority, options);

	key = irq_lock();

	_nano_timeout_add(tcs, NULL, timeout_in_ticks);

	irq_unlock(key);
	return tcs;
}

FUNC_ALIAS(fiber_delayed_start_cancel, fiber_fiber_delayed_start_cancel, void);
FUNC_ALIAS(fiber_delayed_start_cancel, task_fiber_delayed_start_cancel, void);

void fiber_delayed_start_cancel(void *handle)
{
	struct tcs *cancelled_tcs = (struct tcs *)handle;
	int key = irq_lock();

	_nano_timeout_abort(cancelled_tcs);
	_thread_exit(cancelled_tcs);

	irq_unlock(key);
}

#endif /* CONFIG_NANO_TIMEOUTS */
