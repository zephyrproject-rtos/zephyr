/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dhcpv4_server.h>

#include "dhcpv4/dhcpv4_internal.h"
#include "ipv4.h"
#include "udp_internal.h"

/* 00-00-5E-00-53-xx Documentation RFC 7042 */
static uint8_t server_mac_addr[] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x01 };
static uint8_t client_mac_addr[] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x02 };

static struct in_addr server_addr = { { { 192, 0, 2, 1 } } };
static struct in_addr netmask = { { { 255, 255, 255, 0 } } };
static struct in_addr test_base_addr = { { { 192, 0, 2, 10 } } };

/* Only to test Inform. */
static struct in_addr client_addr_static = { { { 192, 0, 2, 2 } } };

typedef void (*test_dhcpv4_server_fn_t)(struct net_if *iface,
					struct net_pkt *pkt);


static struct test_dhcpv4_server_ctx {
	struct net_if *iface;
	struct k_sem test_proceed;
	struct net_pkt *pkt;
	struct in_addr assigned_ip;

	/* Request params */
	const char *client_id;
	int lease_time;
	bool broadcast;
} test_ctx;

struct test_lease_count {
	int reserved;
	int allocated;
	int declined;
};

#define CLIENT_ID_1 "client1"
#define CLIENT_ID_2 "client2"
#define NO_LEASE_TIME -1
#define TEST_XID 0x12345678

#define TEST_TIMEOUT K_MSEC(100)

static void server_iface_init(struct net_if *iface)
{
	net_if_set_link_addr(iface, server_mac_addr, sizeof(server_mac_addr),
			     NET_LINK_ETHERNET);

	test_ctx.iface = iface;

	(void)net_if_ipv4_addr_add(iface, &server_addr, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv4_set_netmask(iface, &netmask);
}

static int server_send(const struct device *dev, struct net_pkt *pkt)
{
	test_ctx.pkt = pkt;
	net_pkt_ref(pkt);

	k_sem_give(&test_ctx.test_proceed);

	return 0;
}

static struct dummy_api server_if_api = {
	.iface_api.init = server_iface_init,
	.send = server_send,
};

NET_DEVICE_INIT(server_iface, "server_iface", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &server_if_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), NET_IPV4_MTU);

static const uint8_t cookie[4] = { 0x63, 0x82, 0x53, 0x63 };

static void test_pkt_free(void)
{
	if (test_ctx.pkt != NULL) {
		net_pkt_unref(test_ctx.pkt);
		test_ctx.pkt = NULL;
	}
}

