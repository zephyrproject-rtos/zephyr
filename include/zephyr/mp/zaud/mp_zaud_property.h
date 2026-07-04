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

/**
 * @defgroup mp_zaud_properties Properties
 * @ingroup mp_zaud
 * @brief Audio property identifiers for zaud elements.
 * @{
 */

#include <zephyr/mp/core/mp_property.h>

/**
 * @brief Audio transform property identifiers
 *
 * Enumeration defining property IDs specific to transform elements.
 * These properties extend the base transform properties.
 */
enum prop_zaud_transform {
	/** Gain control property for audio level adjustment */
	PROP_GAIN = PROP_TRANSFORM_LAST + 1,
};

/**
 * @brief Audio source property identifiers
 *
 * Enumeration defining property IDs specific to source elements.
 * These properties extend the base source properties.
 */
enum prop_zaud_src {
	/** Pointer to source memory slab for audio buffer management */
	PROP_ZAUD_SRC_SLAB_PTR = PROP_SRC_LAST + 1,
	/** Audio source device */
	PROP_ZAUD_SRC_DEVICE,
};

/**
 * @brief Audio sink property identifiers
 *
 * Enumeration defining property IDs specific to sink elements.
 * These properties extend the base sink properties.
 */
enum prop_zaud_sink {
	/** Pointer to sink memory slab for audio buffer management */
	PROP_ZAUD_SINK_SLAB_PTR = PROP_SINK_LAST + 1,
	/** Clock role configuration for audio sink (controller/target) */
	PROP_ZAUD_SINK_CLK_ROLE,
	/** I2S (SAI) sink device */
	PROP_ZAUD_SINK_I2S_DEVICE,
	/** Codec sink device */
	PROP_ZAUD_SINK_CODEC_DEVICE,
};

/** @} */

#endif /* __MP_ZAUD_PROPS_H__ */
