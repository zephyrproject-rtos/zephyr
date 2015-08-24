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
#include <arch/cpu.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

#include <micro_private.h>
#include <nano_private.h>
#include <start_task_arch.h>

extern struct k_task _k_task_list_start[];
extern struct k_task _k_task_list_end[];

/**
 *
 * @brief Get task identifer
 *
 * @return identifier for current task
 */

ktask_t task_id_get(void)
{
	return _k_current_task->Ident;
}

/**
 *
 * @brief Reset the specified task state bits
 *
 * This routine resets the specified task state bits.  When a task's state bits
 * are zero, the task may be scheduled to run.  The tasks's state bits are a
 * bitmask of the TF_xxx bits.  Each TF_xxx bit indicates a reason why the task
 * must not be scheduled to run.
 *
 * @return N/A
 */

void _k_state_bit_reset(struct k_task *X,    /* ptr to task */
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

		struct k_tqhd *H = _k_task_priority_list + X->priority;

		X->next = NULL;
		H->Tail->next = X;
		H->Tail = X;
		_k_task_priority_bitmap[X->priority >> 5] |= (1 << (X->priority & 0x1F));
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

		_k_task_monitor(X, f_new | MO_STBIT0);
	}
#endif
}

/**
 *
 * @brief Set specified task state bits
 *
 * This routine sets the specified task state bits.  When a task's state bits
 * are non-zero, the task will not be scheduled to run.  The task's state bits
 * are a bitmask of the TF_xxx bits.  Each TF_xxx bit indicates a reason why
 * the task must not be scheduled to run.
 *
 * @return N/A
 */

void _k_state_bit_set(
	struct k_task *task_ptr,
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
#if defined(CONFIG_ARM)
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
			struct k_tqhd *task_queue = _k_task_priority_list + task_ptr->priority;
		struct k_task *cur_task = (struct k_task *)(&task_queue->Head);

		/*
		 * Search in the list for this task priority level,
		 * and remove the task.
		 */
		while (cur_task->next != task_ptr) {
			cur_task = cur_task->next;
		}

		cur_task->next = task_ptr->next;

		if (task_queue->Tail == task_ptr) {
			task_queue->Tail = cur_task;
		}

		/*
		 * If there are no more tasks of this priority that are
		 * runnable, then clear that bit in the global priority bit map.
		 */
		if (task_queue->Head == NULL) {
			_k_task_priority_bitmap[task_ptr->priority >> 5] &= ~(1 << (task_ptr->priority & 0x1F));
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

		_k_task_monitor(task_ptr, new_state_bits | MO_STBIT1);
	}
#endif
}

/**
 *
 * @brief Initialize and start a task
 *
 * @return N/A
 */

static void start_task(struct k_task *X,  /* ptr to task control block */
					   void (*func)(void) /* entry point for task */
					   )
{
	unsigned int task_options;

/* Note: the field X->worksize now represents the task size in bytes */

	task_options = 0;
	_START_TASK_ARCH(X, &task_options);

	/*
	 * The 'func' argument to _new_thread() represents the entry point of
	 * the
	 * kernel task.  The 'parameter1', 'parameter2', & 'parameter3'
	 * arguments
	 * are not applicable to such tasks.  A 'priority' of -1 indicates that
	 * the thread is a task, rather than a fiber.
	 */

	_new_thread((char *)X->workspace, /* pStackMem */
				X->worksize,		/* stackSize */
				(_thread_entry_t)func,  /* pEntry */
				(void *)0,		/* parameter1 */
				(void *)0,		/* parameter2 */
				(void *)0,		/* parameter3 */
				-1,			/* priority */
				task_options	/* options */
				);

	X->fabort = NULL;

	_k_state_bit_reset(X, TF_STOP | TF_TERM);
}

/**
 *
 * @brief Abort a task
 *
 * This routine aborts the specified task.
 *
 * @return N/A
 */

static void abort_task(struct k_task *X)
{

	/* Do normal thread exit cleanup */

	_thread_exit((struct tcs *)X->workspace);

	/* Set TF_TERM and TF_STOP state flags */

	_k_state_bit_set(X, TF_STOP | TF_TERM);

	/* Invoke abort function, if there is one */

	if (X->fabort != NULL) {
		X->fabort();
	}
}

#ifndef CONFIG_ARCH_HAS_TASK_ABORT
/**
 *
 * @brief Microkernel handler for fatal task errors
 *
 * To be invoked when a task aborts implicitly, either by returning from its
 * entry point or due to a software or hardware fault.
 *
 * @return does not return
 *
 * \NOMANUAL
 */

