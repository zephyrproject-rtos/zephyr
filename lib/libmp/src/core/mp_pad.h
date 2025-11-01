/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpPad.
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

typedef struct _MpPad MpPad;

#define MP_PAD(self) ((MpPad *)self)

/**
 * @brief Get the peer pad of a pad
 */
#define MP_PAD_PEER(pad) ((MpPad *)pad->peer)

/**
 * @brief Check if a pad is a source pad
 */
#define MP_PAD_IS_SRC(pad) (pad->direction == MP_PAD_SRC)

/**
 * @brief Check if a pad is a sink pad
 */
#define MP_PAD_IS_SINK(pad) (pad->direction == MP_PAD_SINK)

/**
 * @brief Check if a pad is linked to another pad
 */
#define MP_PAD_IS_LINKED(pad) (pad->peer != NULL)

/**
 * @brief Check if a pad is active
 */
#define MP_PAD_IS_ACTIVE(pad) (pad->mode != MP_PAD_MODE_NONE)

/**
 * @brief The direction of a pad
 */
typedef enum {
	/** Direction is unknown */
	MP_PAD_UNKNOWN,
	/** The pad is a source pad */
	MP_PAD_SRC,
	/** The pad is a sink pad */
	MP_PAD_SINK
} MpPadDirection;

/**
 * @brief The operating mode of a @ref MpPad
 *
 * Defines if the pad operates in push or pull mode or none of them.
 */
typedef enum {
	/** Pad will not handle dataflow */
	MP_PAD_MODE_NONE,
	/** Pad handles dataflow in push mode */
	MP_PAD_MODE_PUSH,
	/** Pad handles dataflow in pull mode */
	MP_PAD_MODE_PULL
} MpPadMode;

/**
 * @brief The presence of a pad
 */
typedef enum {
	/** The pad is always present */
	MP_PAD_ALWAYS,
	/** The pad will present depending on the media stream */
	MP_PAD_SOMETIMES,
	/** The pad is only available on request */
	MP_PAD_REQUEST
} MpPadPresence;

/**
 * @brief Pad flags
 */
typedef enum {
	/** Pad needs to pass through the negotiation process */
	MP_PAD_FLAG_NEGOTIATE = BIT(0),
} MpPadFlags;

/**
 * @brief Pad structure
 *
 * The pad structure represents a connection point of an element.
 * Pads are used to negotiate capabilities and transfer data between elements.
 */
struct _MpPad {
	/** Base object */
	MpObject object;
	/** Node for linking in pad lists */
	sys_dnode_t node;
	/** The direction of the pad, cannot change after creating the pad */
	MpPadDirection direction;
	/** The presence of the pad */
	MpPadPresence presence;
	/** The operating mode of the pad */
	MpPadMode mode;
	/** Pointer to the peer pad this pad is linked to */
	MpPad *peer;
	/** Capabilities of the pad */
	MpCaps *caps;
	/** Task associated with this pad */
	MpTask task;
	/** Chain function for handling buffers */
	bool (*chainfn)(MpPad *pad, MpBuffer *buffer);
	/** Query function for handling queries */
	bool (*queryfn)(MpPad *pad, MpQuery *query);
	/** Event function for handling events */
	bool (*eventfn)(MpPad *pad, MpEvent *event);
};

/**
 * @brief Create a new pad
 *
 * Creates a new @ref MpPad with the specified parameters.
 *
 * @param name Name of the pad
 * @param direction Direction of the pad (@ref MpPadDirection)
 * @param presence Presence of the pad (@ref MpPadPresence)
 * @param caps Capabilities of the pad (@ref MpCaps)
 * @return Pointer to the newly created @ref MpPad, or NULL on failure
 */
MpPad *mp_pad_new(const char *name, MpPadDirection direction, MpPadPresence presence, MpCaps *caps);

/**
 * @brief Initialize a pad
 *
 * Initializes an existing @ref MpPad structure with the specified parameters.
 *
 * @param pad Pointer to the @ref MpPad to initialize
 * @param name Name of the pad
 * @param direction Direction of the pad (@ref MpPadDirection)
 * @param presence Presence of the pad (@ref MpPadPresence)
 * @param caps Capabilities of the pad (@ref MpCaps)
 */
void mp_pad_init(MpPad *pad, const char *name, MpPadDirection direction, MpPadPresence presence,
		 MpCaps *caps);

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
bool mp_pad_link(MpPad *srcpad, MpPad *sinkpad);

/**
 * @brief Start a task on a pad
 *
 * Starts a task that repeatedly calls @p func with @p user_data. This function
 * is mostly used to start the dataflow.
 *
 * @param pad Pointer to the @ref MpPad to start the task on
 * @param func The task function to call (@ref MpTaskFunction)
 * @param priority The priority of the task
 * @param user_data User data passed to the task function
 * @return true if the task could be started, false otherwise
 */
bool mp_pad_start_task(MpPad *pad, MpTaskFunction func, int priority, void *user_data);

/**
 * @brief Send an event to a pad
 *
 * Sends an event to the specified pad using the pad's event function.
 *
 * @param pad Pointer to the @ref MpPad where the event should be sent
 * @param event Pointer to the @ref MpEvent to send
 * @return true if the event was successfully sent and handled, false otherwise
 */
bool mp_pad_send_event(MpPad *pad, MpEvent *event);

/**
 * @brief Default event handler for pads
 *
 * Default event handler for pads. This function will forward the event to the
 * peer pad if the event direction matches the pad direction, otherwise it will
 * forward the event to other pads in the same element.
 *
 * @param pad Pointer to the @ref MpPad to send event to
 * @param event Pointer to the @ref MpEvent to send
 * @return true if the event is handled, false otherwise
 */
bool mp_pad_send_event_default(MpPad *pad, MpEvent *event);

/**
 * @brief Send a query to a pad
 *
 * Sends a query to the pad using the pad's query function.
 *
 * @param pad Pointer to the @ref MpPad to send query to
 * @param query Pointer to the @ref MpQuery to send
 * @return true if the query is handled, false otherwise
 */
bool mp_pad_query(MpPad *pad, MpQuery *query);

/**
 * @brief Push a buffer to the peer pad
 *
 * Pushes a buffer to the peer of the specified pad. The pad should be a
 * source pad and must be linked to a sink pad.
 *
 * @param pad Pointer to a source @ref MpPad
 * @param buffer Pointer to the @ref MpBuffer to push
 * @return true if the buffer was successfully pushed, false otherwise
 */
bool mp_pad_push(MpPad *pad, MpBuffer *buffer);

/**
 * @brief Chain a buffer through a pad
 *
 * Calls the pad's chain function to process the buffer.
 *
 * @param pad Pointer to the @ref MpPad
 * @param buffer Pointer to the @ref MpBuffer to chain
 * @return true if the buffer was successfully chained, false otherwise
 */
bool mp_pad_chain(MpPad *pad, MpBuffer *buffer);

/**
 * @}
 */

#endif /* __MP_PAD_H__ */
