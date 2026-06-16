/*
 * Copyright 2026 Meta Platforms, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Software JPEG encoder transform element.
 *
 * Encodes raw RGB565 frames into JPEG-compressed frames using a software
 * encoder. Operates as a transform element within a media pipeline and is
 * the mirror of @ref mp_zjpeg_decoder.
 */

#ifndef ZEPHYR_INCLUDE_MP_ZJPEG_MP_ZJPEG_ENCODER_H_
#define ZEPHYR_INCLUDE_MP_ZJPEG_MP_ZJPEG_ENCODER_H_

#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_transform.h>

#include <zephyr/mp/zjpeg/jpeg_enc.h>

#define MP_ZJPEG_ENCODER(self) ((struct mp_zjpeg_encoder *)self)

/**
 * @brief Software JPEG encoder property identifiers.
 *
 * Extends the base transform properties defined in @ref mp_property.h.
 */
enum prop_zjpeg_encoder {
	/** Compression quality (one of the JPEGE_Q_* values; uint8_t). */
	PROP_ZJPEG_ENC_QUALITY = PROP_TRANSFORM_LAST + 1,
	/** Chroma subsampling (one of the JPEGE_SUBSAMPLE_* values; uint8_t). */
	PROP_ZJPEG_ENC_SUBSAMPLE,
};

/**
 * @brief Software JPEG encoder element.
 *
 * Wraps a @ref JPEGE_IMAGE state and an output buffer pool to encode RGB565
 * frames received on the sink pad into JPEG frames on the source pad.
 */
struct mp_zjpeg_encoder {
	/** Base transform element */
	struct mp_transform transform;
	/** JPEG encoder state */
	JPEGE_IMAGE jpe;
	/** Internal output pool (fallback when downstream does not propose a pool) */
	struct mp_buffer_pool out_pool;
	/** Chroma subsampling (JPEGE_SUBSAMPLE_*) */
	uint8_t subsample;
	/** Compression quality (JPEGE_Q_*) */
	uint8_t qfactor;
	/** Cached frame width in pixels */
	uint32_t width;
	/** Cached frame height in pixels */
	uint32_t height;
};

/**
 * @brief Initialize a software JPEG encoder element.
 *
 * @param self Pointer to the @ref mp_element to initialize.
 */
void mp_zjpeg_encoder_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_ZJPEG_MP_ZJPEG_ENCODER_H_ */
