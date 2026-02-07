/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_pad.
 */

#ifndef __MP_PAD_H__
#define __MP_PAD_H__

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#include "mp_buffer.h"
#include "mp_caps.h"
#include "mp_event.h"
#include "mp_object.h"
#include "mp_query.h"
#include "mp_task.h"

/**
 * @{
 */

#define MP_PAD(self) ((struct mp_pad *)self)

/**
 * @brief The direction of a pad
 */
enum mp_pad_direction {
	/** Direction is unknown */
	MP_PAD_UNKNOWN,
	/** The pad is a source pad */
	MP_PAD_SRC,
	/** The pad is a sink pad */
	MP_PAD_SINK
};

/**
 * @brief The operating mode of a @ref struct mp_pad
 *
 * Defines if the pad operates in push or pull mode or none of them.
 */
enum mp_pad_mode {
	/** Pad will not handle dataflow */
	MP_PAD_MODE_NONE,
	/** Pad handles dataflow in push mode */
	MP_PAD_MODE_PUSH,
	/** Pad handles dataflow in pull mode */
	MP_PAD_MODE_PULL
};

/**
 * @brief The presence of a pad
 */
enum mp_pad_presence {
	/** The pad is always present */
	MP_PAD_ALWAYS,
	/** The pad will present depending on the media stream */
	MP_PAD_SOMETIMES,
	/** The pad is only available on request */
	MP_PAD_REQUEST
};

/**
 * @brief Pad flags
 */
enum mp_pad_flags {
	/** Pad needs to pass through the negotiation process */
	MP_PAD_FLAG_NEGOTIATE = BIT(0),
};

/**
 * @brief Pad structure
 *
 * The pad structure represents a connection point of an element.
 * Pads are used to negotiate capabilities and transfer data between elements.
 */
struct mp_pad {
	/** Base object */
	struct mp_object object;
	/** The direction of the pad, cannot change after creating the pad */
	enum mp_pad_direction direction;
	/** The presence of the pad */
	enum mp_pad_presence presence;
	/** The operating mode of the pad */
	enum mp_pad_mode mode;
	/** Pointer to the peer pad this pad is linked to */
	struct mp_pad *peer;
	/** Capabilities of the pad */
	struct mp_caps *caps;
	/** Task associated with this pad */
	struct mp_task task;
	/** Chain function for handling buffers */
	bool (*chainfn)(struct mp_pad *pad, struct mp_buffer *buffer);
	/** Query function for handling queries */
	bool (*queryfn)(struct mp_pad *pad, struct mp_query *query);
	/** Event function for handling events */
	bool (*eventfn)(struct mp_pad *pad, struct mp_event *event);
	/** Link function for link validation and doing some pad's specific stuffs */
	bool (*linkfn)(struct mp_pad *pad);
};

/**
 * @brief Create a new pad dynamically
 *
 * Creates a new @ref struct mp_pad with the specified parameters.
 *
 * @param id Unique ID of the pad instance in the element
 * @param direction Direction of the pad (@ref enum mp_pad_direction)
 * @param presence Presence of the pad (@ref enum mp_pad_presence)
 * @param caps Capabilities of the pad (@ref struct mp_caps)
 * @return Pointer to the newly created @ref struct mp_pad, or NULL on failure
 */
struct mp_pad *mp_pad_new(uint8_t id, enum mp_pad_direction direction,
			  enum mp_pad_presence presence, struct mp_caps *caps);

/**
 * @brief Initialize a pad
 *
 * Initializes an existing @ref struct mp_pad structure with the specified parameters.
 *
 * @param pad Pointer to the @ref struct mp_pad to initialize
 * @param id Unique ID of the pad instance in the element
 * @param direction Direction of the pad (@ref enum mp_pad_direction)
 * @param presence Presence of the pad (@ref enum mp_pad_presence)
 * @param caps Capabilities of the pad (@ref struct mp_caps)
 */
void mp_pad_init(struct mp_pad *pad, uint8_t id, enum mp_pad_direction direction,
		 enum mp_pad_presence presence, struct mp_caps *caps);

/**
 * @brief Link two pads together
 *
 * Links a source pad to a sink pad, establishing a connection for data flow.
 * Both pads will have their peer pointers set to each other.
 *
 * @param srcpad Source pad to link
 * @param sinkpad Sink pad to link
 * @return true if the pads were successfully linked, false otherwise
 */
bool mp_pad_link(struct mp_pad *srcpad, struct mp_pad *sinkpad);

/**
 * @brief Start a task on a pad
 *
 * Starts a task that repeatedly calls @p func with @p user_data. This function
 * is mostly used to start the dataflow.
 *
 * @param pad Pointer to the @ref struct mp_pad to start the task on
 * @param func The task function to call (@ref mp_task_function)
 * @param priority The priority of the task
 * @param user_data User data passed to the task function
 * @return true if the task could be started, false otherwise
 */
bool mp_pad_start_task(struct mp_pad *pad, mp_task_function func, int priority, void *user_data);

/**
 * @brief Send an event to a pad
 *
 * Sends an event to the specified pad using the pad's event function.
 *
 * @param pad Pointer to the @ref struct mp_pad where the event should be sent
 * @param event Pointer to the @ref struct mp_event to send
 * @return true if the event was successfully sent and handled, false otherwise
 */
bool mp_pad_send_event(struct mp_pad *pad, struct mp_event *event);

/**
 * @brief Default event handler for pads
 *
 * Default event handler for pads. This function will forward the event to the
 * peer pad if the event direction matches the pad direction, otherwise it will
 * forward the event to other pads in the same element.
 *
 * @param pad Pointer to the @ref struct mp_pad to send event to
 * @param event Pointer to the @ref struct mp_event to send
 * @return true if the event is handled, false otherwise
 */
bool mp_pad_send_event_default(struct mp_pad *pad, struct mp_event *event);

/**
 * @brief Send a query to a pad
 *
 * Sends a query to the pad using the pad's query function.
 *
 * @param pad Pointer to the @ref struct mp_pad to send query to
 * @param query Pointer to the @ref struct mp_query to send
 * @return true if the query is handled, false otherwise
 */
bool mp_pad_query(struct mp_pad *pad, struct mp_query *query);

/**
 * @brief Push a buffer to the peer pad
 *
 * Pushes a buffer to the peer of the specified pad. The pad should be a
 * source pad and must be linked to a sink pad.
 *
 * @param pad Pointer to a source @ref struct mp_pad
 * @param buffer Pointer to the @ref struct mp_buffer to push
 * @return true if the buffer was successfully pushed, false otherwise
 */
bool mp_pad_push(struct mp_pad *pad, struct mp_buffer *buffer);

/**
 * @brief Chain a buffer through a pad
 *
 * Calls the pad's chain function to process the buffer.
 *
 * @param pad Pointer to the @ref struct mp_pad
 * @param buffer Pointer to the @ref struct mp_buffer to chain
 * @return true if the buffer was successfully chained, false otherwise
 */
bool mp_pad_chain(struct mp_pad *pad, struct mp_buffer *buffer);

/**
 * @}
 */

#endif /* __MP_PAD_H__ */
