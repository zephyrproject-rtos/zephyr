/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_SOCKET_H
#define SIWX91X_WIFI_SOCKET_H
#include <assert.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/offloaded_netdev.h>

#include "device/silabs/si91x/wireless/inc/sl_si91x_protocol_types.h"

struct siwx91x_dev;
struct siwx91x_nwp_wifi_cb;

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD

enum offloaded_net_if_types siwx91x_sock_get_type(void);
int siwx91x_sock_alloc(struct net_if *iface, struct net_pkt *pkt,
		       size_t size, enum net_ip_protocol proto,
		       k_timeout_t timeout);
void siwx91x_sock_on_select(const struct siwx91x_nwp_wifi_cb *context, int select_slot,
			    uint32_t read_fds, uint32_t write_fds);
void siwx91x_sock_on_join_ipv4(const struct device *dev);
void siwx91x_sock_on_join_ipv6(const struct device *dev);
void siwx91x_sock_init(struct net_if *iface);


#else /* CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD */

static inline enum offloaded_net_if_types siwx91x_sock_get_type(void)
{
	__ASSERT_NO_MSG(0);
}

static inline void siwx91x_sock_on_select(const struct siwx91x_nwp_wifi_cb *context,
					  int select_slot, uint32_t read_fds, uint32_t write_fds)
{
	__ASSERT_NO_MSG(0);
}

static inline void siwx91x_sock_on_join_ipv4(const struct device *dev)
{
}

static inline void siwx91x_sock_on_join_ipv6(const struct device *dev)
{
}

static inline void siwx91x_sock_init(struct net_if *iface)
{
}

#endif /* CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD */

#endif
