/*
 * Copyright (c) 2026 Sergei Shirokov
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two interfaces on the same subnet can both reach a peer. A sendmsg() that
 * names a source address with IP_PKTINFO or IPV6_PKTINFO must leave through the
 * interface that owns that address, and carry it as the source. This is what a
 * server bound to the wildcard address relies on to answer from the address a
 * request came in on.
 */

#include <zephyr/kernel.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include <string.h>

#include "ipv6.h"

#define TEST_PORT 4242
#define WAIT_TIME K_MSEC(1000)

static const char payload[] = "pktinfo";

static const struct net_in_addr my_addr4[] = {
	{ { { 192, 0, 2, 1 } } },
	{ { { 192, 0, 2, 2 } } },
};
static const struct net_in_addr netmask4 = { { { 255, 255, 255, 0 } } };
static const struct net_in_addr peer_addr4 = { { { 192, 0, 2, 10 } } };
static const struct net_in_addr unknown_addr4 = { { { 198, 51, 100, 1 } } };
/* A subnet that only one interface has, at a time. */
static const struct net_in_addr other_addr4 = { { { 203, 0, 113, 2 } } };
static const struct net_in_addr other_peer_addr4 = { { { 203, 0, 113, 10 } } };
/* On no interface's subnet, so reachable only through a gateway. */
static const struct net_in_addr remote_addr4 = { { { 198, 18, 0, 10 } } };
static const struct net_in_addr gw_addr4 = { { { 192, 0, 2, 254 } } };
static const struct net_in_addr mcast_addr4 = { { { 224, 0, 1, 187 } } };

static const struct net_in6_addr my_addr6[] = {
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } },
};
static const struct net_in6_addr prefix6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };
static const struct net_in6_addr peer_addr6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 } } };
/* A prefix that only one interface has, at a time. */
static const struct net_in6_addr other_prefix6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } } };
static const struct net_in6_addr other_peer_addr6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0, 0x1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 } } };
/* A longer prefix than prefix6/64 that covers peer_addr6. */
#define LONG_PREFIX6_LEN 96
/* On no interface's prefix, so reachable only through a router. */
static const struct net_in6_addr remote_addr6 = {
	{ { 0x20, 0x01, 0x0d, 0xb8, 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 } } };
