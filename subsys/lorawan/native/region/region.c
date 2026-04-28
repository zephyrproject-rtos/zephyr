/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "region.h"

#ifdef CONFIG_LORAWAN_REGION_EU868
extern const struct lwan_region_ops eu868_ops;
#endif

const struct lwan_region_ops *lwan_region_get(enum lorawan_region region)
{
	switch (region) {
#ifdef CONFIG_LORAWAN_REGION_EU868
	case LORAWAN_REGION_EU868:
		return &eu868_ops;
#endif
	default:
		return NULL;
	}
}
