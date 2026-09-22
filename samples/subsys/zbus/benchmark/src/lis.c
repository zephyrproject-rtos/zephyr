/*
 * Copyright (c) 2023 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

#define CONSUMER_STACK_SIZE (CONFIG_IDLE_STACK_SIZE + CONFIG_BM_MESSAGE_SIZE)

extern atomic_t count;

static void s_cb(const struct zbus_channel *chan);

ZBUS_CHAN_DECLARE(bm_channel);

#define SFY(i, _) s##i

#define CONSUMERS_NAME_LIST LISTIFY(CONFIG_BM_ONE_TO, SFY, (, /* separator */))

#define CREATE_OBSERVER(s) ZBUS_LISTENER_DEFINE(s, s_cb)

#define CREATE_OBSERVATIONS(s) ZBUS_CHAN_ADD_OBS(bm_channel, s, 3)

/* clang-format off */
FOR_EACH(CREATE_OBSERVER, (;), CONSUMERS_NAME_LIST);

FOR_EACH(CREATE_OBSERVATIONS, (;), CONSUMERS_NAME_LIST);
/* clang-format on */

static void s_cb(const struct zbus_channel *chan)
{

	if (IS_ENABLED(CONFIG_BM_FAIRPLAY)) {
		struct bm_msg msg_received;

		memcpy(zbus_chan_msg(chan), &msg_received, chan->message_size);

		atomic_add(&count, *((uint16_t *)msg_received.bytes));
	} else {
		const struct bm_msg *msg_received = zbus_chan_const_msg(chan);

		atomic_add(&count, *((uint16_t *)msg_received->bytes));
	}
}
