/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio property definitions header file.
 */

#ifndef __MP_ZAUD_PROPS_H__
#define __MP_ZAUD_PROPS_H__

#include <src/core/mp_property.h>

/**
 * @brief Audio transform property identifiers
 *
 * Enumeration defining property IDs specific to transform elements.
 * These properties extend the base transform properties.
 */
typedef enum {
	/** Gain control property for audio level adjustment */
	PROP_GAIN = PROP_TRANSFORM_LAST + 1,
} PROP_ZAUD_TRANSFORM;

/**
 * @brief Audio source property identifiers
 *
 * Enumeration defining property IDs specific to source elements.
 * These properties extend the base source properties.
 */
typedef enum {
	/** Pointer to source memory slab for audio buffer management */
	PROP_ZAUD_SRC_SLAB_PTR = PROP_SRC_LAST + 1,
} PROP_ZAUD_SRC;

/**
 * @brief Audio sink property identifiers
 *
 * Enumeration defining property IDs specific to sink elements.
 * These properties extend the base sink properties.
 */
typedef enum {
	/** Pointer to sink memory slab for audio buffer management */
	PROP_ZAUD_SINK_SLAB_PTR = PROP_SINK_LAST + 1,
} PROP_ZAUD_SINK;

#endif /* __MP_ZAUD_PROPS_H__ */