static void client_prepare_test_msg(
	const struct in_addr *src_addr, const struct in_addr *dst_addr,
	enum net_dhcpv4_msg_type type, const struct in_addr *server_id,
	const struct in_addr *requested_ip, const struct in_addr *ciaddr)
{
	struct dhcp_msg msg = { 0 };
	uint8_t empty_buf[SIZE_OF_FILE] = { 0 };
	struct net_pkt *pkt;

	pkt = net_pkt_alloc_with_buffer(test_ctx.iface, NET_IPV4_MTU, AF_INET,
					IPPROTO_UDP, K_FOREVER);
	zassert_not_null(pkt, "Failed to allocate packet");

	net_pkt_set_ipv4_ttl(pkt, 1);

	zassert_ok(net_ipv4_create(pkt, src_addr, dst_addr),
		   "Failed to create IPv4 header");
	zassert_ok(net_udp_create(pkt, htons(DHCPV4_CLIENT_PORT),
				  htons(DHCPV4_SERVER_PORT)),
		   "Failed to create UDP header");

	msg.op = DHCPV4_MSG_BOOT_REQUEST;
	msg.htype = HARDWARE_ETHERNET_TYPE;
	msg.hlen = sizeof(client_mac_addr);
	msg.xid = htonl(TEST_XID);
	if (test_ctx.broadcast) {
		msg.flags = htons(DHCPV4_MSG_BROADCAST);
	}

	if (ciaddr) {
		memcpy(msg.ciaddr, ciaddr, sizeof(*ciaddr));
	} else {
		memset(msg.ciaddr, 0, sizeof(msg.ciaddr));

	}
	memset(msg.yiaddr, 0, sizeof(msg.ciaddr));
	memset(msg.siaddr, 0, sizeof(msg.siaddr));
	memset(msg.giaddr, 0, sizeof(msg.giaddr));
	memcpy(msg.chaddr, client_mac_addr, sizeof(client_mac_addr));

	net_pkt_write(pkt, &msg, sizeof(msg));
	net_pkt_write(pkt, empty_buf, SIZE_OF_SNAME);
	net_pkt_write(pkt, empty_buf, SIZE_OF_FILE);
	net_pkt_write(pkt, cookie, SIZE_OF_MAGIC_COOKIE);

	/* Options */
	net_pkt_write_u8(pkt, DHCPV4_OPTIONS_MSG_TYPE);
	net_pkt_write_u8(pkt, 1);
	net_pkt_write_u8(pkt, type);

	if (requested_ip) {
		net_pkt_write_u8(pkt, DHCPV4_OPTIONS_REQ_IPADDR);
		net_pkt_write_u8(pkt, sizeof(*requested_ip));
		net_pkt_write(pkt, requested_ip, sizeof(*requested_ip));
	}

	if (server_id) {
		net_pkt_write_u8(pkt, DHCPV4_OPTIONS_SERVER_ID);
		net_pkt_write_u8(pkt, sizeof(*server_id));
		net_pkt_write(pkt, server_id, sizeof(*server_id));
	}

	net_pkt_write_u8(pkt, DHCPV4_OPTIONS_REQ_LIST);
	net_pkt_write_u8(pkt, 1);
	net_pkt_write_u8(pkt, DHCPV4_OPTIONS_SUBNET_MASK);

	if (test_ctx.client_id) {
		net_pkt_write_u8(pkt, DHCPV4_OPTIONS_CLIENT_ID);
		net_pkt_write_u8(pkt, strlen(test_ctx.client_id));
		net_pkt_write(pkt, test_ctx.client_id, strlen(test_ctx.client_id));
	}

	if (test_ctx.lease_time != NO_LEASE_TIME) {
		net_pkt_write_u8(pkt, DHCPV4_OPTIONS_LEASE_TIME);
		net_pkt_write_u8(pkt, 4);
		net_pkt_write_be32(pkt, test_ctx.lease_time);
	}

	net_pkt_write_u8(pkt, DHCPV4_OPTIONS_END);

	net_pkt_cursor_init(pkt);
	net_ipv4_finalize(pkt, IPPROTO_UDP);

	zassert_ok(net_recv_data(test_ctx.iface, pkt), "Failed to receive data");
}

static void client_send_discover(void)
{
	int ret;

	client_prepare_test_msg(
		net_ipv4_unspecified_address(), net_ipv4_broadcast_address(),
		NET_DHCPV4_MSG_TYPE_DISCOVER, NULL, NULL, NULL);

	/* Wait for reply */
	ret = k_sem_take(&test_ctx.test_proceed, TEST_TIMEOUT);
	zassert_ok(ret, "Exchange not completed in required time");
}

static void client_send_request_solicit(void)
{
	int ret;

	client_prepare_test_msg(
		net_ipv4_unspecified_address(), net_ipv4_broadcast_address(),
		NET_DHCPV4_MSG_TYPE_REQUEST, &server_addr, &test_ctx.assigned_ip,
		NULL);

	/* Wait for reply */
	ret = k_sem_take(&test_ctx.test_proceed, TEST_TIMEOUT);
	zassert_ok(ret, "Exchange not completed in required time");
}

static void client_send_request_renew(void)
{
	int ret;

	client_prepare_test_msg(
		&test_ctx.assigned_ip, &server_addr,
		NET_DHCPV4_MSG_TYPE_REQUEST, NULL, NULL,
		&test_ctx.assigned_ip);

	/* Wait for reply */
	ret = k_sem_take(&test_ctx.test_proceed, TEST_TIMEOUT);
	zassert_ok(ret, "Exchange not completed in required time");
}

