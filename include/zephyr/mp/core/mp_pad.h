/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_pad.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_PAD_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_PAD_H_

/**
 * @defgroup mp_pad Pad
 * @ingroup mp_core
 * @brief Connection point for data flow between elements
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/util.h>

#include <zephyr/kernel.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_object.h>

struct mp_dispatch;

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
 * @brief The operating mode of a @ref mp_pad
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
	/** Chain function for handling buffers */
	int (*chainfn)(struct mp_pad *pad, struct net_buf *in_buf, struct net_buf **out_buf);
	/** Query function for handling queries */
	int (*queryfn)(struct mp_pad *pad, struct mp_dispatch *query);
	/** Event function for handling events */
	int (*eventfn)(struct mp_pad *pad, struct mp_dispatch *event);
};

/**
 * @brief Create a new pad dynamically
 *
 * Creates a new @ref mp_pad with the specified parameters.
 *
 * @param id Unique ID of the pad instance in the element
 * @param direction Direction of the pad (@ref mp_pad_direction)
 * @param presence Presence of the pad (@ref mp_pad_presence)
 * @param caps Capabilities of the pad (@ref mp_caps)
 * @return Pointer to the newly created @ref mp_pad, or NULL on failure
 */
struct mp_pad *mp_pad_new(uint8_t id, enum mp_pad_direction direction,
			  enum mp_pad_presence presence, struct mp_caps *caps);

/**
 * @brief Initialize a pad
 *
 * Initializes an existing @ref mp_pad structure with the specified parameters.
 *
 * @param pad Pointer to the @ref mp_pad to initialize
 * @param id Unique ID of the pad instance in the element
 * @param direction Direction of the pad (@ref mp_pad_direction)
 * @param presence Presence of the pad (@ref mp_pad_presence)
 * @param caps Capabilities of the pad (@ref mp_caps)
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
 *
 * @return 0 on success, negative errno on failure
 */
int mp_pad_link(struct mp_pad *srcpad, struct mp_pad *sinkpad);

/**
 * @brief Send an event to a pad
 *
 * Sends an event to the specified pad using the pad's event function.
 *
 * @param pad Pointer to the @ref mp_pad where the event should be sent
 * @param event Pointer to the @ref mp_dispatch to send
 *
 * @return 0 on success, negative errno on failure
 */
int mp_pad_send_event(struct mp_pad *pad, struct mp_dispatch *event);

/**
 * @brief Default event handler for pads
 *
 * Default event handler for pads. This function will forward the event to the
 * peer pad if the event direction matches the pad direction, otherwise it will
 * forward the event to other pads in the same element.
 *
 * @param pad Pointer to the @ref mp_pad to send event to
 * @param event Pointer to the @ref mp_dispatch to send
 *
 * @return 0 on success, negative errno on failure
 */
int mp_pad_send_event_default(struct mp_pad *pad, struct mp_dispatch *event);

/**
 * @brief Send a query to a pad
 *
 * Sends a query to the pad using the pad's query function.
 *
 * @param pad Pointer to the @ref mp_pad to send query to
 * @param query Pointer to the @ref mp_dispatch to send
 *
 * @return 0 on success, negative errno on failure
 */
int mp_pad_query(struct mp_pad *pad, struct mp_dispatch *query);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_PAD_H_ */
