/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bus message: what an element reports to the application.
 * @ingroup mpipe_message
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_MESSAGE_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_MESSAGE_H_

/**
 * @defgroup mpipe_message Messages
 * @ingroup mpipe_framework
 * @brief What an element reports back to the application.
 *
 * A message travels the other way from a dispatch. A dispatch crosses a pad
 * link between two elements; a message leaves the graph entirely, going up the
 * bin's channel to whoever is driving the pipeline, to say that the stream
 * ended or that something failed.
 *
 * It is a small value type, copied into and out of the channel, so posting one
 * hands over no ownership. It carries where the failure happened - the element
 * that emitted it - and what happened, as a type plus, on a failure, a domain
 * saying which phase went wrong and an errno saying why. Those two together are
 * meant to answer the two questions that matter when a pipeline does not run:
 * which element, and in which phase.
 *
 * The message is machine-readable on purpose. It carries no human-facing
 * string; the sentence describing a failure belongs in the log at the site that
 * detected it, while an application branches on the domain and the errno.
 *
 * Message types are single bits so that a type doubles as a filter mask and a
 * consumer can select several by OR-ing them. Note that MPIPE_MESSAGE_UNKNOWN
 * is zero and matches nothing - it is an uninitialized message, not a
 * selectable type.
 *
 * @{
 */

#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/mpipe/mpipe_element.h>

/** @brief Message types, usable as bitmask filters. */
enum mpipe_message_type {
	/** Uninitialized: what a zeroed message carries; cannot be posted */
	MPIPE_MESSAGE_UNKNOWN = 0,
	MPIPE_MESSAGE_EOS = BIT(0),   /**< End of stream */
	MPIPE_MESSAGE_ERROR = BIT(1), /**< Error */
};

/**
 * @brief Filter mask matching any message type.
 *
 * A macro rather than an enumerator: UINT32_MAX does not fit in an int, and
 * a filter mask is a plain uint32_t like the type field it matches against.
 */
#define MPIPE_MESSAGE_ANY UINT32_MAX

/**
 * @brief The domain a failure happened in: what the element was doing.
 *
 * A pipeline fails in a small number of distinct places, and knowing which one
 * is most of the way to knowing why. This names them so a report can say what
 * the element was doing, not only that something went wrong.
 *
 * With the errno in @ref mpipe_message.code, the pair is what an application
 * branches on:
 *
 * @code{.c}
 * if (msg.domain == MPIPE_ERROR_RESOURCE && msg.code == -ENOENT) {
 *	// the input file does not exist
 * }
 * @endcode
 */
enum mpipe_error_domain {
	/**
	 * Nothing more specific fits. Also the neutral value: a message that is
	 * not a failure carries it, so a designated initializer that leaves the
	 * domain out is correct.
	 */
	MPIPE_ERROR_FAILED = 0,
	/** Capability negotiation: the formats would not intersect; try another config */
	MPIPE_ERROR_CAPS,
	/** Buffer negotiation: a pool would not configure or start; reduce counts or sizes */
	MPIPE_ERROR_BUFFER_POOL,
	/** Buffer flow: a chain, acquire or push failed while streaming; stop or restart */
	MPIPE_ERROR_FLOW,
	/** A device, file or driver refused; check the media or the hardware */
	MPIPE_ERROR_RESOURCE,
	/** One past the last domain; not itself a domain */
	MPIPE_ERROR_DOMAIN_END,
};

/**
 * @brief Message structure carrying type and origin of the message.
 *
 * @ref domain and @ref code describe a failure and are only meaningful on an
 * MPIPE_MESSAGE_ERROR.
 */
struct mpipe_message {
	struct mpipe_element *origin; /**< Element that posted this message */
	uint32_t type;                /**< Message type (see @ref mpipe_message_type) */
	int32_t code;                 /**< errno-style code, negative, 0 if unset */
	uint8_t domain; /**< What the element was doing, see @ref mpipe_error_domain */
};

/**
 * @brief Post a message to the bus of the bin holding its origin.
 *
 * Locates the bus from the message's origin via @ref mpipe_element_get_bus_chan
 * and publishes it there. The bus copies the message by value, so stack storage
 * is fine. This is the only way to post: an element is what a message comes
 * from, and it is the origin that says which bus the message belongs on.
 *
 * The caller declares and initializes the message. A field left out of a
 * designated initializer is zero, which is the neutral value for every field:
 * MPIPE_ERROR_FAILED domain, no code.
 *
 * Post an error when a failure is fatal to the stream, which is what an
 * application needs to hear about. A failure the caller can report by
 * returning an errno stays an errno; an error message is not for every
 * failed call.
 *
 * @code{.c}
 * struct mpipe_message msg = {
 *	.origin = &sink->element,
 *	.type = MPIPE_MESSAGE_EOS,
 * };
 *
 * (void)mpipe_message_post(&msg);
 * @endcode
 *
 * @param message Message to post, with at least its origin and type set.
 *
 * @retval 0 Success.
 * @retval -EINVAL The message has no origin or no type
 * @retval -ENODEV The origin has no bus
 * @retval -ENOMSG The bus validator dropped the message, which the pipeline
 *                 does to every end-of-stream but the last
 */
int mpipe_message_post(struct mpipe_message *message);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_MESSAGE_H_ */
