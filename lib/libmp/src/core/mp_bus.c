/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mp_bus.h"

typedef enum {
	MP_BUS_DROP = 0,
	MP_BUS_PASS = 1,
} MpBusSyncReply;

/**
 * Sync handler for the bus.
 * This function is called when a message is posted to the bus.
 * It delivers the message to the correct listener based on the filter type.
 * If a listener consumes the message, it returns MP_BUS_DROP,
 * otherwise it returns MP_BUS_PASS.
 *
 * @param bus: a @ref MpBus to handle the message
 * @param message: the message to handle
 */
static MpBusSyncReply mp_bus_sync_handler(MpBus *bus, MpMessage *message)
{
	MpBusSyncListener *listener;
	bool ret = false;

	/* Deliver the message to the correct listener */
	SYS_SLIST_FOR_EACH_CONTAINER(&bus->sync_listeners, listener, node) {
		if (MP_MESSAGE_TYPE(message) & listener->filter_type) {
			ret |= listener->callback(message, listener->user_data);
		}
	}

	return ret ? MP_BUS_DROP : MP_BUS_PASS;
}

void mp_bus_destroy(MpBus *bus)
{
	void *message;

	/** Drain the FIFO and free all messages */
	while ((message = k_fifo_get(&bus->fifo, K_NO_WAIT)) != NULL) {
		k_free(message);
	}

	/** Now free the bus itself */
	k_free(bus);
}

bool mp_bus_post(MpBus *bus, MpMessage *message)
{
	MpBusSyncReply reply = MP_BUS_PASS;

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

MpMessage *mp_bus_pop_msg(MpBus *bus, MpMessageType type)
{
	__ASSERT_NO_MSG(bus != NULL);

	MpMessage *message = NULL;
	while ((message = k_fifo_get(&bus->fifo, K_FOREVER)) != NULL) {
		if (MP_MESSAGE_TYPE(message) & type) {
			break;
		} else {
			/* Discard unmatched message */
			mp_message_destroy(message);
		}
	}

	return message;
}

MpMessage *mp_bus_pop(MpBus *bus)
{
	return mp_bus_pop_msg(bus, MP_MESSAGE_ANY);
}

MpMessage *mp_bus_peek(MpBus *bus)
{
	MpMessage *message;

	if ((message = k_fifo_peek_head(&bus->fifo)) != NULL) {
		return message;
	}

	return NULL;
}

void mp_bus_flush(MpBus *bus)
{
	MpMessage *message;

	__ASSERT_NO_MSG(bus != NULL);

	/** Drain the FIFO and free all messages */
	while ((message = k_fifo_get(&bus->fifo, K_NO_WAIT)) != NULL) {
		k_free(message);
	}
}

void mp_bus_add_sync_listener(MpBus *bus, MpBusSyncListenerCallback func, MpMessageType type,
			      void *user_data)
{
	MpBusSyncListener *listener = k_malloc(sizeof(MpBusSyncListener));

	__ASSERT_NO_MSG(listener != NULL);
	listener->callback = func;
	listener->filter_type = type;
	listener->user_data = user_data;
	sys_slist_append(&bus->sync_listeners, &listener->node);
}

void mp_bus_remove_sync_listener(MpBus *bus, MpBusSyncListener *listener)
{
	__ASSERT_NO_MSG(bus != NULL && listener != NULL);
	sys_slist_find_and_remove(&bus->sync_listeners, &listener->node);
	k_free(listener);
}
