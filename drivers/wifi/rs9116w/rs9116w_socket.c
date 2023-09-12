/*
 * Copyright (c) 2021 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME wifi_rs9116w_offload
#define LOG_LEVEL	CONFIG_WIFI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "rs9116w.h"
#include "rsi_wlan.h"

#include <zephyr/net/net_context.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_offload.h>
#include <zephyr/net/net_pkt.h>
#include <rsi_wlan_apis.h>
#undef s6_addr
#undef s6_addr32
#define Z_PF_INET  1	      /**< IP protocol family version 4. */
#define Z_PF_INET6 2	      /**< IP protocol family version 6. */
#define Z_AF_INET  Z_PF_INET  /**< IP protocol family version 4. */
#define Z_AF_INET6 Z_PF_INET6 /**< IP protocol family version 6. */

static int rs9116w_dummy_get(sa_family_t family, enum net_sock_type type,
			     enum net_ip_protocol ip_proto,
			     struct net_context **context)
{

	LOG_ERR("NET_SOCKETS_OFFLOAD must be configured for this driver");

	return -1;
}

static struct net_offload rs9116w_offload = {.get = rs9116w_dummy_get,
					     .bind = NULL,
					     .connect = NULL,
					     .send = NULL,
					     .sendto = NULL,
					     .recv = NULL,
					     .put = NULL};

#ifdef CONFIG_PING
#include <zephyr/net/ping.h>
static uint32_t sent_ts;
static void rs9116w_ping_resp_handler(uint16_t status, const uint8_t *buf,
				      uint16_t len)
{
	if (!status) {
		net_ping_resp_notify(k_uptime_get_32() - sent_ts);
	}
}

static int rs9116w_ping_offload(const struct sockaddr *dst, size_t sz)
{
	int stat;
	uint8_t *addr;

	sent_ts = k_uptime_get_32();
	if (dst->sa_family == Z_AF_INET) {
		addr = (uint8_t *)&net_sin(dst)->sin_addr.s_addr;
		stat = rsi_wlan_ping_async(0, addr, sz,
					   rs9116w_ping_resp_handler);
		if (stat) {
			LOG_WRN("rsi_wlan_ping_async error : %d", stat);
			return -EIO;
		}
	} else if (dst->sa_family == Z_AF_INET6) {
		uint32_t addr_raw[4];

		addr = (uint8_t *)addr_raw;
		addr_raw[0] = ntohl(net_sin6(dst)->sin6_addr.s6_addr32[0]);
		addr_raw[1] = ntohl(net_sin6(dst)->sin6_addr.s6_addr32[1]);
		addr_raw[2] = ntohl(net_sin6(dst)->sin6_addr.s6_addr32[2]);
		addr_raw[3] = ntohl(net_sin6(dst)->sin6_addr.s6_addr32[3]);
		stat = rsi_wlan_ping_async(1, addr, sz, rs9116w_ping_resp_handler);
		if (stat) {
			LOG_WRN("rsi_wlan_ping_async error : %d", stat);
			return -EIO;
		}
	} else {
		return -EINVAL;
	}
	return 0;
}
#endif

int rs9116w_offload_init(struct rs9116w_device *rs9116w)
{
	rs9116w->net_iface->if_dev->offload = &rs9116w_offload;

#if defined(CONFIG_NET_SOCKETS_OFFLOAD)
	rs9116w->net_iface->if_dev->socket_offload = rs9116w_socket_create;
#ifdef CONFIG_PING
	rs9116w->net_iface->if_dev->ping_offload = rs9116w_ping_offload;
#endif
	rs9116w_socket_offload_init();
#endif

	return 0;
}
