/*
 * Copyright (c) 2024 Nordic Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/quic.h>

#if defined(CONFIG_NET_CONNECTION_MANAGER)
void wait_for_network(void);
#else
static inline void wait_for_network(void) { }
#endif /* CONFIG_NET_CONNECTION_MANAGER */

#if defined(CONFIG_NET_VLAN)
int init_vlan(void);
#else
static inline int init_vlan(void)
{
	return 0;
}
#endif /* CONFIG_NET_VLAN */

#if defined(CONFIG_NET_L2_IPIP)
int init_tunnel(void);
bool is_tunnel(struct net_if *iface);
#else
static inline int init_tunnel(void)
{
	return 0;
}

static inline bool is_tunnel(struct net_if *iface)
{
	ARG_UNUSED(iface);
	return false;
}
#endif /* CONFIG_NET_L2_IPIP */

#if defined(CONFIG_QUIC)
int setup_quic(const struct net_sockaddr *remote_addr,
	       const struct net_sockaddr *local_addr,
	       enum quic_stream_direction direction,
	       sec_tag_t sec_tag_list[],
	       size_t sec_tag_list_size,
	       const char *alpn_list[],
	       size_t alpn_list_size);
void close_quic(int quic_sock);
#else
static inline int setup_quic(const struct net_sockaddr *remote_addr,
			     const struct net_sockaddr *local_addr,
			     enum quic_stream_direction direction,
			     sec_tag_t sec_tag_list[],
			     size_t sec_tag_list_size,
			     const char *alpn_list[],
			     size_t alpn_list_size)
{
	ARG_UNUSED(remote_addr);
	ARG_UNUSED(local_addr);
	ARG_UNUSED(direction);
	ARG_UNUSED(sec_tag_list);
	ARG_UNUSED(sec_tag_list_size);
	ARG_UNUSED(alpn_list);
	ARG_UNUSED(alpn_list_size);

	return -ENOTSUP;
}

static inline void close_quic(int quic_sock)
{
	ARG_UNUSED(quic_sock);
}

#endif /* CONFIG_QUIC */
