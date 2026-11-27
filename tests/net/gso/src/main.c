/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_INF);

#include <string.h>
#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/udp.h>

#include "ipv4.h"
#include "ipv6.h"
#include "net_private.h"
#include "udp_internal.h"
#include "net_stats.h"
#include "pmtu.h"

#define SRC_PORT 4242
#define DST_PORT 4243
#define GSO_SIZE 1000
#define MAX_FRAMES 16
#define FRAME_MAX (NET_ETH_MTU + sizeof(struct net_eth_hdr))
#define RX_WAIT_MS 500

enum test_iface {
	IF_SW,   /* no offload, every block split in software */
	IF_CSUM, /* TX checksum offload */
	IF_HW,   /* takes UDP blocks as they are */
	IF_COUNT,
};

struct frame {
	size_t len;
	enum net_gso_type gso_type;
	uint16_t gso_size;
	uint8_t data[FRAME_MAX];
};

static struct {
	struct frame frames[MAX_FRAMES];
	int count;
	bool overflow;
} cap;

struct eth_ctx {
	struct net_if *iface;
	uint8_t mac[6];
};

static struct eth_ctx eth_ctx[IF_COUNT];

static const struct net_in_addr my4[IF_COUNT] = {
	{ { { 192, 0, 2, 1 } } },
	{ { { 198, 51, 100, 1 } } },
	{ { { 203, 0, 113, 1 } } },
};

static const struct net_in_addr peer4[IF_COUNT] = {
	{ { { 192, 0, 2, 2 } } },
	{ { { 198, 51, 100, 2 } } },
	{ { { 203, 0, 113, 2 } } },
};

static const struct net_in6_addr my6[IF_COUNT] = {
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } } },
};

static const struct net_in6_addr peer6[IF_COUNT] = {
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 } } },
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2 } } },
};

static const struct net_in_addr mcast4 = { { { 239, 1, 2, 3 } } };

static uint8_t payload[16 * GSO_SIZE];

static int eth_send(const struct device *dev, struct net_pkt *pkt)
{
	size_t len = net_pkt_get_len(pkt);
	struct frame *f;

	ARG_UNUSED(dev);

	if (cap.count >= MAX_FRAMES) {
		cap.overflow = true;
		return 0;
	}

	f = &cap.frames[cap.count++];
	f->len = len;
	f->gso_type = net_pkt_gso_type(pkt);
	f->gso_size = net_pkt_gso_size(pkt);

	net_pkt_cursor_init(pkt);
	(void)net_pkt_read(pkt, f->data, MIN(len, sizeof(f->data)));

	return 0;
}

static enum ethernet_hw_caps eth_caps_csum(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_HW_TX_CHKSUM_OFFLOAD;
}

static enum ethernet_hw_caps eth_caps_none(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	struct eth_ctx *ctx = net_if_get_device(iface)->data;
	int idx = ctx - eth_ctx;

	ctx->iface = iface;
	ctx->mac[0] = 0x00;
	ctx->mac[1] = 0x00;
	ctx->mac[2] = 0x5e;
	ctx->mac[3] = 0x00;
	ctx->mac[4] = 0x53;
	ctx->mac[5] = idx;

	net_if_set_link_addr(iface, ctx->mac, sizeof(ctx->mac), NET_LINK_ETHERNET);

	if (idx == IF_HW) {
		net_if_set_gso_limits(iface, BIT(NET_GSO_UDP_L4), UINT16_MAX, 8);
	}

	ethernet_init(iface);
}

static int eth_dev_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static DEVICE_API(ethernet, eth_api_sw) = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_caps_none,
	.send = eth_send,
};

static DEVICE_API(ethernet, eth_api_csum) = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_caps_csum,
	.send = eth_send,
};

ETH_NET_DEVICE_INIT(eth_gso_sw, "eth_gso_sw", eth_dev_init, NULL, &eth_ctx[IF_SW], NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api_sw, NET_ETH_MTU);
ETH_NET_DEVICE_INIT(eth_gso_csum, "eth_gso_csum", eth_dev_init, NULL, &eth_ctx[IF_CSUM], NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api_csum, NET_ETH_MTU);
ETH_NET_DEVICE_INIT(eth_gso_hw, "eth_gso_hw", eth_dev_init, NULL, &eth_ctx[IF_HW], NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_api_sw, NET_ETH_MTU);

/* Sockets left open by a failed test are closed after it */
static int open_socks[8];

