/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <strings.h>
#include <string.h>
#include <zephyr/net/net_ip.h>

#include "routes.h"

static bool equal_prefix(const struct tailscale_peer_prefix *a,
			 const struct tailscale_peer_prefix *b)
{
	return a->family == b->family && a->length == b->length &&
	       !memcmp(a->address, b->address, a->family == NET_AF_INET ? 4 : 16);
}

static bool node_address(const struct tailscale_peer *peer, const struct tailscale_peer_prefix *p)
{
	for (size_t i = 0; i < peer->address_count; i++) {
		if (equal_prefix(p, &peer->addresses[i])) {
			return true;
		}
	}
	return false;
}

bool tailscale_routes_claim_resolver(const struct tailscale_peer_snapshot *map,
				    net_sa_family_t family, const void *addr, bool accept_routes)
{
	for (size_t i = 0; i < map->count; i++) {
		const struct tailscale_peer *peer = &map->peers[i];

		for (size_t j = 0; j < peer->allowed_ip_count; j++) {
			const struct tailscale_peer_prefix *prefix = &peer->allowed_ips[j];

			if (prefix->family == family && prefix->length != 0U &&
			    net_ipv6_is_prefix(addr, prefix->address, prefix->length) &&
			    (accept_routes || node_address(peer, prefix))) {
				return true;
			}
		}
	}
	return false;
}

static bool usable(const struct tailscale_peer_snapshot *map, const struct tailscale_peer *peer,
		   int64_t now)
{
	const char *limit = CONFIG_TAILSCALE_PEER_STABLE_ID;

	return (!limit[0] || !strcmp(limit, peer->stable_id)) && !peer->expired &&
	       (!peer->key_expires || (map->have_control_time &&
				       peer->key_expiry_ms - map->control_epoch_at_boot_ms > now));
}

static bool fixed_match(const struct tailscale_peer *peer, const char *name)
{
	if (!strcmp(name, peer->stable_id)) {
		return true;
	}

	size_t len = strlen(name);
	size_t n = len - (len > 0 && name[len - 1] == '.');

	if (n && !strncasecmp(name, peer->dns_name, n) &&
	    (!peer->dns_name[n] ||
	     (n == len && !memchr(name, '.', n) && peer->dns_name[n] == '.'))) {
		return true;
	}

	uint8_t ip[16];
	int family = strchr(name, ':') ? NET_AF_INET6 : NET_AF_INET;

	if (net_addr_pton(family, name, ip) == 0) {
		for (size_t i = 0; i < peer->address_count; i++) {
			if (peer->addresses[i].family == family &&
			    !memcmp(peer->addresses[i].address, ip,
				    family == NET_AF_INET ? 4 : 16)) {
				return true;
			}
		}
	}
	return false;
}

static bool is_exit(const struct tailscale_peer *peer)
{
	bool v4 = false, v6 = false;

	for (size_t i = 0; i < peer->allowed_ip_count; ++i) {
		if (!peer->allowed_ips[i].length) {
			v4 |= peer->allowed_ips[i].family == NET_AF_INET;
			v6 |= peer->allowed_ips[i].family == NET_AF_INET6;
		}
	}
	return v4 && v6;
}

static int select_exit(struct route_plan *plan, const char *name, const char *previous, int64_t now)
{
	const struct tailscale_peer_snapshot *map = plan->map;
	bool automatic = !strcmp(name, "auto:any");
	int best = -1;

	if (!*name) {
		return 0;
	}
	for (size_t i = 0; i < map->count; i++) {
		const struct tailscale_peer *p = &map->peers[i];

		if (!usable(map, p, now) || !p->stable_id[0] ||
		    p->online == TAILSCALE_PEER_OFFLINE || !is_exit(p)) {
			continue;
		}
		if (!automatic) {
			if (previous[0] ? strcmp(previous, p->stable_id) != 0
					: !fixed_match(p, name)) {
				continue;
			}
			if (plan->exit) {
				return -EADDRINUSE;
			}
		} else {
			if (!p->suggest_exit) {
				continue;
			}
			/* Prefer a colocated region, then a known region. Retain the
			 * last suggestion among equal choices to avoid moving live flows.
			 */
			int score = p->derp_region
					    ? (p->derp_region == map->self.derp_region ? 4 : 2)
					    : 0;

			score += !strcmp(previous, p->stable_id);
			if (score < best || (score == best && p->id >= plan->exit)) {
				continue;
			}
			best = score;
		}
		plan->exit = p->id;
	}
	return 0;
}

static int select_subnet(struct route_plan *plan, size_t index, size_t prefix_index)
{
	const struct tailscale_peer *peer = &plan->map->peers[index];

	for (size_t i = 0; i < index; i++) {
		const struct tailscale_peer *old = &plan->map->peers[i];

		for (size_t j = 0; j < old->allowed_ip_count; j++) {
			if (!(plan->prefixes[i] & BIT(j)) ||
			    !equal_prefix(&peer->allowed_ips[prefix_index], &old->allowed_ips[j])) {
				continue;
			}
			bool primary = (peer->primary_routes & BIT(prefix_index)) != 0;
			bool old_primary = (old->primary_routes & BIT(j)) != 0;

			if (primary && old_primary) {
				return -EADDRINUSE;
			}
			if (old_primary || (!primary && old->id < peer->id)) {
				return 0;
			}
			plan->prefixes[i] &= ~BIT(j);
		}
	}
	plan->prefixes[index] |= BIT(prefix_index);
	return 0;
}

int tailscale_routes_plan(struct route_plan *plan, const struct tailscale_peer_snapshot *map,
			const struct tailscale_routes *prefs, const char *previous, int64_t now)
{
	*plan = (struct route_plan){.map = map};
	int rc = select_exit(plan, prefs->exit_node, previous, now);

	for (size_t i = 0; rc == 0 && i < map->count; i++) {
		const struct tailscale_peer *peer = &map->peers[i];

		if (!usable(map, peer, now)) {
			continue;
		}
		for (size_t j = 0; rc == 0 && j < peer->allowed_ip_count; j++) {
			const struct tailscale_peer_prefix *p = &peer->allowed_ips[j];

			if (node_address(peer, p) || (!p->length && peer->id == plan->exit)) {
				plan->prefixes[i] |= BIT(j);
			} else if (p->length && prefs->accept_routes &&
				   peer->online != TAILSCALE_PEER_OFFLINE) {
				rc = select_subnet(plan, i, j);
			}
		}
	}
	return rc;
}
