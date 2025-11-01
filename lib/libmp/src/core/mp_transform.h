/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpTransform.
 */

#ifndef __MP_TRANSFORM_H__
#define __MP_TRANSFORM_H__

#include "mp_element.h"
#include "mp_element_factory.h"
#include "mp_pad.h"

#define MP_TRANSFORM(self) ((MpTransform *)self)

typedef struct _MpTransform MpTransform;

/**
 * @brief Operating modes of a transform element
 *
 * The modes specifies how input and output buffers are handled.
 */
typedef enum {
	/** The buffer is kept intact. */
	MP_MODE_PASSTHROUGH = 0,
	/** The input buffer is directly modified. Input and output buffers are the same. */
	MP_MODE_INPLACE = 1,
	/** The output buffer is allocated and differs from the input buffer. */
	MP_MODE_NORMAL = 2,
} MpTransformMode;

/**
 * @brief Transform element structure
 *
 * Base structure for all transform elements in the media pipeline.
 * Transform elements process data from their sink pad and output
 * the result to their source pad.
 */
struct _MpTransform {
	/** Base element structure */
	MpElement element;
	/** Sink pad for receiving input data */
	MpPad sinkpad;
	/** Source pad for sending output data */
	MpPad srcpad;
	/** Input buffer pool for allocating input buffers */
	MpBufferPool *inpool;
	/** Output buffer pool for allocating output buffers */
	MpBufferPool *outpool;
	/** Operating mode determining buffer handling strategy */
	MpTransformMode mode;

	/**
	 * @brief Set capabilities on a pad
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref MpPadDirection)
	 * @param caps Capabilities to set (@ref MpCaps)
	 * @return true on success, false on failure
	 */
	bool (*set_caps)(MpTransform *transform, MpPadDirection direction, MpCaps *caps);
	/**
	 * @brief Get capabilities from a pad
	 * @param transform Pointer to the transform element
	 * @param direction Direction of the pad (@ref MpPadDirection)
	 * @return Pointer to @ref MpCaps or NULL on failure
	 */
	MpCaps *(*get_caps)(MpTransform *transform, MpPadDirection direction);
	/**
	 * @brief Transform capabilities from one pad to another
	 * @param self Pointer to the transform element
	 * @param direction Direction to transform capabilities to (@ref MpPadDirection)
	 * @param incaps Input capabilities (@ref MpCaps)
	 * @return Transformed capabilities (@ref MpCaps) or NULL on failure
	 */
	MpCaps *(*transform_caps)(MpTransform *self, MpPadDirection direction, MpCaps *incaps);

	/**
	 * @brief Propose allocation parameters to upstream
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref MpQuery)
	 * @return true on success, false on failure
	 */
	bool (*propose_allocation)(MpTransform *self, MpQuery *query);
	/**
	 * @brief Decide allocation parameters for downstream
	 * @param self Pointer to the transform element
	 * @param query Allocation query (@ref MpQuery)
	 * @return true on success, false on failure
	 */
	bool (*decide_allocation)(MpTransform *self, MpQuery *query);
};

/**
 * @brief Initialize a transform element
 *
 * This function initializes the base transform element structure,
 * sets up sink and source pads, and configures default function
 * pointers for element operations.
 *
 * @param self Pointer to the element to initialize (@ref MpElement)
 */
void mp_transform_init(MpElement *self);

/**
 * @brief Set a property on a transform element
 *
 * @param obj Pointer to the object (@ref MpObject)
 * @param key Property key identifier
 * @param val Pointer to the property value
 * @return 0 on success, negative error code on failure
 */
int mp_transform_set_property(MpObject *obj, uint32_t key, const void *val);

/**
 * @brief Get a property from a transform element
 *
 * @param obj Pointer to the object (@ref MpObject)
 * @param key Property key identifier
 * @param val Pointer to store the property value
 * @return 0 on success, negative error code on failure
 */
int mp_transform_get_property(MpObject *obj, uint32_t key, void *val);

#endif /* __MP_TRANSFORM_H__ */
