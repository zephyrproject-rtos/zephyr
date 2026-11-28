/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TAILSCALE_ROUTES_INTERNAL_H_
#define TAILSCALE_ROUTES_INTERNAL_H_

#include "tailscale_internal.h"

struct route_plan {
	const struct tailscale_peer_snapshot *map;
	uint64_t exit;
	uint8_t prefixes[TAILSCALE_PEER_INVENTORY_CAPACITY];
};

/* Index bits refer to the corresponding peer's AllowedIPs in the staged map. */
int tailscale_routes_plan(struct route_plan *plan, const struct tailscale_peer_snapshot *map,
			const struct tailscale_routes *prefs, const char *previous, int64_t now);

/* A known tailnet or approved subnet resolver remains managed during an outage. */
bool tailscale_routes_claim_resolver(const struct tailscale_peer_snapshot *map,
				    net_sa_family_t family, const void *addr, bool accept_routes);

#endif