static void client_send_request_rebind(void)
{
	int ret;

	client_prepare_test_msg(
		&test_ctx.assigned_ip, net_ipv4_broadcast_address(),
		NET_DHCPV4_MSG_TYPE_REQUEST, NULL, NULL,
		&test_ctx.assigned_ip);

	/* Wait for reply */
	ret = k_sem_take(&test_ctx.test_proceed, TEST_TIMEOUT);
	zassert_ok(ret, "Exchange not completed in required time");
}

static void client_send_release(void)
{
	client_prepare_test_msg(
		&test_ctx.assigned_ip, &server_addr,
		NET_DHCPV4_MSG_TYPE_RELEASE, &server_addr, NULL,
		&test_ctx.assigned_ip);

	/* Small delay to let the DHCP server process the packet */
	k_msleep(10);
}

static void client_send_decline(void)
{
	client_prepare_test_msg(
		net_ipv4_unspecified_address(), net_ipv4_broadcast_address(),
		NET_DHCPV4_MSG_TYPE_DECLINE, &server_addr,
		&test_ctx.assigned_ip, NULL);

	/* Small delay to let the DHCP server process the packet */
	k_msleep(10);
}

static void client_send_inform(void)
{
	int ret;

	client_prepare_test_msg(
		&client_addr_static, net_ipv4_broadcast_address(),
		NET_DHCPV4_MSG_TYPE_INFORM, NULL, NULL, &client_addr_static);

	/* Wait for reply */
	ret = k_sem_take(&test_ctx.test_proceed, TEST_TIMEOUT);
	zassert_ok(ret, "Exchange not completed in required time");
}

static void lease_count_cb(struct net_if *iface, struct dhcpv4_addr_slot *lease,
			   void *user_data)
{
	struct test_lease_count *count = user_data;

	switch (lease->state) {
	case DHCPV4_SERVER_ADDR_RESERVED:
		count->reserved++;
		break;

	case DHCPV4_SERVER_ADDR_ALLOCATED:
		count->allocated++;
		break;

	case DHCPV4_SERVER_ADDR_DECLINED:
		count->declined++;
		break;

	default:
		break;
	}
}

static void test_get_lease_count(struct test_lease_count *count)
{
	int ret;

	memset(count, 0, sizeof(*count));

	ret = net_dhcpv4_server_foreach_lease(test_ctx.iface, lease_count_cb,
					      count);
	zassert_ok(ret, "Failed to obtain lease count");
}

static void verify_lease_count(int reserved, int allocated, int declined)
{
	struct test_lease_count count;

	test_get_lease_count(&count);
	zassert_equal(count.reserved, reserved,
		      "Incorrect %s count, expected %d got %d", "reserved",
		      reserved, count.reserved);
	zassert_equal(count.allocated, allocated,
		      "Incorrect %s count, expected %d got %d", "allocated",
		      allocated, count.allocated);
	zassert_equal(count.declined, declined,
		      "Incorrect %s count, expected %d got %d", "declined",
		      declined, count.declined);
}

static void get_reserved_cb(struct net_if *iface,
			    struct dhcpv4_addr_slot *lease,
			    void *user_data)
{
	struct in_addr *reserved = user_data;

	if (lease->state == DHCPV4_SERVER_ADDR_RESERVED) {
		reserved->s_addr = lease->addr.s_addr;
	}
}

static void get_reserved_address(struct in_addr *reserved)
{
	int ret;

	ret = net_dhcpv4_server_foreach_lease(test_ctx.iface,
					      get_reserved_cb,
					      reserved);
	zassert_ok(ret, "Failed to obtain reserved address");
}

static void client_get_lease(void)
{
	client_send_discover();
	verify_lease_count(1, 0, 0);
	get_reserved_address(&test_ctx.assigned_ip);
	test_pkt_free();

	client_send_request_solicit();
	verify_lease_count(0, 1, 0);
	test_pkt_free();
}