static int test_socket(net_sa_family_t family)
{
	int sock = zsock_socket(family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	zassert_true(sock >= 0, "socket failed (%d)", errno);

	for (int i = 0; i < ARRAY_SIZE(open_socks); i++) {
		if (open_socks[i] < 0) {
			open_socks[i] = sock;
			return sock;
		}
	}

	zassert_unreachable("too many sockets");

	return -1;
}

static void test_close(int sock)
{
	for (int i = 0; i < ARRAY_SIZE(open_socks); i++) {
		if (open_socks[i] == sock) {
			open_socks[i] = -1;
		}
	}

	zsock_close(sock);
}

static void cap_reset(void)
{
	cap.count = 0;
	cap.overflow = false;
}

static void make_addr(struct net_sockaddr_storage *ss, net_sa_family_t family,
		      const void *addr, uint16_t port)
{
	memset(ss, 0, sizeof(*ss));

	if (family == NET_AF_INET) {
		net_sin(net_sad(ss))->sin_family = NET_AF_INET;
		net_sin(net_sad(ss))->sin_port = net_htons(port);
		net_ipaddr_copy(&net_sin(net_sad(ss))->sin_addr, (const struct net_in_addr *)addr);
	} else {
		net_sin6(net_sad(ss))->sin6_family = NET_AF_INET6;
		net_sin6(net_sad(ss))->sin6_port = net_htons(port);
		net_ipaddr_copy(&net_sin6(net_sad(ss))->sin6_addr,
				(const struct net_in6_addr *)addr);
	}
}

static net_socklen_t addr_len(net_sa_family_t family)
{
	return family == NET_AF_INET ? sizeof(struct net_sockaddr_in) :
				       sizeof(struct net_sockaddr_in6);
}

/* A UDP socket bound to the address of iface, port SRC_PORT */
static int udp_socket(net_sa_family_t family, enum test_iface iface)
{
	struct net_sockaddr_storage local = { 0 };
	int sock;

	sock = test_socket(family);

	make_addr(&local, family, family == NET_AF_INET ? (const void *)&my4[iface] :
							  (const void *)&my6[iface],
		  SRC_PORT);
	zassert_ok(zsock_bind(sock, net_sad(&local), addr_len(family)), "bind failed (%d)",
		   errno);

	return sock;
}

static void peer_addr(struct net_sockaddr_storage *ss, net_sa_family_t family,
		      enum test_iface iface)
{
	make_addr(ss, family, family == NET_AF_INET ? (const void *)&peer4[iface] :
						      (const void *)&peer6[iface],
		  DST_PORT);
}

static int set_segment(int sock, int size)
{
	return zsock_setsockopt(sock, NET_IPPROTO_UDP, ZSOCK_UDP_SEGMENT, &size, sizeof(size));
}

/* sendmsg() with a UDP_SEGMENT control message */
static ssize_t send_with_cmsg(int sock, const struct net_sockaddr_storage *dst,
			      net_sa_family_t family, const void *buf, size_t len,
			      uint16_t gso_size, size_t cmsg_data_len)
{
	uint8_t control[NET_CMSG_SPACE(sizeof(uint32_t))] = { 0 };
	struct net_iovec iov = {
		.iov_base = (void *)buf,
		.iov_len = len,
	};
	struct net_msghdr msg = {
		.msg_name = (void *)dst,
		.msg_namelen = addr_len(family),
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = control,
		.msg_controllen = NET_CMSG_SPACE(cmsg_data_len),
	};
	struct net_cmsghdr *cmsg = NET_CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_level = NET_IPPROTO_UDP;
	cmsg->cmsg_type = ZSOCK_UDP_SEGMENT;
	cmsg->cmsg_len = NET_CMSG_LEN(cmsg_data_len);
	memcpy(NET_CMSG_DATA(cmsg), &gso_size, sizeof(gso_size));

	return zsock_sendmsg(sock, &msg, 0);
}

static struct net_if *test_iface(enum test_iface iface)
{
	return eth_ctx[iface].iface;
}

static net_stats_t ip_sent(net_sa_family_t family, enum test_iface iface)
{
	return family == NET_AF_INET ? GET_STAT(test_iface(iface), ipv4.sent) :
				       GET_STAT(test_iface(iface), ipv6.sent);
}

static net_stats_t udp_sent(enum test_iface iface)
{
	return GET_STAT(test_iface(iface), udp.sent);
}

/* Lengths of the datagrams a send of len bytes in gso_size pieces gives */
static int expected_segments(size_t len, size_t gso_size)
{
	return DIV_ROUND_UP(len, gso_size);
}

static size_t hdr_len(net_sa_family_t family)
{
	return sizeof(struct net_eth_hdr) +
	       (family == NET_AF_INET ? NET_IPV4H_LEN : NET_IPV6H_LEN) + NET_UDPH_LEN;
}

static struct frame ref_frames[MAX_FRAMES];

/* The frames of one UDP_SEGMENT send must equal those of the separate
 * sendto() calls it stands for, byte for byte.
 */
static void check_same_as_separate_sends(net_sa_family_t family, enum test_iface iface,
					 size_t len, bool use_cmsg)
{
	struct net_sockaddr_storage dst = { 0 };
	net_stats_t ip_before, udp_before;
	int segs = expected_segments(len, GSO_SIZE);
	int ref_count;
	int sock;

	peer_addr(&dst, family, iface);
	sock = udp_socket(family, iface);

	/* The first IPv4 datagram to a peer has DF set until its path MTU
	 * entry exists, so create the entry before comparing.
	 */
	zassert_equal(zsock_sendto(sock, payload, 1, 0, net_sad(&dst), addr_len(family)), 1,
		      "sendto failed (%d)", errno);

	cap_reset();
	ip_before = ip_sent(family, iface);
	udp_before = udp_sent(iface);

	for (size_t off = 0; off < len; off += GSO_SIZE) {
		size_t n = MIN(GSO_SIZE, len - off);

		zassert_equal(zsock_sendto(sock, payload + off, n, 0, net_sad(&dst),
					   addr_len(family)),
			      n, "sendto failed (%d)", errno);
	}

	zassert_false(cap.overflow);
	zassert_equal(cap.count, segs, "%d separate frames, expected %d", cap.count, segs);
	zexpect_equal(ip_sent(family, iface) - ip_before, segs);
	zexpect_equal(udp_sent(iface) - udp_before, segs);

	ref_count = cap.count;
	memcpy(ref_frames, cap.frames, sizeof(ref_frames));

	cap_reset();
	ip_before = ip_sent(family, iface);
	udp_before = udp_sent(iface);

	if (use_cmsg) {
		zassert_equal(send_with_cmsg(sock, &dst, family, payload, len, GSO_SIZE,
					     sizeof(uint16_t)),
			      len, "sendmsg failed (%d)", errno);
	} else {
		zassert_ok(set_segment(sock, GSO_SIZE), "setsockopt failed (%d)", errno);
		zassert_equal(zsock_sendto(sock, payload, len, 0, net_sad(&dst),
					   addr_len(family)),
			      len, "sendto failed (%d)", errno);
	}

	zassert_false(cap.overflow);
	zassert_equal(cap.count, ref_count, "%d segments, expected %d", cap.count, ref_count);
	zexpect_equal(ip_sent(family, iface) - ip_before, segs, "IP sent not per datagram");
	zexpect_equal(udp_sent(iface) - udp_before, segs, "UDP sent not per datagram");

	for (int i = 0; i < ref_count; i++) {
		zexpect_equal(cap.frames[i].gso_type, NET_GSO_NONE, "segment %d is a block", i);
		zassert_equal(cap.frames[i].len, ref_frames[i].len, "segment %d length %zu vs %zu",
			      i, cap.frames[i].len, ref_frames[i].len);
		zexpect_mem_equal(cap.frames[i].data, ref_frames[i].data, ref_frames[i].len,
				  "segment %d differs", i);
	}

	test_close(sock);
}

ZTEST(net_gso, test_v4_sw_short_last)
{
	check_same_as_separate_sends(NET_AF_INET, IF_SW, 3 * GSO_SIZE + 500, false);
}

ZTEST(net_gso, test_v4_sw_exact_multiple)
{
	check_same_as_separate_sends(NET_AF_INET, IF_SW, 4 * GSO_SIZE, false);
}

ZTEST(net_gso, test_v6_sw_short_last)
{
	check_same_as_separate_sends(NET_AF_INET6, IF_SW, 3 * GSO_SIZE + 1, false);
}

ZTEST(net_gso, test_v6_sw_exact_multiple)
{
	check_same_as_separate_sends(NET_AF_INET6, IF_SW, 2 * GSO_SIZE, false);
}

ZTEST(net_gso, test_v4_csum_offload)
{
	check_same_as_separate_sends(NET_AF_INET, IF_CSUM, 3 * GSO_SIZE + 77, false);
}

ZTEST(net_gso, test_v6_csum_offload)
{
	check_same_as_separate_sends(NET_AF_INET6, IF_CSUM, 3 * GSO_SIZE + 77, false);
}

ZTEST(net_gso, test_v4_cmsg)
{
	check_same_as_separate_sends(NET_AF_INET, IF_SW, 2 * GSO_SIZE + 10, true);
}

ZTEST(net_gso, test_v6_cmsg)
{
	check_same_as_separate_sends(NET_AF_INET6, IF_SW, 2 * GSO_SIZE + 10, true);
}

/* Over the HW interface limit of 8 segments, the block is split in software */
ZTEST(net_gso, test_v4_hw_over_limit)
{
	check_same_as_separate_sends(NET_AF_INET, IF_HW, 9 * GSO_SIZE, false);
}

ZTEST(net_gso, test_v4_hw_block)
{
	struct net_sockaddr_storage dst = { 0 };
	size_t len = 5 * GSO_SIZE + 123;
	struct net_ipv4_hdr *ip;
	struct net_udp_hdr *udp;
	net_stats_t ip_before;
	int sock;

	peer_addr(&dst, NET_AF_INET, IF_HW);
	sock = udp_socket(NET_AF_INET, IF_HW);
	zassert_ok(set_segment(sock, GSO_SIZE));

	cap_reset();
	ip_before = ip_sent(NET_AF_INET, IF_HW);

	zassert_equal(zsock_sendto(sock, payload, len, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      len, "sendto failed (%d)", errno);

	zassert_equal(cap.count, 1, "%d packets, expected one block", cap.count);
	zexpect_equal(cap.frames[0].gso_type, NET_GSO_UDP_L4);
	zexpect_equal(cap.frames[0].gso_size, GSO_SIZE);
	zexpect_equal(cap.frames[0].len, hdr_len(NET_AF_INET) + len);
	zexpect_equal(ip_sent(NET_AF_INET, IF_HW) - ip_before, expected_segments(len, GSO_SIZE));

	ip = (struct net_ipv4_hdr *)(cap.frames[0].data + sizeof(struct net_eth_hdr));
	udp = (struct net_udp_hdr *)((uint8_t *)ip + NET_IPV4H_LEN);

	zexpect_equal(net_ntohs(ip->len), NET_IPV4H_LEN + NET_UDPH_LEN + len);
	zexpect_equal(net_ntohs(udp->len), NET_UDPH_LEN + len);
	zexpect_mem_equal((uint8_t *)udp + NET_UDPH_LEN, payload,
			  FRAME_MAX - hdr_len(NET_AF_INET));

	test_close(sock);
}

ZTEST(net_gso, test_single_datagram_not_marked)
{
	struct net_sockaddr_storage dst = { 0 };
	int sock;

	peer_addr(&dst, NET_AF_INET, IF_HW);
	sock = udp_socket(NET_AF_INET, IF_HW);
	zassert_ok(set_segment(sock, GSO_SIZE));

	for (size_t len = GSO_SIZE - 1; len <= GSO_SIZE; len++) {
		cap_reset();

		zassert_equal(zsock_sendto(sock, payload, len, 0, net_sad(&dst),
					   addr_len(NET_AF_INET)),
			      len, "sendto failed (%d)", errno);

		zassert_equal(cap.count, 1);
		zexpect_equal(cap.frames[0].gso_type, NET_GSO_NONE, "len %zu sent as a block",
			      len);
		zexpect_equal(cap.frames[0].len, hdr_len(NET_AF_INET) + len);
	}

	test_close(sock);
}

ZTEST(net_gso, test_sockopt)
{
	net_socklen_t optlen = sizeof(int);
	int sock = udp_socket(NET_AF_INET, IF_SW);
	int val = -1;

	zassert_ok(zsock_getsockopt(sock, NET_IPPROTO_UDP, ZSOCK_UDP_SEGMENT, &val, &optlen));
	zexpect_equal(val, 0);

	zassert_ok(set_segment(sock, 1200));
	zassert_ok(zsock_getsockopt(sock, NET_IPPROTO_UDP, ZSOCK_UDP_SEGMENT, &val, &optlen));
	zexpect_equal(val, 1200);
	zexpect_equal(optlen, sizeof(int));

	zexpect_equal(set_segment(sock, -1), -1);
	zexpect_equal(errno, EINVAL);
	zexpect_equal(set_segment(sock, UINT16_MAX + 1), -1);
	zexpect_equal(errno, EINVAL);

	zassert_ok(set_segment(sock, 0));
	zassert_ok(zsock_getsockopt(sock, NET_IPPROTO_UDP, ZSOCK_UDP_SEGMENT, &val, &optlen));
	zexpect_equal(val, 0);

	test_close(sock);
}

/* The control message size applies over the socket option */
ZTEST(net_gso, test_cmsg_overrides_sockopt)
{
	struct net_sockaddr_storage dst = { 0 };
	size_t len = 4 * 500;
	int sock;

	peer_addr(&dst, NET_AF_INET, IF_SW);
	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, GSO_SIZE));

	cap_reset();
	zassert_equal(send_with_cmsg(sock, &dst, NET_AF_INET, payload, len, 500,
				     sizeof(uint16_t)),
		      len, "sendmsg failed (%d)", errno);

	zassert_equal(cap.count, 4);
	for (int i = 0; i < cap.count; i++) {
		zexpect_equal(cap.frames[i].len, hdr_len(NET_AF_INET) + 500);
	}

	test_close(sock);
}

