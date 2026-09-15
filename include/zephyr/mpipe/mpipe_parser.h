/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stream parser base element.
 * @ingroup mpipe_parser
 *
 * Provides a base class for elements that accumulate incoming data
 * into complete frames before pushing them downstream.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_PARSER_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_PARSER_H_

/**
 * @defgroup mpipe_parser Parsers
 * @ingroup mpipe_framework
 * @brief Elements that cut a formless byte stream into frames.
 *
 * A parser sits where a stream stops being bytes and starts being frames. Its
 * input is a file or a socket, which can say nothing about what it carries; its
 * output is whole frames of a format the parser worked out by looking at them.
 *
 * That asymmetry is why a parser is its own base rather than a transform. It
 * cannot map its output back to an input capability, so it does not implement
 * a capability crossing; it simply describes what it produces. And because the
 * element upstream of it knows no format, the format event that arrives from
 * there announces nothing at all - the parser is what turns it into a real
 * announcement by substituting the capability it settled on before passing it
 * downstream.
 *
 * A parser also usually needs one buffer more than the stream in flight, to
 * hold the frame it is still assembling.
 *
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pad.h>

/**
 * @brief Base parser element structure.
 *
 * Extends @ref mpipe_element with sink and source pads, capability
 * negotiation, and buffer pool hooks for frame-reassembly elements.
 */
struct mpipe_parser {
	/** Base element */
	struct mpipe_element element;
	/** Sink pad to receive data */
	struct mpipe_pad sink_pad;
	/** Source pad to send data */
	struct mpipe_pad src_pad;
	/** Pointer to the output buffer pool */
	struct mpipe_buffer_pool *out_pool;

	/**
	 * @brief Set a capability on a pad.
	 *
	 * @param parser Pointer to the parser element.
	 * @param direction Pad direction (see @ref mpipe_pad_direction).
	 * @param caps Pointer to the capability to set.
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*set_caps)(struct mpipe_parser *parser, enum mpipe_pad_direction direction,
			const struct mpipe_structure *caps);
	/**
	 * @brief Propose a buffer pool to upstream.
	 *
	 * @param parser Pointer to the parser element.
	 * @param query Allocation query (see @ref mpipe_dispatch).
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*propose_buffer_pool)(struct mpipe_parser *parser, struct mpipe_dispatch *query);
	/**
	 * @brief Decide the buffer pool for downstream.
	 *
	 * A demand reaches a proposed pool only through
	 * @ref mpipe_buffer_pool_set_config, which the pool may refuse - keep a
	 * fallback. An element deciding for its own pool rebuilds its config
	 * baseline here on every negotiation.
	 *
	 * @param parser Pointer to the parser element.
	 * @param query Allocation query (see @ref mpipe_dispatch).
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*decide_buffer_pool)(struct mpipe_parser *parser, struct mpipe_dispatch *query);
};

/**
 * @brief Initialize a parser element.
 *
 * Initializes the base parser element structure, its sink and source pads and
 * the default callbacks. A concrete parser calls it first from its own init,
 * with the same id, and then overrides what it needs.
 *
 * @param parser Pointer to the @ref mpipe_parser to initialize.
 * @param id     Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_parser_init(struct mpipe_parser *parser, uint8_t id);

/**
 * @brief Change state function for the base parser element
 *
 * On the PAUSED to READY transition this resets the negotiated pad caps back to
 * ANY so a subsequent re-negotiation starts fresh.
 * Derived parsers that override change_state must chain to this base function
 * to inherit that behavior.
 *
 * @param self Pointer to the @ref mpipe_element
 * @param transition Transition state, see @ref mpipe_state_change
 *
 * @return One of @ref mpipe_state_change_return
 */
enum mpipe_state_change_return mpipe_parser_change_state(struct mpipe_element *self,
							 enum mpipe_state_change transition);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_PARSER_H_ */
