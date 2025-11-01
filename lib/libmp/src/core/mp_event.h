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
 * @defgroup MpEvent
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
#define MP_EVENT_CREATE_TYPE(num, flags) (((num) << 8) | (flags))

/**
 * Get the type of an event.
 *
 * @param event Pointer to MpEvent
 *
 * @retval type of event @ref MpEventType
 */
#define MP_EVENT_TYPE(event) (((MpEvent *)event)->type)

/**
 * Get the direction of an event.
 *
 * @param event Pointer to MpEvent
 *
 * @retval direction of event @ref MpEventDirection
 */
#define MP_EVENT_DIRECTION(event) (MP_EVENT_TYPE(event) & 0x0F)

/**
 * Event direction flags.
 */
typedef enum {
	MP_EVENT_DIRECTION_UNKNOWN = 0,           /**< Unknown direction */
	MP_EVENT_DIRECTION_UPSTREAM = BIT(0),     /**< Event flows upstream */
	MP_EVENT_DIRECTION_DOWNSTREAM = BIT(1),   /**< Event flows downstream */
	MP_EVENT_DIRECTION_ANY = BIT(1) | BIT(0), /**< Event can flow in any direction */
} MpEventDirection;

/**
 * Event types.
 */
typedef enum {
	MP_EVENT_UNKNOWN = MP_EVENT_CREATE_TYPE(0, 0), /**< Unknown event */
	MP_EVENT_CAPS =
		MP_EVENT_CREATE_TYPE(1, MP_EVENT_DIRECTION_DOWNSTREAM), /**< Capabilities event */
	MP_EVENT_EOS =
		MP_EVENT_CREATE_TYPE(2, MP_EVENT_DIRECTION_DOWNSTREAM), /**< End-of-stream event */
} MpEventType;

/**
 * Event structure.
 */
typedef struct {
	MpEventType type;       /**< Type of the event */
	MpStructure *structure; /**< Associated metadata structure */
	uint32_t timestamp;     /**< Timestamp of the event */
} MpEvent;

/**
 * Create a new custom event.
 *
 * @param type Event type to assign to new event (See @ref MpEventType)
 * @param structure Pointer to a structure to associate with the event.
 * @return Pointer to new @ref MpEvent, or NULL if allcation fails.
 */
MpEvent *mp_event_new_custom(MpEventType type, MpStructure *structure);

/**
 * Create a new CAPS event.
 *
 * @param caps @ref MpCaps to include
 * @return Pointer to new @ref MpEvent
 */
MpEvent *mp_event_new_caps(MpCaps *caps);

/**
 * Create a new EOS (End-of-Stream) event.
 *
 * @return Pointer to new @ref MpEvent
 */
MpEvent *mp_event_new_eos(void);

/**
 * Get @ref MpCaps from a MP_EVENT_CAPS event.
 *
 * @param event Pointer to a MpEvent
 * @return Pointer to event @ref MpCaps
 */
MpCaps *mp_event_get_caps(MpEvent *event);

/**
 * Set caps to a @ref MP_EVENT_CAPS event.
 *
 * @param event Pointer to a @ref MpEvent
 * @param caps Pointer to a @ref MpCaps
 * @return true if successful, false otherwise
 */
bool mp_event_set_caps(MpEvent *event, MpCaps *caps);

/**
 * Destroy an event and its free its associated resources.
 *
 * @param event Pointer to @ref MpEvent to destroy
 */
void mp_event_destroy(MpEvent *event);

/** @} */

#endif /* __MP_EVENT_H__ */