static const struct net_in6_addr router_addr6 = {
	{ { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static const struct net_in6_addr ll_peer_addr6 = {
	{ { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x10 } } };
static const struct net_in6_addr mcast_addr6 = {
	{ { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } };

/* Neighbour discovery is off, so the peer is added to the cache by hand. */
static const struct net_linkaddr peer_lladdr = {
	.addr = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x10 },
	.len = NET_ETH_ADDR_LEN,
	.type = NET_LINK_ETHERNET,
};

/* What the last packet sent looked like. */
static struct {
	struct net_if *iface;
	uint8_t src[NET_IPV6_ADDR_SIZE];
} sent;

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
	ARG_UNUSED(dev);

	sent.iface = net_pkt_iface(pkt);

	if (net_pkt_family(pkt) == NET_AF_INET) {
		memcpy(sent.src, NET_IPV4_HDR(pkt)->src, NET_IPV4_ADDR_SIZE);
	} else {
		memcpy(sent.src, NET_IPV6_HDR(pkt)->src, NET_IPV6_ADDR_SIZE);
	}

	k_sem_give(&sent_sem);

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

/* The socket a case opened, closed in after() even when the case fails. */
static int test_sock = -1;

static void *setup(void)
{
	ifaces[0] = NET_IF_GET(test_dev0, 0);
	ifaces[1] = NET_IF_GET(test_dev1, 1);

	for (int i = 0; i < ARRAY_SIZE(ifaces); i++) {
		zassert_not_null(net_if_ipv4_addr_add(ifaces[i], &my_addr4[i], NET_ADDR_MANUAL, 0),
				 "Cannot add IPv4 address %d", i);
		zassert_true(net_if_ipv4_set_netmask_by_addr(ifaces[i], &my_addr4[i], &netmask4),
			     "Cannot set IPv4 netmask %d", i);

		zassert_not_null(net_if_ipv6_addr_add(ifaces[i], &my_addr6[i], NET_ADDR_MANUAL, 0),
				 "Cannot add IPv6 address %d", i);
		zassert_not_null(net_if_ipv6_prefix_add(ifaces[i], &prefix6, 64, UINT32_MAX),
				 "Cannot add IPv6 prefix %d", i);
		zassert_not_null(net_ipv6_nbr_add(ifaces[i], &peer_addr6, &peer_lladdr, false,
						  NET_IPV6_NBR_STATE_REACHABLE),
				 "Cannot add IPv6 neighbour %d", i);

		(void)net_if_up(ifaces[i]);
	}

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&sent_sem);
	memset(&sent, 0, sizeof(sent));
}

/* net_ipv6_nbr_rm() takes a non-const address. */
static bool nbr_rm_v6(struct net_if *iface, const struct net_in6_addr *addr)
{
	struct net_in6_addr tmp = *addr;

	return net_ipv6_nbr_rm(iface, &tmp);
}

/* The routers a case added, removed in after() even when the case fails. */
static struct net_if_router *test_routers[2];

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (test_sock >= 0) {
		(void)zsock_close(test_sock);
		test_sock = -1;
	}

	for (int i = 0; i < ARRAY_SIZE(test_routers); i++) {
		if (test_routers[i] != NULL) {
			(void)net_if_ipv6_router_rm(test_routers[i]);
			test_routers[i] = NULL;
		}
	}

	/* Undo what any case may have added; each call fails harmlessly if
	 * there is nothing to remove.
	 */
	for (int i = 0; i < ARRAY_SIZE(ifaces); i++) {
		(void)net_if_ipv4_addr_rm(ifaces[i], &other_addr4);
		net_if_ipv4_set_gw(ifaces[i], net_ipv4_unspecified_address());
		(void)net_if_ipv6_prefix_rm(ifaces[i], &other_prefix6, 64);
		(void)net_if_ipv6_prefix_rm(ifaces[i], &prefix6, LONG_PREFIX6_LEN);
		(void)nbr_rm_v6(ifaces[i], &other_peer_addr6);
		(void)nbr_rm_v6(ifaces[i], &router_addr6);
	}
}

/* Bind to the wildcard address, as a server answering on every interface does. */
static int open_wildcard_sock(net_sa_family_t family)
{
	int sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	int ret;

	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);
	test_sock = sock;

	if (family == NET_AF_INET) {
		struct net_sockaddr_in addr = {
			.sin_family = NET_AF_INET,
			.sin_port = net_htons(TEST_PORT),
		};

		ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	} else {
		struct net_sockaddr_in6 addr = {
			.sin6_family = NET_AF_INET6,
			.sin6_port = net_htons(TEST_PORT),
		};

		ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	}

	zassert_equal(ret, 0, "Cannot bind socket (%d)", -errno);

	return sock;
}

/* sendmsg() to dst with PKTINFO; ifindex 0 leaves the interface unset. */
static int sendmsg_v4(int sock, const struct net_in_addr *dst_addr,
		      const struct net_in_addr *src, unsigned int ifindex)
{
	struct net_sockaddr_in dst = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(TEST_PORT),
		.sin_addr = *dst_addr,
	};
	struct net_in_pktinfo info = { .ipi_ifindex = ifindex, .ipi_spec_dst = *src };
	union {
		struct net_cmsghdr hdr;
		uint8_t buf[NET_CMSG_SPACE(sizeof(struct net_in_pktinfo))];
	} cmsg_storage = { 0 };
	struct net_iovec io_vec = {
		.iov_base = (void *)payload,
		.iov_len = sizeof(payload) - 1,
	};
	struct net_msghdr msg = {
		.msg_name = &dst,
		.msg_namelen = sizeof(dst),
		.msg_iov = &io_vec,
		.msg_iovlen = 1,
		.msg_control = cmsg_storage.buf,
		.msg_controllen = sizeof(cmsg_storage.buf),
	};
	struct net_cmsghdr *cmsg = NET_CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_level = NET_IPPROTO_IP;
	cmsg->cmsg_type = ZSOCK_IP_PKTINFO;
	cmsg->cmsg_len = NET_CMSG_LEN(sizeof(info));
	memcpy(NET_CMSG_DATA(cmsg), &info, sizeof(info));

	return zsock_sendmsg(sock, &msg, 0);
}

