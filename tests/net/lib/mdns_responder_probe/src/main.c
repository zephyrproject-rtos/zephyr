/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdns_resp_probe_test);

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <ipv6.h>

#include <zephyr/net/dns_sd.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdns_responder.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "dns_pack.h"

#define RESPONSE_TIMEOUT (K_MSEC(250))

/* RFC 6762 8.1 PROBE_TIMEOUT is 1750ms, plus up to a 250ms random initial
 * delay (see mdns_addr_event_handler()'s probe_delay) before the probe even
 * starts. Give a comfortable margin above the worst case for the full
 * probe -> init_listener -> announce sequence to complete.
 */
#define PROBE_SEQUENCE_TIMEOUT (K_SECONDS(4))

static struct net_if *iface1;

static struct net_if_test {
	uint8_t idx; /* not used for anything, just a dummy value */
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
} net_iface1_data;

static const uint8_t ipv6_hdr_start[] = {0x60, 0x05, 0xe7, 0x00};

static const uint8_t ipv6_hdr_rest[] = {0x11, 0xff, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39,
					0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb};

static uint8_t mdns_server_ipv6_addr[] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb};

static struct net_in6_addr ll_addr = {
	{{0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39}}};

static struct net_in6_addr sender_ll_addr = {
	{{0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39}}};

static bool test_started;
static struct k_sem wait_data;
static struct net_pkt *response_pkts[8];
static size_t responses_count;

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = 0x01;
	}

	memcpy(data->ll_addr.addr, data->mac_addr, sizeof(data->mac_addr));
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr), NET_LINK_ETHERNET);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	struct net_ipv6_hdr *hdr;

	if (!pkt->buffer) {
		return -ENODATA;
	}

	if (test_started && net_pkt_family(pkt) == NET_AF_INET6) {
		hdr = NET_IPV6_HDR(pkt);

		if (net_ipv6_addr_cmp_raw(hdr->dst, mdns_server_ipv6_addr) &&
		    responses_count < ARRAY_SIZE(response_pkts)) {
			net_pkt_ref(pkt);
			response_pkts[responses_count++] = pkt;
			k_sem_give(&wait_data);
		}
	}

	return 0;
}

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER    DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test, "iface1", iface1, NULL, NULL, &net_iface1_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &net_iface_api, _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE, 127);

