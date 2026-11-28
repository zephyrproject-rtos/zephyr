/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_NET_TAILSCALE_TRANSPORT_H_
#define ZEPHYR_NET_TAILSCALE_TRANSPORT_H_

#include "../wireguard/wg_private.h"
#include "tailscale_internal.h"
#include "control.h"

#define DERP_CONNECTIONS 2

struct tailscale_transport_peer {
	uint8_t key[32];
	uint8_t disco[32];
	struct net_sockaddr_storage endpoint;
	struct tailscale_peer_endpoint candidates[TAILSCALE_PEER_ENDPOINT_CAPACITY];
	uint8_t candidate_count;
	uint16_t region;
	int id;
};

struct tailscale_transport_plan {
#if defined(CONFIG_TAILSCALE_DERP) || defined(CONFIG_TAILSCALE_DISCOVER_ENDPOINT)
	struct tailscale_derp_map map;
#endif
	struct tailscale_transport_peer peers[CONFIG_WIREGUARD_MAX_PEER];
	size_t count;
#ifdef CONFIG_TAILSCALE_DERP
	uint16_t regions[DERP_CONNECTIONS];
	uint16_t home;
#endif
	uint32_t epoch;
};

struct tailscale_transport_endpoints {
	struct net_sockaddr_storage local_ipv4;
	struct net_sockaddr_storage local_ipv6;
	struct net_sockaddr_storage public_ipv4;
};

int tailscale_transport_start(const struct tailscale_control_params *params, struct net_if *lan);
void tailscale_transport_stop(void);
uint16_t tailscale_transport_home(void);
void tailscale_transport_get_endpoints(struct tailscale_transport_endpoints *endpoints);
int tailscale_transport_stage(struct tailscale_transport_plan *plan,
			      const struct tailscale_peer_snapshot *snapshot);
void tailscale_transport_publish(const struct tailscale_transport_plan *plan);
int tailscale_transport_send(int peer_id, const struct wireguard_datagram_path *reply,
			     const uint8_t *data, size_t len, void *epoch);
#endif