static int sendmsg_v4_from(int sock, const struct net_in_addr *src, unsigned int ifindex)
{
	return sendmsg_v4(sock, &peer_addr4, src, ifindex);
}

/* sendmsg() to dst, with a scope ID if it's not 0, and with PKTINFO; ifindex 0
 * leaves the interface unset.
 */
static int sendmsg_v6(int sock, const struct net_in6_addr *dst_addr, uint32_t scope_id,
		      const struct net_in6_addr *src, unsigned int ifindex)
{
	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(TEST_PORT),
		.sin6_addr = *dst_addr,
		.sin6_scope_id = scope_id,
	};
	struct net_in6_pktinfo info = { .ipi6_addr = *src, .ipi6_ifindex = ifindex };
	union {
		struct net_cmsghdr hdr;
		uint8_t buf[NET_CMSG_SPACE(sizeof(struct net_in6_pktinfo))];
	} cmsg_storage = { 0 };
	struct net_iovec io_vec = {
		.iov_base = (void *)payload,
		.iov_len = sizeof(payload) - 1,
	};
	struct net_msghdr msg = {
		.msg_name = &dst,
		.msg_namelen = sizeof(dst),
		.msg_iov = &io_vec,
		.msg_iovlen = 1,
		.msg_control = cmsg_storage.buf,
		.msg_controllen = sizeof(cmsg_storage.buf),
	};
	struct net_cmsghdr *cmsg = NET_CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_level = NET_IPPROTO_IPV6;
	cmsg->cmsg_type = ZSOCK_IPV6_PKTINFO;
	cmsg->cmsg_len = NET_CMSG_LEN(sizeof(info));
	memcpy(NET_CMSG_DATA(cmsg), &info, sizeof(info));

	return zsock_sendmsg(sock, &msg, 0);
}

static int sendmsg_v6_from(int sock, const struct net_in6_addr *src, unsigned int ifindex)
{
	return sendmsg_v6(sock, &peer_addr6, 0U, src, ifindex);
}

/* Send with PKTINFO and wait until the packet has gone out. */
static void send_v4_from(int sock, const struct net_in_addr *src, unsigned int ifindex)
{
	int ret = sendmsg_v4_from(sock, src, ifindex);

	zassert_equal(ret, sizeof(payload) - 1, "sendmsg failed (%d)", -errno);
	zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");
}

static void send_v6_from(int sock, const struct net_in6_addr *src, unsigned int ifindex)
{
	int ret = sendmsg_v6_from(sock, src, ifindex);

	zassert_equal(ret, sizeof(payload) - 1, "sendmsg failed (%d)", -errno);
	zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");
}

ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_selects_iface)
{
	int sock = open_wildcard_sock(NET_AF_INET);

	/* Both, in both orders, so that neither one wins only by being the default. */
	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);

		k_sem_reset(&sent_sem);
		send_v4_from(sock, &my_addr4[n], 0U);

		zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));
		zassert_mem_equal(sent.src, &my_addr4[n], NET_IPV4_ADDR_SIZE,
				  "Wrong source address for interface %d", n);
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_selects_iface)
{
	int sock = open_wildcard_sock(NET_AF_INET6);

	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);

		k_sem_reset(&sent_sem);
		send_v6_from(sock, &my_addr6[n], 0U);

		zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));
		zassert_mem_equal(sent.src, &my_addr6[n], NET_IPV6_ADDR_SIZE,
				  "Wrong source address for interface %d", n);
	}
}

/* Bind to one interface's address, as a server answering on only that one does. */
static int open_bound_sock(net_sa_family_t family, int n)
{
	int sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	int ret;

	zassert_true(sock >= 0, "Cannot create socket (%d)", -errno);
	test_sock = sock;

	if (family == NET_AF_INET) {
		struct net_sockaddr_in addr = {
			.sin_family = NET_AF_INET,
			.sin_port = net_htons(TEST_PORT),
			.sin_addr = my_addr4[n],
		};

		ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	} else {
		struct net_sockaddr_in6 addr = {
			.sin6_family = NET_AF_INET6,
			.sin6_port = net_htons(TEST_PORT),
			.sin6_addr = my_addr6[n],
		};

		ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	}

	zassert_equal(ret, 0, "Cannot bind socket (%d)", -errno);

	return sock;
}

