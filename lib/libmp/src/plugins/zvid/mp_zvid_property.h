/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zvid plugin properties.
 */

#ifndef __MP_ZVID_PROPS_H__
#define __MP_ZVID_PROPS_H__

#include <zephyr/sys/util.h>

#include <src/core/mp_property.h>

/**
 * @brief Zvid Property Identifiers
 *
 * This extends the base source and transform properties defined in @ref prop_src and @ref
 * prop_transform
 *
 * The enumeration starts from @ref PROP_TRANSFORM_LAST + 1 to ensure no
 * conflicts with base source or transform properties.
 */
enum prop_zvid {
	/** Video device property */
	PROP_ZVID_DEVICE = MAX((int)PROP_SRC_LAST, (int)PROP_TRANSFORM_LAST) + 1,
	/** Crop selection target property */
	PROP_ZVID_CROP,
};

#endif /* __MP_ZVID_PROPS_H__ */