ZTEST(net_gso, test_invalid_sends)
{
	struct net_sockaddr_storage dst = { 0 };
	size_t max_gso = NET_ETH_MTU - NET_IPV4H_LEN - NET_UDPH_LEN;
	int sock;

	peer_addr(&dst, NET_AF_INET, IF_SW);
	sock = udp_socket(NET_AF_INET, IF_SW);

	/* A control message of the wrong size */
	zexpect_equal(send_with_cmsg(sock, &dst, NET_AF_INET, payload, 2 * GSO_SIZE, GSO_SIZE,
				     sizeof(uint32_t)),
		      -1);
	zexpect_equal(errno, EINVAL);

	/* A datagram larger than the MTU, even if only one would be sent */
	zassert_ok(set_segment(sock, max_gso + 1));
	zexpect_equal(zsock_sendto(sock, payload, 2 * GSO_SIZE, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EINVAL);
	zexpect_equal(zsock_sendto(sock, payload, 10, 0, net_sad(&dst), addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EINVAL);

	/* The largest datagram that fits is fine */
	cap_reset();
	zassert_ok(set_segment(sock, max_gso));
	zexpect_equal(zsock_sendto(sock, payload, 2 * max_gso, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      2 * max_gso);
	zexpect_equal(cap.count, 2);
	zexpect_equal(cap.frames[0].len, sizeof(struct net_eth_hdr) + NET_ETH_MTU);

	/* One datagram more than allowed */
	zassert_ok(set_segment(sock, 10));
	zexpect_equal(zsock_sendto(sock, payload, 10 * CONFIG_NET_UDP_GSO_MAX_SEGMENTS + 1, 0,
				   net_sad(&dst), addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EINVAL);

	test_close(sock);
}

ZTEST(net_gso, test_too_large)
{
	static uint8_t big[UINT16_MAX];
	struct net_sockaddr_storage dst = { 0 };
	size_t max_len = UINT16_MAX - NET_IPV4H_LEN - NET_UDPH_LEN;
	int sock;

	peer_addr(&dst, NET_AF_INET, IF_SW);
	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, 1400));

	zexpect_equal(zsock_sendto(sock, big, max_len + 1, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EMSGSIZE);

	test_close(sock);
}

ZTEST(net_gso, test_pmtu)
{
	/* A peer of its own, so that its path MTU affects no other test */
	struct net_in_addr peer = { { { 192, 0, 2, 3 } } };
	struct net_sockaddr_storage dst = { 0 };
	int sock;

	make_addr(&dst, NET_AF_INET, &peer, DST_PORT);
	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, GSO_SIZE));

	/* The path MTU dropped after the option was set */
	zassert_true(net_pmtu_update_mtu(net_sad(&dst), 900) >= 0);

	zexpect_equal(zsock_sendto(sock, payload, 2 * GSO_SIZE, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EINVAL);

	zassert_ok(set_segment(sock, 900 - NET_IPV4H_LEN - NET_UDPH_LEN));
	cap_reset();
	zexpect_equal(zsock_sendto(sock, payload, 2 * GSO_SIZE, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      2 * GSO_SIZE);
	zexpect_equal(cap.count, 3);
	zexpect_equal(cap.frames[0].len, sizeof(struct net_eth_hdr) + 900);

	test_close(sock);
}

static int rx_socket(net_sa_family_t family)
{
	struct net_sockaddr_storage local = { 0 };
	struct zsock_timeval tv = {
		.tv_sec = 0,
		.tv_usec = RX_WAIT_MS * USEC_PER_MSEC,
	};
	int sock;

	sock = test_socket(family);

	net_sad(&local)->sa_family = family;
	if (family == NET_AF_INET) {
		net_sin(net_sad(&local))->sin_port = net_htons(DST_PORT);
	} else {
		net_sin6(net_sad(&local))->sin6_port = net_htons(DST_PORT);
	}

	zassert_ok(zsock_bind(sock, net_sad(&local), addr_len(family)));
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv)));

	return sock;
}

