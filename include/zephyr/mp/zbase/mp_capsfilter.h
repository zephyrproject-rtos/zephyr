/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mp
 * @brief Capsfilter element.
 *
 * This element does not modify data, but used to enforce limitations on the data format.
 *
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_CAPSFILTER_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_CAPSFILTER_H_

#include <zephyr/mp/core/mp_element.h>
#include <zephyr/mp/core/mp_property.h>
#include <zephyr/mp/core/mp_transform.h>

/**
 * @brief Caps filter Property Identifiers
 *
 * Defined property identifiers specific to the capsfilter element. These
 * properties extend the base transform properties defined in @ref prop_transform.
 *
 * The enumeration starts from PROP_TRANSFORM_LAST + 1 to ensure no
 * conflicts with base transform properties.
 */
enum {
	/** Caps ID property */
	PROP_CAPS = PROP_TRANSFORM_LAST + 1,
};

/**
 * @brief Capsfilter Element Structure
 *
 */
struct mp_caps_filter {
	/** Base transform element */
	struct mp_transform transform;
};

/**
 * @brief Initialize a caps filter element
 *
 * @param self Pointer to the @ref mp_element to initialize as a caps filter
 */
void mp_caps_filter_init(struct mp_element *self);

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_CAPSFILTER_H_ */
