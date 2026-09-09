/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Element: the unit a pipeline is built from.
 * @ingroup mpipe_element
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_ELEMENT_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_ELEMENT_H_

/**
 * @defgroup mpipe_element Elements
 * @ingroup mpipe_framework
 * @brief The unit a pipeline is built from.
 *
 * An element is one processing step: it produces buffers, consumes them, or
 * turns some into others. It exposes @ref mpipe_pad connectors, and linking two
 * elements links a source pad of one to a sink pad of the other. What an
 * element does is its own business; what the framework asks of it is that it
 * answer queries, handle events, and follow the state machine.
 *
 * @section mpipe_element_states States
 *
 * An element is in one of three states, and moves between neighbors one step
 * at a time:
 *
 *     READY <-> PAUSED <-> PLAYING
 *
 * READY means constructed and linked but holding no negotiated format and no
 * buffers. The READY to PAUSED transition is where the work happens: the
 * source drives capability negotiation across the whole graph, then the buffer
 * pool query settles who provides buffers and how many, and the pools start.
 * PAUSED to PLAYING lets data flow. Going back down reverses it, and PAUSED to
 * READY drops the negotiated capabilities so the next run negotiates afresh.
 *
 * A transition is reported with @ref mpipe_state_change_return. An element that
 * refuses one stays where it is; nothing unwinds the elements that already
 * moved, which is deliberate - the graph then shows exactly which element
 * refused and in which transition.
 *
 * @section mpipe_element_subclass Specializing an element
 *
 * The bases in @ref mpipe_src, @ref mpipe_sink, @ref mpipe_transform,
 * @ref mpipe_parser and @ref mpipe_bin implement the protocol; a concrete
 * element embeds one of them as its first member, calls that base's init with
 * its own id, and overrides only the hooks it cares about. Chaining to the base
 * change_state is mandatory - that is what performs the reset and the pool
 * teardown every element is expected to do.
 *
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/sys/dlist.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_object.h>

struct mpipe_dispatch;
struct zbus_channel;

/** @cond INTERNAL_HIDDEN */
#if defined(CONFIG_MPIPE_DUMP)
#define MPIPE_ELEMENT_SET_NAME(e, n) ((e)->name = (n))
#else
#define MPIPE_ELEMENT_SET_NAME(e, n) ((void)0)
#endif
/** @endcond */

/**
 * @brief Calculate the next state
 *
 * Given a current state and a target state, calculate the next intermediate state
 *
 * @param cur Current state, see @ref mpipe_state
 * @param target Target state, see @ref mpipe_state
 * @return Next intermediate state
 *
 */
#define MPIPE_STATE_GET_NEXT(cur, target)                                                          \
	((enum mpipe_state)((int)cur + SYS_SIGN((int)target - (int)cur)))

/**
 * @brief Create state transition value
 *
 * @param cur Current state
 * @param next Next state
 * @return State transition value
 */
#define MPIPE_STATE_TRANSITION(cur, next) (((cur) << 2) | (next))

/**
 * @brief Extract the current state from the state transition
 *
 * Given a state transition, extract the current state.
 *
 * @param trans A transition state, see @ref mpipe_state_change
 * @return The current state
 *
 */
#define MPIPE_STATE_TRANSITION_CURRENT(trans) ((enum mpipe_state)((trans) >> 2))

/**
 * @brief Extract the next state from the state transition
 *
 * Given a state transition, extract the next state.
 *
 * @param trans A transition state, see @ref mpipe_state_change
 * @return The next state
 *
 */
#define MPIPE_STATE_TRANSITION_NEXT(trans) ((enum mpipe_state)((trans) & 0x3))

/**
 * @brief States of an element
 *
 * All possible states that an element can be in.
 */
enum mpipe_state {
	/** The element is initialized READY to go to PAUSED. */
	MPIPE_STATE_READY = 0,
	/** The element is PAUSED and ready to receive, process or transfer data. */
	MPIPE_STATE_PAUSED = 1,
	/** The element is PLAYING, data is flowing through the element */
	MPIPE_STATE_PLAYING = 2,
};

/**
 * @brief enum mpipe_state_change
 *
 * Different possible state changes that an element can go through.
 */
enum mpipe_state_change {
	/** State change from READY to PAUSED */
	MPIPE_STATE_CHANGE_READY_TO_PAUSED =
		MPIPE_STATE_TRANSITION(MPIPE_STATE_READY, MPIPE_STATE_PAUSED),
	/** State change from PAUSED to PLAYING*/
	MPIPE_STATE_CHANGE_PAUSED_TO_PLAYING =
		MPIPE_STATE_TRANSITION(MPIPE_STATE_PAUSED, MPIPE_STATE_PLAYING),
	/** State change from PLAYING to PAUSED */
	MPIPE_STATE_CHANGE_PLAYING_TO_PAUSED =
		MPIPE_STATE_TRANSITION(MPIPE_STATE_PLAYING, MPIPE_STATE_PAUSED),
	/** State change from PAUSED to READY */
	MPIPE_STATE_CHANGE_PAUSED_TO_READY =
		MPIPE_STATE_TRANSITION(MPIPE_STATE_PAUSED, MPIPE_STATE_READY),
};

