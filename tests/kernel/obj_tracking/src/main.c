/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

void dummy_fn(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}

K_TIMER_DEFINE(timer_s, dummy_fn, NULL);
K_MEM_SLAB_DEFINE(slab_s, 8, 2, 8);
K_SEM_DEFINE(sem_s, 0, 1);
K_MUTEX_DEFINE(mutex_s);
K_STACK_DEFINE(stack_s, 64);
K_MSGQ_DEFINE(msgq_s, sizeof(int), 2, 4);
K_MBOX_DEFINE(mbox_s);
K_PIPE_DEFINE(pipe_s, 64, 4);
K_QUEUE_DEFINE(queue_s);
K_EVENT_DEFINE(event_s);
K_EVENT_DEFINE(double_init_event_s);

unsigned char __aligned(4) pipe_buffer[64];
char __aligned(4) slab_buffer[8 * 4];
stack_data_t stack_array[8 * 4];
int msgq_buffer[64];

static int find_op(struct k_obj_core *obj_core, void *data)
{
	return (obj_core == data) ? 1 : 0;
}

static int count_op(struct k_obj_core *obj_core, void *data)
{
	ARG_UNUSED(obj_core);
	(*(int *)data)++;

	return 0;
}

static int count_objects(uint32_t type_id)
{
	int count = 0;

	k_obj_type_walk_locked(k_obj_type_find(type_id), count_op, &count);

	return count;
}

static void check_tracked(uint32_t type_id, struct k_obj_core *obj_core, bool tracked,
			  const char *name)
{
	struct k_obj_type *type = k_obj_type_find(type_id);

	zassert_not_null(type, "%s type not found", name);
	zassert_equal(k_obj_type_walk_locked(type, find_op, obj_core), tracked ? 1 : 0,
		      "%s %stracked", name, tracked ? "not " : "");
}

ZTEST(obj_tracking, test_obj_tracking_coherence)
{
	struct k_timer timer;
	struct k_mem_slab slab;
	struct k_sem sem;
	struct k_mutex mutex;
	struct k_stack stack;
	struct k_msgq msgq;
	struct k_mbox mbox;
	struct k_pipe pipe;
	struct k_queue queue;
	struct k_event event;
	int count;

	/* Objects on the stack are not tracked and do not disturb the walks */
	k_timer_init(&timer, dummy_fn, NULL);
	k_mem_slab_init(&slab, slab_buffer, 8, 4);
	k_sem_init(&sem, 0, 1);
	k_mutex_init(&mutex);
	k_stack_init(&stack, stack_array, 8 * 4);
	k_msgq_init(&msgq, (char *)msgq_buffer, sizeof(int), 64);
	k_mbox_init(&mbox);
	k_pipe_init(&pipe, pipe_buffer, sizeof(pipe_buffer));
	k_queue_init(&queue);
	k_event_init(&event);

	check_tracked(K_OBJ_TYPE_TIMER_ID, K_OBJ_CORE(&timer), false, "stack timer");
	check_tracked(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem), false, "stack semaphore");
	check_tracked(K_OBJ_TYPE_QUEUE_ID, K_OBJ_CORE(&queue), false, "stack queue");

	check_tracked(K_OBJ_TYPE_TIMER_ID, K_OBJ_CORE(&timer_s), true, "timer");
	check_tracked(K_OBJ_TYPE_MEM_SLAB_ID, K_OBJ_CORE(&slab_s), true, "memory slab");
	check_tracked(K_OBJ_TYPE_SEM_ID, K_OBJ_CORE(&sem_s), true, "semaphore");
	check_tracked(K_OBJ_TYPE_MUTEX_ID, K_OBJ_CORE(&mutex_s), true, "mutex");
	check_tracked(K_OBJ_TYPE_STACK_ID, K_OBJ_CORE(&stack_s), true, "stack");
	check_tracked(K_OBJ_TYPE_MSGQ_ID, K_OBJ_CORE(&msgq_s), true, "message queue");
	check_tracked(K_OBJ_TYPE_MBOX_ID, K_OBJ_CORE(&mbox_s), true, "mailbox");
	check_tracked(K_OBJ_TYPE_PIPE_ID, K_OBJ_CORE(&pipe_s), true, "pipe");
	check_tracked(K_OBJ_TYPE_QUEUE_ID, K_OBJ_CORE(&queue_s), true, "queue");
	check_tracked(K_OBJ_TYPE_EVENT_ID, K_OBJ_CORE(&event_s), true, "event");

	/* Initializing a statically defined object again changes nothing */
	count = count_objects(K_OBJ_TYPE_EVENT_ID);
	k_event_init(&double_init_event_s);
	zassert_equal(count_objects(K_OBJ_TYPE_EVENT_ID), count, "event count changed");
	check_tracked(K_OBJ_TYPE_EVENT_ID, K_OBJ_CORE(&double_init_event_s), true, "event");
}

ZTEST_SUITE(obj_tracking, NULL, NULL, NULL, NULL, NULL);
