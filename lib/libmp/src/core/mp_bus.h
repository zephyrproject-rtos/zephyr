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
 * @defgroup mp_bus
 * @brief Message bus for the communication between elements to application
 *
 * @{
 */

/**
 * @struct mp_bus
 * Message bus structure
 */
struct mp_bus {
	/**
	 * FIFO queue used to store messages that are not handled by any
	 * listener and can manually get using the mp_bus_pop
	 */
	struct k_fifo fifo;
	/**
	 * List of listeners registered to the bus, the message will be
	 * delivered to these listeners first
	 */
	sys_slist_t sync_listeners;
};

typedef bool (*callback_fn)(struct mp_message *message, void *data);

/**
 * @struct mp_bus_sync_listener
 * Structure representing a synchronous listener on the message bus.
 */
struct mp_bus_sync_listener {
	/** Callback function for message handling */
	callback_fn cb;
	/** Message type filter */
	enum mp_message_type filter_type;
	/** User-defined data passed to callback */
	void *user_data;
	/** Node for linked list management */
	sys_snode_t node;
};

/**
 * Initialize a message bus.
 *
 * @param bus Pointer to the struct mp_bus to initialize
 */
static inline void mp_bus_init(struct mp_bus *bus)
{
	k_fifo_init(&bus->fifo);
	sys_slist_init(&bus->sync_listeners);
}

/**
 * Post a message to the bus.
 *
 * @param bus Pointer to the struct mp_bus
 * @param message Pointer to the message to post
 * @return true if the message was posted successfully, false otherwise
 */
bool mp_bus_post(struct mp_bus *bus, struct mp_message *message);

/**
 * Flush all messages from the bus.
 *
 * @param bus Pointer to the struct mp_bus to flush
 */
void mp_bus_flush(struct mp_bus *bus);

/**
 * Peek at the last message in the bus without removing it.
 *
 * @param bus Pointer to the struct mp_bus
 * @return Pointer to the message if available, NULL otherwise
 */
struct mp_message *mp_bus_peek(struct mp_bus *bus);

/**
 * Pop the last message from the bus.
 *
 * @param bus Pointer to the struct mp_bus
 * @return Pointer to the popped message, or NULL if none available
 */
struct mp_message *mp_bus_pop(struct mp_bus *bus);

/**
 * Pop a message from the bus matching a specific type.
 *
 * This function waits for a message matching the specified type mask.
 *
 * @param bus Pointer to the struct mp_bus
 * @param type Message type mask to match
 * @return Pointer to the matching message, or NULL if none found
 */
struct mp_message *mp_bus_pop_msg(struct mp_bus *bus, enum mp_message_type type);

/**
 * Add a synchronous listener to the bus.
 *
 * @param bus Pointer to the struct mp_bus
 * @param cb Callback function to invoke when a matching message is received
 * @param type Message type to listen for
 * @param user_data User-defined data passed to the callback
 */
void mp_bus_add_sync_listener(struct mp_bus *bus, callback_fn cb, enum mp_message_type type,
			      void *user_data);

/**
 * Remove a synchronous listener from the bus.
 *
 * @param bus Pointer to the struct mp_bus
 * @param listener Pointer to the listener to remove
 */
void mp_bus_remove_sync_listener(struct mp_bus *bus, struct mp_bus_sync_listener *listener);

/** @} */

#endif /* __MP_BUS_H__ */
