/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_INGRESS_H_
#define ZEPHYR_NET_TAILSCALE_INGRESS_H_

#include "tailscale_internal.h"

struct net_pkt;

int tailscale_ingress_validate(const struct tailscale_service_rule *rules, size_t count);

#define TAILSCALE_SOCKET_MAX_FLOWS 8
#define TAILSCALE_SOCKET_UDP_REPLY_MS 30000

/* Called only for a structurally valid packet addressed to the managed interface. */
bool tailscale_ingress_admit(const struct tailscale_filter_tuple *tuple, bool tailscale_allowed,
			   uint64_t revision, uint64_t peer_identity);

void tailscale_ingress_queue(const struct net_pkt *pkt);
bool tailscale_ingress_transmit(const struct net_pkt *pkt,
				const struct tailscale_filter_tuple *tuple,
				bool service_reply, uint64_t revision, uint64_t identity);

/* Publish the forwarding revision and its exclusive monotonic lease deadline. */
void tailscale_ingress_set_revision(uint64_t revision, int64_t deadline_ms);

int tailscale_ingress_bind(struct net_if *iface, int fd, const struct net_sockaddr *local);
void tailscale_ingress_abort_tcp(int fd);

#endif /* ZEPHYR_NET_TAILSCALE_INGRESS_H_ */
