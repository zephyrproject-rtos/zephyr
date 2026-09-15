/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mpipe_img_jpeg_decoders
 * @brief Software JPEG decoder transform element.
 *
 * Decodes JPEG-compressed frames into raw pixel data using a software
 * decoder. Operates as a transform element within a media pipeline.
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_JPEG_DECODER_H_
#define ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_JPEG_DECODER_H_

/**
 * @defgroup mpipe_img_jpeg_decoders Decoders
 * @ingroup mpipe_img
 * @brief JPEG decoder elements.
 * @{
 */

#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/img/jpeg_dec.h>

/**
 * @brief Software JPEG decoder element.
 *
 * Wraps a @ref JPEGIMAGE state and an output buffer pool to decode
 * JPEG frames received on the sink pad into raw video on the source pad.
 */
struct mpipe_img_jpeg_decoder {
	/** Base transform element */
	struct mpipe_transform transform;
	/** JPEG decoder state */
	JPEGIMAGE jpg;
	/** Output pixel format (VIDEO_PIX_FMT_*) */
	uint32_t out_pixfmt;
	/** Internal output pool (fallback when downstream does not propose a pool) */
	struct mpipe_buffer_pool out_pool;
};

/**
 * @brief Initialize a software JPEG decoder element.
 *
 * @param dec Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_img_jpeg_decoder_init(struct mpipe_img_jpeg_decoder *dec, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_JPEG_DECODER_H_ */
