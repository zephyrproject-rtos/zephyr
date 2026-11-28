/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_POLICY_H_
#define ZEPHYR_NET_TAILSCALE_POLICY_H_

#include <zephyr/net/net_if.h>
#include "tailscale_internal.h"

void tailscale_client_policy_set_underlay(struct net_if *iface);

/* Claim wg0 with the stored identity and require its public key in the map. */
int tailscale_client_policy_start_hosted(struct net_if *iface, const uint8_t private_key[32],
			       const uint8_t public_key[32]);

/* Call with an authenticated frame in stream order. The decoder borrows json. */
int tailscale_client_policy_apply(uint32_t generation, uint8_t *json, size_t len,
			enum tailscale_peer_inventory_frame *frame);
int tailscale_client_policy_summary(struct tailscale_peer_inventory_summary *summary);

#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
int tailscale_client_policy_route_dns(const struct net_sockaddr *remote, struct net_if **iface,
				struct net_sockaddr_storage *local, uint64_t *revision);
#endif

uint64_t tailscale_client_policy_route_revision(void);

/* Return -ENOTSUP only when the query may use the system's ordinary DNS. */
int tailscale_client_policy_dns_authority(const char *name);

/* Select DNS for an opted-in absolute name while the installed lease is live. */
int tailscale_client_policy_select_dns(const char *name, struct tailscale_dns_selection *selection);

/* Copy a search suffix from this live DNS publication. */
int tailscale_client_policy_dns_search_domain(uint64_t revision, size_t index,
				     char domain[TAILSCALE_DNS_NAME_SIZE]);

/* Resolve a fully qualified MagicDNS A/AAAA name from live local and peer host addresses. */
int tailscale_client_policy_resolve_magic_dns(const char *name, uint8_t family,
				     struct tailscale_dns_answer *answer);

/* Withdraw policy after authenticated authority loss and retain the local key. */
int tailscale_client_policy_revoke(void);

/* Retire the interface, its key and the local addresses installed by policy. */
int tailscale_client_policy_stop(void);

#endif /* ZEPHYR_NET_TAILSCALE_POLICY_H_ */
