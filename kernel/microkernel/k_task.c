/* task kernel services */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
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


#include <microkernel.h>
#include <nanokernel.h>
#include <nanokernel/cpu.h>
#include <toolchain.h>
#include <sections.h>

#include <minik.h>
#include <nanok.h>
#include <ktask.h>



/*******************************************************************************
*
* reset_state_bit - reset the specified task state bits
*
* This routine resets the specified task state bits.  When a task's state bits
* are zero, the task may be scheduled to run.  The tasks's state bits are a
* bitmask of the TF_xxx bits.  Each TF_xxx bit indicates a reason why the task
* must not be scheduled to run.
*
* RETURNS: N/A
*/

void reset_state_bit(struct k_proc *X,    /* ptr to task */
					   uint32_t bits /* bitmask of TF_xxx
							    bits to reset */
					   )
{
	uint32_t f_old = X->State;      /* old state bits */
	uint32_t f_new = f_old & ~bits; /* new state bits */

	X->State = f_new; /* Update task's state bits */

	if ((f_old != 0) && (f_new == 0)) {
		/*
		 * The task may now be scheduled to run (but could not
		 * previously) as all the TF_xxx bits are clear.  It must
		 * be added to the list of schedulable tasks.
		 */

#ifndef LITE
		struct k_tqhd *H = _k_task_priority_list + X->Prio;
#else
		struct k_tqhd *H = _k_task_priority_list;
#endif

		X->Forw = NULL;
		H->Tail->Forw = X;
		H->Tail = X;
		_k_task_priority_bitmap[X->Prio >> 5] |= (1 << (X->Prio & 0x1F));
	}

#ifdef CONFIG_TASK_MONITOR
	f_new ^= f_old;
	if ((_k_monitor_mask & MON_STATE) && (f_new)) {
		/*
		 * Task monitoring is enabled and the new state bits are
		 * different than the old state bits.
		 *
		 * <f_new> now contains the bits that are different.
		 */

		K_monitor_task(X, f_new | MO_STBIT0);
	}
#endif
}

/*******************************************************************************
*
* set_state_bit - set specified task state bits
*
* This routine sets the specified task state bits.  When a task's state bits
* are non-zero, the task will not be scheduled to run.  The task's state bits
* are a bitmask of the TF_xxx bits.  Each TF_xxx bit indicates a reason why
* the task must not be scheduled to run.
*
* RETURNS: N/A
*/

void set_state_bit(
	struct k_proc *task_ptr,
	uint32_t bits /* bitmask of TF_xxx bits to set */
	)
{
	uint32_t old_state_bits = task_ptr->State;
	uint32_t new_state_bits = old_state_bits | bits;

	task_ptr->State = new_state_bits;

	if ((old_state_bits == 0) && (new_state_bits != 0)) {
		/*
		 * The task could have been scheduled to run ([State] was 0)
		 * but can not be scheduled to run anymore at least one TF_xxx
		 * bit has been set.  Remove it from the list of schedulable
		 * tasks.
		 */
#if defined(__GNUC__)
#if defined(VXMICRO_ARCH_arm)
		/*
		 * Avoid bad code generation by certain gcc toolchains for ARM
		 * when an optimization setting of -O2 or above is used.
		 *
		 * Specifically, this issue has been seen with ARM gcc version
		 * 4.6.3 (Sourcery CodeBench Lite 2012.03-56): The 'volatile'
		 * attribute is added to the following variable to prevent it
		 * from being lost--otherwise the register that holds its value
		 * is reused, but the compiled code uses it later on as if it
		 * was still that variable.
		 */
		volatile
#endif
#endif
			struct k_tqhd *task_queue = _k_task_priority_list + task_ptr->Prio;
		struct k_proc *cur_task = (struct k_proc *)(&task_queue->Head);

		/*
		 * Search in the list for this task priority level,
		 * and remove the task.
		 */
		while (cur_task->Forw != task_ptr) {
			cur_task = cur_task->Forw;
		}

		cur_task->Forw = task_ptr->Forw;

		if (task_queue->Tail == task_ptr) {
			task_queue->Tail = cur_task;
		}

		/*
		 * If there are no more tasks of this priority that are
		 * runnable, then clear that bit in the global priority bit map.
		 */
		if (task_queue->Head == NULL) {
			_k_task_priority_bitmap[task_ptr->Prio >> 5] &= ~(1 << (task_ptr->Prio & 0x1F));
		}
	}

#ifdef CONFIG_TASK_MONITOR
	new_state_bits ^= old_state_bits;
	if ((_k_monitor_mask & MON_STATE) && (new_state_bits)) {
		/*
		 * Task monitoring is enabled and the new state bits are
		 * different than the old state bits.
		 *
		 * <new_state_bits> now contains the bits that are different.
		 */

		K_monitor_task(task_ptr, new_state_bits | MO_STBIT1);
	}
#endif
}

