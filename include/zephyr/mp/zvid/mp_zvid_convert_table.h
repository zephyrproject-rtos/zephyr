/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Pixel format conversion lookup table for @ref mp_zvid_convert.
 *
 * Provides a table of conversion descriptors that map input/output pixel
 * format pairs to their conversion functions.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_CONVERT_TABLE_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_CONVERT_TABLE_H_

#include <stddef.h>
#include <stdint.h>

#include <zephyr/drivers/video.h>

struct net_buf;
struct mp_zvid_convert;

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
typedef int (*mp_zvid_convert_fn_t)(struct mp_zvid_convert *conv, const struct net_buf *in,
				    struct net_buf *out);

/**
 * @brief Pixel format conversion descriptor.
 *
 * Maps an input/output pixel format pair to its conversion function.
 */
struct mp_zvid_convert_desc {
	/** Input pixel format (VIDEO_PIX_FMT_*) */
	uint32_t in_pixfmt;
	/** Output pixel format (VIDEO_PIX_FMT_*) */
	uint32_t out_pixfmt;
	/** Conversion function for this format pair */
	mp_zvid_convert_fn_t fn;
};

/** Table of supported conversion descriptors. */
extern const struct mp_zvid_convert_desc mp_zvid_convert_descs[];

/** Number of entries in @ref mp_zvid_convert_descs. */
extern const size_t mp_zvid_convert_descs_len;

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_CONVERT_TABLE_H_ */