static void close_test_sock(void)
{
	zassert_ok(zsock_close(test_sock), "Cannot close socket (%d)", -errno);
	test_sock = -1;
}

/* A plain sendto() on a socket bound to a unicast address leaves through the
 * interface that owns it, not the one the destination would select.
 */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendto_bound_addr_selects_iface)
{
	struct net_sockaddr_in dst = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(TEST_PORT),
		.sin_addr = peer_addr4,
	};

	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);
		int sock = open_bound_sock(NET_AF_INET, n);
		int ret;

		k_sem_reset(&sent_sem);
		ret = zsock_sendto(sock, payload, sizeof(payload) - 1, 0,
				   (struct net_sockaddr *)&dst, sizeof(dst));
		zassert_equal(ret, sizeof(payload) - 1, "sendto failed (%d)", -errno);
		zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");

		zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));
		zassert_mem_equal(sent.src, &my_addr4[n], NET_IPV4_ADDR_SIZE,
				  "Wrong source address for interface %d", n);

		close_test_sock();
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendto_bound_addr_selects_iface)
{
	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(TEST_PORT),
		.sin6_addr = peer_addr6,
	};

	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);
		int sock = open_bound_sock(NET_AF_INET6, n);
		int ret;

		k_sem_reset(&sent_sem);
		ret = zsock_sendto(sock, payload, sizeof(payload) - 1, 0,
				   (struct net_sockaddr *)&dst, sizeof(dst));
		zassert_equal(ret, sizeof(payload) - 1, "sendto failed (%d)", -errno);
		zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");

		zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));
		zassert_mem_equal(sent.src, &my_addr6[n], NET_IPV6_ADDR_SIZE,
				  "Wrong source address for interface %d", n);

		close_test_sock();
	}
}

/* A source address that no interface owns falls back to the usual selection. */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_unknown_addr)
{
	int sock = open_wildcard_sock(NET_AF_INET);

	send_v4_from(sock, &unknown_addr4, 0U);

	zassert_true(sent.iface == ifaces[0] || sent.iface == ifaces[1],
		     "Sent on an unexpected interface %d", net_if_get_by_iface(sent.iface));
	zassert_true(memcmp(sent.src, &my_addr4[0], NET_IPV4_ADDR_SIZE) == 0 ||
			     memcmp(sent.src, &my_addr4[1], NET_IPV4_ADDR_SIZE) == 0,
		     "Unexpected source address");
}

/* A non-zero ipi_ifindex selects the output interface. */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_ifindex)
{
	int sock = open_wildcard_sock(NET_AF_INET);

	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);

		k_sem_reset(&sent_sem);
		send_v4_from(sock, net_ipv4_unspecified_address(),
			     net_if_get_by_iface(ifaces[n]));

		zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));
		zassert_mem_equal(sent.src, &my_addr4[n], NET_IPV4_ADDR_SIZE,
				  "Wrong source address for interface %d", n);
	}
}

/* Index and address disagree: the index decides, and the source is then that
 * interface's own address, since an interface only sends from its addresses.
 */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_ifindex_wins)
{
	int sock = open_wildcard_sock(NET_AF_INET);

	send_v4_from(sock, &my_addr4[0], net_if_get_by_iface(ifaces[1]));

	zassert_equal_ptr(sent.iface, ifaces[1], "Sent on interface %d, expected %d",
			  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[1]));
	zassert_mem_equal(sent.src, &my_addr4[1], NET_IPV4_ADDR_SIZE,
			  "Wrong source address");
}

ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_bad_ifindex)
{
	int sock = open_wildcard_sock(NET_AF_INET);
	int ret = sendmsg_v4_from(sock, &my_addr4[0], 200U);

	zassert_equal(ret, -1, "sendmsg should fail");
	zassert_equal(errno, ENODEV, "Wrong errno %d", errno);
	zassert_equal(k_sem_take(&sent_sem, K_MSEC(100)), -EAGAIN, "Something was sent");
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_ifindex)
{
	int sock = open_wildcard_sock(NET_AF_INET6);

	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);

		k_sem_reset(&sent_sem);
		send_v6_from(sock, net_ipv6_unspecified_address(),
			     net_if_get_by_iface(ifaces[n]));

		zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
				  net_if_get_by_iface(sent.iface), net_if_get_by_iface(ifaces[n]));
		zassert_mem_equal(sent.src, &my_addr6[n], NET_IPV6_ADDR_SIZE,
				  "Wrong source address for interface %d", n);
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_bad_ifindex)
{
	int sock = open_wildcard_sock(NET_AF_INET6);
	int ret = sendmsg_v6_from(sock, &my_addr6[0], 200U);

	zassert_equal(ret, -1, "sendmsg should fail");
	zassert_equal(errno, ENODEV, "Wrong errno %d", errno);
	zassert_equal(k_sem_take(&sent_sem, K_MSEC(100)), -EAGAIN, "Something was sent");
}

