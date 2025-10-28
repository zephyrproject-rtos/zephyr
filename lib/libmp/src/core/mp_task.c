/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "mp_task.h"

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, CONFIG_MP_THREADS_NUM, CONFIG_MP_THREAD_STACK_SIZE);

uint8_t mp_thread_pool[CONFIG_MP_THREADS_NUM] = {0};

static int mp_thread_stack_acquire()
{
	int stack_id = -1;
	for (stack_id = 0; stack_id < CONFIG_MP_THREADS_NUM; stack_id++) {
		if (mp_thread_pool[stack_id] == 0) {
			break;
		}
	}

	if (stack_id == CONFIG_MP_THREADS_NUM) {
		return -1;
	}

	mp_thread_pool[stack_id] = 1;

	return stack_id;
}

static void mp_thread_stack_release(int stack_id)
{
	__ASSERT(stack_id < CONFIG_MP_THREADS_NUM, "Invalid pool id");
	__ASSERT(mp_thread_pool[stack_id] == 1, "Pool not acquired");

	mp_thread_pool[stack_id] = 0;
}

k_tid_t mp_task_create(MpTask *task, k_thread_entry_t func, void *p1, void *p2, void *p3,
		       int priority)
{
	task->stack_id = mp_thread_stack_acquire();
	if (task->stack_id < 0) {
		printk("No more thread stacks available\n");
		return NULL;
	}

	return k_thread_create(&task->thread_data, thread_stack[task->stack_id],
			       K_THREAD_STACK_SIZEOF(thread_stack[task->stack_id]), func, p1, p2,
			       p3, priority, 0, K_NO_WAIT);
}

void mp_task_destroy(MpTask *task)
{
	task->running = false;
	mp_thread_stack_release(task->stack_id);
}