/* A local receiver gets the datagrams, never the block */
static void check_local_delivery(net_sa_family_t family, const void *dst_ip,
				 enum test_iface src_iface, size_t gso_size, size_t len)
{
	static uint8_t rx_buf[2 * GSO_SIZE];
	struct net_sockaddr_storage dst = { 0 };
	int segs = expected_segments(len, gso_size);
	int rx = rx_socket(family);
	int tx = udp_socket(family, src_iface);

	make_addr(&dst, family, dst_ip, DST_PORT);
	zassert_ok(set_segment(tx, gso_size));

	zassert_equal(zsock_sendto(tx, payload, len, 0, net_sad(&dst), addr_len(family)), len,
		      "sendto failed (%d)", errno);

	for (int i = 0; i < segs; i++) {
		size_t expected = MIN(gso_size, len - i * gso_size);
		ssize_t got = zsock_recv(rx, rx_buf, sizeof(rx_buf), 0);

		zassert_equal(got, expected, "datagram %d: %zd bytes, expected %zu (%d)", i, got,
			      expected, errno);
		zexpect_mem_equal(rx_buf, payload + i * gso_size, expected, "datagram %d", i);
	}

	zexpect_equal(zsock_recv(rx, rx_buf, sizeof(rx_buf), ZSOCK_MSG_DONTWAIT), -1,
		      "more datagrams than expected");