static void *test_setup(void)
{
	struct net_if_addr *ifaddr;

	memset(response_pkts, 0, sizeof(response_pkts));

	k_sem_init(&wait_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(1);
	zassert_not_null(iface1, "Iface1 is NULL");

	((struct net_if_test *)net_if_get_device(iface1)->data)->idx = net_if_get_by_iface(iface1);

	/* An IPv6 address is set up unconditionally (regardless of which test
	 * runs) so that the "is the responder now reachable" check -- an AAAA
	 * query for the hostname, same pattern proven in
	 * tests/net/lib/mdns_responder/ -- can be reused as-is. init_listener()
	 * opens both the IPv4 and IPv6 listeners together in one call, so this
	 * is a valid way to observe whether init_listener() ran at all,
	 * regardless of which address family triggered the race being tested.
	 */
	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Failed to add LL-addr");
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_nbr_add(iface1, &sender_ll_addr, net_if_get_link_addr(iface1), false,
			 NET_IPV6_NBR_STATE_STATIC);

	net_if_up(iface1);

	return NULL;
}

static void before(void *d)
{
	ARG_UNUSED(d);

	responses_count = 0;
	test_started = true;
}

static void cleanup(void *d)
{
	ARG_UNUSED(d);

	test_started = false;

	for (size_t i = 0; i < responses_count; ++i) {
		if (response_pkts[i]) {
			net_pkt_unref(response_pkts[i]);
			response_pkts[i] = NULL;
		}
	}

	while (k_sem_take(&wait_data, K_NO_WAIT) == 0) {
		/* NOP */
	}
}

static void send_msg(const uint8_t *data, size_t len)
{
	struct net_pkt *pkt;
	int res;

	pkt = net_pkt_alloc_with_buffer(iface1, NET_IPV6UDPH_LEN + len, NET_AF_UNSPEC, 0,
					K_FOREVER);
	zassert_not_null(pkt, "PKT is null");

	res = net_pkt_write(pkt, ipv6_hdr_start, sizeof(ipv6_hdr_start));
	zassert_equal(res, 0, "pkt write for header start failed");

	res = net_pkt_write_be16(pkt, len + NET_UDPH_LEN);
	zassert_equal(res, 0, "pkt write for header length failed");

	res = net_pkt_write(pkt, ipv6_hdr_rest, sizeof(ipv6_hdr_rest));
	zassert_equal(res, 0, "pkt write for rest of the header failed");

	res = net_pkt_write_be16(pkt, 5353);
	zassert_equal(res, 0, "pkt write for UDP src port failed");

	res = net_pkt_write_be16(pkt, 5353);
	zassert_equal(res, 0, "pkt write for UDP dst port failed");

	res = net_pkt_write_be16(pkt, len + NET_UDPH_LEN);
	zassert_equal(res, 0, "pkt write for UDP length failed");

	/* to simplify testing checking of UDP checksum is disabled in prj.conf */
	res = net_pkt_write_be16(pkt, 0);
	zassert_equal(res, 0, "net_pkt_write_be16() for UDP checksum failed");

	res = net_pkt_write(pkt, data, len);
	zassert_equal(res, 0, "net_pkt_write() for data failed");

	res = net_recv_data(iface1, pkt);
	zassert_equal(res, 0, "net_recv_data() failed");
}

/* Basic mDNS query for zephyr.local (AAAA) -- same query used to probe
 * responder reachability in tests/net/lib/mdns_responder/.
 */
static const uint8_t zephyr_local_query[] = {
	/* Header */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* zephyr.local */
	0x06, 0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00,
	/* AAAA record */
	0x00, 0x1c, 0x00, 0x01};

static bool responder_answers_query(void)
{
	responses_count = 0;
	while (k_sem_take(&wait_data, K_NO_WAIT) == 0) {
		/* drain stale responses */
	}

	send_msg(zephyr_local_query, sizeof(zephyr_local_query));

	return k_sem_take(&wait_data, RESPONSE_TIMEOUT) == 0;
}

/* Regression test for the "DHCP-bound announce races ahead of the RFC 6762
 * probe" bug: start_announce() (fired on NET_EVENT_IPV4_DHCP_BOUND) used to
 * set do_announce = true unconditionally and immediately, before the probe
 * (a 0-250ms scheduling delay, then the ~1.75-3s RFC 6762 window) had even
 * started. do_init_listener() branches on that same do_announce flag to
 * decide whether to call init_listener() at all -- so on real hardware,
 * DHCP binding always won that race (same thread, no delay, immediately
 * after the address is added), meaning init_listener() -- and thus the
 * mDNS listener socket -- never started on any boot with DHCPv4 + probing
 * both enabled.
 *
 * This reproduces the exact race: net_if_ipv4_addr_add() (which schedules
 * the probe) immediately followed by a NET_EVENT_IPV4_DHCP_BOUND
 * notification, back to back with no delay in between, exactly mirroring
 * dhcpv4_handle_msg_ack()'s real sequence in dhcpv4.c. It then waits out
 * the full probe window and checks that the responder is reachable --
 * proving init_listener() actually ran despite the race.
 */
ZTEST(test_mdns_responder_probe, test_dhcp_bound_race_does_not_block_listener)
{
	static struct net_in_addr ipv4_addr = {{{192, 0, 2, 1}}};
	struct net_if_addr *ifaddr;

	ifaddr = net_if_ipv4_addr_add(iface1, &ipv4_addr, NET_ADDR_DHCP, 0);
	zassert_not_null(ifaddr, "Failed to add IPv4 address");

	/* No delay here on purpose -- this is the race itself. */
	net_mgmt_event_notify(NET_EVENT_IPV4_DHCP_BOUND, iface1);

	k_sleep(PROBE_SEQUENCE_TIMEOUT);

	zassert_true(responder_answers_query(),
		     "Responder never became reachable -- DHCP-bound announce blocked "
		     "init_listener() from ever running");
}

ZTEST_SUITE(test_mdns_responder_probe, NULL, test_setup, before, cleanup, NULL);
