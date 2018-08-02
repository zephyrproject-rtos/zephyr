/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <debug/object_tracing.h>

enum obj_name {
	TIMER,
	MEM_SLAB,
	SEM,
	MUTEX,
	ALERT,
	STACK,
	MSGQ,
	MBOX,
	PIPE,
	QUEUE
};

static inline void expiry_dummy_fn(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}
static inline void stop_dummy_fn(struct k_timer *timer)
{
	ARG_UNUSED(timer);
}

static inline void alert_handler_dummy(struct k_alert *alert)
{
	ARG_UNUSED(alert);
}

K_TIMER_DEFINE(ktimer, expiry_dummy_fn, stop_dummy_fn);
K_MEM_SLAB_DEFINE(kmslab, 4, 2, 4);
K_SEM_DEFINE(ksema, 0, 1);
K_MUTEX_DEFINE(kmutex);
K_ALERT_DEFINE(kalert, alert_handler_dummy, 1);
K_STACK_DEFINE(kstack, 512);
K_MSGQ_DEFINE(kmsgq, 4, 2, 4);
K_MBOX_DEFINE(kmbox);
K_PIPE_DEFINE(kpipe, 256, 4);
K_QUEUE_DEFINE(kqueue);

static struct k_timer timer;
static struct k_mem_slab mslab;
static struct k_sem sema;
static struct k_mutex mutex;
static struct k_alert alert;
static struct k_stack stack;
static struct k_msgq msgq;
static struct k_mbox mbox;
static struct k_pipe pipe;
static struct k_queue queue;

#define BLOCK_SIZE 4
#define NUM_BLOCKS 4

static char slab[BLOCK_SIZE * NUM_BLOCKS];
static u32_t sdata[BLOCK_SIZE * NUM_BLOCKS];
static char buffer[BLOCK_SIZE * NUM_BLOCKS];
static char data[] = "test";

static void get_obj_count(int obj_type)
{
	void *obj_list;
	int obj_found = 0;

	switch (obj_type) {
	case TIMER:
		k_timer_init(&timer, expiry_dummy_fn, stop_dummy_fn);

		obj_list = SYS_TRACING_HEAD(struct k_timer, k_timer);
		while (obj_list != NULL) {
			/* TESTPOINT: Check if the object created
			 * is added to the list
			 */
			if (obj_list == &ktimer || obj_list == &timer) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found timer objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_timer, k_timer,
						    obj_list);
		}
		zassert_equal(obj_found, 2,  "Didn't find timer objects");
		break;
	case MEM_SLAB:
		k_mem_slab_init(&mslab, slab, BLOCK_SIZE, NUM_BLOCKS);

		obj_list = SYS_TRACING_HEAD(struct k_mem_slab, k_mem_slab);
		while (obj_list != NULL) {
			if (obj_list == &kmslab || obj_list == &mslab) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found memory slab objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_mem_slab,
						    k_mem_slab, obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find mem_slab objects");
		break;
	case SEM:
		k_sem_init(&sema, 0, 1);

		obj_list = SYS_TRACING_HEAD(struct k_sem, k_sem);
		while (obj_list != NULL) {
			if (obj_list == &ksema || obj_list == &sema) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found semaphore objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_sem, k_sem,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find semaphore objects");
		break;
	case MUTEX:
		k_mutex_init(&mutex);

		obj_list = SYS_TRACING_HEAD(struct k_mutex, k_mutex);
		while (obj_list != NULL) {
			if (obj_list == &kmutex || obj_list == &mutex) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found mutex objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_mutex, k_mutex,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find mutex objects");
		break;
	case ALERT:
		k_alert_init(&alert, K_ALERT_IGNORE, 1);

		obj_list = SYS_TRACING_HEAD(struct k_alert, k_alert);
		while (obj_list != NULL) {
			if (obj_list == &kalert || obj_list == &alert) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found alert objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_alert, k_alert,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find alert objects");
		break;
	case STACK:
		k_stack_init(&stack, sdata, NUM_BLOCKS);

		obj_list = SYS_TRACING_HEAD(struct k_stack, k_stack);
		while (obj_list != NULL) {
			if (obj_list == &kstack || obj_list == &stack) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found stack objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_stack, k_stack,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find stack objects");
		break;
	case MSGQ:
		k_msgq_init(&msgq, buffer, BLOCK_SIZE, NUM_BLOCKS);

		obj_list = SYS_TRACING_HEAD(struct k_msgq, k_msgq);
		while (obj_list != NULL) {
			if (obj_list == &kmsgq || obj_list == &msgq) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found message queue objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_msgq, k_msgq,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find msgq objects");
		break;
	case MBOX:
		k_mbox_init(&mbox);

		obj_list = SYS_TRACING_HEAD(struct k_mbox, k_mbox);
		while (obj_list != NULL) {
			if (obj_list == &kmbox || obj_list == &mbox) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found mail box objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_mbox, k_mbox,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find mbox objects");
		break;
	case PIPE:
		k_pipe_init(&pipe, data, 8);

		obj_list = SYS_TRACING_HEAD(struct k_pipe, k_pipe);
		while (obj_list != NULL) {
			if (obj_list == &kpipe || obj_list == &pipe) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found pipe objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_pipe, k_pipe,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find pipe objects");
		break;
	case QUEUE:
		k_queue_init(&queue);

		obj_list = SYS_TRACING_HEAD(struct k_queue, k_queue);
		while (obj_list != NULL) {
			if (obj_list == &kqueue || obj_list == &queue) {
				obj_found++;
				if (obj_found == 2) {
					TC_PRINT("Found queue objects\n");
					break;
				}
			}
			obj_list = SYS_TRACING_NEXT(struct k_queue, k_queue,
						    obj_list);
		}
		zassert_equal(obj_found, 2, "Didn't find queue objects\n");
		break;
	default:
		zassert_unreachable("Undefined kernel object");
	}
}

/**
 * @brief Verify tracing of kernel objects
 * @details Statically create kernel objects
 * and check if they are added to trace object
 * list with object tracing enabled.
 * @ingroup kernel_objtracing_tests
 * @see SYS_TRACING_HEAD(), SYS_TRACING_NEXT()
 */
void test_obj_tracing(void)
{
	for (int i = TIMER; i < QUEUE; i++) {
		get_obj_count(i);
	}
}
