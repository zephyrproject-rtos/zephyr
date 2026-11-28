/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NET_LIB_TAILSCALE_UNDERLAY_DNS_H_
#define ZEPHYR_SUBSYS_NET_LIB_TAILSCALE_UNDERLAY_DNS_H_

#include <zephyr/net/net_ip.h>

struct net_if;

/* Resolve control or relay endpoints through the physical interface.
 * Returns the number of addresses stored, or a negative errno.
 */
int tailscale_underlay_resolve(struct net_if *iface, const char *host, net_sa_family_t family,
			      uint16_t port,
			      struct net_sockaddr_storage *addresses, size_t capacity);

#endif
