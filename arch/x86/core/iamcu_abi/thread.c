/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef CONFIG_MICROKERNEL
#include <microkernel.h>
#include <micro_private_types.h>
#endif /* CONFIG_MICROKERNEL */
#ifdef CONFIG_INIT_STACKS
#include <string.h>
#endif /* CONFIG_INIT_STACKS */

#include <toolchain.h>
#include <sections.h>
#include <nano_private.h>
#include <wait_q.h>

/* the one and only nanokernel control structure */

tNANO _nanokernel = {0};

/* forward declaration to asm function to adjust setup the arguments
 * to _thread_entry()
 */
void _thread_entry_wrapper(_thread_entry_t, _thread_arg_t,
			   _thread_arg_t, _thread_arg_t);

/**
 *
 * @brief Create a new kernel execution context
 *
 * This function initializes a thread control structure (TCS) for a
 * new kernel execution context. A fake stack frame is created as if
 * the context had been "swapped out" via _Swap()
 *
 * @param stack_memory pointer to the context stack area
 * @param stack_size size of contexts stack area
 * @param thread_func new contexts entry function
 * @param parameter1 first entry function parameter
 * @param parameter2 second entry function parameter
 * @param parameter3 third entry function parameter
 * @param priority Priority of the new context
 * @param options Additional options for the context
 *
 * @return none
 *
 * \NOMANUAL
 */

void _new_thread(char *stack_memory, unsigned stack_size,
		 _thread_entry_t thread_func, void *parameter1,
		 void *parameter2, void *parameter3, int priority,
		 unsigned options)
{
	unsigned long *thread_context;
	struct tcs *tcs = (struct tcs *)stack_memory;

	ARG_UNUSED(options);

#ifdef CONFIG_INIT_STACKS
	memset(stack_memory, 0xaa, stack_size);
#endif

	tcs->link = (struct tcs *)NULL; /* thread not inserted into list yet */
	tcs->prio = priority;

	if (priority == -1) {
		tcs->flags = PREEMPTIBLE | TASK;
	} else {
		tcs->flags = FIBER;
	}

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */

	tcs->custom_data = NULL;
#endif

	/* carve the thread entry struct from the "base" of the stack */

	thread_context =
		(unsigned long *)STACK_ROUND_DOWN(stack_memory + stack_size);

	/*
	 * Create an initial context on the stack expected by the _Swap()
	 * primitive.
	 * Given that both task and fibers execute at privilege 0, the
	 * setup for both threads are equivalent.
	 */

	/* push arguments required by _thread_entry() */

	*--thread_context = (unsigned long)parameter3;
	*--thread_context = (unsigned long)parameter2;
	*--thread_context = (unsigned long)parameter1;
	*--thread_context = (unsigned long)thread_func;

	/* push initial EFLAGS; only modify IF and IOPL bits */

	*--thread_context = (unsigned long)_thread_entry_wrapper;
	*--thread_context = (EflagsGet() & ~EFLAGS_MASK) | EFLAGS_INITIAL;
	*--thread_context = 0;
	*--thread_context = 0;
	*--thread_context = 0;
	--thread_context;
	*thread_context = (unsigned long)(thread_context + 4);
	*--thread_context = 0;

	tcs->coopReg.esp = (unsigned long)thread_context;
#if defined(CONFIG_THREAD_MONITOR)
	{
		unsigned int imask;

		/*
		 * Add the newly initialized thread to head of the
		 * list of threads. This singly linked list of threads
		 * maintains ALL the threads in the system: both tasks
		 * and fibers regardless of whether they are runnable.
		 */

		imask = irq_lock();
		tcs->next_thread = _nanokernel.threads;
		_nanokernel.threads = tcs;
		irq_unlock(imask);
	}
#endif /* CONFIG_THREAD_MONITOR */

	_nano_timeout_tcs_init(tcs);
}
