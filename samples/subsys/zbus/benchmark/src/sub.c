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

ZBUS_CHAN_DECLARE(bm_channel);

#define SFY(i, _) s##i

#define CONSUMERS_NAME_LIST LISTIFY(CONFIG_BM_ONE_TO, SFY, (, /* separator */))

#define CREATE_OBSERVER(s) ZBUS_SUBSCRIBER_DEFINE(s, 4)

#define CREATE_OBSERVATIONS(s) ZBUS_CHAN_ADD_OBS(bm_channel, s, 3)

/* clang-format off */
FOR_EACH(CREATE_OBSERVER, (;), CONSUMERS_NAME_LIST);

FOR_EACH(CREATE_OBSERVATIONS, (;), CONSUMERS_NAME_LIST);
/* clang-format on */

static int sub_thread(void *sub_ref, void *ptr2, void *ptr3)
{
	ARG_UNUSED(ptr2);
	ARG_UNUSED(ptr3);

	const struct zbus_channel *chan;
	struct zbus_observer *sub = sub_ref;

	while (1) {
		if (zbus_sub_wait(sub, &chan, K_FOREVER) == 0) {
			if (zbus_chan_claim(chan, K_FOREVER) != 0) {
				k_oops();
			}

			if (IS_ENABLED(CONFIG_BM_FAIRPLAY)) {
				struct bm_msg message;

				memcpy(zbus_chan_msg(chan), &message, chan->message_size);

				atomic_add(&count, *((uint16_t *)message.bytes));
			} else {
				const struct bm_msg *msg_received = zbus_chan_const_msg(chan);

				atomic_add(&count, *((uint16_t *)msg_received->bytes));
			}

			zbus_chan_finish(chan);
		} else {
			k_oops();
		}
	}
	return -EFAULT;
}

#define CREATE_THREADS(name)                                                                       \
	K_THREAD_DEFINE(name##_sub_id, CONSUMER_STACK_SIZE, sub_thread, &name, NULL, NULL, 3, 0, 0);

/* clang-format off */
FOR_EACH(CREATE_THREADS, (;), CONSUMERS_NAME_LIST);
/* clang-format on */
