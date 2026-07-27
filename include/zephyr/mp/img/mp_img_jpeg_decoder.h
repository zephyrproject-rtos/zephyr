/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp_img_jpeg_decoders
 * @brief Software JPEG decoder transform element.
 *
 * Decodes JPEG-compressed frames into raw pixel data using a software
 * decoder. Operates as a transform element within a media pipeline.
 */

#ifndef ZEPHYR_INCLUDE_MP_IMG_MP_IMG_JPEG_DECODER_H_
#define ZEPHYR_INCLUDE_MP_IMG_MP_IMG_JPEG_DECODER_H_

/**
 * @defgroup mp_img_jpeg_decoders Decoders
 * @ingroup mp_img
 * @brief JPEG decoder elements.
 * @{
 */

#include <zephyr/mp/mp_transform.h>
#include <zephyr/mp/img/jpeg_dec.h>

/**
 * @brief Software JPEG decoder element.
 *
 * Wraps a @ref JPEGIMAGE state and an output buffer pool to decode
 * JPEG frames received on the sink pad into raw video on the source pad.
 */
struct mp_img_jpeg_decoder {
	/** Base transform element */
	struct mp_transform transform;
	/** JPEG decoder state */
	JPEGIMAGE jpg;
	/** Output pixel format (VIDEO_PIX_FMT_*) */
	uint32_t out_pixfmt;
	/** Internal output pool (fallback when downstream does not propose a pool) */
	struct mp_buffer_pool out_pool;
};

/**
 * @brief Initialize a software JPEG decoder element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_img_jpeg_decoder_init(struct mp_element *self);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_IMG_MP_IMG_JPEG_DECODER_H_ */
