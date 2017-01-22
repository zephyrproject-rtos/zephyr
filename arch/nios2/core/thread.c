/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <nano_internal.h>
#include <kernel_structs.h>
#include <wait_q.h>
#include <string.h>

#if defined(CONFIG_THREAD_MONITOR)
/*
 * Add a thread to the kernel's list of active threads.
 */
static ALWAYS_INLINE void thread_monitor_init(struct k_thread *thread)
{
	unsigned int key;

	key = irq_lock();
	thread->next_thread = _kernel.threads;
	_kernel.threads = thread;
	irq_unlock(key);
}
#else
#define thread_monitor_init(thread) \
	do {/* do nothing */     \
	} while ((0))
#endif /* CONFIG_THREAD_MONITOR */

/* forward declaration to asm function to adjust setup the arguments
 * to _thread_entry() since this arch puts the first four arguments
 * in r4-r7 and not on the stack
 */
void _thread_entry_wrapper(_thread_entry_t, void *, void *, void *);

struct init_stack_frame {
	/* top of the stack / most recently pushed */

	/* Used by _thread_entry_wrapper. pulls these off the stack and
	 * into argument registers before calling _thread_entry()
	 */
	_thread_entry_t entry_point;
	void *arg1;
	void *arg2;
	void *arg3;

	/* least recently pushed */
};


void _new_thread(char *stack_memory, size_t stack_size,
		 _thread_entry_t thread_func,
		 void *arg1, void *arg2, void *arg3,
		 int priority, unsigned options)
{
	_ASSERT_VALID_PRIO(priority, thread_func);

	struct k_thread *thread;
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

	/* Initialize various struct k_thread members */
	thread = (struct k_thread *)stack_memory;

	_init_thread_base(&thread->base, priority, _THREAD_PRESTART, options);

	/* static threads overwrite it afterwards with real value */
	thread->init_data = NULL;
	thread->fn_abort = NULL;

#ifdef CONFIG_THREAD_CUSTOM_DATA
	/* Initialize custom data field (value is opaque to kernel) */
	thread->custom_data = NULL;
#endif
	thread->callee_saved.sp = (uint32_t)iframe;
	thread->callee_saved.ra = (uint32_t)_thread_entry_wrapper;
	thread->callee_saved.key = NIOS2_STATUS_PIE_MSK;
	/* Leave the rest of thread->callee_saved junk */

	thread_monitor_init(thread);
}
