/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "mp_pad.h"
#include "mp_task.h"

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, CONFIG_MP_THREADS_NUM, CONFIG_MP_THREAD_STACK_SIZE);

uint8_t mp_thread_pool[CONFIG_MP_THREADS_NUM] = {0};

static int mp_thread_stack_acquire(void)
{
	int stack_id = 0;

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

k_tid_t mp_task_create(struct mp_task *task, k_thread_entry_t func, void *user_data, int priority)
{

	struct mp_pad *pad = CONTAINER_OF(task, struct mp_pad, task);

	task->stack_id = mp_thread_stack_acquire();
	if (task->stack_id < 0) {
		printk("No more thread stacks available\n");
		return NULL;
	}

	return k_thread_create(&task->thread_data, thread_stack[task->stack_id],
			       K_THREAD_STACK_SIZEOF(thread_stack[task->stack_id]), func, pad,
			       user_data, NULL, priority, 0, K_NO_WAIT);
}

void mp_task_destroy(struct mp_task *task)
{
	task->running = false;
	mp_thread_stack_release(task->stack_id);
}
