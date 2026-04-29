/*
 * Copyright (c) 2026 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/rtp.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/dhcpv4.h>

#include "sine.h"

LOG_MODULE_REGISTER(main);

#define SOURCE_MCAST_ADDR "224.0.1.100"
#define SOURCE_MCAST_PORT 51100

#define PAYLOAD_TYPE 97

static RTP_SESSION_DEFINE(my_rtp_session, 0);

#ifdef CONFIG_NET_DHCPV4
static struct net_mgmt_event_callback mgmt_cb;

K_SEM_DEFINE(ipv4_sem, 0, 1)

static void handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	k_sem_give(&ipv4_sem);
}

static void start_dhcpv4_client(struct net_if *iface, void *user_data)
{
	ARG_UNUSED(user_data);

	net_dhcpv4_start(iface);
}
#endif /* CONFIG_NET_DHCPV4 */

int main(void)
{
	struct net_sockaddr_in source_sockaddr_in;
	struct net_in_addr source_in_addr;
	int ret;

	LOG_INF("RTP transmitter on %s", CONFIG_BOARD);

#ifdef CONFIG_NET_DHCPV4

	net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	net_if_foreach(start_dhcpv4_client, NULL);

	(void)k_sem_take(&ipv4_sem, K_FOREVER);
#endif /* CONFIG_NET_DHCPV4 */

	if (net_addr_pton(NET_AF_INET, SOURCE_MCAST_ADDR, &source_in_addr) != 0) {
		LOG_ERR("Invalid address: %s", SOURCE_MCAST_ADDR);
		return -EINVAL;
	}

	source_sockaddr_in.sin_family = NET_AF_INET;
	source_sockaddr_in.sin_addr = source_in_addr;
	source_sockaddr_in.sin_port = net_htons(SOURCE_MCAST_PORT);

	(void)rtp_session_init_tx(&my_rtp_session, net_if_get_default(),
				  (struct net_sockaddr *)&source_sockaddr_in, PAYLOAD_TYPE, 0);

	ret = rtp_session_start(&my_rtp_session);
	if (ret < 0) {
		LOG_ERR("Failed to start rtp session (%d)", ret);
	}

	ARRAY_FOR_EACH(sine_100_8000_16_mono, i) {
		sine_100_8000_16_mono[i] = sys_cpu_to_be16(sine_100_8000_16_mono[i]);
	}
	void *payload = (void *)&sine_100_8000_16_mono;
	size_t size = sizeof(sine_100_8000_16_mono);
	int period_ms = 50;
	int delta_ts = 5; // 5 sampling periods in payload

	LOG_INF("Start sending RTP packets");
	while (true) {
		ret = rtp_session_send_simple(&my_rtp_session, payload, size, delta_ts);
		if (ret < 0) {
			LOG_ERR("Failed to send rtp (%d)", ret);
			return ret;
		}

		k_msleep(period_ms);
	}

	return 0;
}
