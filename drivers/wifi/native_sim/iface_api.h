/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_if.h>

void wifi_if_init(struct net_if *iface);
int wifi_if_start(const struct device *dev);
int wifi_if_stop(const struct device *dev);
int wifi_if_set_config(const struct device *dev,
		       enum ethernet_config_type type,
		       const struct ethernet_config *config);
#ifdef CONFIG_NET_STATISTICS_ETHERNET
struct net_stats_eth *wifi_if_stats_get(const struct device *dev);
#endif
enum ethernet_hw_caps wifi_if_caps_get(const struct device *dev);
int wifi_if_send(const struct device *dev, struct net_pkt *pkt);
