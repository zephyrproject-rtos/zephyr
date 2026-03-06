/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Native LoRaWAN engine thread
 *
 * A dedicated thread serialises all MAC processing.  API calls post a
 * request to the engine via a message queue and block on a result queue
 * until the engine finishes.
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
 *        |  k_msgq_get(result_msgq)      |
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
K_MSGQ_DEFINE(join_result_msgq, sizeof(int), 1, 4);
K_MSGQ_DEFINE(send_result_msgq, sizeof(int), 1, 4);

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

int engine_post_req(const struct lwan_req *req)
{
	return k_msgq_put(&engine_msgq, req, K_NO_WAIT);
}

void engine_signal_join_result(int result)
{
	k_msgq_put(&join_result_msgq, &result, K_FOREVER);
}

void engine_signal_send_result(int result)
{
	k_msgq_put(&send_result_msgq, &result, K_FOREVER);
}

int engine_wait_join_result(void)
{
	int result;

	k_msgq_get(&join_result_msgq, &result, K_FOREVER);

	return result;
}

int engine_wait_send_result(void)
{
	int result;

	k_msgq_get(&send_result_msgq, &result, K_FOREVER);

	return result;
}
