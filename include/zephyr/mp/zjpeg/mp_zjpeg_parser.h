/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief JPEG stream parser element.
 *
 * Accumulates incoming data until a complete JPEG frame (SOI … EOI)
 * is assembled, then pushes it downstream as a single buffer.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZJPEG_PARSER_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZJPEG_PARSER_H_

#include <zephyr/mp/core/mp_parser.h>

/**
 * @brief JPEG stream parser element.
 *
 * Extends @ref mp_parser to reassemble JPEG frames from a byte stream.
 */
struct mp_zjpeg_parser {
	/** Base parser element */
	struct mp_parser base;
	/** Partial frame buffer, accumulated with memcpy until EOI */
	struct net_buf *partial_frame;
	/** Output pool used when downstream pool is not available */
	struct mp_buffer_pool out_pool;
};

/**
 * @brief Initialize a JPEG stream parser element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_zjpeg_parser_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZJPEG_PARSER_H_ */
