/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Queries and events exchanged between elements
 * @ingroup mpipe_dispatch
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_DISPATCH_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_DISPATCH_H_

/**
 * @defgroup mpipe_dispatch Dispatches
 * @ingroup mpipe_framework
 * @brief Queries and events exchanged between elements.
 *
 * A dispatch is what an element hands its peer across a pad link when what
 * it sends is not data. It is one of two things:
 *
 * - a **query**, passed to mpipe_pad_query(): the peer answers it in place,
 *   so the asker reads the answer back out of the dispatch;
 * - an **event**, passed to mpipe_pad_send_event(): a one-way announcement
 *   the peer acts on and forwards.
 *
 * One fixed-size structure carries both, because a negotiation must allocate
 * nothing. A dispatch is not an @ref mpipe_message either: a message travels
 * up the bus to the application, a dispatch travels across a pad link between
 * two elements.
 *
 * Nothing in the structure says which of the two it is. The type says what
 * the dispatch is about, and the function it is passed to decides whether it
 * asks or announces:
 *
 * | Type | As a query | As an event |
 * | :--- | :--- | :--- |
 * | CAPS | what do you accept, given this filter? | this is the format, apply it |
 * | BUFFER_POOL | what buffers do you need? | - |
 * | EOS | - | the stream is over |
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_structure.h>

/**
 * @enum mpipe_dispatch_type
 * @brief Supported dispatch types.
 */
enum mpipe_dispatch_type {
	MPIPE_DISPATCH_UNKNOWN = 0,     /**< Unknown dispatch type */
	MPIPE_DISPATCH_EOS,             /**< EOS dispatch type */
	MPIPE_DISPATCH_CAPS,            /**< CAPS dispatch type */
	MPIPE_DISPATCH_BUFFER_POOL,     /**< Buffer pool negotiation type */
	MPIPE_DISPATCH_END = UINT8_MAX, /**< Maximum dispatch type identifier */
};

/**
 * @struct mpipe_dispatch
 * @brief A query or an event traveling a pad link.
 *
 * A dispatch travels a synchronous walk, so it references storage that
 * outlives the walk instead of embedding a copy of everything it carries.
 * It is declared with a designated initializer and has no init function -
 * zero is the neutral value of every field:
 *
 * @code
 * struct mpipe_dispatch query = {
 *         .type = MPIPE_DISPATCH_CAPS,
 *         .caps = &caps_storage,
 * };
 * @endcode
 */
struct mpipe_dispatch {
	/** Dispatch type from @ref mpipe_dispatch_type */
	uint8_t type;
	/**
	 * The capability carried, or NULL when the dispatch carries none.
	 *
	 * The creator points this at storage it owns for the whole walk;
	 * every other element reads through it and answers by copying INTO
	 * it. Repointing it at storage of your own dangles the moment your
	 * frame returns.
	 *
	 * | Value | On a query (in/out storage) | On an event (an announcement) |
	 * | :--- | :--- | :--- |
	 * | a capability | the filter to answer within | the format decided on |
	 * | ANY | no constraint: answer with all you support | never send one |
	 * | NULL | caller error: nowhere to answer | nothing to announce |
	 *
	 * A query function may dereference this without checking: a caps query
	 * carrying no storage is refused by mpipe_pad_query() with -EINVAL
	 * before any of them is called. An event handler must check, since NULL
	 * is a valid thing for an event to carry.
	 *
	 * A caps event carrying none is a trigger rather than an
	 * announcement: a source that knows no format (a file source) still
	 * sends one, and the first element that does know one puts it in and
	 * announces it downstream.
	 */
	struct mpipe_structure *caps;
	/** Buffer pool its proposer makes available (not owned) */
	struct mpipe_buffer_pool *pool;
	/** A pool config proposed without a pool, negotiated by value */
	struct mpipe_buffer_pool_config pool_cfg;
};

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_DISPATCH_H_ */
