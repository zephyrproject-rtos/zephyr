/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "region.h"

#ifdef CONFIG_LORAWAN_REGION_AS923
extern const struct lwan_region_ops as923_ops;
#endif
#ifdef CONFIG_LORAWAN_REGION_EU868
extern const struct lwan_region_ops eu868_ops;
#endif

#ifdef CONFIG_LORAWAN_REGION_KR920
extern const struct lwan_region_ops kr920_ops;
#endif
#ifdef CONFIG_LORAWAN_REGION_US915
extern const struct lwan_region_ops us915_ops;
#endif

const struct lwan_region_ops *lwan_region_get(enum lorawan_region region)
{
	switch (region) {
#ifdef CONFIG_LORAWAN_REGION_AS923
	case LORAWAN_REGION_AS923:
		return &as923_ops;
#endif
#ifdef CONFIG_LORAWAN_REGION_EU868
	case LORAWAN_REGION_EU868:
		return &eu868_ops;
#endif
#ifdef CONFIG_LORAWAN_REGION_KR920
	case LORAWAN_REGION_KR920:
		return &kr920_ops;
#endif
#ifdef CONFIG_LORAWAN_REGION_US915
	case LORAWAN_REGION_US915:
		return &us915_ops;
#endif
	default:
		return NULL;
	}
}
