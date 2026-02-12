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

LOG_MODULE_REGISTER(main);

#define SINK_MCAST_ADDR "224.0.1.100"
#define SINK_MCAST_PORT 51100

static RTP_SESSION_DEFINE(my_rtp_session, 0);

enum rtp_receiver_state {
	RTP_RX_IDLE,
	RTP_RX_ACTIVE,
	RTP_RX_TIMEOUT,
} static rtp_rx_state;

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

static void timeout_worker(struct k_work *work)
{
	ARG_UNUSED(work);

	rtp_rx_state = RTP_RX_TIMEOUT;
	LOG_WRN("Timeout occured");
}

K_WORK_DELAYABLE_DEFINE(timeout_work, timeout_worker);

static void my_rtp_session_recv_cb(struct rtp_session *session, struct rtp_packet *packet,
				   void *user_data)
{
	if (rtp_rx_state != RTP_RX_ACTIVE) {
		LOG_INF("Receiving rtp stream");
		rtp_rx_state = RTP_RX_ACTIVE;
	}

	k_work_reschedule(&timeout_work, K_MSEC(500));
}

int main(void)
{
	struct net_sockaddr_in sink_sockaddr_in;
	struct net_in_addr sink_in_addr;
	int ret;

	LOG_INF("RTP receiver on %s", CONFIG_BOARD);

#ifdef CONFIG_NET_DHCPV4

	net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	net_if_foreach(start_dhcpv4_client, NULL);

	(void)k_sem_take(&ipv4_sem, K_FOREVER);
#endif /* CONFIG_NET_DHCPV4 */

	if (net_addr_pton(NET_AF_INET, SINK_MCAST_ADDR, &sink_in_addr) != 0) {
		LOG_ERR("Invalid address: %s", SINK_MCAST_ADDR);
		return -EINVAL;
	}

	sink_sockaddr_in.sin_family = NET_AF_INET;
	sink_sockaddr_in.sin_addr = sink_in_addr;
	sink_sockaddr_in.sin_port = net_htons(SINK_MCAST_PORT);

	(void)rtp_session_init_rx(&my_rtp_session, net_if_get_default(),
				  (struct net_sockaddr *)&sink_sockaddr_in, my_rtp_session_recv_cb,
				  NULL);

	rtp_rx_state = RTP_RX_IDLE;
	ret = rtp_session_start(&my_rtp_session);
	if (ret < 0) {
		LOG_ERR("Failed to start rtp session (%d)", ret);
		return ret;
	}

	return 0;
}
