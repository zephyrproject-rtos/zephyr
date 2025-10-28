/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_transform.
 */

#ifndef __MP_TRANSFORM_H__
#define __MP_TRANSFORM_H__

#include "mp_element.h"
#include "mp_pad.h"

#define MP_TRANSFORM(self) ((struct mp_transform *)self)

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
	/** Input buffer pool for allocating input buffers */
	struct mp_buffer_pool *inpool;
	/** Output buffer pool for allocating output buffers */
	struct mp_buffer_pool *outpool;
	/** Operating mode determining buffer handling strategy */
	enum mp_transform_mode mode;

	/**
	 * @brief Set capabilities on a pad
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref enum mp_pad_direction)
	 * @param caps Capabilities to set (@ref struct mp_caps)
	 * @return true on success, false on failure
	 */
	bool (*set_caps)(struct mp_transform *transform, enum mp_pad_direction direction,
			 struct mp_caps *caps);
	/**
	 * @brief Get capabilities from a pad
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref enum mp_pad_direction)
	 * @return Pointer to @ref struct mp_caps or NULL on failure
	 */
	struct mp_caps *(*get_caps)(struct mp_transform *transform,
				    enum mp_pad_direction direction);
	/**
	 * @brief Transform capabilities from one pad to another
	 * @param self Pointer to the transform element
	 * @param direction Direction to transform capabilities to (@ref enum mp_pad_direction)
	 * @param incaps Input capabilities (@ref struct mp_caps)
	 * @return Transformed capabilities (@ref struct mp_caps) or NULL on failure
	 */
	struct mp_caps *(*transform_caps)(struct mp_transform *self,
					  enum mp_pad_direction direction, struct mp_caps *incaps);

	/**
	 * @brief Propose allocation parameters to upstream
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref struct mp_query)
	 * @return true on success, false on failure
	 */
	bool (*propose_allocation)(struct mp_transform *self, struct mp_query *query);
	/**
	 * @brief Decide allocation parameters for downstream
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref struct mp_query)
	 * @return true on success, false on failure
	 */
	bool (*decide_allocation)(struct mp_transform *self, struct mp_query *query);
};

/**
 * @brief Initialize a transform element
 *
 * This function initializes the base transform element structure,
 * sets up sink and source pads, and configures default function
 * pointers for element operations.
 *
 * @param self Pointer to the element to initialize (@ref struct mp_element)
 */
void mp_transform_init(struct mp_element *self);

/**
 * @brief Set a property on a transform element
 *
 * @param obj Pointer to the object (@ref struct mp_object)
 * @param key Property key identifier
 * @param val Pointer to the property value
 * @return 0 on success, negative error code on failure
 */
int mp_transform_set_property(struct mp_object *obj, uint32_t key, const void *val);

/**
 * @brief Get a property from a transform element
 *
 * @param obj Pointer to the object (@ref struct mp_object)
 * @param key Property key identifier
 * @param val Pointer to store the property value
 * @return 0 on success, negative error code on failure
 */
int mp_transform_get_property(struct mp_object *obj, uint32_t key, void *val);

bool mp_transform_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
			   struct mp_caps *caps);

#endif /* __MP_TRANSFORM_H__ */