static int ifindex_of(int n)
{
	return net_if_get_by_iface(ifaces[n]);
}

static void assert_sent_on(int n)
{
	zassert_equal_ptr(sent.iface, ifaces[n], "Sent on interface %d, expected %d",
			  net_if_get_by_iface(sent.iface), ifindex_of(n));
}

static void assert_nothing_sent(void)
{
	zassert_equal(k_sem_take(&sent_sem, K_MSEC(100)), -EAGAIN, "Something was sent");
}

/* A plain sendto() to dst, waiting until the packet has gone out. */
static void sendto_v4(int sock, const struct net_in_addr *dst_addr)
{
	struct net_sockaddr_in dst = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(TEST_PORT),
		.sin_addr = *dst_addr,
	};
	int ret;

	k_sem_reset(&sent_sem);
	ret = zsock_sendto(sock, payload, sizeof(payload) - 1, 0, (struct net_sockaddr *)&dst,
			   sizeof(dst));
	zassert_equal(ret, sizeof(payload) - 1, "sendto failed (%d)", -errno);
	zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");
}

static void sendto_v6(int sock, const struct net_in6_addr *dst_addr)
{
	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(TEST_PORT),
		.sin6_addr = *dst_addr,
	};
	int ret;

	k_sem_reset(&sent_sem);
	ret = zsock_sendto(sock, payload, sizeof(payload) - 1, 0, (struct net_sockaddr *)&dst,
			   sizeof(dst));
	zassert_equal(ret, sizeof(payload) - 1, "sendto failed (%d)", -errno);
	zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");
}

/* The bound address's interface doesn't have the destination on-link, but
 * the other one does: the destination decides, as without the binding.
 */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendto_bound_addr_onlink_elsewhere)
{
	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		int other = 1 - n;

		zassert_not_null(net_if_ipv4_addr_add(ifaces[other], &other_addr4,
						      NET_ADDR_MANUAL, 0),
				 "Cannot add IPv4 address");
		zassert_true(net_if_ipv4_set_netmask_by_addr(ifaces[other], &other_addr4,
							     &netmask4),
			     "Cannot set IPv4 netmask");

		sendto_v4(open_bound_sock(NET_AF_INET, n), &other_peer_addr4);
		assert_sent_on(other);

		close_test_sock();
		zassert_true(net_if_ipv4_addr_rm(ifaces[other], &other_addr4),
			     "Cannot remove IPv4 address");
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendto_bound_addr_onlink_elsewhere)
{
	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		int other = 1 - n;

		zassert_not_null(net_if_ipv6_prefix_add(ifaces[other], &other_prefix6, 64,
							UINT32_MAX),
				 "Cannot add IPv6 prefix");
		zassert_not_null(net_ipv6_nbr_add(ifaces[other], &other_peer_addr6, &peer_lladdr,
						  false, NET_IPV6_NBR_STATE_REACHABLE),
				 "Cannot add IPv6 neighbour");

		sendto_v6(open_bound_sock(NET_AF_INET6, n), &other_peer_addr6);
		assert_sent_on(other);

		close_test_sock();
		zassert_true(net_if_ipv6_prefix_rm(ifaces[other], &other_prefix6, 64),
			     "Cannot remove IPv6 prefix");
		zassert_true(nbr_rm_v6(ifaces[other], &other_peer_addr6),
			     "Cannot remove IPv6 neighbour");
	}
}

