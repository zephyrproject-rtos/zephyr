/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Tensor decoder element.
 * @ingroup mpipe_nn
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_DECODER_H_
#define ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_DECODER_H_

/**
 * @addtogroup mpipe_nn
 * @{
 */

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_transform.h>
#include <zephyr/mpipe/nn/mpipe_nn.h>
#include <zephyr/mpipe/nn/mpipe_nn_property.h>
#include <zephyr/mpipe/nn/mpipe_nn_results.h>

/**
 * @brief Tensor decoder element
 *
 * A transform turning the output tensors of an inference into a results
 * stream: it dequantizes and ranks class scores (classification) or filters
 * boxes by score (detection). Two detection layouts are understood: the
 * four-tensor TFLite_Detection_PostProcess convention (boxes, classes,
 * scores, count), and a scored box list, one tensor [N, C + 4] of C class
 * scores (class 0 being the background when C > 1) and a normalized
 * (xmin, ymin, xmax, ymax) box per row, as the Ultraface models produce; the
 * latter goes through non-maximum suppression. Each produced buffer carries
 * one struct mpipe_nn_results and negotiates as MPIPE_MEDIA_NN_RESULTS; an
 * app_sink typically terminates the branch and hands the results to the
 * application.
 */
struct mpipe_nn_decoder {
	/** Base transform element (must be first) */
	struct mpipe_transform transform;
	/** Kind of results to produce, an @ref mpipe_nn_results_type value */
	uint8_t decode_type;
	/** Detection score threshold */
	float threshold;
	/** Intersection over union above which a box is suppressed */
	float nms_iou;
	/** Boxes above the threshold, ranked, awaiting suppression */
	struct mpipe_nn_box cand[CONFIG_MPIPE_NN_DECODER_CANDIDATES_MAX];
	/** Sequence counter across inferences */
	uint32_t seq;
	/** Internal output buffer pool */
	struct mpipe_buffer_pool out_pool;
};

/**
 * @brief Initialize a tensor decoder element
 *
 * @param decode Pointer to the element to initialize.
 * @param id Unique element identifier.
 *
 * @return 0 on success, negative errno otherwise.
 */
int mpipe_nn_decoder_init(struct mpipe_nn_decoder *decode, uint8_t id);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_NN_MPIPE_NN_DECODER_H_ */
