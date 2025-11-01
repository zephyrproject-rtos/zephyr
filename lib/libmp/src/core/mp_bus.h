/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Message Bus header file.
 */

#ifndef __MP_BUS_H__
#define __MP_BUS_H__

#include <stdint.h>

#include <zephyr/kernel.h>

#include "mp_messages.h"

/**
 * @defgroup MpBus
 * @brief Message bus for the communication between elements to application
 *
 * @{
 */

/**
 * Callback function for synchronous message listeners.
 *
 * @typedef MpBusSyncListenerCallback
 *
 * @param message Pointer to the received message
 * @param data User-defined data passed to the callback
 * @return true if the message was handled, false otherwise
 */
typedef bool (*MpBusSyncListenerCallback)(MpMessage *message, void *data);

/**
 * @struct MpBus
 * Message bus structure
 */
typedef struct {
	/**
	 * FIFO queue used to store messages that are not handled by any
	 * listener and can manually get using the @ref mp_bus_pop
	 */
	struct k_fifo fifo;
	/**
	 * List of listeners registered to the bus, the message will be
	 * delivered to these listeners first
	 */
	sys_slist_t sync_listeners;
} MpBus;

/**
 * @struct MpBusSyncListener
 * Structure representing a synchronous listener on the message bus.
 */
typedef struct {
	/** Callback function for message handling */
	MpBusSyncListenerCallback callback;
	/** Message type filter */
	MpMessageType filter_type;
	/** User-defined data passed to callback */
	void *user_data;
	/** Node for linked list management */
	sys_snode_t node;
} MpBusSyncListener;

/**
 * Initialize a message bus.
 *
 * @param bus Pointer to the MpBus to initialize
 */
static inline void mp_bus_init(MpBus *bus)
{
	k_fifo_init(&bus->fifo);
	sys_slist_init(&bus->sync_listeners);
}

/**
 * Destroy a message bus and release its resources.
 *
 * @param bus Pointer to the MpBus to destroy
 */
void mp_bus_destroy(MpBus *bus);

/**
 * Post a message to the bus.
 *
 * @param bus Pointer to the MpBus
 * @param message Pointer to the message to post
 * @return true if the message was posted successfully, false otherwise
 */
bool mp_bus_post(MpBus *bus, MpMessage *message);

/**
 * Flush all messages from the bus.
 *
 * @param bus Pointer to the MpBus to flush
 */
void mp_bus_flush(MpBus *bus);

/**
 * Peek at the last message in the bus without removing it.
 *
 * @param bus Pointer to the MpBus
 * @return Pointer to the message if available, NULL otherwise
 */
MpMessage *mp_bus_peek(MpBus *bus);

/**
 * Pop the last message from the bus.
 *
 * @param bus Pointer to the MpBus
 * @return Pointer to the popped message, or NULL if none available
 */
MpMessage *mp_bus_pop(MpBus *bus);

/**
 * Pop a message from the bus matching a specific type.
 *
 * This function waits for a message matching the specified type mask.
 *
 * @param bus Pointer to the MpBus
 * @param type Message type mask to match
 * @return Pointer to the matching message, or NULL if none found
 */
MpMessage *mp_bus_pop_msg(MpBus *bus, MpMessageType type);

/**
 * Add a synchronous listener to the bus.
 *
 * @param bus Pointer to the MpBus
 * @param func Callback function to invoke when a matching message is received
 * @param type Message type to listen for
 * @param user_data User-defined data passed to the callback
 */
void mp_bus_add_sync_listener(MpBus *bus, MpBusSyncListenerCallback func, MpMessageType type,
			      void *user_data);

/**
 * Remove a synchronous listener from the bus.
 *
 * @param bus Pointer to the MpBus
 * @param listener Pointer to the listener to remove
 */
void mp_bus_remove_sync_listener(MpBus *bus, MpBusSyncListener *listener);

/** @} */

#endif /* __MP_BUS_H__ */
