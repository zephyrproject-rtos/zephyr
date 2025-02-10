/*
 * Copyright (c) 2023 Antmicro
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_SOCKET_H
#define SIWX91X_WIFI_SOCKET_H

#include <zephyr/net/net_if.h>
#include <zephyr/net/offloaded_netdev.h>
#include <assert.h>

struct siwx91x_dev;

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD

enum offloaded_net_if_types siwx91x_get_type(void);
void siwx91x_on_join_ipv4(struct siwx91x_dev *sidev);
void siwx91x_on_join_ipv6(struct siwx91x_dev *sidev);
void siwx91x_sock_init(struct net_if *iface);

#else /* CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD */

enum offloaded_net_if_types siwx91x_get_type(void)
{
	assert(0);
}

void siwx91x_on_join_ipv4(struct siwx91x_dev *sidev)
{
}

void siwx91x_on_join_ipv6(struct siwx91x_dev *sidev)
{
}

void siwx91x_sock_init(struct net_if *iface)
{
}

#endif /* CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_OFFLOAD */

#endif
