/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Stream parser base element.
 *
 * Provides a base class for elements that accumulate incoming data
 * into complete frames before pushing them downstream.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_PARSER_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_PARSER_H_

/**
 * @defgroup mp_parser Parsers
 * @ingroup mp_core
 * @brief Base parser element that parses encoded streams into frames.
 *
 * @{
 */

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>

/**
 * @brief Base parser element structure.
 *
 * Extends @ref mp_element with sink and source pads, capability
 * negotiation, and allocation hooks for frame-reassembly elements.
 */
struct mp_parser {
	/** Base element */
	struct mp_element element;
	/** Sink pad to receive data */
	struct mp_pad sinkpad;
	/** Source pad to send data */
	struct mp_pad srcpad;
	/** @cond INTERNAL_HIDDEN */
	/** Pointer to the supported caps at sink */
	struct mp_caps *sink_caps;
	/** Pointer to the supported caps at source */
	struct mp_caps *src_caps;
	/** @endcond */
	/** Pointer to the output buffer pool */
	struct mp_buffer_pool *outpool;

	/**
	 * @brief Get supported capabilities from a pad.
	 *
	 * To get the current caps, use sinkpad->caps or srcpad->caps instead.
	 *
	 * @param parser Pointer to the parser element.
	 * @param direction Pad direction (see @ref mp_pad_direction).
	 *
	 * @return Pointer to supported caps, or NULL on failure.
	 */
	struct mp_caps *(*get_caps)(struct mp_parser *parser, enum mp_pad_direction direction);
	/**
	 * @brief Set capabilities on a pad.
	 *
	 * @param parser Pointer to the parser element.
	 * @param direction Pad direction (see @ref mp_pad_direction).
	 * @param caps Pointer to the caps to set.
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*set_caps)(struct mp_parser *parser, enum mp_pad_direction direction,
			struct mp_caps *caps);
	/**
	 * @brief Propose allocation parameters to upstream.
	 *
	 * @param parser Pointer to the parser element.
	 * @param query Allocation query (see @ref mp_dispatch).
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*propose_allocation)(struct mp_parser *parser, struct mp_dispatch *query);
	/**
	 * @brief Decide allocation parameters for downstream.
	 *
	 * @param parser Pointer to the parser element.
	 * @param query Allocation query (see @ref mp_dispatch).
	 *
	 * @return 0 on success, negative errno on failure
	 */
	int (*decide_allocation)(struct mp_parser *parser, struct mp_dispatch *query);
};

/**
 * @brief Update the supported capabilities of a parser.
 *
 * @param parser    Pointer to the parser element.
 * @param sink_caps New sink capabilities (may be NULL).
 * @param src_caps  New source capabilities (may be NULL).
 */
void mp_parser_update_caps(struct mp_parser *parser, struct mp_caps *sink_caps,
			   struct mp_caps *src_caps);

/**
 * @brief Initialize a parser element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_parser_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_PARSER_H_ */
