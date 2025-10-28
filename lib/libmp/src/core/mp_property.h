/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for MpProperty.
 */

#ifndef __MP_PROPS_H__
#define __MP_PROPS_H__

/**
 * @{
 */

/** Sentinel value to mark the end of property lists */
#define PROP_LIST_END -1

/**
 * @brief Properties for a base source element
 *
 * Enumeration of properties that can be configured for base source
 */
typedef enum {
	/** Number of buffers */
	PROP_NUM_BUFS,
	/** Last source property marker (for validation/iteration) */
	PROP_SRC_LAST,
} PROP_SRC;

/**
 * @brief Properties for a transform element
 *
 * Enumeration of properties that can be configured for base transform
 */
typedef enum {
	/** Last transform property marker (for validation/iteration) */
	PROP_TRANSFORM_LAST,
} PROP_TRANSFORM;

/**
 * @brief Properties for a sink element
 *
 * Enumeration of properties that can be configured for base sink
 */
typedef enum {
	/** Last sink property marker (for validation/iteration) */
	PROP_SINK_LAST,
} PROP_SINK;

/**
 * @}
 */

#endif /* __MP_PROPS_H__ */
