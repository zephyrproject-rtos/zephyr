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

K_MSGQ_DEFINE(engine_msgq, sizeof(struct lwan_req), ENGINE_MSGQ_DEPTH, 4);

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
	struct k_sem done;
	int result;
	int ret;

	k_sem_init(&done, 0, 1);
	req->done = &done;
	req->result = &result;

	ret = k_msgq_put(&engine_msgq, req, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Failed to post engine request %d: %d", req->type, ret);
		return ret;
	}

	k_sem_take(&done, K_FOREVER);

	return result;
}

void engine_signal_result(const struct lwan_req *req, int result)
{
	*req->result = result;
	k_sem_give(req->done);
}
