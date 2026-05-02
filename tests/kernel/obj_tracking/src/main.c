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

unsigned char __aligned(4) pipe_buffer[64];
char __aligned(4) slab_buffer[8 * 4];
stack_data_t stack_array[8 * 4];
int msgq_buffer[64];

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
	void *list;
	int count;

	k_timer_init(&timer, dummy_fn, NULL);
	count = 0;
	list = _track_list_k_timer;
	while (list != NULL) {
		if (list == &timer || list == &timer_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_timer *)list);
	}
	zassert_equal(count, 2, "Wrong number of timer objects");

	k_mem_slab_init(&slab, slab_buffer, 8, 4);
	count = 0;
	list = _track_list_k_mem_slab;
	while (list != NULL) {
		if (list == &slab || list == &slab_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_mem_slab *)list);
	}
	zassert_equal(count, 2, "Wrong number of mem_slab objects");

	k_sem_init(&sem, 1, 2);
	count = 0;
	list = _track_list_k_sem;
	while (list != NULL) {
		if (list == &sem || list == &sem_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_sem *)list);
	}
	zassert_equal(count, 2, "Wrong number of semaphore objects");

	k_mutex_init(&mutex);
	count = 0;
	list = _track_list_k_mutex;
	while (list != NULL) {
		if (list == &mutex || list == &mutex_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_mutex *)list);
	}
	zassert_equal(count, 2, "Wrong number of mutex objects");

	k_stack_init(&stack, stack_array, 20);
	count = 0;
	list = _track_list_k_stack;
	while (list != NULL) {
		if (list == &stack || list == &stack_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_stack *)list);
	}
	zassert_equal(count, 2, "Wrong number of stack objects");

	k_msgq_init(&msgq, (char *)msgq_buffer, sizeof(int), 8);
	count = 0;
	list = _track_list_k_msgq;
	while (list != NULL) {
		if (list == &msgq || list == &msgq_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_msgq *)list);
	}
	zassert_equal(count, 2, "Wrong number of message queue objects");

	k_mbox_init(&mbox);
	count = 0;
	list = _track_list_k_mbox;
	while (list != NULL) {
		if (list == &mbox || list == &mbox_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_mbox *)list);
	}
	zassert_equal(count, 2, "Wrong number of mbox objects");

	k_pipe_init(&pipe, pipe_buffer, sizeof(pipe_buffer));
	count = 0;
	list = _track_list_k_pipe;
	while (list != NULL) {
		if (list == &pipe || list == &pipe_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_pipe *)list);
	}
	zassert_equal(count, 2, "Wrong number of pipe objects");

	k_queue_init(&queue);
	count = 0;
	list = _track_list_k_queue;
	while (list != NULL) {
		if (list == &queue || list == &queue_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_queue *)list);
	}
	zassert_equal(count, 2, "Wrong number of queue objects");

	k_event_init(&event);
	count = 0;
	list = _track_list_k_event;
	while (list != NULL) {
		if (list == &event || list == &event_s) {
			count++;
		}
		list = SYS_PORT_TRACK_NEXT((struct k_event *)list);
	}
	zassert_equal(count, 2, "Wrong number of event objects");


}

ZTEST_SUITE(obj_tracking, NULL, NULL, NULL, NULL, NULL);
