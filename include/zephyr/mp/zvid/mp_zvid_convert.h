/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Software pixel format conversion transform element.
 *
 * Performs pixel format and colorspace conversions (no scaling or rotation).
 *
 * Currently supported conversions:
 *  - NV12 → RGB565
 *  - XRGB32 → ARGB32
 *  - ARGB32 → XRGB32
 */

#ifndef ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_CONVERT_H_
#define ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_CONVERT_H_

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_transform.h>

struct mp_zvid_convert_desc;

/**
 * @brief Software pixel format conversion element.
 *
 * Extends @ref mp_transform to convert frames between pixel formats
 * using a table-driven approach (see @ref mp_zvid_convert_desc).
 */
struct mp_zvid_convert {
	/** Base transform element */
	struct mp_transform transform;
	/** Negotiated frame width in pixels */
	uint16_t width;
	/** Negotiated frame height in pixels */
	uint16_t height;
	/** Input pixel format (VIDEO_PIX_FMT_*) */
	uint32_t in_pixfmt;
	/** Output pixel format (VIDEO_PIX_FMT_*) */
	uint32_t out_pixfmt;
	/** Internal output pool that allocates video_buffer-backed memory */
	struct mp_buffer_pool out_pool;
	/** FIFO of pre-allocated free buffers returned by @c out_pool */
	struct k_fifo free_fifo;
	/** Pre-allocated video buffers managed by @c out_pool */
	struct video_buffer *vbufs[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	/** Number of allocated video buffers */
	uint8_t vbuf_count;
	/** Selected conversion descriptor based on negotiated caps */
	const struct mp_zvid_convert_desc *desc;
	/** Internal implementation hooks (shared with table code) */
	struct {
		/** NV12 to RGB565 conversion function pointer */
		void (*nv12_to_rgb565)(uint16_t width, uint16_t height, const uint8_t *y_plane,
				       const uint8_t *uv_plane, uint16_t *rgb);
	} impl;
};

/**
 * @brief Initialize a software pixel format conversion element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_zvid_convert_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZVID_MP_ZVID_CONVERT_H_ */
