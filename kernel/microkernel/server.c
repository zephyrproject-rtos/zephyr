/* server.c - microkernel server */

/*
 * Copyright (c) 2010, 2012-2015 Wind River Systems, Inc.
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

/*
DESCRIPTION
This module implements the microkernel server, which processes service requests
from tasks (and, less commonly, fibers and ISRs). The requests are service by
a high priority fiber, thereby ensuring that requests are processed in a timely
manner and in a single threaded manner that prevents simultaneous requests from
interfering with each other.
*/

#include <toolchain.h>
#include <sections.h>
#include <minik.h>
#include <nanok.h>
#include <kevent.h>
#include <microkernel.h>
#include <microkernel/entries.h> /* kernelfunc */
#include <nanokernel.h>
#include <misc/__assert.h>
#include <drivers/system_timer.h>

extern const kernelfunc _k_server_dispatch_table[];

/*******************************************************************************
*
* next_task_select - select task to be executed by microkernel
*
* Locates that highest priority task queue that is non-empty and chooses the
* task at the head of that queue. It's guaranteed that there will always be
* a non-empty queue, since the idle task is always executable.
*
* RETURNS: pointer to selected task
*/

static struct k_proc *next_task_select(void)
{
	int K_PrioListIdx;

#if (CONFIG_NUM_TASK_PRIORITIES <= 32)
	K_PrioListIdx = find_first_set_inline(_k_task_priority_bitmap[0]) - 1;
#else
	int bit_map;
	int set_bit_pos;

	K_PrioListIdx = -1;
	for (bit_map = 0; ; bit_map++) {
		set_bit_pos = find_first_set_inline(_k_task_priority_bitmap[bit_map]);
		if (set_bit_pos) {
			K_PrioListIdx += set_bit_pos;
			break;
		}
		K_PrioListIdx += 32;
	}
#endif

	return _k_task_priority_list[K_PrioListIdx].Head;
}

/*******************************************************************************
*
* K_swapper - the microkernel thread entry point
*
* This function implements the microkernel fiber.  It waits for command
* packets to arrive on its stack channel. It executes all commands on the
* stack and then sets up the next task that is ready to run. Next it
* goes to wait on further inputs on its stack channel.
*
* RETURNS: Does not return.
*/

FUNC_NORETURN void K_swapper(int parameter1, /* not used */
					   int parameter2  /* not used */
					   )
{
	struct k_args *pArgs;
	struct k_proc *pNextTask;

	ARG_UNUSED(parameter1);
	ARG_UNUSED(parameter2);

	/* indicate that failure of this fiber may be fatal to the entire system
	 */

	_NanoKernel.current->flags |= ESSENTIAL;

	while (1) { /* forever */
		pArgs = (struct k_args *)nano_fiber_stack_pop_wait(
			&_k_command_stack); /* will schedule */
		do {
			kevent_t event;
			/* if event < _k_num_events, it's a well-known event */
			event = (kevent_t)(pArgs);
			if (event < (kevent_t)_k_num_events) {
#ifdef CONFIG_TASK_MONITOR
				if (_k_monitor_mask & MON_EVENT) {
					_k_task_monitor_args(pArgs);
				}
#endif
				_k_do_event_signal(event);
			} else {
#ifdef CONFIG_TASK_MONITOR
				if (_k_monitor_mask & MON_KSERV) {
					_k_task_monitor_args(pArgs);
				}
#endif
				_k_server_dispatch_table[pArgs->Comm](pArgs);
			}

			/* check if another fiber (of equal or greater priority)
			 * needs to run */

			if (_NanoKernel.fiber) {
				fiber_yield();
			}
		} while (nano_fiber_stack_pop(&_k_command_stack, (void *)&pArgs));

		pNextTask = next_task_select();

		if (_k_current_task != pNextTask) {

			/* switch from currently selected task to a different one */

#ifdef CONFIG_WORKLOAD_MONITOR
			if (pNextTask->Ident == 0x00000000) {
				_k_workload_monitor_idle_start();
			} else if (_k_current_task->Ident == 0x00000000) {
				_k_workload_monitor_idle_end();
			}
#endif

			_k_current_task = pNextTask;
			_NanoKernel.task = (tCCS *)pNextTask->workspace;

#ifdef CONFIG_TASK_MONITOR
			if (_k_monitor_mask & MON_TSWAP) {
				_k_task_monitor(_k_current_task, 0);
			}
#endif
		}
	}

	/*
	 * Code analyzers may complain that K_swapper() uses an infinite loop
	 * unless we indicate that this is intentional
	 */

	CODE_UNREACHABLE;
}

/*******************************************************************************
*
* _Cget - remove the first element from a linked list LIFO
*
* Remove the first element from the specified system-level linked list LIFO.
* If no elements are available, a context yield will occur.  Upon return from
* the context yield operation, an attempt to remove the first element from the
* LIFO will occur again.
*
* This routine will only return when an element becomes available.
*
* RETURNS: Pointer to first element in the list
*
* INTERNAL
* Apparently only the microkernel utilizes the _Cget() API.  Thus to
* prevent (very minor) code bloat in a nanokernel only system, the _Cget()
* implementation appears in microk.c instead of nano_lifo.c
*
* \NOMANUAL
*/

void *_Cget(struct nano_lifo *chan)
{
	void *element;

	element = nano_fiber_lifo_get(chan);
	__ASSERT(element != NULL,
		 "panic: depleted CMD packets from LIFO @ 0x%x\n",
		 chan);

	return element;
}
