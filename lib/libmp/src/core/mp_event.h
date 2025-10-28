/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Event header file.
 */

#ifndef __MP_EVENT_H__
#define __MP_EVENT_H__

#include "mp_caps.h"
#include "mp_object.h"
#include "mp_structure.h"

/**
 * @defgroup mp_event
 * @brief Pipeline event
 *
 * @{
 */

/**
 * Create an event type from an id number and flags.
 *
 * @param num	Event id number (A new type of event must have unique number id)
 * @param flags Event flags
 */
#define MP_EVENT_CREATE_TYPE(num, flags) (((num) << 2) | (flags))

/**
 * Get the direction of an event.
 *
 * @param event Pointer to struct mp_event
 *
 * @retval direction of event @ref enum mp_event_direction
 */
#define MP_EVENT_DIRECTION(event) (event->type & 0b11)

/**
 * Event direction flags.
 */
enum mp_event_direction {
	MP_EVENT_DIRECTION_UNKNOWN = 0,           /**< Unknown direction */
	MP_EVENT_DIRECTION_UPSTREAM = BIT(0),     /**< Event flows upstream */
	MP_EVENT_DIRECTION_DOWNSTREAM = BIT(1),   /**< Event flows downstream */
	MP_EVENT_DIRECTION_ANY = BIT(1) | BIT(0), /**< Event can flow in any direction */
};

/**
 * Event types.
 */
enum mp_event_type {
	MP_EVENT_UNKNOWN = MP_EVENT_CREATE_TYPE(0, 0), /**< Unknown event */
	MP_EVENT_CAPS =
		MP_EVENT_CREATE_TYPE(1, MP_EVENT_DIRECTION_DOWNSTREAM), /**< Capabilities event */
	MP_EVENT_EOS =
		MP_EVENT_CREATE_TYPE(2, MP_EVENT_DIRECTION_DOWNSTREAM), /**< End-of-stream event */

	MP_EVENT_END = UINT8_MAX, /**< Maximum event type identifer */
};

/**
 * Event structure.
 */
struct mp_event {
	uint8_t type;                   /**< Type of the event */
	struct mp_structure *structure; /**< Associated metadata structure */
	uint32_t timestamp;             /**< Timestamp of the event */
};

/**
 * Create a new custom event.
 *
 * @param type Event type to assign to new event (See @ref enum mp_event_type)
 * @param structure Pointer to a structure to associate with the event.
 * @return Pointer to new @ref struct mp_event, or NULL if allcation fails.
 */
struct mp_event *mp_event_new_custom(enum mp_event_type type, struct mp_structure *structure);

/**
 * Create a new CAPS event.
 *
 * @param caps @ref struct mp_caps to include
 * @return Pointer to new @ref struct mp_event
 */
struct mp_event *mp_event_new_caps(struct mp_caps *caps);

/**
 * Create a new EOS (End-of-Stream) event.
 *
 * @return Pointer to new @ref struct mp_event
 */
struct mp_event *mp_event_new_eos(void);

/**
 * Get @ref struct mp_caps from a MP_EVENT_CAPS event.
 *
 * @param event Pointer to a struct mp_event
 * @return Pointer to event @ref struct mp_caps
 */
struct mp_caps *mp_event_get_caps(struct mp_event *event);

/**
 * Set caps to a @ref MP_EVENT_CAPS event.
 *
 * @param event Pointer to a @ref struct mp_event
 * @param caps Pointer to a @ref struct mp_caps
 * @return true if successful, false otherwise
 */
bool mp_event_set_caps(struct mp_event *event, struct mp_caps *caps);

/**
 * Destroy an event and its free its associated resources.
 *
 * @param event Pointer to @ref struct mp_event to destroy
 */
void mp_event_destroy(struct mp_event *event);

/** @} */

#endif /* __MP_EVENT_H__ */
