/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "region.h"
#include "../lorawan.h"

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

int lwan_region_tx_params(const struct lwan_ctx *ctx, uint8_t dr,
			  uint8_t tx_power_idx, struct lwan_dr_params *p,
			  int8_t *power_dbm)
{
	if (ctx->mac.ul_dwell_time && ctx->region->get_dwell_tx_params != NULL) {
		return ctx->region->get_dwell_tx_params(dr, tx_power_idx, p, power_dbm);
	}

	return ctx->region->get_tx_params(dr, tx_power_idx, p, power_dbm);
}

int lwan_region_rx1_params(const struct lwan_ctx *ctx, uint32_t tx_freq,
			   uint8_t tx_dr, uint8_t offset, uint32_t *rx1_freq,
			   struct lwan_dr_params *p)
{
	if (ctx->mac.dl_dwell_time && ctx->region->get_dwell_rx1_params != NULL) {
		return ctx->region->get_dwell_rx1_params(tx_freq, tx_dr, offset,
							 rx1_freq, p);
	}

	return ctx->region->get_rx1_params(tx_freq, tx_dr, offset, rx1_freq, p);
}
