/*
 * Copyright (c) 2026 Sergei Shirokov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A CoAP server answers on the interface a request came in on. Here both
 * interfaces have the same address, so only the interface index tells them
 * apart: the reply must go back through the interface of the request, for a
 * request to the unicast address and to a multicast group alike.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/coap_service.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/ztest.h>

#include <string.h>

#include "ipv4.h"
#include "ipv6.h"
#include "udp_internal.h"

#define PEER_PORT 4343
#define WAIT_TIME K_MSEC(1000)

static const uint16_t service_v4_port = 5683;
static const uint16_t service_v6_port = 5684;

/* The same address on both interfaces. */
static const struct net_in_addr my_addr4 = { { { 192, 0, 2, 1 } } };
static const struct net_in_addr netmask4 = { { { 255, 255, 255, 0 } } };
static const struct net_in_addr peer_addr4 = { { { 192, 0, 2, 10 } } };
/* All CoAP Nodes, RFC 7252 section 12.8. */
static const struct net_in_addr mcast_addr4 = { { { 224, 0, 1, 187 } } };

static const struct net_in6_addr my_addr6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static const struct net_in6_addr prefix6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };
static const struct net_in6_addr peer_addr6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 } } };
static const struct net_in6_addr mcast_addr6 = {
	{ { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } };

/* Neighbour discovery is off, so the peer is added to the cache by hand. */
static const struct net_linkaddr peer_lladdr = {
	.addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x10 },
	.len = NET_ETH_ADDR_LEN,
	.type = NET_LINK_ETHERNET,
};

/* The interface and source address of the last UDP packet sent. */
static struct net_if *sent_iface;
static uint8_t sent_src[NET_IPV6_ADDR_SIZE];
static K_SEM_DEFINE(sent_sem, 0, 1);

struct test_dev_context {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
};

static struct test_dev_context test_dev_ctx[2] = {
	{ .mac_addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 } },
	{ .mac_addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x02 } },
};

static void test_iface_init(struct net_if *iface)
{
	struct test_dev_context *ctx = net_if_get_device(iface)->data;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr), NET_LINK_ETHERNET);
}

static int test_send(const struct device *dev, struct net_pkt *pkt)
{
	uint8_t proto;

	ARG_UNUSED(dev);

	proto = (net_pkt_family(pkt) == NET_AF_INET) ? NET_IPV4_HDR(pkt)->proto :
						       NET_IPV6_HDR(pkt)->nexthdr;
	if (proto == NET_IPPROTO_UDP) {
		sent_iface = net_pkt_iface(pkt);
		if (net_pkt_family(pkt) == NET_AF_INET) {
			memcpy(sent_src, NET_IPV4_HDR(pkt)->src, NET_IPV4_ADDR_SIZE);
		} else {
			memcpy(sent_src, NET_IPV6_HDR(pkt)->src, NET_IPV6_ADDR_SIZE);
		}
		k_sem_give(&sent_sem);
	}

	return 0;
}

static DEVICE_API(dummy, test_if_api) = {
	.iface_api.init = test_iface_init,
	.send = test_send,
};

NET_DEVICE_INIT_INSTANCE(test_dev0, "test_dev0", 0, NULL, NULL, &test_dev_ctx[0], NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_if_api, DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2), 1280);

NET_DEVICE_INIT_INSTANCE(test_dev1, "test_dev1", 1, NULL, NULL, &test_dev_ctx[1], NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_if_api, DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2), 1280);

static struct net_if *ifaces[2];

static int ping_get(struct coap_resource *resource, struct coap_packet *request,
		    struct net_sockaddr *addr, net_socklen_t addr_len)
{
	uint8_t data[32];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl = coap_header_get_token(request, token);
	uint8_t type = (coap_header_get_type(request) == COAP_TYPE_CON) ? COAP_TYPE_ACK :
									  COAP_TYPE_NON_CON;
	struct coap_packet response;
	int ret;

