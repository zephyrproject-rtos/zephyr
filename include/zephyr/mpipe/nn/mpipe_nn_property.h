/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Property identifiers of the neural network plugin.
 * @ingroup mpipe_nn
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_PROPERTY_H_
#define ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_PROPERTY_H_

/**
 * @addtogroup mpipe_nn
 * @{
 */

#include <zephyr/sys/util_macro.h>

#include <zephyr/mpipe/mpipe_sink.h>
#include <zephyr/mpipe/mpipe_src.h>
#include <zephyr/mpipe/mpipe_transform.h>

/**
 * @brief Neural network element property identifiers
 *
 * The enumeration starts past the last identifier of every base class so it
 * never collides with one.
 */
enum mpipe_prop_nn {
	/** Model blob, a const struct mpipe_nn_model pointer, copied */
	MPIPE_PROP_NN_MODEL = MAX((int)MPIPE_PROP_SINK_LAST,
				  MAX((int)MPIPE_PROP_SRC_LAST, (int)MPIPE_PROP_TRANSFORM_LAST)),
	/** Backend working memory, a const struct mpipe_nn_arena pointer, copied */
	MPIPE_PROP_NN_ARENA,
	/**
	 * Quantization zero point of the converter, an int32_t pointer: the
	 * value added to each 8-bit sample before it is stored, clamped to the
	 * tensor type's range. Unset, samples are stored as raw bytes. A model
	 * trained on pixels scaled to -1..1, the stock person detection model
	 * among them, needs -128.
	 */
	MPIPE_PROP_NN_CONVERTER_ZERO_POINT,
	/**
	 * Kind of results to decode, a uint8_t pointer, an mpipe_nn_results_type
	 * value; classification by default
	 */
	MPIPE_PROP_NN_DECODER_TYPE,
	/** Detection score threshold, a float pointer; 0.5 by default */
	MPIPE_PROP_NN_DECODER_THRESHOLD,
	/**
	 * Intersection over union above which non-maximum suppression drops a
	 * box overlapping a better one, a float pointer; scored box list
	 * layout only, 0.5 by default
	 */
	MPIPE_PROP_NN_DECODER_NMS_IOU,
};

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_PROPERTY_H_ */
