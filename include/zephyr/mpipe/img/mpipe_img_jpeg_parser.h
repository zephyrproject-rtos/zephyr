/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mpipe_img_jpeg_parsers
 * @brief JPEG stream parser element.
 *
 * Accumulates incoming data until a complete JPEG frame (SOI to EOI)
 * is assembled, then pushes it downstream as a single buffer.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_JPEG_PARSER_H_
#define ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_JPEG_PARSER_H_

/**
 * @defgroup mpipe_img_jpeg_parsers Parsers
 * @ingroup mpipe_img
 * @brief JPEG parser elements.
 * @{
 */

#include <zephyr/mpipe/mpipe_parser.h>

/**
 * @brief JPEG stream parser element.
 *
 * Extends @ref mpipe_parser to reassemble JPEG frames from a byte stream.
 */
struct mpipe_img_jpeg_parser {
	/** Base parser element */
	struct mpipe_parser base;
	/** Partial frame buffer, accumulated with memcpy until EOI */
	struct net_buf *partial_frame;
	/** Output pool used when downstream pool is not available */
	struct mpipe_buffer_pool out_pool;
};

/**
 * @brief Initialize a JPEG stream parser element.
 *
 * @param jpeg_parser Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_img_jpeg_parser_init(struct mpipe_img_jpeg_parser *jpeg_parser, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_JPEG_PARSER_H_ */