/* The destination is on no interface's subnet: the bound address's interface
 * is used if it has a gateway, and otherwise the one that has.
 */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendto_bound_addr_gateway)
{
	for (int g = 0; g < ARRAY_SIZE(ifaces); g++) {
		net_if_ipv4_set_gw(ifaces[g], &gw_addr4);

		for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
			sendto_v4(open_bound_sock(NET_AF_INET, n), &remote_addr4);
			assert_sent_on(g);
			close_test_sock();
		}

		net_if_ipv4_set_gw(ifaces[g], net_ipv4_unspecified_address());
	}

	/* With a gateway on both, the bound address decides. */
	for (int g = 0; g < ARRAY_SIZE(ifaces); g++) {
		net_if_ipv4_set_gw(ifaces[g], &gw_addr4);
	}

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		sendto_v4(open_bound_sock(NET_AF_INET, n), &remote_addr4);
		assert_sent_on(n);
		close_test_sock();
	}
}

static void add_router_v6(int g)
{
	test_routers[g] = net_if_ipv6_router_add(ifaces[g], &router_addr6, true, 0);
	zassert_not_null(test_routers[g], "Cannot add IPv6 router");
	zassert_not_null(net_ipv6_nbr_add(ifaces[g], &router_addr6, &peer_lladdr, true,
					  NET_IPV6_NBR_STATE_REACHABLE),
			 "Cannot add IPv6 router neighbour");
}

static void rm_router_v6(int g)
{
	zassert_true(net_if_ipv6_router_rm(test_routers[g]), "Cannot remove IPv6 router");
	test_routers[g] = NULL;
	zassert_true(nbr_rm_v6(ifaces[g], &router_addr6),
		     "Cannot remove IPv6 router neighbour");
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendto_bound_addr_gateway)
{
	for (int g = 0; g < ARRAY_SIZE(ifaces); g++) {
		add_router_v6(g);

		for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
			sendto_v6(open_bound_sock(NET_AF_INET6, n), &remote_addr6);
			assert_sent_on(g);
			close_test_sock();
		}

		rm_router_v6(g);
	}

	/* With a router on both, the bound address decides. */
	for (int g = 0; g < ARRAY_SIZE(ifaces); g++) {
		add_router_v6(g);
	}

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		sendto_v6(open_bound_sock(NET_AF_INET6, n), &remote_addr6);
		assert_sent_on(n);
		close_test_sock();
	}
}

/* On a socket tied to ifaces[n], a PKTINFO index may only name that one. */
static void check_ifindex_tied_to(int sock, net_sa_family_t family, int n)
{
	int ret;

	k_sem_reset(&sent_sem);
	if (family == NET_AF_INET) {
		send_v4_from(sock, net_ipv4_unspecified_address(), ifindex_of(n));
	} else {
		send_v6_from(sock, net_ipv6_unspecified_address(), ifindex_of(n));
	}

	assert_sent_on(n);

	k_sem_reset(&sent_sem);
	ret = (family == NET_AF_INET)
		      ? sendmsg_v4_from(sock, net_ipv4_unspecified_address(), ifindex_of(1 - n))
		      : sendmsg_v6_from(sock, net_ipv6_unspecified_address(), ifindex_of(1 - n));
	zassert_equal(ret, -1, "sendmsg with another interface should fail");
	zassert_equal(errno, EINVAL, "Wrong errno %d", errno);

	ret = (family == NET_AF_INET)
		      ? sendmsg_v4_from(sock, net_ipv4_unspecified_address(), 200U)
		      : sendmsg_v6_from(sock, net_ipv6_unspecified_address(), 200U);
	zassert_equal(ret, -1, "sendmsg with an unknown interface should fail");
	zassert_equal(errno, ENODEV, "Wrong errno %d", errno);

	assert_nothing_sent();
}

/* Connect to the peer, and find the interface a plain send() then uses. */
static int connect_and_find_iface(int sock, net_sa_family_t family)
{
	struct net_sockaddr_in dst4 = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(TEST_PORT),
		.sin_addr = peer_addr4,
	};
	struct net_sockaddr_in6 dst6 = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(TEST_PORT),
		.sin6_addr = peer_addr6,
	};
	int ret;

	if (family == NET_AF_INET) {
		ret = zsock_connect(sock, (struct net_sockaddr *)&dst4, sizeof(dst4));
	} else {
		ret = zsock_connect(sock, (struct net_sockaddr *)&dst6, sizeof(dst6));
	}

	zassert_equal(ret, 0, "Cannot connect (%d)", -errno);

	k_sem_reset(&sent_sem);
	ret = zsock_send(sock, payload, sizeof(payload) - 1, 0);
	zassert_equal(ret, sizeof(payload) - 1, "send failed (%d)", -errno);
	zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");

	return (sent.iface == ifaces[0]) ? 0 : 1;
}

