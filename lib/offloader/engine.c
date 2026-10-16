/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/sys/zassert.h>
#include <zephyr/offloader/engine.h>

ZASSERT_MODULE(DEFAULT);

void offloader_engine_main(void *p1, void *p2, void *p3)
{
	struct offloader_engine *engine = p1;
	struct offloader_req *req;

	ARGS_UNUSED(p2, p3);

	while (true) {
		req = CONTAINER_OF(k_fifo_get(&engine->fifo, K_FOREVER), struct offloader_req,
				   node);
		req->fn(req->args);
		k_sem_give(&req->done);
	}
}

void offloader_dispatch(struct offloader_engine *engine, offloader_work_fn_t fn, void *args)
{
	struct offloader_req req = {
		.fn = fn,
		.args = args,
	};

	ZASSERT(engine != NULL && fn != NULL);
	ZASSERT(!k_is_in_isr(), "Cannot dispatch from ISR context");

	k_tid_t current_tid = k_current_get();

	if (engine->threads <= current_tid && current_tid < engine->threads + engine->num_threads) {
		/* This chceck only works because the threads are statically defined in the thread
		 * defined macro. If the threads are dynamically created, this check will not work.
		 */
		fn(args);
		return;
	}


	(void)k_sem_init(&req.done, 0, 1);
	k_fifo_put(&engine->fifo, &req.node);
	(void)k_sem_take(&req.done, K_FOREVER);
}
