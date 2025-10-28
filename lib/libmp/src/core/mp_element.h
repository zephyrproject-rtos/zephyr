/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpElement.
 */

#ifndef __MP_ELEMENT_H__
#define __MP_ELEMENT_H__

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/dlist.h>

#include "mp_bus.h"
#include "mp_event.h"
#include "mp_object.h"
#include "mp_query.h"
#include "utils/mp_utils.h"

typedef struct _MpElement MpElement;
typedef struct _MpPad MpPad;

struct _MpElementFactory;

#define MP_ELEMENT_CAST(self) ((MpElement *)self)

/**
 * @brief Helper macro to define element structures
 *
 * @param type The new element type name
 * @param basetype The base type (usually MpElement)
 * @param base The base member name
 * @param ... Additional members for the element
 */
#define MP_ELEMENT(type, basetype, base, ...)                                                      \
	typedef struct {                                                                           \
		basetype base;                                                                     \
		__VA_ARGS__                                                                        \
	} type

/**
 * @brief Calculate the next state
 *
 * Given a current state and a target state, calculate the next intermediate state
 *
 * @param cur Current state, see @ref MpState
 * @param target Target state, see @ref MpState
 * @return Next intermediate state
 *
 */
#define MP_STATE_GET_NEXT(cur, target) ((MpState)((int)cur + MP_SIGN((int)target - (int)cur)))

/**
 * @brief Create state transition value
 *
 * @param cur Current state
 * @param next Next state
 * @return State transition value
 */
#define MP_STATE_TRANSITION(cur, next) (((cur) << 2) | (next))

/**
 * @brief Extract the current state from the state transition
 *
 * Given a state transition, extract the current state.
 *
 * @param trans A transition state, see @ref MpStateChange
 * @return The current state
 *
 */
#define MP_STATE_TRANSITION_CURRENT(trans) ((MpState)((trans) >> 2))

/**
 * @brief Extract the next state from the state transition
 *
 * Given a state transition, extract the next state.
 *
 * @param trans A transition state, see @ref MpStateChange
 * @return The next state
 *
 */
#define MP_STATE_TRANSITION_NEXT(trans) ((MpState)((trans) & 0x3))

/**
 * @brief Get current state of element
 *
 * @param element The element
 * @return Current state
 */
#define MP_STATE_CURRENT(element) (MP_ELEMENT_CAST(element)->current_state)

/**
 * @brief Get next state of element
 *
 * @param element The element
 * @return Next state
 */
#define MP_STATE_NEXT(element)    (MP_ELEMENT_CAST(element)->next_state)

/**
 * @brief Get target state of element
 *
 * @param element The element
 * @return Target state
 */
#define MP_STATE_TARGET(element)  (MP_ELEMENT_CAST(element)->target_state)

/**
 * @brief States of an element
 *
 * All possible states that an element can be in.
 */
typedef enum {
	/** The element is initialized READY to go to PAUSED. */
	MP_STATE_READY = 0,
	/** The element is PAUSED and ready to receive, process or transfer data. */
	MP_STATE_PAUSED = 1,
	/** The element is PLAYING, data is flowing through the element */
	MP_STATE_PLAYING = 2,
} MpState;

/**
 * @brief MpStateChange
 *
 * Different possible state changes that an element can go through.
 */
typedef enum {
	/** State change from READY to PAUSED */
	MP_STATE_CHANGE_READY_TO_PAUSED = MP_STATE_TRANSITION(MP_STATE_READY, MP_STATE_PAUSED),
	/** State change from PAUSED to PLAYING*/
	MP_STATE_CHANGE_PAUSED_TO_PLAYING = MP_STATE_TRANSITION(MP_STATE_PAUSED, MP_STATE_PLAYING),
	/** State change from PLAYING to PAUSED */
	MP_STATE_CHANGE_PLAYING_TO_PAUSED = MP_STATE_TRANSITION(MP_STATE_PLAYING, MP_STATE_PAUSED),
	/** State change from PAUSED to READY */
	MP_STATE_CHANGE_PAUSED_TO_READY = MP_STATE_TRANSITION(MP_STATE_PAUSED, MP_STATE_READY),
} MpStateChange;

/**
 * @brief MpStateChangeReturn
 *
 * Possible returned values from a state change function
 */
typedef enum {
	/* The state change has failed */
	MP_STATE_CHANGE_FAILURE = 0,
	/* The state change has succeeded */
	MP_STATE_CHANGE_SUCCESS = 1,
	/* The state change will happen asynchronously */
	MP_STATE_CHANGE_ASYNC = 2,
} MpStateChangeReturn;

/**
 * @brief Element base class
 *
 * The base class for all elements. Elements are the basic building blocks
 * of a multimedia pipeline. They can have source pads (outputs) and sink
 * pads (inputs) and can be linked together to form processing chains.
 */
struct _MpElement {
	/** Base object */
	MpObject object;

	/** Factory that created this element */
	struct _MpElementFactory *factory;

	/** List of source pads */
	sys_dlist_t srcpads;
	/** List of sink pads */
	sys_dlist_t sinkpads;

	/** Current state of the element */
	MpState current_state;
	/** Next state (for transitions) */
	MpState next_state;
	/** Pending state (for async transitions) */
	MpState pending_state;
	/** Target state */
	MpState target_state;

	/** Bus for posting messages */
	MpBus *bus;

	/** Event handler function */
	bool (*eventfn)(MpElement *element, MpEvent *event);
	/** Query handler function */
	bool (*queryfn)(MpElement *element, MpQuery *query);

	/** Get current state function */
	MpStateChangeReturn (*get_state)(MpElement *element, MpState *state);
	/** Set state function */
	MpStateChangeReturn (*set_state)(MpElement *element, MpState state);
	/** Change state function */
	MpStateChangeReturn (*change_state)(MpElement *element, MpStateChange transition);
};

/**
 * @brief Initialize an element
 *
 * Initializes the base @ref MpElement structure.
 *
 * @param self The element to initialize
 */
void mp_element_init(MpElement *self);

/**
 * @brief Add a pad to an element
 *
 * Adds a pad to the element. The pad's container will be set to the element.
 *
 * @param element The @MpElement to add the pad to
 * @param pad The @ref MpPad to add to the element
 */
void mp_element_add_pad(MpElement *element, MpPad *pad);

/**
 * @brief Link elements together
 *
 * Links multiple elements together in a chain. Elements should have only
 * one source and/or one sink pad. If not, the first src/sink pads will be used.
 * The function takes a variable number of elements and links them sequentially.
 *
 * @param element_1 First element in the chain
 * @param element_2 Second element in the chain
 * @param ... Additional elements to link (terminated by NULL)
 * @retval true If all elements were successfully linked
 * @retval false If any link operation failed
 */
bool mp_element_link(MpElement *element_1, MpElement *element_2, ...);

/**
 * @brief Set the state of an element
 *
 * Sets the state of an element. This function will try to set the
 * requested state by going through all the intermediary states and calling
 * the element's state change function for each.
 *
 * @param element The element to change state of
 * @param state The element's new @ref MpState
 * @return Result of the state change, one of @ref MpStateChangeReturn
 */
MpStateChangeReturn mp_element_set_state(MpElement *element, MpState state);

/**
 * @brief Get the bus of an element
 *
 * Retrieves the @ref MpBus associated with the element.
 *
 * @param element The element to get the bus from
 * @return The @ref MpBus of the element or NULL if no bus is found
 */
MpBus *mp_element_get_bus(MpElement *self);

#endif /* __MP_ELEMENT_H__ */