	test_close(tx);
	test_close(rx);
}

/* To an address of the sending interface, short cut back to RX */
ZTEST(net_gso, test_v4_self_addressed)
{
	check_local_delivery(NET_AF_INET, &my4[IF_SW], IF_SW, GSO_SIZE, 3 * GSO_SIZE + 5);
}

ZTEST(net_gso, test_v6_self_addressed)
{
	check_local_delivery(NET_AF_INET6, &my6[IF_SW], IF_SW, GSO_SIZE, 3 * GSO_SIZE + 5);
}

/* Through the loopback interface and its driver */
ZTEST(net_gso, test_v4_loopback)
{
	struct net_in_addr lo = { { { 127, 0, 0, 1 } } };
	struct net_sockaddr_storage local = { 0 };
	struct net_sockaddr_storage dst = { 0 };
	int segs = 4;
	int rx = rx_socket(NET_AF_INET);
	int tx;

	tx = test_socket(NET_AF_INET);
	make_addr(&local, NET_AF_INET, &lo, SRC_PORT);
	zassert_ok(zsock_bind(tx, net_sad(&local), addr_len(NET_AF_INET)));
	zassert_ok(set_segment(tx, GSO_SIZE));
	make_addr(&dst, NET_AF_INET, &lo, DST_PORT);

	zassert_equal(zsock_sendto(tx, payload, segs * GSO_SIZE, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      segs * GSO_SIZE, "sendto failed (%d)", errno);

	for (int i = 0; i < segs; i++) {
		static uint8_t rx_buf[2 * GSO_SIZE];

		zassert_equal(zsock_recv(rx, rx_buf, sizeof(rx_buf), 0), GSO_SIZE,
			      "datagram %d (%d)", i, errno);
		zexpect_mem_equal(rx_buf, payload + i * GSO_SIZE, GSO_SIZE);
	}

	test_close(tx);
	test_close(rx);
}

/* Multicast loop gets the datagrams, the wire gets the same */
ZTEST(net_gso, test_v4_multicast_loop)
{
	size_t len = 2 * GSO_SIZE + 300;

	cap_reset();
	check_local_delivery(NET_AF_INET, &mcast4, IF_SW, GSO_SIZE, len);

	zassert_equal(cap.count, expected_segments(len, GSO_SIZE));
	for (int i = 0; i < cap.count; i++) {
		zexpect_equal(cap.frames[i].gso_type, NET_GSO_NONE);
		zexpect_equal(cap.frames[i].len,
			      hdr_len(NET_AF_INET) + MIN(GSO_SIZE, len - i * GSO_SIZE));
	}
}

/* Segment allocation failure in the TX path: the datagrams are counted as
 * dropped and no packet or buffer is leaked.
 */
ZTEST(net_gso, test_alloc_failure)
{
	struct k_mem_slab *rx_slab, *tx_slab;
	struct net_buf_pool *rx_data, *tx_data;
	struct net_pkt *held[CONFIG_NET_PKT_TX_COUNT];
	struct net_sockaddr_storage dst = { 0 };
	uint32_t free_pkts, free_bufs;
	size_t len = 3 * GSO_SIZE;
	int n_held = 0;
	net_stats_t drop;
	int sock;

	net_pkt_get_info(&rx_slab, &tx_slab, &rx_data, &tx_data);

	peer_addr(&dst, NET_AF_INET, IF_SW);
	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, GSO_SIZE));

	free_pkts = k_mem_slab_num_free_get(tx_slab);
	free_bufs = net_buf_get_available(tx_data);

	/* Leave only the packet the block itself needs */
	while (k_mem_slab_num_free_get(tx_slab) > 1) {
		held[n_held] = net_pkt_alloc(K_NO_WAIT);
		zassert_not_null(held[n_held]);
		n_held++;
	}

	cap_reset();
	drop = GET_STAT(test_iface(IF_SW), ipv4.drop);

	zassert_equal(zsock_sendto(sock, payload, len, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      len, "sendto failed (%d)", errno);

	zexpect_equal(cap.count, 0);
	zexpect_equal(GET_STAT(test_iface(IF_SW), ipv4.drop) - drop,
		      expected_segments(len, GSO_SIZE));

	while (n_held > 0) {
		net_pkt_unref(held[--n_held]);
	}

	zexpect_equal(k_mem_slab_num_free_get(tx_slab), free_pkts, "TX packet leaked");
	zexpect_equal(net_buf_get_available(tx_data), free_bufs, "TX buffer leaked");

	test_close(sock);
}

