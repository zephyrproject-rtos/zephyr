/*
 * Copyright (c) 2025 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

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

		 NULL,                                  /* Validator */
		 NULL,                                  /* User data */
		 ZBUS_OBSERVERS(lis_foo, alis_bar),     /* observers */
		 ZBUS_MSG_INIT(.type = EVENT_UNDEFINED) /* Initial value */
);

static void listener_callback(const struct zbus_channel *chan)
{
	if (chan != &chan_event) {
		LOG_ERR("Unexpected channel");
		return;
	}

	const struct msg_event *evt = zbus_chan_const_msg(chan);

	LOG_INF("From listener -> Evt=%d       | %s", evt->type,
		k_thread_name_get(k_current_get()));
}

ZBUS_LISTENER_DEFINE(lis_foo, listener_callback);

static void async_listener_callback(const struct zbus_channel *chan, const void *message)
{
	if (chan != &chan_event) {
		LOG_ERR("Unexpected channel");
		return;
	}

	const struct msg_event *evt = message;

	LOG_INF("From async listener -> Evt=%d | %s", evt->type,
		k_thread_name_get(k_current_get()));
}

ZBUS_ASYNC_LISTENER_DEFINE(alis_bar, async_listener_callback);

int main(void)
{
	LOG_INF("Output                       | Thread Context");
	LOG_INF("=============================================");

	for (int i = 0; i < EVENT_COUNT; ++i) {
		struct msg_event msg = {.type = i};
		zbus_chan_pub(&chan_event, &msg, K_MSEC(500));
		k_msleep(1000);
	}

	LOG_INF("=============================================");

	for (int i = 0; i < EVENT_COUNT; ++i) {
		struct msg_event msg = {.type = i};
		zbus_chan_pub(&chan_event, &msg, K_MSEC(500));
	}

	return 0;
}
