/*
 * Copyright (c) 2024-2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SIWX91X_WIFI_DATA_H
#define SIWX91X_WIFI_DATA_H
#include <assert.h>

struct device;
struct net_buf;
struct net_pkt;
struct siwx91x_nwp_wifi_cb;

#ifdef CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE

int siwx91x_wifi_send(const struct device *dev, struct net_pkt *pkt);
void siwx91x_wifi_on_rx(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf);

#else

static inline void siwx91x_wifi_on_rx(const struct siwx91x_nwp_wifi_cb *ctxt, struct net_buf *buf)
{
	__ASSERT(0, "Unexpected call");
}

#endif /* CONFIG_WIFI_SILABS_SIWX91X_NET_STACK_NATIVE */

#endif