/* Buffers a block of len bytes in gso_size datagrams takes, header included */
static size_t block_bufs(size_t len, size_t gso_size)
{
#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
	size_t bufs = 1U;

	for (size_t off = 0U; off < len; off += gso_size) {
		bufs += DIV_ROUND_UP(MIN(gso_size, len - off), CONFIG_NET_BUF_DATA_SIZE);
	}

	return bufs;
#else
	return 1U + DIV_ROUND_UP(len, gso_size);
#endif
}

/* The software split moves the payload buffers of the block to the
 * datagrams: with only a few buffers free beyond those of the block, all
 * datagrams still go out. Copying the payload would need a buffer chain per
 * datagram.
 */
ZTEST(net_gso, test_zero_copy)
{
	struct k_mem_slab *rx_slab, *tx_slab;
	struct net_buf_pool *rx_data, *tx_data;
	struct net_buf *held[CONFIG_NET_BUF_TX_COUNT];
	struct net_sockaddr_storage dst = { 0 };
	size_t len = 4 * GSO_SIZE + 100;
	size_t spare = block_bufs(len, GSO_SIZE) + 4U;
	int n_held = 0;
	int sock;

	net_pkt_get_info(&rx_slab, &tx_slab, &rx_data, &tx_data);

	peer_addr(&dst, NET_AF_INET, IF_SW);
	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, GSO_SIZE));

	while (net_buf_get_available(tx_data) > spare) {
		held[n_held] = net_pkt_get_reserve_tx_data(1, K_NO_WAIT);
		zassert_not_null(held[n_held]);
		n_held++;
	}

	cap_reset();
	zexpect_equal(zsock_sendto(sock, payload, len, 0, net_sad(&dst), addr_len(NET_AF_INET)),
		      len, "sendto failed (%d)", errno);
	zexpect_equal(cap.count, expected_segments(len, GSO_SIZE), "%d datagrams sent",
		      cap.count);

	while (n_held > 0) {
		net_buf_unref(held[--n_held]);
	}

	test_close(sock);
}

