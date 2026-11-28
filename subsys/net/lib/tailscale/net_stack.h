/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_NET_STACK_H_
#define ZEPHYR_NET_TAILSCALE_NET_STACK_H_

#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/socket.h>

struct net_if;
struct net_pkt;

struct net_if *tailscale_net_route(net_sa_family_t family, const void *destination);
bool tailscale_net_active(void);

void tailscale_net_queue(struct net_if *iface, const struct net_pkt *pkt);
void tailscale_net_unqueue(const struct net_pkt *pkt);
bool tailscale_net_transmit(struct net_if *iface, const struct net_pkt *pkt, uint64_t peer,
			    const uint8_t *ip, size_t len, const void *policy_input);

bool tailscale_net_dns_offload_allowed(const char *host);
int tailscale_net_query(const char *host, net_sa_family_t family, dns_resolve_cb_t callback,
		       void *user_data);

#endif /* ZEPHYR_NET_TAILSCALE_NET_STACK_H_ */
