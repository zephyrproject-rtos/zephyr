/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/mp/mp_bus.h>

/**
 * Run the sync handler for a posted message and decide its fate.
 *
 * The handler (if any) runs inline in the posting thread. Its reply decides
 * whether the message is enqueued for the asynchronous consumer. With no
 * handler installed the default is MP_BUS_PASS (enqueue).
 *
 * @param bus: the bus the message was posted to
 * @param message: the message to handle
 * @return the sync reply, one of enum mp_bus_sync_reply
 */
static enum mp_bus_sync_reply mp_bus_sync_handler(struct mp_bus *bus, struct mp_message *message)
{
	if (bus->sync_handler == NULL) {
		return MP_BUS_PASS;
	}

	return bus->sync_handler(bus, message, bus->sync_handler_user_data);
}

int mp_bus_post(struct mp_bus *bus, struct mp_message *message)
{
	enum mp_bus_sync_reply reply;

	if (bus == NULL || message == NULL) {
		return -EINVAL;
	}

	/* Step 1: run the sync handler in the caller's thread */
	reply = mp_bus_sync_handler(bus, message);

	/*
	 * Step 2: enqueue unless the handler dropped the message. MP_BUS_ASYNC
	 * is reserved for future use and is currently treated like MP_BUS_PASS.
	 */
	if (reply != MP_BUS_DROP) {
		return k_msgq_put(&bus->msgq, message, K_NO_WAIT);
	}

	return 0;
}

int mp_bus_pop_msg(struct mp_bus *bus, uint32_t filter_mask, struct mp_message *out)
{
	struct mp_message tmp;
	int ret;

	if (bus == NULL || out == NULL) {
		return -EINVAL;
	}

	while (1) {
		ret = k_msgq_get(&bus->msgq, &tmp, K_FOREVER);
		if (ret != 0) {
			return ret;
		}

		if ((uint32_t)tmp.type & filter_mask) {
			*out = tmp;
			return 0;
		}
	}
}

int mp_bus_pop(struct mp_bus *bus, struct mp_message *out)
{
	return mp_bus_pop_msg(bus, MP_MESSAGE_ANY, out);
}

int mp_bus_peek(struct mp_bus *bus, struct mp_message *out)
{
	if (bus == NULL || out == NULL) {
		return -EINVAL;
	}

	return k_msgq_peek(&bus->msgq, out);
}

int mp_bus_flush(struct mp_bus *bus)
{
	if (bus == NULL) {
		return -EINVAL;
	}

	k_msgq_purge(&bus->msgq);

	return 0;
}

int mp_bus_set_sync_handler(struct mp_bus *bus, mp_bus_sync_handler_fn handler, void *user_data)
{
	if (bus == NULL) {
		return -EINVAL;
	}

	bus->sync_handler = handler;
	bus->sync_handler_user_data = user_data;

	return 0;
}
