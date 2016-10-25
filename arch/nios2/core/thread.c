/*
 * Copyright (c) 2016 Intel Corporation
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

#include <nanokernel.h>
#include <nano_private.h>
#include <wait_q.h>
#include <string.h>

tNANO _nanokernel = {0};

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Add a thread to the kernel's list of active threads.
 */
static ALWAYS_INLINE void thread_monitor_init(struct tcs *tcs)
{
	unsigned int key;

	key = irq_lock();
	tcs->next_thread = _nanokernel.threads;
	_nanokernel.threads = tcs;
	irq_unlock(key);
}
#else
#define thread_monitor_init(tcs) \
	do {/* do nothing */     \
	} while ((0))
#endif /* CONFIG_THREAD_MONITOR */

/* forward declaration to asm function to adjust setup the arguments
 * to _thread_entry() since this arch puts the first four arguments
 * in r4-r7 and not on the stack
 */
void _thread_entry_wrapper(_thread_entry_t, _thread_arg_t,
			   _thread_arg_t, _thread_arg_t);


struct init_stack_frame {
	/* top of the stack / most recently pushed */

	/* Used by _thread_entry_wrapper. pulls these off the stack and
	 * into argument registers before calling _thread_entry()
	 */
	_thread_entry_t entry_point;
	_thread_arg_t arg1;
	_thread_arg_t arg2;
	_thread_arg_t arg3;

	/* least recently pushed */
};


void _new_thread(char *stack_memory, unsigned stack_size,
		 void *uk_task_ptr, _thread_entry_t thread_func,
		 _thread_arg_t arg1, _thread_arg_t arg2,
		 _thread_arg_t arg3,
		 int priority, unsigned options)
{
	struct tcs *tcs;
	struct init_stack_frame *iframe;

#ifdef CONFIG_INIT_STACKS
	memset(stack_memory, 0xaa, stack_size);
#endif
	/* Initial stack frame data, stored at the base of the stack */
	iframe = (struct init_stack_frame *)
		STACK_ROUND_DOWN(stack_memory + stack_size - sizeof(*iframe));

	/* Setup the initial stack frame */
	iframe->entry_point = thread_func;
	iframe->arg1 = arg1;
	iframe->arg2 = arg2;
	iframe->arg3 = arg3;

	/* Initialize various struct tcs members */
	tcs = (struct tcs *)stack_memory;

	tcs->link = (struct tcs *)NULL;
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

#ifdef CONFIG_MICROKERNEL
	tcs->uk_task_ptr = uk_task_ptr;
#else
	ARG_UNUSED(uk_task_ptr);
#endif
	tcs->coopReg.sp = (uint32_t)iframe;
	tcs->coopReg.ra = (uint32_t)_thread_entry_wrapper;
	tcs->coopReg.key = NIOS2_STATUS_PIE_MSK;
	/* Leave the rest of tcs->coopReg junk */

#ifdef CONFIG_NANO_TIMEOUTS
	_nano_timeout_tcs_init(tcs);
#endif

	thread_monitor_init(tcs);
}
