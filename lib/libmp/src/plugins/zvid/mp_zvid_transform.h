/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid transform element.
 */

#ifndef __MP_ZVID_TRANSFORM_H__
#define __MP_ZVID_TRANSFORM_H__

#include <src/core/mp_element.h>
#include <src/core/mp_transform.h>

#include "mp_zvid_object.h"

#define MP_ZVID_TRANSFORM(self) ((MpZvidTransform *)self)

/**
 * @brief Video Transform Element Structure
 *
 * This structure represents a video transform element. It contains separate
 * video objects for input and output operations, allowing for memory-to-memory
 * (m2m) video transformations.
 *
 * The transform element can perform various video operations such as:
 * - Format conversion between different pixel formats
 * - Video scaling (upscaling/downscaling)
 * - Other hardware-supported video transformations
 */
typedef struct {
	/** Base transform element */
	MpTransform transform;
	/** Input video object for receiving video data */
	MpZvidObject zvid_obj_in;
	/** Output video object for producing transformed video data */
	MpZvidObject zvid_obj_out;
} MpZvidTransform;

/**
 * @brief Initialize a Video Transform Element
 *
 * @param self Pointer to the @ref MpElement to initialize as a video transform
 */
void mp_zvid_transform_init(MpElement *self);

#endif /* __MP_ZVID_TRANSFORM_H__ */
