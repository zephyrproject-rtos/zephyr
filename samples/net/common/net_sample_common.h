/*
 * Copyright (c) 2024 Nordic Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_if.h>

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