static void verify_no_option(struct net_pkt *pkt, uint8_t opt_type)
{
	struct net_pkt_cursor cursor;

	net_pkt_cursor_backup(pkt, &cursor);

	while (true) {
		uint8_t type;
		uint8_t len;

		if (net_pkt_read_u8(pkt, &type) < 0) {
			break;
		}

		if (net_pkt_read_u8(pkt, &len) < 0) {
			break;
		}

		zassert_not_equal(type, opt_type,
				 "Option %d should not be present", opt_type);

		(void)net_pkt_skip(pkt, len);
	}

	net_pkt_cursor_restore(pkt, &cursor);
}

static void verify_option(struct net_pkt *pkt, uint8_t opt_type,
			  void *optval, uint8_t optlen)
{
	struct net_pkt_cursor cursor;

	net_pkt_cursor_backup(pkt, &cursor);

	while (true) {
		static uint8_t buf[255];
		uint8_t type;
		uint8_t len;

		if (net_pkt_read_u8(pkt, &type) < 0) {
			break;
		}

		if (net_pkt_read_u8(pkt, &len) < 0) {
			break;
		}

		if (net_pkt_read(pkt, buf, len)) {
			break;
		}

		if (type == opt_type) {
			zassert_equal(len, optlen, "Invalid option length");
			zassert_mem_equal(buf, optval, optlen, "Invalid option value");

			net_pkt_cursor_restore(pkt, &cursor);
			return;
		}
	}

	zassert_true(false, "Option %d not found", opt_type);
}

static void verify_option_uint32(struct net_pkt *pkt, uint8_t opt_type,
				 uint32_t optval)
{
	optval = htonl(optval);

	verify_option(pkt, opt_type, &optval, sizeof(optval));
}

static void verify_option_uint8(struct net_pkt *pkt, uint8_t opt_type,
				uint8_t optval)
{
	verify_option(pkt, opt_type, &optval, sizeof(optval));
}

