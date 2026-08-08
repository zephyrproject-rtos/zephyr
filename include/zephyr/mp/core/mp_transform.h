/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_transform.
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_TRANSFORM_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_TRANSFORM_H_

/**
 * @defgroup mp_transform Transforms
 * @ingroup mp_core
 * @brief Elements that process data between sink and source pads.
 *
 * @{
 */

#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_pad.h>

/**
 * @brief Operating modes of a transform element
 *
 * The modes specifies how input and output buffers are handled.
 */
enum mp_transform_mode {
	/** The buffer is kept intact. */
	MP_MODE_PASSTHROUGH = 0,
	/** The input buffer is directly modified. Input and output buffers are the same. */
	MP_MODE_INPLACE = 1,
	/** The output buffer is allocated and differs from the input buffer. */
	MP_MODE_NORMAL = 2,
};

/**
 * @brief Transform element structure
 *
 * Base structure for all transform elements in the media pipeline.
 * Transform elements process data from their sink pad and output
 * the result to their source pad.
 */
struct mp_transform {
	/** Base element structure */
	struct mp_element element;
	/** Sink pad for receiving input data */
	struct mp_pad sinkpad;
	/** Source pad for sending output data */
	struct mp_pad srcpad;
	/** @cond INTERNAL_HIDDEN */
	/** Pointer to the supported caps at sink */
	struct mp_caps *sink_caps;
	/** Pointer to the supported caps at source */
	struct mp_caps *src_caps;
	/** @endcond */
	/** Pointer to the input buffer pool for allocating input buffers */
	struct mp_buffer_pool *inpool;
	/** Pointer to the output buffer pool for allocating output buffers */
	struct mp_buffer_pool *outpool;
	/** Operating mode determining buffer handling strategy */
	enum mp_transform_mode mode;

	/**
	 * @brief Get the supported caps from an element's pad
	 *
	 * To get the current caps, use sinkpad->caps or srcpad->caps instead
	 *
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref mp_pad_direction)
	 * @return Pointer to @ref mp_caps or NULL on failure
	 */
	struct mp_caps *(*get_caps)(struct mp_transform *transform,
				    enum mp_pad_direction direction);
	/**
	 * @brief Set a given caps to an element's pad
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref mp_pad_direction)
	 * @param caps Capabilities to set (@ref mp_caps)
	 * @return 0 on success, negative errno on failure
	 */
	int (*set_caps)(struct mp_transform *transform, enum mp_pad_direction direction,
			struct mp_caps *caps);
	/**
	 * @brief Transform capabilities from one pad to another
	 * @param self Pointer to the transform element
	 * @param direction Direction to transform capabilities to (@ref mp_pad_direction)
	 * @param incaps Input capabilities (@ref mp_caps)
	 * @return Transformed capabilities (@ref mp_caps) or NULL on failure
	 */
	struct mp_caps *(*transform_caps)(struct mp_transform *self,
					  enum mp_pad_direction direction, struct mp_caps *incaps);

	/**
	 * @brief Propose allocation parameters to upstream
	 *
	 * The transform element may propose its input buffer pool to its upstream peer.
	 * All the proposed pool's ops are then intended to be called by the upstream element.
	 *
	 * For in-place transform, the same pool may be used for both input and output. If the
	 * pool is proposed to upstream, the element has to use @ref mp_transform::inpool to
	 * point to the pool and leave @ref mp_transform::outpool as NULL so that the pool is
	 * configured / started only by the upstream and not by the transform element itself.
	 *
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref mp_dispatch)
	 * @return 0 on success, negative errno on failure
	 */
	int (*propose_allocation)(struct mp_transform *self, struct mp_dispatch *query);
	/**
	 * @brief Decide allocation parameters for downstream
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref mp_dispatch)
	 * @return 0 on success, negative errno on failure
	 */
	int (*decide_allocation)(struct mp_transform *self, struct mp_dispatch *query);
};

/**
 * @brief Initialize a transform element
 *
 * This function initializes the base transform element structure,
 * sets up sink and source pads, and configures default function
 * pointers for element operations.
 *
 * @param self Pointer to the element to initialize (@ref mp_element)
 */
void mp_transform_init(struct mp_element *self);

/**
 * @brief Set capabilities on a transform element's pad.
 *
 * @param transform Pointer to the transform element.
 * @param direction Direction of the pad to set caps on (@ref mp_pad_direction).
 * @param caps Pointer to the capabilities to set.
 *
 * @return 0 on success, negative errno on failure
 */
int mp_transform_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
			  struct mp_caps *caps);

/**
 * @brief Update the capabilities of a transform element
 *
 * Updates the transform element's supported caps on sink/src and resets the negotiated
 * caps on both pads.
 *
 * @param transform Pointer to the transform element
 * @param sink_caps Supported caps for the sink pad
 * @param src_caps Supported caps for the src pad
 */
void mp_transform_update_caps(struct mp_transform *transform, struct mp_caps *sink_caps,
			      struct mp_caps *src_caps);

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_TRANSFORM_H_ */
