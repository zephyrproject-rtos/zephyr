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

void z_thread_entry_wrapper(k_thread_entry_t thread,
			   void *arg1,
			   void *arg2,
			   void *arg3);

void z_arch_new_thread(struct k_thread *thread, k_thread_stack_t *stack,
		       size_t stack_size, k_thread_entry_t thread_func,
		       void *arg1, void *arg2, void *arg3,
		       int priority, unsigned int options)
{
	char *stack_memory = Z_THREAD_STACK_BUFFER(stack);
	Z_ASSERT_VALID_PRIO(priority, thread_func);

	struct __esf *stack_init;

	z_new_thread_init(thread, stack_memory, stack_size, priority, options);

	/* Initial stack frame for thread */
	stack_init = (struct __esf *)
		STACK_ROUND_DOWN(stack_memory +
				 stack_size - sizeof(struct __esf));

	/* Setup the initial stack frame */
	stack_init->a0 = (ulong_t)thread_func;
	stack_init->a1 = (ulong_t)arg1;
	stack_init->a2 = (ulong_t)arg2;
	stack_init->a3 = (ulong_t)arg3;
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
	 * within the RISCV architecture implementation, initially set:
	 * 1) MSTATUS to SOC_MSTATUS_DEF_RESTORE in the thread stack to enable
	 *    interrupts when the newly created thread will be scheduled;
	 * 2) MEPC to the address of the z_thread_entry_wrapper in the thread
	 *    stack.
	 * Hence, when going out of an interrupt/exception/context-switch,
	 * after scheduling the newly created thread:
	 * 1) interrupts will be enabled, as the MSTATUS register will be
	 *    restored following the MSTATUS value set within the thread stack;
	 * 2) the core will jump to z_thread_entry_wrapper, as the program
	 *    counter will be restored following the MEPC value set within the
	 *    thread stack.
	 */
	stack_init->mstatus = SOC_MSTATUS_DEF_RESTORE;
	stack_init->mepc = (ulong_t)z_thread_entry_wrapper;

	thread->callee_saved.sp = (ulong_t)stack_init;
}
