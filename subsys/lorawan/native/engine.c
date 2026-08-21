/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native LoRaWAN engine thread
 *
 * A dedicated thread serialises all MAC processing.  API calls post a
 * request to the engine via a message queue and block on a per-request
 * completion until the engine finishes.
 *
 *  Application thread(s)                Engine thread
 *  ~~~~~~~~~~~~~~~~~~~~~~               ~~~~~~~~~~~~~
 *
 *  lorawan_join() / lorawan_send()
 *        |
 *        |  k_msgq_put(engine_msgq)
 *        +------------------------------>+
 *        |                               | mac_process_req()
 *        |                               |   mac_do_join() / mac_do_send()
 *        |                               |     radio_tx()
 *        |                               |     radio_rx()  (RX1, RX2)
 *        |                               |     mac_dispatch_downlink()
 *        |                               |       \---> k_work_submit()
 *        |                               |              (system workqueue)
 *        |  k_sem_take(req.done)         |
 *        +<------------------------------+
 *        |                               |
 *     return ret                   k_msgq_get() (next)
 *
 *                                System workqueue
 *                                ~~~~~~~~~~~~~~~~
 *                                dl_work_handler()
 *                                  cb->cb(port, ...)
 */

#include <zephyr/kernel.h>

#include "engine.h"
#include "mac/mac.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_engine, CONFIG_LORAWAN_LOG_LEVEL);

#define ENGINE_MSGQ_DEPTH	8

K_MSGQ_DEFINE_STATIC_TYPE(engine_msgq, struct lwan_req, ENGINE_MSGQ_DEPTH);

static K_THREAD_STACK_DEFINE(engine_stack,
			     CONFIG_LORAWAN_NATIVE_ENGINE_STACK_SIZE);
static struct k_thread engine_thread;

static void engine_thread_fn(void *p1, void *p2, void *p3)
{
	struct lwan_ctx *ctx = p1;
	struct lwan_req req;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Native LoRaWAN engine started");

	while (true) {
		k_msgq_get(&engine_msgq, &req, K_FOREVER);

		LOG_DBG("Engine request: %d", req.type);

		mac_process_req(ctx, &req);
	}
}

void engine_init(struct lwan_ctx *ctx)
{
	k_thread_create(&engine_thread, engine_stack,
			K_THREAD_STACK_SIZEOF(engine_stack),
			engine_thread_fn, ctx, NULL, NULL,
			CONFIG_LORAWAN_NATIVE_ENGINE_PRIORITY,
			0, K_NO_WAIT);
	k_thread_name_set(&engine_thread, "lorawan_engine");
}

int engine_post_req_wait(struct lwan_req *req)
{
	struct k_poll_signal done;
	struct k_poll_event event;
	unsigned int signaled;
	int result;
	int ret;

	/* A poll signal is used rather than a semaphore as it, unlike kernel
	 * objects, may live on the stack.
	 */
	k_poll_signal_init(&done);
	k_poll_event_init(&event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &done);
	req->done = &done;

	ret = k_msgq_put(&engine_msgq, req, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Failed to post engine request %d: %d", req->type, ret);
		return ret;
	}

	(void)k_poll(&event, 1, K_FOREVER);
	k_poll_signal_check(&done, &signaled, &result);

	return result;
}

void engine_signal_result(const struct lwan_req *req, int result)
{
	k_poll_signal_raise(req->done, result);
}