FUNC_NORETURN void _TaskAbort(void)
{
	_task_ioctl(_k_current_task->Ident, TASK_ABORT);

	/*
	 * Compiler can't tell that _task_ioctl() won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
#endif

/**
 *
 * @brief Install an abort handler
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
 * @return N/A
 */

void task_abort_handler_set(void (*func)(void) /* abort handler */
			    )
{
	_k_current_task->fabort = func;
}

/**
 *
 * @brief Handle a task operation request
 *
 * This routine handles any one of the following task operation requests:
 *   starting either a kernel or user task, aborting a task, suspending a task,
 *   resuming a task, blocking a task or unblocking a task
 *
 * @return N/A
 */

void _k_task_op(struct k_args *A)
{
	ktask_t Tid = A->Args.g1.task;
		struct k_task *X = (struct k_task *)Tid;

		switch (A->Args.g1.opt) {
		case TASK_START:
			start_task(X, X->fstart);
			break;
		case TASK_ABORT:
			abort_task(X);
			break;
		case TASK_SUSPEND:
			_k_state_bit_set(X, TF_SUSP);
			break;
		case TASK_RESUME:
			_k_state_bit_reset(X, TF_SUSP);
			break;
		case TASK_BLOCK:
			_k_state_bit_set(X, TF_BLCK);
			break;
		case TASK_UNBLOCK:
			_k_state_bit_reset(X, TF_BLCK);
			break;
		}
}

/**
 *
 * @brief Task operations
 *
 * @return N/A
 */

void _task_ioctl(ktask_t task, /* task on which to operate */
				    int opt      /* task operation */
				    )
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_OP;
	A.Args.g1.task = task;
	A.Args.g1.opt = opt;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Handle task group operation request
 *
 * This routine handles any one of the following task group operations requests:
 *   starting either kernel or user tasks, aborting tasks, suspending tasks,
 *   resuming tasks, blocking tasks or unblocking tasks
 *
 * @return N/A
 */

void _k_task_group_op(struct k_args *A)
{
	ktask_group_t grp = A->Args.g1.group;
	int opt = A->Args.g1.opt;
	struct k_task *X;

#ifdef CONFIG_TASK_DEBUG
	if (opt == TASK_GROUP_BLOCK)
		_k_debug_halt = 1;
	if (opt == TASK_GROUP_UNBLOCK)
		_k_debug_halt = 0;
#endif

	for (X = _k_task_list_start; X < _k_task_list_end; X++) {
		if (X->Group & grp) {
			switch (opt) {
			case TASK_GROUP_START:
				start_task(X, X->fstart);
				break;
			case TASK_GROUP_ABORT:
				abort_task(X);
				break;
			case TASK_GROUP_SUSPEND:
				_k_state_bit_set(X, TF_SUSP);
				break;
			case TASK_GROUP_RESUME:
				_k_state_bit_reset(X, TF_SUSP);
				break;
			case TASK_GROUP_BLOCK:
				_k_state_bit_set(X, TF_BLCK);
				break;
			case TASK_GROUP_UNBLOCK:
				_k_state_bit_reset(X, TF_BLCK);
				break;
			}
		}
	}

}

/**
 *
 * @brief Task group operations
 *
 * @return N/A
 */

void _task_group_ioctl(ktask_group_t group, /* task group */
		   int opt	 /* operation */
		   )
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_GROUP_OP;
	A.Args.g1.group = group;
	A.Args.g1.opt = opt;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Get task groups for task
 *
 * @return task groups associated with current task
 */

kpriority_t task_group_mask_get(void)
{
	return _k_current_task->Group;
}

/**
 *
 * @brief Add task to task group(s)
 *
 * @return N/A
 */

void task_group_join(uint32_t groups)
{
	_k_current_task->Group |= groups;
}

/**
 *
 * @brief Remove task from task group(s)
 *
 * @return N/A
 */

void task_group_leave(uint32_t groups)
{
	_k_current_task->Group &= ~groups;
}

/**
 *
 * @brief Get task priority
 *
 * @return priority of current task
 */

kpriority_t task_priority_get(void)
{
	return _k_current_task->priority;
}

/**
 *
 * @brief Handle task set priority request
 *
 * @return N/A
 */

void _k_task_priority_set(struct k_args *A)
{
	ktask_t Tid = A->Args.g1.task;
		struct k_task *X = (struct k_task *)Tid;

		_k_state_bit_set(X, TF_PRIO);
		X->priority = A->Args.g1.prio;
		_k_state_bit_reset(X, TF_PRIO);

	if (A->alloc)
		FREEARGS(A);
}

/**
 *
 * @brief Set the priority of a task
 *
 * This routine changes the priority of the specified task.
 *
 * The call has immediate effect. If the calling task is no longer the highest
 * priority runnable task, a task switch occurs.
 *
 * The priority should be specified in the range 0 to 62. 0 is the highest
 * priority.
 *
 * @return N/A
 */

void task_priority_set(ktask_t task, /* task whose priority is to be set */
		    kpriority_t prio  /* new priority */
		    )
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_PRIORITY_SET;
	A.Args.g1.task = task;
	A.Args.g1.prio = prio;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Handle task yield request
 *
 * @return N/A
 */

void _k_task_yield(struct k_args *A)
{
	struct k_tqhd *H = _k_task_priority_list + _k_current_task->priority;
	struct k_task *X = _k_current_task->next;

	ARG_UNUSED(A);
	if (X && H->Head == _k_current_task) {
		_k_current_task->next = NULL;
		H->Tail->next = _k_current_task;
		H->Tail = _k_current_task;
		H->Head = X;
	}
}

/**
 *
 * @brief Yield the CPU to another task
 *
 * This routine yields the processor to the next equal priority task that is
 * runnable. Using task_yield(), it is possible to achieve the effect of round
 * robin scheduling. If no task with the same priority is runnable then no task
 * switch occurs and the calling task resumes execution.
 *
 * @return N/A
 */

void task_yield(void)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_YIELD;
	KERNEL_ENTRY(&A);
}

/**
 *
 * @brief Set the entry point of a task
 *
 * This routine sets the entry point of a task to a given routine. It is only
 * needed if the entry point is different from that specified in the project
 * file. It must be called before task_start() to have any effect, so it
 * cannot work with members of the EXE group or of any group that automatically
 * starts when the application is loaded.
 *
 * The routine is executed when the task is started
 *
 * @return N/A
 */

void task_entry_set(ktask_t task,       /* task */
		     void (*func)(void) /* entry point */
		     )
{
	struct k_task *X = (struct k_task *)task;

	X->fstart = func;
}

