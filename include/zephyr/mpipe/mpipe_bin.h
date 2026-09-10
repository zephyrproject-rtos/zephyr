/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bin: an element that contains other elements.
 * @ingroup mpipe_bin
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_BIN_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_BIN_H_

/**
 * @defgroup mpipe_bin Bins
 * @ingroup mpipe_framework
 * @brief An element that contains other elements.
 *
 * A bin is itself an @ref mpipe_element, so a graph can be treated as one
 * element from the outside. Its job is to hold children and to forward a state
 * change to them in an order that is safe.
 *
 * That order is not the order they were added. The bin sorts its children
 * topologically by their links and walks them from the sink towards the source
 * when the transition goes up, so a downstream element is ready before anything
 * is pushed into it, and from the source towards the sink when it goes down, so
 * nothing keeps producing into an element that has already been torn down.
 * @c CONFIG_MPIPE_BIN_MAX_CHILDREN bounds the arrays that sort uses.
 *
 * A bin also owns the message channel its children report on, which is how a
 * failure deep in a graph reaches the application. See @ref mpipe_message.
 *
 * @{
 */

#include <stdint.h>

#include <zephyr/sys/dlist.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_message.h>

/**
 * @brief Bin structure
 *
 * A container element that can hold multiple child elements.
 *
 * A bin manages the state changes of its children and handles the topology
 * of the pipeline elements within it.
 */
struct mpipe_bin {
	/** Base element structure */
	struct mpipe_element element;
	/** Number of children in the bin */
	int children_num;
	/** List of children elements in the bin */
	sys_dlist_t children;
	/**
	 * @brief The bin's message bus.
	 *
	 * A one-way, out-of-band notification channel that carries
	 * @ref mpipe_message events (end-of-stream, errors) from the elements
	 * up to the application. It is "out-of-band" because it does not travel
	 * along the data path (pads/buffers); it is a side channel for control
	 * and status notifications only.
	 *
	 * Bus is a single zbus channel, owned by the bin and shared by all of
	 * its children. @ref mpipe_bin_init initializes it; it needs no teardown,
	 * as it lives entirely inside the bin and is registered nowhere else.
	 *
	 * A channel involves three roles:
	 *
	 * - Producers: elements post messages with @ref mpipe_message_post, which
	 *   locates the bus from the message's origin element. The message is
	 *   copied by value into the channel, so stack storage is fine. The
	 *   application must not post to a bus that it consumes.
	 * - Consumers: a channel can have one or more observers. Reach the channel
	 *   with @ref mpipe_element_get_bus_chan and attach with zbus_chan_add_obs();
	 *   three kinds are useful here:
	 *   - Listener: the callback runs inline in the posting thread, holding the
	 *     channel lock. Cheapest by far - no queue, no buffer - but it must not
	 *     block or change pipeline state. Define it with ZBUS_LISTENER_DEFINE().
	 *   - Message subscriber (needs CONFIG_ZBUS_MSG_SUBSCRIBER): the application
	 *     reads messages from its own thread, blocking until one arrives, and no
	 *     message is lost. Costs a net_buf per publish, taken from a pool shared
	 *     with every other zbus channel in the system. Define it with
	 *     ZBUS_MSG_SUBSCRIBER_DEFINE() and read with zbus_sub_wait_msg().
	 *   - Async listener (needs CONFIG_ZBUS_ASYNC_LISTENER, which pulls in the
	 *     message subscriber machinery): the callback runs later on the system
	 *     work queue. Define it with ZBUS_ASYNC_LISTENER_DEFINE().
	 *   Detach any observer with zbus_chan_rm_obs() before the bin goes away.
	 * - Validator: an optional zbus validator, installed by
	 *   @ref mpipe_bin_set_bus_validator, runs in the posting thread before the
	 *   message is published, and may drop messages (e.g. collapsing N
	 *   end-of-stream events into one). Keep it short and non-blocking.
	 */
	struct zbus_runtime_channel bus;

	/** @cond INTERNAL_HIDDEN */
	/* Bus channel mutable runtime state: observer bookkeeping, lock, counters. */
	struct zbus_channel_data chan_data;
	/* Bus channel message buffer: holds the last published mpipe_message. */
	struct mpipe_message chan_msg;
	/** @endcond */
};

/**
 * @brief Initialize a bin
 *
 * Initializes the bin structure and sets up the necessary function pointers
 * and data structures.
 *
 * @param bin Pointer to the @ref mpipe_bin to initialize.
 * @param id  Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_bin_init(struct mpipe_bin *bin, uint8_t id);

/**
 * @brief Install a validator and user data on the bin's bus.
 *
 * @ref mpipe_bin_init brings the channel up with no validator, since a bin has
 * no opinion on the messages passing through it. An element wrapping a bin (a
 * pipeline, say) uses this to install its own.
 *
 * Only those two fields change, under the channel's own lock. Observers already
 * attached stay attached, and may be attached before or after this call.
 *
 * @param bin           Pointer to the @ref mpipe_bin.
 * @param bus_validator Message validator to install, or NULL for no validation.
 * @param user_data     User data associated with the bus,
 *                      retrievable from the validator/observers via
 *                      zbus_chan_user_data(), or NULL if unused.
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_bin_set_bus_validator(struct mpipe_bin *bin, zbus_validator bus_validator,
				void *user_data);

/**
 * @brief Add elements to a bin
 *
 * Adds the given element(s) to the bin.
 *
 * An element can only be added to one bin. Element ids must be unique within
 * the bin.
 *
 * The function accepts a variable number of elements, terminated by NULL.
 *
 * Adding an element leaves its pads alone, so elements may be added before or
 * after they are linked.
 *
 * @param bin Pointer to the @ref mpipe_bin to add elements to
 * @param element First @ref mpipe_element to add
 * @param ... Additional mpipe_element pointers, terminated by NULL
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_bin_add(struct mpipe_bin *bin, struct mpipe_element *element, ...);

/**
 * @brief Bin state change function
 *
 * Handles state changes for the bin by propagating the state change to all
 * child elements in the appropriate order. The bin manages the topology
 * and ensures proper sequencing of state changes.
 *
 * @param element Pointer to the @ref mpipe_element (bin) changing state
 * @param transition The state transition being performed
 *
 * @return State change return value indicating success, failure, or async operation
 */
enum mpipe_state_change_return mpipe_bin_change_state_func(struct mpipe_element *element,
							   enum mpipe_state_change transition);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_BIN_H_ */
