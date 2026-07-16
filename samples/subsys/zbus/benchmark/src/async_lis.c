/*
 * Copyright (c) 2026 Rodrigo Peixoto <rodrigopex@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#include "messages.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/zbus/zbus.h>

extern atomic_t count;

static void s_cb(const struct zbus_channel *chan, const void *msg);

ZBUS_CHAN_DECLARE(bm_channel);

#define SFY(i, _) s##i

#define CONSUMERS_NAME_LIST LISTIFY(CONFIG_BM_ONE_TO, SFY, (, /* separator */))

#define CREATE_OBSERVER(s) ZBUS_ASYNC_LISTENER_DEFINE(s, s_cb)

#define CREATE_OBSERVATIONS(s) ZBUS_CHAN_ADD_OBS(bm_channel, s, 3)

/* clang-format off */
FOR_EACH(CREATE_OBSERVER, (;), CONSUMERS_NAME_LIST);

FOR_EACH(CREATE_OBSERVATIONS, (;), CONSUMERS_NAME_LIST);
/* clang-format on */

static void s_cb(const struct zbus_channel *chan, const void *msg)
{
	ARG_UNUSED(chan);

	const struct bm_msg *msg_received = msg;

	atomic_add(&count, *((uint16_t *)msg_received->bytes));
}
