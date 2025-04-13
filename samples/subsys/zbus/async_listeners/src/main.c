/*
 * Copyright (c) 2025 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

#define WQ_STACK_SIZE 1024
#define WQ_PRIORITY   5

enum event_type {
	EVENT_UNDEFINED,
	EVENT1,
	EVENT2,
	EVENT3,
	EVENT_COUNT
};

struct msg_event {
	enum event_type type;
};

ZBUS_CHAN_DEFINE(chan_event,       /* Name */
		 struct msg_event, /* Message type */

		 NULL,                                        /* Validator */
		 NULL,                                        /* User data */
		 ZBUS_OBSERVERS(lis_foo, alis_bar, alis_baz), /* observers */
		 ZBUS_MSG_INIT(.type = EVENT_UNDEFINED)       /* Initial value */
);

static void listener_callback(const struct zbus_channel *chan)
{
	if (chan != &chan_event) {
		LOG_ERR("Unexpected channel");
		return;
	}

	LOG_INF("From listener -> Evt=%d       | %s",
		((const struct msg_event *)zbus_chan_const_msg(chan))->type,
		k_thread_name_get(k_current_get()));
}

ZBUS_LISTENER_DEFINE(lis_foo, listener_callback);

static void async_listener_callback(const struct zbus_channel *chan, const void *message)
{
	if (chan != &chan_event) {
		LOG_ERR("Unexpected channel");
		return;
	}

	LOG_INF("From async listener -> Evt=%d | %s", ((const struct msg_event *)message)->type,
		k_thread_name_get(k_current_get()));
}

ZBUS_ASYNC_LISTENER_DEFINE(alis_bar, async_listener_callback);

/* This second async listener uses an isolated work queue to process the async data. */
K_THREAD_STACK_DEFINE(wq_stack_area, WQ_STACK_SIZE);
struct k_work_q my_work_q;
ZBUS_ASYNC_LISTENER_DEFINE(alis_baz, async_listener_callback);

int main(void)
{
	/* Steps necessary to set an isolated queue to an async listener: setup BEGIN */
	const struct k_work_queue_config cfg = {.name = "My work queue"};

	k_work_queue_init(&my_work_q);

	k_work_queue_start(&my_work_q, wq_stack_area, K_THREAD_STACK_SIZEOF(wq_stack_area),
			   WQ_PRIORITY, &cfg);

	zbus_async_listener_set_work_queue(&alis_baz, &my_work_q);
	/* setup END */

	LOG_INF("Output                       | Thread Context");
	LOG_INF("=============================================");

	for (int i = 0; i < EVENT_COUNT; ++i) {
		struct msg_event msg = {.type = i};

		zbus_chan_pub(&chan_event, &msg, K_MSEC(500));
		k_msleep(1000);
	}

	LOG_INF("=============================================");

	for (int i = 0; i < EVENT_COUNT; ++i) {
		struct msg_event msg = {.type = 10 + i};

		zbus_chan_pub(&chan_event, &msg, K_MSEC(500));
	}

	return 0;
}