static void collect_segment(struct net_pkt *seg, void *user_data)
{
	ARG_UNUSED(user_data);

	(void)eth_send(NULL, seg);
	net_pkt_unref(seg);
}

/* A block whose buffers do not end where its datagrams do: the payload of
 * the first datagram is in a buffer that also holds part of the second, so
 * it must be copied, not moved.
 */
ZTEST(net_gso, test_unaligned_block)
{
	size_t gso_size = 100;
	size_t len = 2 * gso_size;
	size_t ip_len = NET_IPV4H_LEN + NET_UDPH_LEN;
	struct net_pkt *blk;

	blk = net_pkt_alloc_on_iface(test_iface(IF_SW), K_NO_WAIT);
	zassert_not_null(blk);
	net_pkt_set_family(blk, NET_AF_INET);

	/* The headers in a buffer of their own, the payload in one chain */
	zassert_ok(net_pkt_alloc_buffer_exact(blk, ip_len, K_NO_WAIT));
	zassert_ok(net_pkt_alloc_buffer_exact(blk, len, K_NO_WAIT));
	zassert_true(blk->buffer->frags->size > gso_size, "payload buffer too small");

	zassert_ok(net_ipv4_create(blk, &my4[IF_SW], &peer4[IF_SW]));
	zassert_ok(net_udp_create(blk, net_htons(SRC_PORT), net_htons(DST_PORT)));
	zassert_ok(net_pkt_write(blk, payload, len));
	net_pkt_cursor_init(blk);
	zassert_ok(net_ipv4_finalize(blk, NET_IPPROTO_UDP));
	net_pkt_set_gso(blk, NET_GSO_UDP_L4, gso_size);

	cap_reset();
	zassert_ok(net_gso_segment(blk, false, K_NO_WAIT, collect_segment, NULL));

	zassert_equal(cap.count, 2);
	for (int i = 0; i < cap.count; i++) {
		struct net_udp_hdr *udp =
			(struct net_udp_hdr *)(cap.frames[i].data + NET_IPV4H_LEN);

		zexpect_equal(cap.frames[i].len, ip_len + gso_size, "datagram %d", i);
		zexpect_equal(net_ntohs(udp->len), NET_UDPH_LEN + gso_size, "datagram %d", i);
		zexpect_mem_equal(cap.frames[i].data + ip_len, payload + i * gso_size, gso_size,
				  "datagram %d", i);
	}
}