/* On a connected socket the index wins, whichever interface the socket has: it
 * follows the packets the socket receives, so it can't be held against the
 * caller. An unknown index still fails.
 */
static void check_ifindex_wins_connected(int sock, net_sa_family_t family)
{
	int ret;

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		k_sem_reset(&sent_sem);
		if (family == NET_AF_INET) {
			send_v4_from(sock, net_ipv4_unspecified_address(), ifindex_of(n));
		} else {
			send_v6_from(sock, net_ipv6_unspecified_address(), ifindex_of(n));
		}

		assert_sent_on(n);
	}

	k_sem_reset(&sent_sem);
	ret = (family == NET_AF_INET)
		      ? sendmsg_v4_from(sock, net_ipv4_unspecified_address(), 200U)
		      : sendmsg_v6_from(sock, net_ipv6_unspecified_address(), 200U);
	zassert_equal(ret, -1, "sendmsg with an unknown interface should fail");
	zassert_equal(errno, ENODEV, "Wrong errno %d", errno);

	assert_nothing_sent();
}

ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_ifindex_connected)
{
	int sock = open_wildcard_sock(NET_AF_INET);

	(void)connect_and_find_iface(sock, NET_AF_INET);
	check_ifindex_wins_connected(sock, NET_AF_INET);
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_ifindex_connected)
{
	int sock = open_wildcard_sock(NET_AF_INET6);

	(void)connect_and_find_iface(sock, NET_AF_INET6);
	check_ifindex_wins_connected(sock, NET_AF_INET6);
}

/* A socket bound to one interface's address can't send through the other with
 * a PKTINFO index: that interface would send from an address it doesn't own.
 * Its own interface's index works, from the bound address.
 */
static void check_ifindex_bound_addr(net_sa_family_t family)
{
	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		int sock = open_bound_sock(family, n);
		int ret;

		k_sem_reset(&sent_sem);
		if (family == NET_AF_INET) {
			send_v4_from(sock, net_ipv4_unspecified_address(), ifindex_of(n));
			zassert_mem_equal(sent.src, &my_addr4[n], NET_IPV4_ADDR_SIZE,
					  "Wrong source address for interface %d", n);
		} else {
			send_v6_from(sock, net_ipv6_unspecified_address(), ifindex_of(n));
			zassert_mem_equal(sent.src, &my_addr6[n], NET_IPV6_ADDR_SIZE,
					  "Wrong source address for interface %d", n);
		}

		assert_sent_on(n);

		k_sem_reset(&sent_sem);
		ret = (family == NET_AF_INET)
			      ? sendmsg_v4_from(sock, net_ipv4_unspecified_address(),
						ifindex_of(1 - n))
			      : sendmsg_v6_from(sock, net_ipv6_unspecified_address(),
						ifindex_of(1 - n));
		zassert_equal(ret, -1, "sendmsg through the other interface should fail");
		zassert_equal(errno, EINVAL, "Wrong errno %d", errno);

		assert_nothing_sent();
		close_test_sock();
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_ifindex_bound_addr)
{
	check_ifindex_bound_addr(NET_AF_INET);
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_ifindex_bound_addr)
{
	check_ifindex_bound_addr(NET_AF_INET6);
}

static void bind_to_device(int sock, int n)
{
	struct net_ifreq ifreq = { 0 };
	int ret;

	ret = net_if_get_name(ifaces[n], ifreq.ifr_name, sizeof(ifreq.ifr_name));
	zassert_true(ret > 0, "Cannot get interface name (%d)", ret);

	ret = zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed (%d)", -errno);
}

ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_ifindex_bound_to_device)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_INTERFACE_NAME);

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		int sock = open_wildcard_sock(NET_AF_INET);

		bind_to_device(sock, n);
		check_ifindex_tied_to(sock, NET_AF_INET, n);
		close_test_sock();
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_ifindex_bound_to_device)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_INTERFACE_NAME);

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		int sock = open_wildcard_sock(NET_AF_INET6);

		bind_to_device(sock, n);
		check_ifindex_tied_to(sock, NET_AF_INET6, n);
		close_test_sock();
	}
}

