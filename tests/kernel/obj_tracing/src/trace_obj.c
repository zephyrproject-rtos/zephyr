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

struct k_mutex mutex;

static void get_obj_count(int obj_type)
{
	void *obj_list;
	int obj_found = 0;

	switch (obj_type) {
	case TIMER:
		obj_list = SYS_TRACING_HEAD(struct k_timer, k_timer);
		while (obj_list != NULL) {
			/* TESTPOINT: Check if the object created
			 * is added to the list
			 */
			if (obj_list == &ktimer) {
				obj_found = 1;
				TC_PRINT("Found timer object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_timer, k_timer,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find timer object");
		break;
	case MEM_SLAB:
		obj_list = SYS_TRACING_HEAD(struct k_mem_slab, k_mem_slab);
		while (obj_list != NULL) {
			if (obj_list == &kmslab) {
				obj_found = 1;
				TC_PRINT("Found memory slab object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_mem_slab,
					k_mem_slab, obj_list);
		}
		zassert_true(obj_found, "Didn't find mem_slab object");
		break;
	case SEM:
		obj_list = SYS_TRACING_HEAD(struct k_sem, k_sem);
		while (obj_list != NULL) {
			if (obj_list == &ksema) {
				obj_found = 1;
				TC_PRINT("Found semaphore object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_sem, k_sem,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find semaphore object");
		break;
	case MUTEX:
		k_mutex_init(&mutex);
		obj_list = SYS_TRACING_HEAD(struct k_mutex, k_mutex);
		while (obj_list != NULL) {
			if (obj_list == &kmutex || obj_list == &mutex) {
				obj_found = 1;
				TC_PRINT("Found mutex object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_mutex, k_mutex,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find mutex object");
		break;
	case ALERT:
		obj_list = SYS_TRACING_HEAD(struct k_alert, k_alert);
		while (obj_list != NULL) {
			if (obj_list == &kalert) {
				obj_found = 1;
				TC_PRINT("Found alert object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_alert, k_alert,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find alert object");
		break;
	case STACK:
		obj_list = SYS_TRACING_HEAD(struct k_stack, k_stack);
		while (obj_list != NULL) {
			if (obj_list == &kstack) {
				obj_found = 1;
				TC_PRINT("Found stack object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_stack, k_stack,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find stack object");
		break;
	case MSGQ:
		obj_list = SYS_TRACING_HEAD(struct k_msgq, k_msgq);
		while (obj_list != NULL) {
			if (obj_list == &kmsgq) {
				obj_found = 1;
				TC_PRINT("Found message queue object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_msgq, k_msgq,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find msgq object");
		break;
	case MBOX:
		obj_list = SYS_TRACING_HEAD(struct k_mbox, k_mbox);
		while (obj_list != NULL) {
			if (obj_list == &kmbox) {
				obj_found = 1;
				TC_PRINT("Found mail box object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_mbox, k_mbox,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find mbox object");
		break;
	case PIPE:
		obj_list = SYS_TRACING_HEAD(struct k_pipe, k_pipe);
		while (obj_list != NULL) {
			if (obj_list == &kpipe) {
				obj_found = 1;
				TC_PRINT("Found pipe object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_pipe, k_pipe,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find pipe object");
		break;
	case QUEUE:
		obj_list = SYS_TRACING_HEAD(struct k_queue, k_queue);
		while (obj_list != NULL) {
			if (obj_list == &kqueue) {
				obj_found = 1;
				TC_PRINT("Found queue object\n");
				break;
			}
			obj_list = SYS_TRACING_NEXT(struct k_queue, k_queue,
						    obj_list);
		}
		zassert_true(obj_found, "Didn't find queue object\n");
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