/* Options that cannot be combined with UDP_SEGMENT */
ZTEST(net_gso, test_rejected_options)
{
	struct net_sockaddr_storage dst = { 0 };
	int one = 1;
	int sock;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_UDP_OPTIONS);

	peer_addr(&dst, NET_AF_INET, IF_SW);

	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, GSO_SIZE));
	zassert_ok(zsock_setsockopt(sock, NET_IPPROTO_UDP, ZSOCK_UDP_OPT, &one, sizeof(one)));
	zexpect_equal(zsock_sendto(sock, payload, 2 * GSO_SIZE, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EINVAL);
	test_close(sock);

#if defined(CONFIG_NET_CONTEXT_TIMESTAMPING)
	uint8_t flags = ZSOCK_SOF_TIMESTAMPING_TX_HARDWARE;

	sock = udp_socket(NET_AF_INET, IF_SW);
	zassert_ok(set_segment(sock, GSO_SIZE));
	zassert_ok(zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING, &flags,
				    sizeof(flags)));
	zexpect_equal(zsock_sendto(sock, payload, 2 * GSO_SIZE, 0, net_sad(&dst),
				   addr_len(NET_AF_INET)),
		      -1);
	zexpect_equal(errno, EINVAL);
	test_close(sock);
#endif

	/* A UDP_SEGMENT control message is not taken for a UDP option */
	sock = udp_socket(NET_AF_INET, IF_SW);
	cap_reset();
	zexpect_equal(send_with_cmsg(sock, &dst, NET_AF_INET, payload, 2 * GSO_SIZE, GSO_SIZE,
				     sizeof(uint16_t)),
		      2 * GSO_SIZE, "sendmsg failed (%d)", errno);
	zexpect_equal(cap.count, 2);
	test_close(sock);
}

/* Neighbor discovery is off, so the IPv6 peers need a cache entry */
static void add_neighbor(struct net_if *iface, const struct net_in6_addr *addr)
{
	struct net_linkaddr lladdr = {
		.addr = { 0x02, 0x00, 0x5e, 0x00, 0x53, 0x99 },
		.len = 6U,
		.type = NET_LINK_ETHERNET,
	};

	zassert_not_null(net_ipv6_nbr_add(iface, addr, &lladdr, false,
					  NET_IPV6_NBR_STATE_REACHABLE));
}

static void *gso_setup(void)
{
	for (int i = 0; i < IF_COUNT; i++) {
		struct net_if *iface = eth_ctx[i].iface;
		struct net_in_addr netmask = { { { 255, 255, 255, 0 } } };

		zassert_not_null(iface, "interface %d missing", i);

		zassert_not_null(net_if_ipv4_addr_add(iface, (struct net_in_addr *)&my4[i],
						      NET_ADDR_MANUAL, 0));
		zassert_true(net_if_ipv4_set_netmask_by_addr(iface, &my4[i], &netmask));
		zassert_not_null(net_if_ipv6_addr_add(iface, (struct net_in6_addr *)&my6[i],
						      NET_ADDR_MANUAL, 0));
		zassert_not_null(net_if_ipv6_prefix_add(iface, (struct net_in6_addr *)&my6[i],
							64, NET_IPV6_ND_INFINITE_LIFETIME));
		net_if_up(iface);
		add_neighbor(iface, &peer6[i]);
	}

	struct net_if_mcast_addr *maddr;

	maddr = net_if_ipv4_maddr_add(eth_ctx[IF_SW].iface, &mcast4);
	zassert_not_null(maddr);
	net_if_ipv4_maddr_join(eth_ctx[IF_SW].iface, maddr);

	for (size_t i = 0; i < sizeof(payload); i++) {
		payload[i] = (uint8_t)(i * 7 + i / 251);
	}

	for (int i = 0; i < ARRAY_SIZE(open_socks); i++) {
		open_socks[i] = -1;
	}

	return NULL;
}

static void gso_after(void *fixture)
{
	ARG_UNUSED(fixture);

	for (int i = 0; i < ARRAY_SIZE(open_socks); i++) {
		if (open_socks[i] >= 0) {
			zsock_close(open_socks[i]);
			open_socks[i] = -1;
		}
	}
}

ZTEST_SUITE(net_gso, NULL, gso_setup, NULL, gso_after, NULL);
