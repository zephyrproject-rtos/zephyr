/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Capsfilter element.
 *
 * This element does not modify data, but used to enforce limitations on the data format.
 *
 */

#ifndef __MP_CAPSFILTER_H__
#define __MP_CAPSFILTER_H__

#include "mp_element.h"
#include "mp_property.h"
#include "mp_transform.h"

/**
 * @brief Caps filter Property Identifiers
 *
 * Defined property identifiers specific to the capsfilter element. These
 * properties extend the base transform properties defined in @ref PROP_TRANSFORM.
 *
 * The enumeration starts from @ref PROP_TRANSFORM_LAST + 1 to ensure no
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
 * @param self Pointer to the @ref struct mp_element to initialize as a caps filter
 */
void mp_caps_filter_init(struct mp_element *self);

#endif /* __MP_CAPSFILTER_H__ */