/*******************************************************************************
*
* abort_task - abort a task
*
* This routine aborts the specified task.
*
* RETURNS: N/A
*/

void abort_task(struct k_proc *X)
{

	/* Do normal context exit cleanup */

	_context_exit((tCCS *)X->workspace);

	/* Set TF_TERM and TF_STOP state flags */

	set_state_bit(X, TF_STOP | TF_TERM);

	/* Invoke abort function, if there is one */

	if (X->fabort != NULL) {
		X->fabort();
	}
}

/*******************************************************************************
*
* task_abort_handler_set - install an abort handler
*
* This routine installs an abort handler for the calling task.
*
* The abort handler is run when the calling task is aborted by a _TaskAbort()
* or task_group_abort() call.
*
* Each call to task_abort_handler_set() replaces the previously installed
* handler.
*
* To remove an abort handler, set the parameter to NULL as below:
*      task_abort_handler_set (NULL)
*
* RETURNS: N/A
*/

void task_abort_handler_set(void (*func)(void) /* abort handler */
			    )
{
	_k_current_task->fabort = func;
}

/*******************************************************************************
*
* K_taskop - handle a task operation request
*
* This routine handles any one of the following task operation requests:
*   starting either a kernel or user task, aborting a task, suspending a task,
*   resuming a task, blocking a task or unblocking a task
*
* RETURNS: N/A
*/

void K_taskop(struct k_args *A)
{
	ktask_t Tid = A->Args.g1.task;
		struct k_proc *X = _k_task_list + OBJ_INDEX(Tid);

		switch (A->Args.g1.opt) {
		case TASK_START:
			start_task(X, X->fstart);
			break;
		case TASK_ABORT:
			abort_task(X);
			break;
		case TASK_SUSPEND:
			set_state_bit(X, TF_SUSP);
			break;
		case TASK_RESUME:
			reset_state_bit(X, TF_SUSP);
			break;
		case TASK_BLOCK:
			set_state_bit(X, TF_BLCK);
			break;
		case TASK_UNBLOCK:
			reset_state_bit(X, TF_BLCK);
			break;
		}
}

/*******************************************************************************
*
* _task_ioctl - task operations
*
* RETURNS: N/A
*/