static void verify_offer(bool broadcast)
{
	NET_PKT_DATA_ACCESS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(dhcp_access, struct dhcp_msg);
	uint8_t cookie_buf[SIZE_OF_MAGIC_COOKIE];
	struct net_pkt *pkt = test_ctx.pkt;
	struct net_ipv4_hdr *ipv4_hdr;
	struct net_udp_hdr *udp_hdr;
	struct dhcp_msg *msg;
	int ret;

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	zassert_not_null(ipv4_hdr, "Failed to access IPv4 header.");
	net_pkt_acknowledge_data(pkt, &ipv4_access);

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	zassert_not_null(udp_hdr, "Failed to access UDP header.");
	net_pkt_acknowledge_data(pkt, &udp_access);

	msg = (struct dhcp_msg *)net_pkt_get_data(pkt, &dhcp_access);
	zassert_not_null(msg, "Failed to access DHCP data.");
	net_pkt_acknowledge_data(pkt, &dhcp_access);

	/* IPv4 */
	zassert_mem_equal(ipv4_hdr->src, server_addr.s4_addr,
			  sizeof(struct in_addr), "Incorrect source address");
	if (broadcast) {
		zassert_mem_equal(ipv4_hdr->dst, net_ipv4_broadcast_address(),
				  sizeof(struct in_addr),
				  "Destination should be broadcast");
	} else {
		zassert_mem_equal(ipv4_hdr->dst, msg->yiaddr,
				  sizeof(struct in_addr),
				  "Destination should match address lease");
	}
	zassert_equal(ipv4_hdr->proto, IPPROTO_UDP, "Wrong protocol");

	/* UDP */
	zassert_equal(udp_hdr->src_port, htons(DHCPV4_SERVER_PORT),
		      "Wrong source port");
	zassert_equal(udp_hdr->dst_port, htons(DHCPV4_CLIENT_PORT),
		      "Wrong client port");

	/* DHCPv4 */
	zassert_equal(msg->op, DHCPV4_MSG_BOOT_REPLY, "Incorrect %s value", "op");
	zassert_equal(msg->htype, HARDWARE_ETHERNET_TYPE, "Incorrect %s value", "htype");
	zassert_equal(msg->hlen, sizeof(client_mac_addr), "Incorrect %s value", "hlen");
	zassert_equal(msg->hops, 0, "Incorrect %s value", "hops");
	zassert_equal(msg->xid, htonl(TEST_XID), "Incorrect %s value", "xid");
	zassert_equal(msg->secs, 0, "Incorrect %s value", "secs");
	zassert_equal(sys_get_be32(msg->ciaddr), 0, "Incorrect %s value", "ciaddr");
	zassert_true((sys_get_be32(msg->yiaddr) >=
			sys_get_be32(test_base_addr.s4_addr)) &&
		     (sys_get_be32(msg->yiaddr) <
			sys_get_be32(test_base_addr.s4_addr) +
				CONFIG_NET_DHCPV4_SERVER_ADDR_COUNT),
		     "Assigned DHCP address outside of address pool");
	zassert_equal(sys_get_be32(msg->siaddr), 0, "Incorrect %s value", "siaddr");
	if (broadcast) {
		zassert_equal(msg->flags, htons(DHCPV4_MSG_BROADCAST),
			      "Incorrect %s value", "flags");
	} else {
		zassert_equal(msg->flags, 0, "Incorrect %s value", "flags");
	}
	zassert_equal(sys_get_be32(msg->giaddr), 0, "Incorrect %s value", "giaddr");
	zassert_mem_equal(msg->chaddr, client_mac_addr, sizeof(client_mac_addr),
			  "Incorrect %s value", "chaddr");

	memcpy(&test_ctx.assigned_ip, msg->yiaddr, sizeof(struct in_addr));

	ret = net_pkt_skip(pkt, SIZE_OF_SNAME + SIZE_OF_FILE);
	zassert_ok(ret, "DHCP Offer too short");

	ret = net_pkt_read(pkt, cookie_buf, SIZE_OF_MAGIC_COOKIE);
	zassert_ok(ret, "DHCP Offer too short");
	zassert_mem_equal(cookie_buf, cookie, SIZE_OF_MAGIC_COOKIE,
			 "Incorrect cookie value");

	verify_option_uint32(pkt, DHCPV4_OPTIONS_LEASE_TIME,
			     CONFIG_NET_DHCPV4_SERVER_ADDR_LEASE_TIME);
	verify_option_uint8(pkt, DHCPV4_OPTIONS_MSG_TYPE,
			    NET_DHCPV4_MSG_TYPE_OFFER);
	verify_option(pkt, DHCPV4_OPTIONS_SERVER_ID, server_addr.s4_addr,
		      sizeof(struct in_addr));
	verify_option(pkt, DHCPV4_OPTIONS_SUBNET_MASK, netmask.s4_addr,
		      sizeof(struct in_addr));
	verify_no_option(pkt, DHCPV4_OPTIONS_REQ_IPADDR);
	verify_no_option(pkt, DHCPV4_OPTIONS_REQ_LIST);
	verify_no_option(pkt, DHCPV4_OPTIONS_CLIENT_ID);
}

static void reserved_address_cb(struct net_if *iface,
				struct dhcpv4_addr_slot *lease,
				void *user_data)
{
	struct in_addr *reserved = user_data;

	zassert_equal(lease->state, DHCPV4_SERVER_ADDR_RESERVED,
		      "Wrong lease state");
	zassert_equal(reserved->s_addr, lease->addr.s_addr,
		      "Reserved wrong address");
}

static void verify_reserved_address(struct in_addr *reserved)
{
	int ret;

	ret = net_dhcpv4_server_foreach_lease(test_ctx.iface,
					      reserved_address_cb,
					      reserved);
	zassert_ok(ret, "Failed to verify reserved address");
}

/* Verify that the DHCP server replies with Offer for a Discover message. */
ZTEST(dhcpv4_server_tests, test_discover)
{
	client_send_discover();
	verify_offer(false);
	test_pkt_free();

	verify_lease_count(1, 0, 0);
	verify_reserved_address(&test_ctx.assigned_ip);
}

/* Verify that the DHCP server offers the same IP address for repeated Discover
 * message.
 */
