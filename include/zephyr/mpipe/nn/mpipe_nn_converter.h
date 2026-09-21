/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Video to tensor converter element.
 * @ingroup mpipe_nn
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_CONVERTER_H_
#define ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_CONVERTER_H_

/**
 * @addtogroup mpipe_nn
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/nn/mpipe_nn.h>
#include <zephyr/mpipe/nn/mpipe_nn_property.h>

struct mpipe_nn_converter;

/**
 * @brief One conversion the element implements
 */
struct mpipe_nn_converter_desc {
	/** Input pixel format */
	uint32_t pixfmt;
	/** Output tensor channels */
	uint8_t channels;
	/** Output tensor data type, an @ref mpipe_nn_tensor_type value */
	uint8_t type;
	/**
	 * @brief Convert one frame into one tensor
	 * @param conv The element, holding the negotiated geometry
	 * @param in Input frame
	 * @param out Output tensor buffer
	 * @return 0 on success, negative errno on failure
	 */
	int (*fn)(const struct mpipe_nn_converter *conv, const struct net_buf *in,
		  struct net_buf *out);
};

/**
 * @brief Video to tensor converter element
 *
 * Converts a video frame into a model input tensor in a single pass:
 * nearest-neighbor scaling, color conversion and quantization together. The
 * element always scales, so the video geometry and the tensor geometry
 * negotiate independently: the camera or display fixes one side, the model
 * fixes the other, and no separate resize element is needed.
 */
struct mpipe_nn_converter {
	/** Base transform element (must be first) */
	struct mpipe_transform transform;
	/** Input frame width */
	uint16_t in_w;
	/** Input frame height */
	uint16_t in_h;
	/** Output tensor width */
	uint16_t out_w;
	/** Output tensor height */
	uint16_t out_h;
	/** Input pixel format */
	uint32_t in_pixfmt;
	/** Output tensor data type, an @ref mpipe_nn_tensor_type value */
	uint8_t out_type;
	/** Output tensor channels */
	uint8_t out_channels;
	/** Quantization zero point applied to each sample */
	int32_t zero_point;
	/** True once MPIPE_PROP_NN_CONVERTER_ZERO_POINT overrode the default */
	bool zero_point_set;
	/** The conversion picked once both sides are negotiated */
	const struct mpipe_nn_converter_desc *desc;
	/** Internal output buffer pool */
	struct mpipe_buffer_pool out_pool;
};

/**
 * @brief Initialize a video to tensor converter element
 *
 * @param conv Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_nn_converter_init(struct mpipe_nn_converter *conv, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_CONVERTER_H_ */
