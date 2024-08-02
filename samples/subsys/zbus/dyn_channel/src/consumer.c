/*
 * Copyright (c) 2022 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
LOG_MODULE_DECLARE(zbus, CONFIG_ZBUS_LOG_LEVEL);

ZBUS_CHAN_DECLARE(pkt_chan, data_ready_chan, ack);

ZBUS_SUBSCRIBER_DEFINE(consumer_sub, 4);

static void consumer_thread(void)
{
	struct external_data_msg *dyn_alloc_msg;
	const struct zbus_channel *chan;

	while (!zbus_sub_wait(&consumer_sub, &chan, K_FOREVER)) {
		LOG_INF("Channel %s\n", zbus_chan_name(chan));

		__ASSERT_NO_MSG(chan == &data_ready_chan);

		if (!zbus_chan_claim(&pkt_chan, K_MSEC(100))) {
			dyn_alloc_msg = zbus_chan_msg(&pkt_chan);

			LOG_WRN("size=%02d", (int)dyn_alloc_msg->size);
			LOG_HEXDUMP_WRN(dyn_alloc_msg->reference, dyn_alloc_msg->size, "Content");

			/* Cleanup the dynamic memory after using that */
			k_free(dyn_alloc_msg->reference);
			dyn_alloc_msg->reference = NULL;
			dyn_alloc_msg->size = 0;

			zbus_chan_finish(&pkt_chan);
		}
		struct ack_msg a = {1};

		zbus_chan_pub(&ack, &a, K_MSEC(250));
	}
}

K_THREAD_DEFINE(consumer_thread_id, 1024, consumer_thread, NULL, NULL, NULL, 4, 0, 1000);