ZTEST(dhcpv4_server_tests, test_discover_repeat)
{
	struct in_addr first_addr;

	client_send_discover();
	verify_offer(false);
	test_pkt_free();

	first_addr = test_ctx.assigned_ip;
	verify_lease_count(1, 0, 0);

	/* Repeat Discover with the same client ID */
	client_send_discover();
	verify_offer(false);
	test_pkt_free();

	verify_lease_count(1, 0, 0);
	zassert_equal(first_addr.s_addr, test_ctx.assigned_ip.s_addr,
		      "Received different address for the same client ID");

	/* Send Discover with a different client ID */
	test_ctx.client_id = CLIENT_ID_2;

	client_send_discover();
	verify_offer(false);
	test_pkt_free();

	verify_lease_count(2, 0, 0);
	zassert_not_equal(first_addr.s_addr, test_ctx.assigned_ip.s_addr,
			  "Received same address for the different client ID");
}

/* Verify that the DHCP server replies to broadcast address if broadcast flag
 * is set.
 */
ZTEST(dhcpv4_server_tests, test_discover_with_broadcast)
{
	test_ctx.broadcast = true;

	client_send_discover();
	verify_offer(true);
	verify_lease_count(1, 0, 0);
	test_pkt_free();
}

static void verify_ack(bool inform, bool renew)
{
	NET_PKT_DATA_ACCESS_DEFINE(ipv4_access, struct net_ipv4_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	NET_PKT_DATA_ACCESS_DEFINE(dhcp_access, struct dhcp_msg);
	uint8_t cookie_buf[SIZE_OF_MAGIC_COOKIE];
	struct net_pkt *pkt = test_ctx.pkt;
	struct net_ipv4_hdr *ipv4_hdr;
	struct net_udp_hdr *udp_hdr;
	struct dhcp_msg *msg;
	int ret;

	ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(pkt, &ipv4_access);
	zassert_not_null(ipv4_hdr, "Failed to access IPv4 header.");
	net_pkt_acknowledge_data(pkt, &ipv4_access);

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	zassert_not_null(udp_hdr, "Failed to access UDP header.");
	net_pkt_acknowledge_data(pkt, &udp_access);

	msg = (struct dhcp_msg *)net_pkt_get_data(pkt, &dhcp_access);
	zassert_not_null(msg, "Failed to access DHCP data.");
	net_pkt_acknowledge_data(pkt, &dhcp_access);

	/* IPv4 */
	zassert_mem_equal(ipv4_hdr->src, server_addr.s4_addr,
			  sizeof(struct in_addr), "Incorrect source address");
	if (inform || renew) {
		zassert_mem_equal(ipv4_hdr->dst, msg->ciaddr, sizeof(struct in_addr),
				  "Destination should match client address");
	} else {
		zassert_mem_equal(ipv4_hdr->dst, msg->yiaddr, sizeof(struct in_addr),
				  "Destination should match client address");
	}

	zassert_equal(ipv4_hdr->proto, IPPROTO_UDP, "Wrong protocol");

	/* UDP */
	zassert_equal(udp_hdr->src_port, htons(DHCPV4_SERVER_PORT),
		      "Wrong source port");
	zassert_equal(udp_hdr->dst_port, htons(DHCPV4_CLIENT_PORT),
		      "Wrong client port");

	/* DHCPv4 */
	zassert_equal(msg->op, DHCPV4_MSG_BOOT_REPLY, "Incorrect %s value", "op");
	zassert_equal(msg->htype, HARDWARE_ETHERNET_TYPE, "Incorrect %s value", "htype");
	zassert_equal(msg->hlen, sizeof(client_mac_addr), "Incorrect %s value", "hlen");
	zassert_equal(msg->hops, 0, "Incorrect %s value", "hops");
	zassert_equal(msg->xid, htonl(TEST_XID), "Incorrect %s value", "xid");
	zassert_equal(msg->secs, 0, "Incorrect %s value", "secs");

	if (inform) {
		zassert_mem_equal(msg->ciaddr, client_addr_static.s4_addr,
				  sizeof(struct in_addr),
				  "Incorrect %s value", "ciaddr");
	} else if (renew) {
		zassert_mem_equal(msg->ciaddr, test_ctx.assigned_ip.s4_addr,
				  sizeof(struct in_addr),
				  "Incorrect %s value", "ciaddr");
	} else {
		zassert_equal(sys_get_be32(msg->ciaddr), 0, "Incorrect %s value", "ciaddr");
	}

	if (inform) {
		zassert_equal(sys_get_be32(msg->yiaddr), 0, "Incorrect %s value", "yiaddr");
	} else {
		zassert_mem_equal(msg->yiaddr, test_ctx.assigned_ip.s4_addr,
				  sizeof(struct in_addr), "Incorrect %s value", "yiaddr");
	}

	zassert_equal(sys_get_be32(msg->siaddr), 0, "Incorrect %s value", "siaddr");
	zassert_equal(msg->flags, 0, "Incorrect %s value", "flags");
	zassert_equal(sys_get_be32(msg->giaddr), 0, "Incorrect %s value", "giaddr");
	zassert_mem_equal(msg->chaddr, client_mac_addr, sizeof(client_mac_addr),
			  "Incorrect %s value", "chaddr");

	if (!inform) {
		memcpy(&test_ctx.assigned_ip, msg->yiaddr, sizeof(struct in_addr));
	}

	ret = net_pkt_skip(pkt, SIZE_OF_SNAME + SIZE_OF_FILE);
	zassert_ok(ret, "DHCP Offer too short");

	ret = net_pkt_read(pkt, cookie_buf, SIZE_OF_MAGIC_COOKIE);
	zassert_ok(ret, "DHCP Offer too short");
	zassert_mem_equal(cookie_buf, cookie, SIZE_OF_MAGIC_COOKIE,
			 "Incorrect cookie value");

	if (inform) {
		verify_no_option(pkt, DHCPV4_OPTIONS_LEASE_TIME);
	} else {
		verify_option_uint32(pkt, DHCPV4_OPTIONS_LEASE_TIME,
				     CONFIG_NET_DHCPV4_SERVER_ADDR_LEASE_TIME);
	}

	verify_option_uint8(pkt, DHCPV4_OPTIONS_MSG_TYPE,
			    NET_DHCPV4_MSG_TYPE_ACK);
	verify_option(pkt, DHCPV4_OPTIONS_SERVER_ID, server_addr.s4_addr,
		      sizeof(struct in_addr));
	verify_option(pkt, DHCPV4_OPTIONS_SUBNET_MASK, netmask.s4_addr,
		      sizeof(struct in_addr));
	verify_no_option(pkt, DHCPV4_OPTIONS_REQ_IPADDR);
	verify_no_option(pkt, DHCPV4_OPTIONS_REQ_LIST);
	verify_no_option(pkt, DHCPV4_OPTIONS_CLIENT_ID);
}

static void allocated_address_cb(struct net_if *iface,
				 struct dhcpv4_addr_slot *lease,
				 void *user_data)
{
	struct in_addr *allocated = user_data;

	zassert_equal(lease->state, DHCPV4_SERVER_ADDR_ALLOCATED,
		      "Wrong lease state");
	zassert_equal(allocated->s_addr, lease->addr.s_addr,
		      "Reserved wrong address");
}

static void verify_allocated_address(struct in_addr *allocated)
{
	int ret;

	ret = net_dhcpv4_server_foreach_lease(test_ctx.iface,
					      allocated_address_cb,
					      allocated);
	zassert_ok(ret, "Failed to verify allocated address");
}

/* Verify that the DHCP server replies with ACK for a Request message. */
ZTEST(dhcpv4_server_tests, test_request)
{
	client_send_discover();
	verify_offer(false);
	verify_lease_count(1, 0, 0);
	test_pkt_free();

	client_send_request_solicit();
	verify_ack(false, false);
	verify_lease_count(0, 1, 0);
	verify_allocated_address(&test_ctx.assigned_ip);
	test_pkt_free();
}

/* Verify that the DHCP server replies with ACK for a Request message
 * (renewing).
 */
ZTEST(dhcpv4_server_tests, test_renew)
{
	client_get_lease();

	client_send_request_renew();
	verify_ack(false, true);
	verify_lease_count(0, 1, 0);
	test_pkt_free();
}

/* Verify that the DHCP server replies with ACK for a Request message
 * (rebinding).
 */
ZTEST(dhcpv4_server_tests, test_rebind)
{
	client_get_lease();

	client_send_request_rebind();
	verify_ack(false, true);
	verify_lease_count(0, 1, 0);
	test_pkt_free();
}

/* Verify that the DHCP server lease expires after the lease timeout. */
ZTEST(dhcpv4_server_tests, test_expiry)
{
	test_ctx.lease_time = 1;
	client_get_lease();

	/* Add extra 10ms to avoid race. */
	k_msleep(1000 + 10);
	verify_lease_count(0, 0, 0);
}

/* Verify that the DHCP server releases the lease after receiving Release
 * message.
 */
ZTEST(dhcpv4_server_tests, test_release)
{
	client_get_lease();

	client_send_release();
	verify_lease_count(0, 0, 0);
}

static void declined_address_cb(struct net_if *iface,
				struct dhcpv4_addr_slot *lease,
				void *user_data)
{
	struct in_addr *declined = user_data;

	zassert_equal(lease->state, DHCPV4_SERVER_ADDR_DECLINED,
		      "Wrong lease state");
	zassert_equal(declined->s_addr, lease->addr.s_addr,
		      "Declined wrong address");
}

static void verify_declined_address(struct in_addr *declined)
{
	int ret;

	ret = net_dhcpv4_server_foreach_lease(test_ctx.iface,
					      declined_address_cb,
					      declined);
	zassert_ok(ret, "Failed to verify declined address");
}

/* Verify that the DHCP server blocks the address after receiving Decline
 * message.
 */
ZTEST(dhcpv4_server_tests, test_decline)
{
	client_get_lease();
	verify_lease_count(0, 1, 0);

	client_send_decline();
	verify_lease_count(0, 0, 1);
	verify_declined_address(&test_ctx.assigned_ip);
}

/* Verify that the DHCP server replies with ACK for a Inform message, w/o
 * address assignment.
 */
ZTEST(dhcpv4_server_tests, test_inform)
{
	client_send_inform();
	verify_ack(true, false);
	verify_lease_count(0, 0, 0);
}

/* Verify that the DHCP server can start and validate input properly. */
ZTEST(dhcpv4_server_tests_no_init, test_initialization)
{
	struct in_addr base_addr_wrong_subnet = { { { 192, 0, 3, 10 } } };
	struct in_addr base_addr_overlap = { { { 192, 0, 2, 1 } } };
	int ret;

	ret = net_dhcpv4_server_start(test_ctx.iface, &base_addr_wrong_subnet);
	zassert_equal(ret, -EINVAL, "Started server for wrong subnet");

	ret = net_dhcpv4_server_start(test_ctx.iface, &base_addr_overlap);
	zassert_equal(ret, -EINVAL, "Started server for overlapping address");

	ret = net_dhcpv4_server_start(test_ctx.iface, &test_base_addr);
	zassert_ok(ret, "Failed to start server for valid address range");

	net_dhcpv4_server_stop(test_ctx.iface);
}

static void dhcpv4_server_tests_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_init(&test_ctx.test_proceed, 0, 1);
	test_ctx.client_id = CLIENT_ID_1;
	test_ctx.broadcast = false;
	test_ctx.pkt = NULL;
	test_ctx.lease_time = NO_LEASE_TIME;
	memset(&test_ctx.assigned_ip, 0, sizeof(test_ctx.assigned_ip));

	net_dhcpv4_server_start(test_ctx.iface, &test_base_addr);
}

static void dhcpv4_server_tests_after(void *fixture)
{
	ARG_UNUSED(fixture);

	test_pkt_free();

	net_dhcpv4_server_stop(test_ctx.iface);
}

ZTEST_SUITE(dhcpv4_server_tests, NULL, NULL, dhcpv4_server_tests_before,
	    dhcpv4_server_tests_after, NULL);

ZTEST_SUITE(dhcpv4_server_tests_no_init, NULL, NULL, NULL, NULL, NULL);