/**
 * @brief enum mpipe_state_change_return
 *
 * Possible returned values from a state change function
 */
enum mpipe_state_change_return {
	/** The state change has failed */
	MPIPE_STATE_CHANGE_FAILURE = 0,
	/** The state change has succeeded */
	MPIPE_STATE_CHANGE_SUCCESS = 1,
	/** The state change will happen asynchronously */
	MPIPE_STATE_CHANGE_ASYNC = 2,
};

struct mpipe_element;

/**
 * @brief Element base class
 *
 * The base class for all elements. Elements are the basic building blocks
 * of a multimedia pipeline. They can have source pads (outputs) and sink
 * pads (inputs) and can be linked together to form processing chains.
 */
struct mpipe_element {
	/** Base object */
	struct mpipe_object object;

#if defined(CONFIG_MPIPE_DUMP) || defined(__DOXYGEN__)
	/**
	 * Name of the element, set to its type by the element's init function and
	 * overridable per instance with @ref mpipe_element_set_name. Debugging only.
	 *
	 * @kconfig_dep{CONFIG_MPIPE_DUMP}
	 */
	const char *name;
#endif

	/** List of source pads */
	sys_dlist_t src_pads;
	/** List of sink pads */
	sys_dlist_t sink_pads;

	/** Current state of the element */
	enum mpipe_state current_state;

	/** Set state function */
	enum mpipe_state_change_return (*set_state)(struct mpipe_element *element,
						    enum mpipe_state state);
	/** Change state function */
	enum mpipe_state_change_return (*change_state)(struct mpipe_element *element,
						       enum mpipe_state_change transition);
};

/**
 * @brief Initialize an element
 *
 * Initializes the base @ref mpipe_element structure.
 *
 * @param self Pointer to the @ref mpipe_element to initialize.
 * @param id   Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_element_init(struct mpipe_element *self, uint8_t id);

/**
 * @brief Name an element for the pipeline dump.
 *
 * Every element init function names its element after its type, so a dump reads
 * @c vid_transform rather than an address. Call this afterwards to give one
 * instance a distinct name, which is what tells two elements of the same type
 * apart in a dump.
 *
 * @code
 * ret = mpipe_vid_transform_init(&jpeg_dec, JPEG_DEC_ID);
 * mpipe_element_set_name(&jpeg_dec.transform.element, "jpeg_dec");
 * @endcode
 *
 * Compiles to nothing when @kconfig{CONFIG_MPIPE_DUMP} is disabled, so @p name
 * costs no ROM in a build without the dump.
 *
 * @param self Element to name.
 * @param name Name to report. Must outlive the element; a string literal does.
 */
static inline void mpipe_element_set_name(struct mpipe_element *self, const char *name)
{
	ARG_UNUSED(self);
	ARG_UNUSED(name);

	MPIPE_ELEMENT_SET_NAME(self, name);
}

struct mpipe_pad;

/**
 * @brief Reset every pad of an element to constraining nothing.
 *
 * Drops the negotiated capability from each of the element's pads so a
 * subsequent re-negotiation starts fresh. Base classes call this on the
 * PAUSED to READY transition; a derived element that overrides change_state
 * must chain to its base function to inherit the reset.
 *
 * @param element Pointer to the element whose pads to reset.
 */
void mpipe_element_reset_pad_caps(struct mpipe_element *element);

/**
 * @brief Attach a pad to an element.
 *
 * Adds @p pad to the element's pad list and makes the element the pad's
 * container, which is how a pad callback reaches the element that owns it.
 * A base class calls this for the pads it declares, so an element only calls
 * it for a pad of its own.
 *
 * @param element Pointer to the element to attach the pad to.
 * @param pad Pointer to the pad to attach.
 */
void mpipe_element_add_pad(struct mpipe_element *element, struct mpipe_pad *pad);

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
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_element_link(struct mpipe_element *element_1, struct mpipe_element *element_2, ...);

/**
 * @brief Set the state of an element
 *
 * Sets the state of an element. This function will try to set the
 * requested state by going through all the intermediary states and calling
 * the element's state change function for each.
 *
 * @param element The element to change state of
 * @param state The element's new @ref mpipe_state
 * @return Result of the state change, one of @ref mpipe_state_change_return
 */
enum mpipe_state_change_return mpipe_element_set_state(struct mpipe_element *element,
						       enum mpipe_state state);

/**
 * @brief Get the bus channel of an element
 *
 * Retrieves the @ref zbus_channel associated with the element, which is the bus channel of
 * the nearest bin that contains the element. This is how an application reaches the channel
 * to attach an observer to, without depending on where the bin keeps it.
 *
 * @param element Pointer to the @ref mpipe_element to get the channel from.
 *
 * @return Pointer to the @ref zbus_channel, or NULL if no bus channel is found.
 */
struct zbus_channel *mpipe_element_get_bus_chan(struct mpipe_element *element);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_ELEMENT_H_ */