/* SO_BINDTODEVICE keeps its interface, though both have the destination
 * on-link with the same prefix.
 */
ZTEST(net_socket_udp_pktinfo_src, test_v6_sendto_bound_to_device_onlink)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_NET_INTERFACE_NAME);

	for (int i = 0; i < 2 * ARRAY_SIZE(ifaces); i++) {
		int n = i % ARRAY_SIZE(ifaces);
		int sock = open_wildcard_sock(NET_AF_INET6);

		bind_to_device(sock, n);
		sendto_v6(sock, &peer_addr6);
		assert_sent_on(n);
		close_test_sock();
	}
}

/* The source address breaks only a tie: a longer on-link prefix on the other
 * interface still wins.
 */
ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_longest_prefix)
{
	for (int m = 0; m < ARRAY_SIZE(ifaces); m++) {
		int sock;

		zassert_not_null(net_if_ipv6_prefix_add(ifaces[m], &prefix6, LONG_PREFIX6_LEN,
							UINT32_MAX),
				 "Cannot add IPv6 prefix");

		sock = open_wildcard_sock(NET_AF_INET6);
		k_sem_reset(&sent_sem);
		send_v6_from(sock, &my_addr6[1 - m], 0U);
		assert_sent_on(m);
		close_test_sock();

		zassert_true(net_if_ipv6_prefix_rm(ifaces[m], &prefix6, LONG_PREFIX6_LEN),
			     "Cannot remove IPv6 prefix");
	}
}

/* A PKTINFO index wins over IP_MULTICAST_IF, as on Linux. */
ZTEST(net_socket_udp_pktinfo_src, test_v4_sendmsg_pktinfo_ifindex_over_mcast_if)
{
	int sock = open_wildcard_sock(NET_AF_INET);

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		struct net_ip_mreqn mreqn = { .imr_ifindex = ifindex_of(1 - n) };
		int ret;

		ret = zsock_setsockopt(sock, NET_IPPROTO_IP, ZSOCK_IP_MULTICAST_IF, &mreqn,
				       sizeof(mreqn));
		zassert_equal(ret, 0, "Cannot set IP_MULTICAST_IF (%d)", -errno);

		k_sem_reset(&sent_sem);
		ret = sendmsg_v4(sock, &mcast_addr4, net_ipv4_unspecified_address(),
				 ifindex_of(n));
		zassert_equal(ret, sizeof(payload) - 1, "sendmsg failed (%d)", -errno);
		zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");
		assert_sent_on(n);
	}
}

ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_ifindex_over_mcast_if)
{
	int sock = open_wildcard_sock(NET_AF_INET6);

	for (int n = 0; n < ARRAY_SIZE(ifaces); n++) {
		int mcast_ifindex = ifindex_of(1 - n);
		int ret;

		ret = zsock_setsockopt(sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_MULTICAST_IF,
				       &mcast_ifindex, sizeof(mcast_ifindex));
		zassert_equal(ret, 0, "Cannot set IPV6_MULTICAST_IF (%d)", -errno);

		k_sem_reset(&sent_sem);
		ret = sendmsg_v6(sock, &mcast_addr6, 0U, net_ipv6_unspecified_address(),
				 ifindex_of(n));
		zassert_equal(ret, sizeof(payload) - 1, "sendmsg failed (%d)", -errno);
		zassert_ok(k_sem_take(&sent_sem, WAIT_TIME), "Nothing was sent");
		assert_sent_on(n);
	}
}

/* A link-local destination's scope and the PKTINFO index must agree. */
ZTEST(net_socket_udp_pktinfo_src, test_v6_sendmsg_pktinfo_ifindex_scope_conflict)
{
	int sock = open_wildcard_sock(NET_AF_INET6);
	int ret = sendmsg_v6(sock, &ll_peer_addr6, ifindex_of(0), net_ipv6_unspecified_address(),
			     ifindex_of(1));

	zassert_equal(ret, -1, "sendmsg should fail");
	zassert_equal(errno, EINVAL, "Wrong errno %d", errno);
	assert_nothing_sent();
}

ZTEST_SUITE(net_socket_udp_pktinfo_src, NULL, setup, before, after, NULL);
