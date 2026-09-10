/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mpipe_vid_converters
 * @brief Software pixel format conversion transform element.
 *
 * Performs pixel format and colorspace conversions (no scaling or rotation).
 *
 * Currently supported conversions:
 *  - NV12 -> RGB565
 *  - XRGB32 -> ARGB32
 *  - ARGB32 -> XRGB32
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_CONVERT_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_CONVERT_H_

/**
 * @defgroup mpipe_vid_converters Converters
 * @ingroup mpipe_vid
 * @brief Software pixel format conversion elements.
 * @{
 */

#include <zephyr/drivers/video.h>
#include <zephyr/kernel.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_transform.h>

struct mpipe_vid_convert_desc;

/**
 * @brief Software pixel format conversion element.
 *
 * Extends @ref mpipe_transform to convert frames between pixel formats
 * using a table-driven approach (see @ref mpipe_vid_convert_desc).
 */
struct mpipe_vid_convert {
	/** Base transform element */
	struct mpipe_transform transform;
	/** Negotiated frame width in pixels */
	uint16_t width;
	/** Negotiated frame height in pixels */
	uint16_t height;
	/** Input pixel format (VIDEO_PIX_FMT_*) */
	uint32_t in_pixfmt;
	/** Output pixel format (VIDEO_PIX_FMT_*) */
	uint32_t out_pixfmt;
	/** Internal output pool that allocates video_buffer-backed memory */
	struct mpipe_buffer_pool out_pool;
	/** FIFO of pre-allocated free buffers returned by @c out_pool */
	struct k_fifo free_fifo;
	/** Pre-allocated video buffers managed by @c out_pool */
	struct video_buffer *vbufs[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
	/** Number of allocated video buffers */
	uint8_t vbuf_count;
	/** Selected conversion descriptor based on negotiated caps */
	const struct mpipe_vid_convert_desc *desc;
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
 * @param conv Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_vid_convert_init(struct mpipe_vid_convert *conv, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_CONVERT_H_ */
