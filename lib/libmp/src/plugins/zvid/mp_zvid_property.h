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

#include <src/core/mp_property.h>

/**
 * @brief Zvid Transform Property Identifiers
 *
 * Defined property identifiers specific to zvid transform element. These
 * properties extend the base transform properties defined in @ref PROP_TRANSFORM.
 *
 * The enumeration starts from @ref PROP_TRANSFORM_LAST + 1 to ensure no
 * conflicts with base transform properties.
 */
typedef enum {
	/** Device property identifier */
	PROP_DEVICE = PROP_TRANSFORM_LAST + 1,
} PROP_ZVID_TRANSFORM;

#endif /* __MP_ZVID_PROPS_H__ */
