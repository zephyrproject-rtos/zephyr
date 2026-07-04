/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Message Bus header file.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_BUS_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_BUS_H_

/**
 * @defgroup mp_bus Message Bus
 * @ingroup mp_core
 * @brief Message bus for the communication between elements to application
 *
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/slist.h>

#include <zephyr/mp/core/mp_message.h>

/**
 * @brief Message type filter mask matching any message type.
 *
 * Kept as a macro (not an enumerator) so that the enum below stays
 * within the range of int and filter masks are plain uint32_t.
 */
#define MP_MESSAGE_ANY UINT32_MAX

/**
 * @brief Callback function type for bus message listeners.
 *
 * @param message Pointer to the received message.
 * @param user_data User-defined data passed during listener registration.
 *
 * @retval true  Message was handled.
 * @retval false Message was not handled.
 */
typedef bool (*callback_fn)(struct mp_message *message, void *user_data);

/**
 * @struct mp_bus_sync_listener
 * Structure representing a synchronous listener on the message bus.
 */
struct mp_bus_sync_listener {
	/** Callback function for message handling */
	callback_fn cb;
	/** Message type filter */
	uint32_t filter_mask;
	/** User-defined data passed to callback */
	void *user_data;
	/** Node for singly-linked list (internal use) */
	sys_snode_t node;
};

/**
 * @struct mp_bus
 * Message bus structure
 */
struct mp_bus {
	/** Message queue used to store unhandled messages */
	struct k_msgq msgq;
	/** Backing buffer for the message queue */
	char msgq_buf[sizeof(struct mp_message) * CONFIG_MP_BUS_QUEUE_DEPTH]
		__aligned(__alignof__(struct mp_message));
	/** List of synchronous listeners registered to the bus */
	sys_slist_t sync_listeners;
};

/**
 * @brief Initialize a message bus
 *
 * @param bus Pointer to the bus to initialize
 */
static inline void mp_bus_init(struct mp_bus *bus)
{
	k_msgq_init(&bus->msgq, bus->msgq_buf, sizeof(struct mp_message),
		    CONFIG_MP_BUS_QUEUE_DEPTH);
	sys_slist_init(&bus->sync_listeners);
}

/**
 * @brief Post a message to the bus.
 *
 * @param bus Pointer to the bus.
 * @param message Pointer to the message to post.
 * @retval 0 on success (message queued or consumed by a listener)
 * @retval -EINVAL if @p bus or @p message is NULL
 * @retval -ENOMSG if the queue is full
 */
int mp_bus_post(struct mp_bus *bus, struct mp_message *message);

/**
 * @brief Pop a message from the bus matching a given type filter
 *
 * @param bus Pointer to the bus
 * @param filter_mask Message type filter mask
 * @param out Pointer to output message buffer
 * @retval 0 on success
 * @retval -EINVAL if @p bus or @p out is NULL
 */
int mp_bus_pop_msg(struct mp_bus *bus, uint32_t filter_mask, struct mp_message *out);

/**
 * @brief Pop any message from the bus
 *
 * Blocks until a message is available
 *
 * @param bus Pointer to the bus
 * @param out Pointer to output message buffer
 * @retval 0 on success
 * @retval -EINVAL if @p bus or @p out is NULL
 */
int mp_bus_pop(struct mp_bus *bus, struct mp_message *out);

/**
 * @brief Peek at the head message without removing it
 *
 * @param bus Pointer to the bus
 * @param out Pointer to output message buffer
 * @retval 0 on success
 * @retval -EINVAL if @p bus or @p out is NULL
 * @retval -ENOMSG if the queue is empty
 */
int mp_bus_peek(struct mp_bus *bus, struct mp_message *out);

/**
 * @brief Flush all messages from the bus
 *
 * @param bus Pointer to the bus
 * @retval 0 on success
 * @retval -EINVAL if @p bus is NULL
 */
int mp_bus_flush(struct mp_bus *bus);

/**
 * @brief Add a synchronous listener to the bus
 *
 * @param bus Pointer to the bus
 * @param listener Pointer to the caller-owned listener to register
 * @retval 0 on success
 * @retval -EINVAL if @p bus, @p listener or @p listener->cb is NULL
 */
int mp_bus_add_sync_listener(struct mp_bus *bus, struct mp_bus_sync_listener *listener);

/**
 * @brief Remove a synchronous listener from the bus.
 *
 * @param bus Pointer to the bus.
 * @param listener Pointer to the listener to remove.
 * @retval 0 on success
 * @retval -EINVAL if @p bus or @p listener is NULL
 * @retval -ENOENT if the listener was not registered on this bus
 */
int mp_bus_remove_sync_listener(struct mp_bus *bus, struct mp_bus_sync_listener *listener);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_BUS_H_ */
