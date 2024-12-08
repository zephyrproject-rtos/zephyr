/*
 * Copyright (c) 2024 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>

LOG_MODULE_REGISTER(ethernet_test, LOG_LEVEL_INF);

#include "net_private.h"

#define DHCP_OPTION_NTP (42)
#define TEST_DATA       "ICMP dummy data"

K_SEM_DEFINE(net_event, 0, 1);

static struct net_if *iface;

static uint8_t ntp_server[4];
static struct net_mgmt_event_callback mgmt_cb;
static struct net_dhcpv4_option_callback dhcp_cb;

static void ipv4_event(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
		       struct net_if *iface)
{
	if ((mgmt_event != NET_EVENT_IPV4_ADDR_ADD) ||
	    (iface->config.ip.ipv4->unicast[0].ipv4.addr_type != NET_ADDR_DHCP)) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];

	LOG_INF("Address[%d]: %s", net_if_get_by_iface(iface),
		net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr, buf,
			      sizeof(buf)));
	LOG_INF("Subnet[%d]: %s", net_if_get_by_iface(iface),
		net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[0].netmask, buf,
			      sizeof(buf)));
	LOG_INF("Router[%d]: %s", net_if_get_by_iface(iface),
		net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf)));
	LOG_INF("Lease time[%d]: %u seconds", net_if_get_by_iface(iface),
		iface->config.dhcpv4.lease_time);

	/* release processing of IPV4 event by test case */
	k_sem_give(&net_event);
}

static int icmp_event(struct net_icmp_ctx *ctx, struct net_pkt *pkt, struct net_icmp_ip_hdr *hdr,
		      struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	struct net_ipv4_hdr *ip_hdr = hdr->ipv4;

	LOG_INF("Received echo reply from %s", net_sprint_ipv4_addr(&ip_hdr->src));

	k_sem_give(&net_event);

	return 0;
}

static void option_handler(struct net_dhcpv4_option_callback *cb, size_t length,
			   enum net_dhcpv4_msg_type msg_type, struct net_if *iface)
{
	char buf[NET_IPV4_ADDR_LEN];

	LOG_INF("DHCP Option %d: %s", cb->option,
		net_addr_ntop(AF_INET, cb->data, buf, sizeof(buf)));
}

ZTEST(ethernet, test_dhcp_check)
{
	LOG_INF("Waiting for IPV4 assign event...");

	zassert_equal(k_sem_take(&net_event, K_SECONDS(CONFIG_DHCP_ASSIGN_TIMEOUT)), 0,
		      "IPV4 address assign event timeout");

	LOG_INF("DHCP check successful");
}

ZTEST(ethernet, test_icmp_check)
{
	struct net_icmp_ping_params params;
	struct net_icmp_ctx ctx;
	struct in_addr gw_addr_4;
	struct sockaddr_in dst4 = {0};
	int ret;

	gw_addr_4 = net_if_ipv4_get_gw(iface);
	zassert_not_equal(gw_addr_4.s_addr, 0, "Gateway address is not set");

	ret = net_icmp_init_ctx(&ctx, NET_ICMPV4_ECHO_REPLY, 0, icmp_event);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	dst4.sin_family = AF_INET;
	memcpy(&dst4.sin_addr, &gw_addr_4, sizeof(gw_addr_4));

	params.identifier = 1234;
	params.sequence = 5678;
	params.tc_tos = 1;
	params.priority = 2;
	params.data = TEST_DATA;
	params.data_size = sizeof(TEST_DATA);

	LOG_INF("Pinging the gateway...");

	ret = net_icmp_send_echo_request(&ctx, iface, (struct sockaddr *)&dst4, &params, NULL);
	zassert_equal(ret, 0, "Cannot send ICMP echo request (%d)", ret);

	zassert_equal(k_sem_take(&net_event, K_SECONDS(CONFIG_GATEWAY_PING_TIMEOUT)), 0,
		      "Gateway ping (ICMP) timed out");

	net_icmp_cleanup_ctx(&ctx);
}

static void *ethernet_setup(void)
{
	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));

	net_mgmt_init_event_callback(&mgmt_cb, ipv4_event, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	net_dhcpv4_init_option_callback(&dhcp_cb, option_handler, DHCP_OPTION_NTP, ntp_server,
					sizeof(ntp_server));
	net_dhcpv4_add_option_callback(&dhcp_cb);

	/* reset semaphore that tracks IPV4 assign event */
	k_sem_reset(&net_event);

	LOG_INF("Starting DHCPv4 client on %s: index=%d", net_if_get_device(iface)->name,
		net_if_get_by_iface(iface));

	net_dhcpv4_start(iface);

	return NULL;
}

ZTEST_SUITE(ethernet, NULL, ethernet_setup, NULL, NULL, NULL);
