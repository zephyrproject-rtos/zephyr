/* microk.c - VxMicro microkernel context implementation */

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
This module provides the microkernel context implementation.
*/

/*
 * Wrap the entire file in an #ifdef CONFIG_MICROKERNEL since it's located
 * in a nanokernel build directory.  This will ensure a zero sized object
 * module for a nanokernel only build.
 */

#ifdef CONFIG_MICROKERNEL

#include <toolchain.h>
#include <sections.h>
#include <minik.h>
#include <nanok.h>
#include <kevent.h> /* K_sigevent */
#include <microkernel.h>
#include <microkernel/entries.h> /* kernelfunc */
#include <nanokernel.h>
#include <misc/__assert.h>
#include <drivers/system_timer.h>

extern const kernelfunc _k_server_dispatch_table[];

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
	int K_PrioListIdx;

	ARG_UNUSED(parameter1);
	ARG_UNUSED(parameter2);

	/* indicate that failure of this fiber may be fatal to the entire system
	 */

	_NanoKernel.current->flags |= ESSENTIAL;

	while (1) { /* forever */
		pArgs = (struct k_args *)nano_fiber_stack_pop_wait(
			&K_Args); /* will schedule */
		do {
			kevent_t event;
			/* if event < K_max_eventnr, it's a well-known event */
			event = (kevent_t)(pArgs);
			if (event < (kevent_t)K_max_eventnr) {
#ifdef CONFIG_TASK_MONITOR
				if (K_monitor_mask & MON_EVENT) {
					K_monitor_args(pArgs);
				}
#endif
				K_sigevent(event);
			} else {
#ifdef CONFIG_TASK_MONITOR
				if (K_monitor_mask & MON_KSERV) {
					K_monitor_args(pArgs);
				}
#endif
				_k_server_dispatch_table[pArgs->Comm](pArgs);
			}

			/* check if another fiber (of equal or greater priority)
			 * needs to run */

			if (_NanoKernel.fiber) {
				fiber_yield();
			}
		} while (nano_fiber_stack_pop(&K_Args, (void *)&pArgs));

		/*
		 * Always check higher priorities (lower numbers) first.
		 * Check the low bitmask is not 0. Some built in bit scanning
		 * functions have the result undefined if the argument is 0.
		 */
		if (K_PrioBitMap[0])
			K_PrioListIdx = find_first_set_inline(K_PrioBitMap[0]) - 1;
		else
			K_PrioListIdx = find_first_set_inline(K_PrioBitMap[1]) + 31;
		/*
		 * There is no need to check whether K_PrioListIx is 0 since
		 * it's guaranteed that there will always be at least one task
		 * in the ready state (idle task)
		 */
		pNextTask = _k_task_priority_list[K_PrioListIdx].Head;

		if (K_Task != pNextTask) {
/*
 * Need to swap the low priority task,
 * the task was saved on kernel_entry
 */
#ifndef CONFIG_POWERSAVEOFF
#ifdef CONFIG_WORKLOAD_MONITOR
			/*
			 * Workload variable update in case of
			 * power save mode
			 */
			extern volatile unsigned int Wld_i;
			extern volatile unsigned int Wld_i0;
			extern volatile unsigned int WldTDelta;
			extern volatile unsigned int WldT_start;
			extern volatile unsigned int WldT_end;

			if (pNextTask->Ident == 0x00000000) {
				WldT_start = timer_read();
			}
			if (K_Task->Ident == 0x00000000) {
				WldT_end = timer_read();
				Wld_i += (Wld_i0 * (WldT_end - WldT_start)) /
					 WldTDelta;
			}
#endif
#endif
			K_Task = pNextTask;
			_NanoKernel.task = (tCCS *)pNextTask->workspace;

#ifdef CONFIG_TASK_MONITOR
			if (K_monitor_mask & MON_TSWAP) {
				K_monitor_task(K_Task, 0);
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

#ifndef CONFIG_ARCH_HAS_TASK_ABORT
/*******************************************************************************
*
* _TaskAbort - microkernel handler for fatal task errors
*
* To be invoked when a task aborts implicitly, either by returning from its
* entry point or due to a software or hardware fault.
*
* RETURNS: does not return
*
* \NOMANUAL
*/

FUNC_NORETURN void _TaskAbort(void)
{
	_task_ioctl(K_Task->Ident, TASK_ABORT);

	/*
	 * Compiler can't tell that _task_ioctl() won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
#endif

#endif /* CONFIG_MICROKERNEL */
