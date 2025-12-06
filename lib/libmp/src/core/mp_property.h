/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for mp_property.
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
enum prop_src {
	/** Number of buffers that the source outputs before sending EOS */
	PROP_NUM_BUFS,
	/** Last source property marker (for validation/iteration) */
	PROP_SRC_LAST,
};

/**
 * @brief Properties for a transform element
 *
 * Enumeration of properties that can be configured for base transform
 */
enum prop_transform {
	/** Last transform property marker (for validation/iteration) */
	PROP_TRANSFORM_LAST,
};

/**
 * @brief Properties for a sink element
 *
 * Enumeration of properties that can be configured for base sink
 */
enum prop_sink {
	/** Last sink property marker (for validation/iteration) */
	PROP_SINK_LAST,
};

/**
 * @}
 */

#endif /* __MP_PROPS_H__ */