	ret = coap_packet_init(&response, data, sizeof(data), COAP_VERSION_1, type, tkl, token,
			       COAP_RESPONSE_CODE_CONTENT,
			       (type == COAP_TYPE_ACK) ? coap_header_get_id(request) :
							 coap_next_id());
	if (ret < 0) {
		return ret;
	}

	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

static const char * const ping_path[] = { "ping", NULL };

COAP_SERVICE_DEFINE(service_v4, "0.0.0.0", &service_v4_port, COAP_SERVICE_AUTOSTART);
COAP_RESOURCE_DEFINE(resource_v4, service_v4, {
	.path = ping_path,
	.get = ping_get,
});

COAP_SERVICE_DEFINE(service_v6, "::", &service_v6_port, COAP_SERVICE_AUTOSTART);
COAP_RESOURCE_DEFINE(resource_v6, service_v6, {
	.path = ping_path,
	.get = ping_get,
});

static void *setup(void)
{
	struct net_if_mcast_addr *maddr4;
	struct net_if_mcast_addr *maddr6;

	ifaces[0] = NET_IF_GET(test_dev0, 0);
	ifaces[1] = NET_IF_GET(test_dev1, 1);

	for (int i = 0; i < ARRAY_SIZE(ifaces); i++) {
		zassert_not_null(net_if_ipv4_addr_add(ifaces[i], &my_addr4, NET_ADDR_MANUAL, 0),
				 "Cannot add IPv4 address %d", i);
		zassert_true(net_if_ipv4_set_netmask_by_addr(ifaces[i], &my_addr4, &netmask4),
			     "Cannot set IPv4 netmask %d", i);

		zassert_not_null(net_if_ipv6_addr_add(ifaces[i], &my_addr6, NET_ADDR_MANUAL, 0),
				 "Cannot add IPv6 address %d", i);
		zassert_not_null(net_if_ipv6_prefix_add(ifaces[i], &prefix6, 64, UINT32_MAX),
				 "Cannot add IPv6 prefix %d", i);
		zassert_not_null(net_ipv6_nbr_add(ifaces[i], &peer_addr6, &peer_lladdr, false,
						  NET_IPV6_NBR_STATE_REACHABLE),
				 "Cannot add IPv6 neighbour %d", i);

		/* IGMP and MLD are off, so joining sends nothing. */
		maddr4 = net_if_ipv4_maddr_add(ifaces[i], &mcast_addr4);
		zassert_not_null(maddr4, "Cannot add IPv4 multicast group %d", i);
		net_if_ipv4_maddr_join(ifaces[i], maddr4);

		maddr6 = net_if_ipv6_maddr_add(ifaces[i], &mcast_addr6);
		zassert_not_null(maddr6, "Cannot add IPv6 multicast group %d", i);
		net_if_ipv6_maddr_join(ifaces[i], maddr6);

		(void)net_if_up(ifaces[i]);
	}

	for (int i = 0; i < 100 && (coap_service_is_running(&service_v4) != 1 ||
				    coap_service_is_running(&service_v6) != 1);
	     i++) {
		k_sleep(K_MSEC(10));
	}

	zassert_equal(coap_service_is_running(&service_v4), 1, "IPv4 service not running");
	zassert_equal(coap_service_is_running(&service_v6), 1, "IPv6 service not running");

	return NULL;
}

/* A GET of /ping, as a client would send it. */
static uint16_t make_request(uint8_t *buf, size_t size, uint8_t type)
{
	struct coap_packet request;

	zassert_ok(coap_packet_init(&request, buf, size, COAP_VERSION_1, type, 0, NULL,
				    COAP_METHOD_GET, coap_next_id()),
		   "Cannot create request");
	zassert_ok(coap_packet_append_option(&request, COAP_OPTION_URI_PATH, "ping", 4),
		   "Cannot add path");

	return request.offset;
}

/* Deliver a CoAP request from the peer, as if it came in on iface: confirmable
 * to the unicast address, or non-confirmable to the multicast group, as
 * RFC 7252 section 8.1 requires.
 */
static void inject_request(struct net_if *iface, net_sa_family_t family, uint16_t port,
			   bool mcast)
{
	uint8_t buf[32];
	uint16_t len =
		make_request(buf, sizeof(buf), mcast ? COAP_TYPE_NON_CON : COAP_TYPE_CON);
	struct net_pkt *pkt =
		net_pkt_rx_alloc_with_buffer(iface, len, family, NET_IPPROTO_UDP, K_NO_WAIT);

	zassert_not_null(pkt, "Cannot allocate packet");

	if (family == NET_AF_INET) {
		zassert_ok(net_ipv4_create(pkt, &peer_addr4, mcast ? &mcast_addr4 : &my_addr4),
			   "Cannot create IPv4 header");
	} else {
		zassert_ok(net_ipv6_create(pkt, &peer_addr6, mcast ? &mcast_addr6 : &my_addr6),
			   "Cannot create IPv6 header");
	}

	zassert_ok(net_udp_create(pkt, net_htons(PEER_PORT), net_htons(port)),
		   "Cannot create UDP header");
	zassert_ok(net_pkt_write(pkt, buf, len), "Cannot write request");
	net_pkt_cursor_init(pkt);

	if (family == NET_AF_INET) {
		zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP), "Cannot finalize packet");
	} else {
		zassert_ok(net_ipv6_finalize(pkt, NET_IPPROTO_UDP), "Cannot finalize packet");
	}

	zassert_ok(net_recv_data(iface, pkt), "Cannot receive packet");
}

/* Each request in turn, in both orders, so that neither interface wins only by
 * being first.
 */
static void check_reply_iface(net_sa_family_t family, uint16_t port, bool mcast)
{
	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = (i == 1 || i == 2) ? 1 : 0;

		k_sem_reset(&sent_sem);
		sent_iface = NULL;

		inject_request(ifaces[n], family, port, mcast);

		zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "No reply");
		zassert_equal_ptr(sent_iface, ifaces[n], "Reply on interface %d, expected %d",
				  net_if_get_by_iface(sent_iface),
				  net_if_get_by_iface(ifaces[n]));

		/* Never from the multicast group, but from the interface's own address. */
		if (family == NET_AF_INET) {
			zassert_mem_equal(sent_src, &my_addr4, NET_IPV4_ADDR_SIZE,
					  "Wrong source address");
		} else {
			zassert_mem_equal(sent_src, &my_addr6, NET_IPV6_ADDR_SIZE,
					  "Wrong source address");
		}
	}
}

ZTEST(coap_server_iface, test_v4_reply_on_request_iface)
{
	check_reply_iface(NET_AF_INET, service_v4_port, false);
}

ZTEST(coap_server_iface, test_v6_reply_on_request_iface)
{
	check_reply_iface(NET_AF_INET6, service_v6_port, false);
}

ZTEST(coap_server_iface, test_v4_reply_on_request_iface_mcast)
{
	check_reply_iface(NET_AF_INET, service_v4_port, true);
}

ZTEST(coap_server_iface, test_v6_reply_on_request_iface_mcast)
{
	check_reply_iface(NET_AF_INET6, service_v6_port, true);
}

ZTEST_SUITE(coap_server_iface, NULL, setup, NULL, NULL, NULL);
