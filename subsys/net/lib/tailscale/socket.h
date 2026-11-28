/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_SOCKET_INTERNAL_H_
#define ZEPHYR_NET_TAILSCALE_SOCKET_INTERNAL_H_

#include "tailscale_internal.h"

/* Connect to the literal resolver selected by this DNS publication. */
int tailscale_socket_connect_dns(int type, const struct net_sockaddr *remote, net_socklen_t len,
			      uint64_t route_revision, int32_t timeout_ms);

#endif /* ZEPHYR_NET_TAILSCALE_SOCKET_INTERNAL_H_ */
