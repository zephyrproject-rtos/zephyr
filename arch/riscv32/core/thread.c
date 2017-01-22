/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
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
#define thread_monitor_init(thread)		\
	do {/* do nothing */			\
	} while ((0))
#endif /* CONFIG_THREAD_MONITOR */

void _thread_entry_wrapper(_thread_entry_t thread,
			   void *arg1,
			   void *arg2,
			   void *arg3);

void _new_thread(char *stack_memory, size_t stack_size,
		 _thread_entry_t thread_func,
		 void *arg1, void *arg2, void *arg3,
		 int priority, unsigned options)
{
	_ASSERT_VALID_PRIO(priority, thread_func);

	struct k_thread *thread;
	struct __esf *stack_init;

#ifdef CONFIG_INIT_STACKS
	memset(stack_memory, 0xaa, stack_size);
#endif
	/* Initial stack frame for thread */
	stack_init = (struct __esf *)
		STACK_ROUND_DOWN(stack_memory +
				 stack_size - sizeof(struct __esf));

	/* Setup the initial stack frame */
	stack_init->a0 = (uint32_t)thread_func;
	stack_init->a1 = (uint32_t)arg1;
	stack_init->a2 = (uint32_t)arg2;
	stack_init->a3 = (uint32_t)arg3;
	/*
	 * Following the RISC-V architecture,
	 * the MSTATUS register (used to globally enable/disable interrupt),
	 * as well as the MEPC register (used to by the core to save the
	 * value of the program counter at which an interrupt/exception occcurs)
	 * need to be saved on the stack, upon an interrupt/exception
	 * and restored prior to returning from the interrupt/exception.
	 * This shall allow to handle nested interrupts.
	 *
	 * Given that context switching is performed via a system call exception
	 * within the RISCV32 architecture implementation, initially set:
	 * 1) MSTATUS to SOC_MSTATUS_DEF_RESTORE in the thread stack to enable
	 *    interrupts when the newly created thread will be scheduled;
	 * 2) MEPC to the address of the _thread_entry_wrapper in the thread
	 *    stack.
	 * Hence, when going out of an interrupt/exception/context-switch,
	 * after scheduling the newly created thread:
	 * 1) interrupts will be enabled, as the MSTATUS register will be
	 *    restored following the MSTATUS value set within the thread stack;
	 * 2) the core will jump to _thread_entry_wrapper, as the program
	 *    counter will be restored following the MEPC value set within the
	 *    thread stack.
	 */
	stack_init->mstatus = SOC_MSTATUS_DEF_RESTORE;
	stack_init->mepc = (uint32_t)_thread_entry_wrapper;

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

	thread->callee_saved.sp = (uint32_t)stack_init;

	thread_monitor_init(thread);
}