void _task_ioctl(ktask_t task, /* task on which to operate */
				    int opt      /* task operation */
				    )
{
	struct k_args A;

	A.Comm = TSKOP;
	A.Args.g1.task = task;
	A.Args.g1.opt = opt;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* K_groupop - handle task group operation request
*
* This routine handles any one of the following task group operations requests:
*   starting either kernel or user tasks, aborting tasks, suspending tasks,
*   resuming tasks, blocking tasks or unblocking tasks
*
* RETURNS: N/A
*/

void K_groupop(struct k_args *A)
{
	ktask_group_t grp = A->Args.g1.group;
	int opt = A->Args.g1.opt;
	int i;
	struct k_proc *X;

#ifdef CONFIG_TASK_DEBUG
	if (opt == GROUP_TASK_BLOCK)
		_k_debug_halt = 1;
	if (opt == GROUP_TASK_UNBLOCK)
		_k_debug_halt = 0;
#endif

	for (i = 0, X = _k_task_list; i < _k_task_count; i++, X++) {
		if (X->Group & grp) {
			switch (opt) {
			case GROUP_TASK_START:
				start_task(X, X->fstart);
				break;
			case GROUP_TASK_ABORT:
				abort_task(X);
				break;
			case GROUP_TASK_SUSPEND:
				set_state_bit(X, TF_SUSP);
				break;
			case GROUP_TASK_RESUME:
				reset_state_bit(X, TF_SUSP);
				break;
			case GROUP_TASK_BLOCK:
				set_state_bit(X, TF_BLCK);
				break;
			case GROUP_TASK_UNBLOCK:
				reset_state_bit(X, TF_BLCK);
				break;
			}
		}
	}

}

/*******************************************************************************
*
* _task_group_ioctl - task group operations
*
* RETURNS: N/A
*/

void _task_group_ioctl(ktask_group_t group, /* task group */
		   int opt	 /* operation */
		   )
{
	struct k_args A;

	A.Comm = GRPOP;
	A.Args.g1.group = group;
	A.Args.g1.opt = opt;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* K_set_prio - handle task set priority request
*
* RETURNS: N/A
*/

void K_set_prio(struct k_args *A)
{
	ktask_t Tid = A->Args.g1.task;
		struct k_proc *X = _k_task_list + OBJ_INDEX(Tid);

		set_state_bit(X, TF_PRIO);
		X->Prio = A->Args.g1.prio;
		reset_state_bit(X, TF_PRIO);

	if (A->alloc)
		FREEARGS(A);
}

/*******************************************************************************
*
* task_priority_set - set the priority of a task
*
* This routine changes the priority of the specified task.
*
* The call has immediate effect. If the calling task is no longer the highest
* priority runnable task, a task switch occurs.
*
* The priority should be specified in the range 0 to 62. 0 is the highest
* priority.
*
* RETURNS: N/A
*/

void task_priority_set(ktask_t task, /* task whose priority is to be set */
		    kpriority_t prio  /* new priority */
		    )
{
	struct k_args A;

	A.Comm = SPRIO;
	A.Args.g1.task = task;
	A.Args.g1.prio = prio;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* K_yield - handle task yield request
*
* RETURNS: N/A
*/

void K_yield(struct k_args *A)
{
	struct k_tqhd *H = _k_task_priority_list + _k_current_task->Prio;
	struct k_proc *X = _k_current_task->Forw;

	ARG_UNUSED(A);
	if (X && H->Head == _k_current_task) {
		_k_current_task->Forw = NULL;
		H->Tail->Forw = _k_current_task;
		H->Tail = _k_current_task;
		H->Head = X;
	}
}

/*******************************************************************************
*
* task_yield - yield the CPU to another task
*
* This routine yields the processor to the next equal priority task that is
* runnable. Using task_yield(), it is possible to achieve the effect of round
* robin scheduling. If no task with the same priority is runnable then no task
* switch occurs and the calling task resumes execution.
*
* RETURNS: N/A
*/

void task_yield(void)
{
	struct k_args A;

	A.Comm = YIELD;
	KERNEL_ENTRY(&A);
}

/*******************************************************************************
*
* task_entry_set - set the entry point of a task
*
* This routine sets the entry point of a task to a given routine. It is only
* needed if the entry point is different from that specified in the project
* file. It must be called before task_start() to have any effect, so it
* cannot work with members of the EXE group or of any group that automatically
* starts when the application is loaded.
*
* The routine is executed when the task is started
*
* RETURNS: N/A
*/

void task_entry_set(ktask_t task,       /* task */
		     void (*func)(void) /* entry point */
		     )
{
	_k_task_list[OBJ_INDEX(task)].fstart = func;
}

