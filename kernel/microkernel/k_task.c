/* task kernel services */

/*
 * Copyright (c) 1997-2010, 2013-2015 Wind River Systems, Inc.
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


#include <microkernel.h>
#include <nanokernel.h>
#include <arch/cpu.h>
#include <string.h>
#include <toolchain.h>
#include <sections.h>

#include <micro_private.h>
#include <nano_private.h>
#include <start_task_arch.h>

extern ktask_t _k_task_ptr_start[];
extern ktask_t _k_task_ptr_end[];


ktask_t task_id_get(void)
{
	return _k_current_task->id;
}

/**
 * @brief Reset the specified task state bits
 *
 * This routine resets the specified task state bits.  When a task's state bits
 * are zero, the task may be scheduled to run.  The tasks's state bits are a
 * bitmask of the TF_xxx bits.  Each TF_xxx bit indicates a reason why the task
 * must not be scheduled to run.
 *
 * @param X Pointer to task
 * @param bits Bitmask of TF_xxx bits to reset
 * @return N/A
 */
void _k_state_bit_reset(struct k_task *X, uint32_t bits)
{
	uint32_t f_old = X->state;      /* old state bits */
	uint32_t f_new = f_old & ~bits; /* new state bits */

	X->state = f_new; /* Update task's state bits */

	if ((f_old != 0) && (f_new == 0)) {
		/*
		 * The task may now be scheduled to run (but could not
		 * previously) as all the TF_xxx bits are clear.  It must
		 * be added to the list of schedulable tasks.
		 */

		struct k_tqhd *H = _k_task_priority_list + X->priority;

		X->next = NULL;
		H->tail->next = X;
		H->tail = X;
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
 * @brief Set specified task state bits
 *
 * This routine sets the specified task state bits.  When a task's state bits
 * are non-zero, the task will not be scheduled to run.  The task's state bits
 * are a bitmask of the TF_xxx bits.  Each TF_xxx bit indicates a reason why
 * the task must not be scheduled to run.
 * @param task_ptr Task pointer
 * @param bitmask of TF_xxx bits to set
 * @return N/A
 */
void _k_state_bit_set(struct k_task *task_ptr, uint32_t bits)
{
	uint32_t old_state_bits = task_ptr->state;
	uint32_t new_state_bits = old_state_bits | bits;

	task_ptr->state = new_state_bits;

	if ((old_state_bits == 0) && (new_state_bits != 0)) {
		/*
		 * The task could have been scheduled to run ([state] was 0)
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
		struct k_task *cur_task = (struct k_task *)(&task_queue->head);

		/*
		 * Search in the list for this task priority level,
		 * and remove the task.
		 */
		while (cur_task->next != task_ptr) {
			cur_task = cur_task->next;
		}

		cur_task->next = task_ptr->next;

		if (task_queue->tail == task_ptr) {
			task_queue->tail = cur_task;
		}

		/*
		 * If there are no more tasks of this priority that are
		 * runnable, then clear that bit in the global priority bit map.
		 */
		if (task_queue->head == NULL) {
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
 * @brief Initialize and start a task
 *
 * @param X Pointer to task control block
 * @param func Entry point for task
 * @return N/A
 */
static void start_task(struct k_task *X, void (*func)(void))
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

	X->fn_abort = NULL;

	_k_state_bit_reset(X, TF_STOP | TF_TERM);
}

/**
 * @brief Abort a task
 *
 * This routine aborts the specified task.
 * @param X Task pointer
 * @return N/A
 */
static void abort_task(struct k_task *X)
{

	/* Do normal thread exit cleanup */

	_thread_exit((struct tcs *)X->workspace);

	/* Set TF_TERM and TF_STOP state flags */

	_k_state_bit_set(X, TF_STOP | TF_TERM);

	/* Invoke abort function, if there is one */

	if (X->fn_abort != NULL) {
		X->fn_abort();
	}
}

#ifndef CONFIG_ARCH_HAS_TASK_ABORT
/**
 * @brief Microkernel handler for fatal task errors
 *
 * To be invoked when a task aborts implicitly, either by returning from its
 * entry point or due to a software or hardware fault.
 *
 * @return does not return
 */
FUNC_NORETURN void _TaskAbort(void)
{
	_task_ioctl(_k_current_task->id, TASK_ABORT);

	/*
	 * Compiler can't tell that _task_ioctl() won't return and issues
	 * a warning unless we explicitly tell it that control never gets this
	 * far.
	 */

	CODE_UNREACHABLE;
}
#endif


void task_abort_handler_set(void (*func)(void))
{
	_k_current_task->fn_abort = func;
}

/**
 * @brief Handle a task operation request
 *
 * This routine handles any one of the following task operation requests:
 *   starting either a kernel or user task, aborting a task, suspending a task,
 *   resuming a task, blocking a task or unblocking a task
 * @param A  Arguments
 * @return N/A
 */
void _k_task_op(struct k_args *A)
{
	ktask_t Tid = A->args.g1.task;
	struct k_task *X = (struct k_task *)Tid;

	switch (A->args.g1.opt) {
	case TASK_START:
		start_task(X, X->fn_start);
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
 * @brief Task operations
 * @param task Task on which to operate
 * @param opt Task operation
 * @return N/A
 */
void _task_ioctl(ktask_t task, int opt)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_OP;
	A.args.g1.task = task;
	A.args.g1.opt = opt;
	KERNEL_ENTRY(&A);
}

/**
 * @brief Handle task group operation request
 *
 * This routine handles any one of the following task group operations requests:
 *   starting either kernel or user tasks, aborting tasks, suspending tasks,
 *   resuming tasks, blocking tasks or unblocking tasks
 * @param A  Arguments
 * @return N/A
 */
void _k_task_group_op(struct k_args *A)
{
	ktask_group_t grp = A->args.g1.group;
	int opt = A->args.g1.opt;
	struct k_task *X;
	ktask_t *task_id;

#ifdef CONFIG_TASK_DEBUG
	if (opt == TASK_GROUP_BLOCK)
		_k_debug_halt = 1;
	if (opt == TASK_GROUP_UNBLOCK)
		_k_debug_halt = 0;
#endif

	for (task_id = _k_task_ptr_start; task_id < _k_task_ptr_end;
	     task_id++) {
		X = (struct k_task *)(*task_id);
		if (X->group & grp) {
			switch (opt) {
			case TASK_GROUP_START:
				start_task(X, X->fn_start);
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
 * @brief Task group operations
 * @param group Task group
 * @param opt Operation
 * @return N/A
 */
void _task_group_ioctl(ktask_group_t group, int opt)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_GROUP_OP;
	A.args.g1.group = group;
	A.args.g1.opt = opt;
	KERNEL_ENTRY(&A);
}


kpriority_t task_group_mask_get(void)
{
	return _k_current_task->group;
}

void task_group_join(uint32_t groups)
{
	_k_current_task->group |= groups;
}

void task_group_leave(uint32_t groups)
{
	_k_current_task->group &= ~groups;
}

/**
 * @brief Get task priority
 *
 * @return priority of current task
 */
kpriority_t task_priority_get(void)
{
	return _k_current_task->priority;
}

/**
 * @brief Handle task set priority request
 * @param A  Arguments
 * @return N/A
 */
void _k_task_priority_set(struct k_args *A)
{
	ktask_t Tid = A->args.g1.task;
	struct k_task *X = (struct k_task *)Tid;

	_k_state_bit_set(X, TF_PRIO);
	X->priority = A->args.g1.prio;
	_k_state_bit_reset(X, TF_PRIO);

	if (A->alloc)
		FREEARGS(A);
}


void task_priority_set(ktask_t task, kpriority_t prio)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_PRIORITY_SET;
	A.args.g1.task = task;
	A.args.g1.prio = prio;
	KERNEL_ENTRY(&A);
}

/**
 * @brief Handle task yield request
 *
 * @param A  Arguments
 * @return N/A
 */
void _k_task_yield(struct k_args *A)
{
	struct k_tqhd *H = _k_task_priority_list + _k_current_task->priority;
	struct k_task *X = _k_current_task->next;

	ARG_UNUSED(A);
	if (X && H->head == _k_current_task) {
		_k_current_task->next = NULL;
		H->tail->next = _k_current_task;
		H->tail = _k_current_task;
		H->head = X;
	}
}


void task_yield(void)
{
	struct k_args A;

	A.Comm = _K_SVC_TASK_YIELD;
	KERNEL_ENTRY(&A);
}


void task_entry_set(ktask_t task, void (*func)(void))
{
	struct k_task *X = (struct k_task *)task;

	X->fn_start = func;
}

