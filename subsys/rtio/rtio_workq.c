/*
 * Copyright (c) 2024 Croxel Inc.
 * Copyright (c) 2025 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/work.h>
#include <zephyr/kernel.h>

K_MEM_SLAB_DEFINE_STATIC(rtio_work_items_slab,
			 sizeof(struct rtio_work_req),
			 CONFIG_RTIO_WORKQ_POOL_ITEMS,
			 4);
static K_THREAD_STACK_ARRAY_DEFINE(rtio_workq_threads_stack,
				   CONFIG_RTIO_WORKQ_THREADS_POOL,
				   CONFIG_RTIO_WORKQ_THREADS_POOL_STACK_SIZE);
static struct k_thread rtio_work_threads[CONFIG_RTIO_WORKQ_THREADS_POOL];
static K_QUEUE_DEFINE(rtio_workq);

struct rtio_work_req *rtio_work_req_alloc(void)
{
	struct rtio_work_req *req;
	int err;

	err = k_mem_slab_alloc(&rtio_work_items_slab, (void **)&req, K_NO_WAIT);
	if (err) {
		return NULL;
	}

	return req;
}

void rtio_work_req_submit(struct rtio_work_req *req,
			  struct rtio_iodev_sqe *iodev_sqe,
			  rtio_work_submit_t handler)
{
	if (!req) {
		return;
	}

	if (!iodev_sqe || !handler) {
		k_mem_slab_free(&rtio_work_items_slab, req);
		return;
	}

	req->iodev_sqe = iodev_sqe;
	req->handler = handler;

	/** For now we're simply treating this as a FIFO queue. It may be
	 * desirable to expand this to handle queue ordering based on RTIO
	 * SQE priority.
	 */
	k_queue_append(&rtio_workq, req);
}

uint32_t rtio_work_req_used_count_get(void)
{
	return k_mem_slab_num_used_get(&rtio_work_items_slab);
}

static void rtio_workq_thread_fn(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (true) {
		struct rtio_work_req *req = k_queue_get(&rtio_workq, K_FOREVER);

		if (req != NULL) {
			req->handler(req->iodev_sqe);

			k_mem_slab_free(&rtio_work_items_slab, req);
		}
	}
}

static int static_init(void)
{
	for (size_t i = 0 ; i < ARRAY_SIZE(rtio_work_threads) ; i++) {
		k_thread_create(&rtio_work_threads[i],
				rtio_workq_threads_stack[i],
				CONFIG_RTIO_WORKQ_THREADS_POOL_STACK_SIZE,
				rtio_workq_thread_fn,
				NULL, NULL, NULL,
				CONFIG_RTIO_WORKQ_THREADS_POOL_PRIO,
				0,
				K_NO_WAIT);
	}

	return 0;
}

SYS_INIT(static_init, POST_KERNEL, 1);
