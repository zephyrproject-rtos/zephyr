/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/mp/core/mp_bus.h>

enum mp_bus_sync_reply {
	MP_BUS_DROP = 0,
	MP_BUS_PASS = 1,
};

/**
 * Sync handler for the bus.
 * This function is called when a message is posted to the bus.
 * It delivers the message to the correct listener based on the filter type.
 * If a listener consumes the message, it returns MP_BUS_DROP,
 * otherwise it returns MP_BUS_PASS.
 *

 * @param bus: a struct mp_bus to handle the message
 * @param message: the message to handle
 */
static enum mp_bus_sync_reply mp_bus_sync_handler(struct mp_bus *bus, struct mp_message *message)
{

	struct mp_bus_sync_listener *listener;
	bool ret = false;

	/* Deliver the message to the correct listener */
	SYS_SLIST_FOR_EACH_CONTAINER(&bus->sync_listeners, listener, node) {
		if (message->type & listener->filter_mask) {
			ret |= listener->cb(message, listener->user_data);
		}
	}

	return ret ? MP_BUS_DROP : MP_BUS_PASS;
}

int mp_bus_post(struct mp_bus *bus, struct mp_message *message)
{
	enum mp_bus_sync_reply reply = MP_BUS_PASS;

	if (bus == NULL || message == NULL) {
		return -EINVAL;
	}

	/* Step 1: Notify sync listeners first */
	reply = mp_bus_sync_handler(bus, message);

	/* Step 2: Queue message if not consumed */
	if (reply == MP_BUS_PASS) {
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

int mp_bus_add_sync_listener(struct mp_bus *bus, struct mp_bus_sync_listener *listener)
{
	if (bus == NULL || listener == NULL || listener->cb == NULL) {
		return -EINVAL;
	}

	sys_slist_append(&bus->sync_listeners, &listener->node);

	return 0;
}

int mp_bus_remove_sync_listener(struct mp_bus *bus, struct mp_bus_sync_listener *listener)
{
	bool found;

	if (bus == NULL || listener == NULL) {
		return -EINVAL;
	}

	found = sys_slist_find_and_remove(&bus->sync_listeners, &listener->node);

	return found ? 0 : -ENOENT;
}
