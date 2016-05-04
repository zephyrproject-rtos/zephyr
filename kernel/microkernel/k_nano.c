/*
 * Copyright (c) 2016 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Nanokernel object kernel service
 *
 * This module provides routines used by the nanokernel for pending/unpending
 * microkernel tasks on nanokernel objects.
 */

#include <micro_private.h>
#include <toolchain.h>

#include <wait_q.h>
#include <nanokernel.h>
#include <microkernel/event.h>
#include <drivers/system_timer.h>

extern void _task_nop(void);
extern void _nano_nop(void);

/**
 * @brief Pend a task on a nanokernel object
 *
 * This routine pends a task on a nanokernel object. It is expected to only
 * be called by internal nanokernel code with the interrupts locked.
 *
 * @return N/A
 */
void _task_nano_pend_task(struct _nano_queue *queue, int32_t timeout)
{
	_NANO_TIMEOUT_ADD(queue, timeout);

	/* Add task to nanokernel object's task wait queue */
	_nano_wait_q_put(queue);

	_k_state_bit_set(_k_current_task, TF_NANO);

	_task_nop();    /* Trigger microkernel scheduler */
}

#ifdef CONFIG_NANO_TIMERS
/**
 * @brief Pend a task on a nanokernel timer
 *
 * This routine pends a task on a nanokernel timer. It is expected to only
 * be called by internal nanokernel code with the interrupts locked.
 *
 * @return N/A
 */
void _task_nano_timer_pend_task(struct nano_timer *timer)
{
	struct _nano_timeout *t = &timer->timeout_data;

	t->tcs = (struct tcs *) _k_current_task->workspace;
	_k_state_bit_set(_k_current_task, TF_NANO);

	_task_nop();    /* Trigger microkernel scheduler */
}

/**
 * @brief Ready the task waiting on a nanokernel timer
 *
 * This routine readies the task that was waiting on a a nanokernel timer.
 * It is expected to only be called by internal nanokernel code (fiber or
 * ISR context) with the interrupts locked.
 *
 * @return N/A
 */
void _nano_timer_task_ready(void *ptr)
{
	struct k_task *uk_task_ptr = ptr;

	_k_state_bit_reset(uk_task_ptr, TF_NANO);

	_nano_nop();    /* Trigger microkernel scheduler */
}

/**
 * @brief Ready the task waiting on a nanokernel timer
 *
 * This routine readies the task that was waiting on a a nanokernel timer.
 * It is expected to only be called by internal nanokernel code (task context)
 * with the interrupts locked.
 *
 * @return N/A
 */
void _task_nano_timer_task_ready(void *ptr)
{
	struct k_task *uk_task_ptr = ptr;

	_k_state_bit_reset(uk_task_ptr, TF_NANO);

	_task_nop();    /* Trigger microkernel scheduler */
}
#endif /* CONFIG_NANO_TIMERS */

/**
 * @brief Ready a microkernel task due to timeout
 *
 * This routine makes a microkernel task ready.  As it is invoked in the
 * context of the kernel server fiber, there is no need to explicitly trigger
 * the microkernel task scheduler here. Interrupts are already locked.
 *
 * @return N/A
 */
void _nano_task_ready(void *ptr)
{
	struct k_task *uk_task_ptr = ptr;

	_k_state_bit_reset(uk_task_ptr, TF_NANO);
}

/**
 * @brief Unpend all tasks from nanokernel object
 *
 * This routine unpends all tasks the nanokernel object's task queue.
 * It is expected to only be called by internal nanokernel code with
 * the interrupts locked.
 *
 * @return Number of tasks that were unpended
 */
int _nano_unpend_tasks(struct _nano_queue *queue)
{
	struct tcs *task = (struct tcs *)queue->head;
	struct tcs *prev;
	int num = 0;

	/* Drain the nanokernel object's waiting task queue */
	while (task != NULL) {
		_nano_timeout_abort(task);
		_k_state_bit_reset(task->uk_task_ptr, TF_NANO);
		prev = task;
		task = task->link;
		prev->link = NULL;
		num++;
	}

	_nano_wait_q_reset(queue);

	return num;
}
