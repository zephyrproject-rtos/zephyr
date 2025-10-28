/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Main header for zdisp plugin properties.
 */

#ifndef __MP_ZDISP_PROPS_H__
#define __MP_ZDISP_PROPS_H__

#include <src/core/mp_property.h>

/**
 * @brief Zdisp Sink Property Identifiers
 *
 * Defined property identifiers specific to the zdisp sink element. These
 * properties extend the base sink properties defined in @ref prop_sink.
 *
 * The enumeration starts from @ref PROP_SINK_LAST + 1 to ensure no
 * conflicts with base sink properties.
 */
enum {
	/** Display device property identifier */
	PROP_ZDISP_SINK_DEVICE = PROP_SINK_LAST + 1,
};

#endif /* __MP_ZDISP_PROPS_H__ */
