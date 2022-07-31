/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DECLARE(pkt_chan);

ZBUS_SUBSCRIBER_DEFINE(producer_sub, 4);

static void producer_thread(void)
{
	const struct zbus_channel *chan;
	void *msg;

	for (uint8_t i = 0; (i < 16); ++i) {
		msg = k_malloc(i + 1);

		__ASSERT_NO_MSG(msg != NULL);

		struct external_data_msg dyn_alloc_msg = {.reference = msg, .size = i + 1};

		for (int j = 0; j < dyn_alloc_msg.size; ++j) {
			((uint8_t *)dyn_alloc_msg.reference)[j] = i;
		}

		zbus_chan_pub(&pkt_chan, &dyn_alloc_msg, K_MSEC(250));

		/* Wait for an consumer's ACK */
		if (zbus_sub_wait(&producer_sub, &chan, K_FOREVER)) {
			LOG_ERR("Ack not received!");
		}
	}
}

K_THREAD_DEFINE(producer_thread_id, 1024, producer_thread, NULL, NULL, NULL, 5, 0, 1000);
