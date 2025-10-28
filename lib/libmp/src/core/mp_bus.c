/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mp_bus.h"

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
		if (message->type & listener->filter_type) {
			ret |= listener->cb(message, listener->user_data);
		}
	}

	return ret ? MP_BUS_DROP : MP_BUS_PASS;
}

bool mp_bus_post(struct mp_bus *bus, struct mp_message *message)
{

	enum mp_bus_sync_reply reply = MP_BUS_PASS;

	if (bus == NULL || message == NULL) {
		return false;
	}

	/* Step 1: Notify the sync handler first if any */
	if (!sys_slist_is_empty(&bus->sync_listeners)) {
		reply = mp_bus_sync_handler(bus, message);
	}

	/* Step 2: Put message to FIFO if not dropped */
	if (reply != MP_BUS_DROP) {
		k_fifo_put(&bus->fifo, message);
	}

	return true;
}

struct mp_message *mp_bus_pop_msg(struct mp_bus *bus, enum mp_message_type type)
{
	__ASSERT_NO_MSG(bus != NULL);

	struct mp_message *message = NULL;

	while ((message = k_fifo_get(&bus->fifo, K_FOREVER)) != NULL) {
		if (message->type & type) {
			break;
		}

		/* Discard unmatched message */
		mp_message_destroy(message);
	}

	return message;
}

struct mp_message *mp_bus_pop(struct mp_bus *bus)
{
	return mp_bus_pop_msg(bus, MP_MESSAGE_ANY);
}

struct mp_message *mp_bus_peek(struct mp_bus *bus)
{
	return bus != NULL ? k_fifo_peek_head(&bus->fifo) : NULL;
}
void mp_bus_flush(struct mp_bus *bus)
{
	struct mp_message *message;

	__ASSERT_NO_MSG(bus != NULL);

	/** Drain the FIFO and free all messages */
	while ((message = k_fifo_get(&bus->fifo, K_NO_WAIT)) != NULL) {
		mp_message_destroy(message);
	}
}

void mp_bus_add_sync_listener(struct mp_bus *bus, callback_fn cb, enum mp_message_type type,
			      void *user_data)
{

	struct mp_bus_sync_listener *listener = k_malloc(sizeof(struct mp_bus_sync_listener));

	__ASSERT_NO_MSG(listener != NULL);
	listener->cb = cb;
	listener->filter_type = type;
	listener->user_data = user_data;
	sys_slist_append(&bus->sync_listeners, &listener->node);
}

void mp_bus_remove_sync_listener(struct mp_bus *bus, struct mp_bus_sync_listener *listener)
{
	__ASSERT_NO_MSG(bus != NULL && listener != NULL);
	sys_slist_find_and_remove(&bus->sync_listeners, &listener->node);
	k_free(listener);
}
