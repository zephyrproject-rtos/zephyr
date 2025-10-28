/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_element.
 */

#ifndef __MP_ELEMENT_H__
#define __MP_ELEMENT_H__

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>

#include "mp_bus.h"
#include "mp_event.h"
#include "mp_object.h"
#include "mp_query.h"

#define MP_ELEMENT_INIT(elem, type, id)                                                            \
	({                                                                                         \
		mp_element_init(MP_ELEMENT(elem), id);                                             \
		mp_##type##_init(MP_ELEMENT(elem));                                                \
	})

/**
 * @brief Calculate the next state
 *
 * Given a current state and a target state, calculate the next intermediate state
 *
 * @param cur Current state, see @ref enum mp_state
 * @param target Target state, see @ref enum mp_state
 * @return Next intermediate state
 *
 */
#define MP_STATE_GET_NEXT(cur, target)                                                             \
	((enum mp_state)((int)cur + SYS_SIGN((int)target - (int)cur)))

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
 * @param trans A transition state, see @ref enum mp_state_change
 * @return The current state
 *
 */
#define MP_STATE_TRANSITION_CURRENT(trans) ((enum mp_state)((trans) >> 2))

/**
 * @brief Extract the next state from the state transition
 *
 * Given a state transition, extract the next state.
 *
 * @param trans A transition state, see @ref enum mp_state_change
 * @return The next state
 *
 */
#define MP_STATE_TRANSITION_NEXT(trans) ((enum mp_state)((trans) & 0x3))

/**
 * @brief States of an element
 *
 * All possible states that an element can be in.
 */
enum mp_state {
	/** The element is initialized READY to go to PAUSED. */
	MP_STATE_READY = 0,
	/** The element is PAUSED and ready to receive, process or transfer data. */
	MP_STATE_PAUSED = 1,
	/** The element is PLAYING, data is flowing through the element */
	MP_STATE_PLAYING = 2,
};

/**
 * @brief enum mp_state_change
 *
 * Different possible state changes that an element can go through.
 */
enum mp_state_change {
	/** State change from READY to PAUSED */
	MP_STATE_CHANGE_READY_TO_PAUSED = MP_STATE_TRANSITION(MP_STATE_READY, MP_STATE_PAUSED),
	/** State change from PAUSED to PLAYING*/
	MP_STATE_CHANGE_PAUSED_TO_PLAYING = MP_STATE_TRANSITION(MP_STATE_PAUSED, MP_STATE_PLAYING),
	/** State change from PLAYING to PAUSED */
	MP_STATE_CHANGE_PLAYING_TO_PAUSED = MP_STATE_TRANSITION(MP_STATE_PLAYING, MP_STATE_PAUSED),
	/** State change from PAUSED to READY */
	MP_STATE_CHANGE_PAUSED_TO_READY = MP_STATE_TRANSITION(MP_STATE_PAUSED, MP_STATE_READY),
};

/**
 * @brief enum mp_state_change_return
 *
 * Possible returned values from a state change function
 */
enum mp_state_change_return {
	/* The state change has failed */
	MP_STATE_CHANGE_FAILURE = 0,
	/* The state change has succeeded */
	MP_STATE_CHANGE_SUCCESS = 1,
	/* The state change will happen asynchronously */
	MP_STATE_CHANGE_ASYNC = 2,
};

struct mp_element;

#define MP_ELEMENT(self) ((struct mp_element *)self)

/**
 * @brief Element base class
 *
 * The base class for all elements. Elements are the basic building blocks
 * of a multimedia pipeline. They can have source pads (outputs) and sink
 * pads (inputs) and can be linked together to form processing chains.
 */
struct mp_element {
	/** Base object */
	struct mp_object object;

	/** Factory that created this element */
	struct mp_element_factory *factory;

	/** List of source pads */
	sys_dlist_t srcpads;
	/** List of sink pads */
	sys_dlist_t sinkpads;

	/** Current state of the element */
	enum mp_state current_state;
	/** Next state (for transitions) */
	enum mp_state next_state;
	/** Pending state (for async transitions) */
	enum mp_state pending_state;
	/** Target state */
	enum mp_state target_state;

	/** Event handler function */
	bool (*eventfn)(struct mp_element *element, struct mp_event *event);
	/** Query handler function */
	bool (*queryfn)(struct mp_element *element, struct mp_query *query);

	/** Get current state function */
	enum mp_state_change_return (*get_state)(struct mp_element *element, enum mp_state *state);
	/** Set state function */
	enum mp_state_change_return (*set_state)(struct mp_element *element, enum mp_state state);
	/** Change state function */
	enum mp_state_change_return (*change_state)(struct mp_element *element,
						    enum mp_state_change transition);
};

/**
 * @brief Initialize an element
 *
 * Initializes the base @ref struct mp_element structure.
 *
 * @param self The element to initialize
 */
void mp_element_init(struct mp_element *self, uint8_t id);

struct mp_pad;

/**
 * @brief Add a pad to an element
 *
 * Adds a pad to the element. The pad's container will be set to the element.
 *
 * @param element The @struct mp_element to add the pad to
 * @param pad The @ref struct mp_pad to add to the element
 */
void mp_element_add_pad(struct mp_element *element, struct mp_pad *pad);

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
bool mp_element_link(struct mp_element *element_1, struct mp_element *element_2, ...);

/**
 * @brief Set the state of an element
 *
 * Sets the state of an element. This function will try to set the
 * requested state by going through all the intermediary states and calling
 * the element's state change function for each.
 *
 * @param element The element to change state of
 * @param state The element's new @ref enum mp_state
 * @return Result of the state change, one of @ref enum mp_state_change_return
 */
enum mp_state_change_return mp_element_set_state(struct mp_element *element, enum mp_state state);

/**
 * @brief Get the bus of an element
 *
 * Retrieves the @ref struct mp_bus associated with the element.
 *
 * @param element The element to get the bus from
 * @return The @ref struct mp_bus of the element or NULL if no bus is found
 */
struct mp_bus *mp_element_get_bus(struct mp_element *self);

#endif /* __MP_ELEMENT_H__ */
