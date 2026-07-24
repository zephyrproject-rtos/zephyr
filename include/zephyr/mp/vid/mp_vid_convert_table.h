/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Pixel format conversion lookup table for @ref mp_vid_convert.
 *
 * Provides a table of conversion descriptors that map input/output pixel
 * format pairs to their conversion functions.
 */

#ifndef ZEPHYR_INCLUDE_MP_VID_MP_VID_CONVERT_TABLE_H_
#define ZEPHYR_INCLUDE_MP_VID_MP_VID_CONVERT_TABLE_H_

/**
 * @defgroup mp_vid_conversion_tables Conversion Tables
 * @ingroup mp_vid
 * @brief Pixel format conversion table definitions.
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/video.h>

struct net_buf;
struct mp_vid_convert;

/**
 * @brief Conversion function signature.
 *
 * Converts one complete frame from @p in to @p out using the
 * parameters stored in @p conv.
 *
 * @param conv Pointer to the convert element holding negotiated parameters.
 * @param in   Input buffer containing the source frame.
 * @param out  Output buffer to receive the converted frame.
 *
 * @return 0 on success or a negative errno code on failure.
 */
typedef int (*mp_vid_convert_fn_t)(struct mp_vid_convert *conv, const struct net_buf *in,
				    struct net_buf *out);

/**
 * @brief Pixel format conversion descriptor.
 *
 * Maps an input/output pixel format pair to its conversion function.
 */
struct mp_vid_convert_desc {
	/** Input pixel format (VIDEO_PIX_FMT_*) */
	uint32_t in_pixfmt;
	/** Output pixel format (VIDEO_PIX_FMT_*) */
	uint32_t out_pixfmt;
	/** Conversion function for this format pair */
	mp_vid_convert_fn_t fn;
};

/** Table of supported conversion descriptors. */
extern const struct mp_vid_convert_desc mp_vid_convert_descs[];

/** Number of entries in @ref mp_vid_convert_descs. */
extern const size_t mp_vid_convert_descs_len;

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_VID_MP_VID_CONVERT_TABLE_H_ */
