/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#include <zephyr/ztest.h>

/* This test suite verify that RAW IP sockets behave according to well known behaviors.
 * Note, that this is not well standardized and rely on behaviors known from Linux or FreeBSD.
 *
 * Sending data (TX):
 *
 *   * (AF_INET/6, SOCK_RAW, 0) - The IP header needs to be supplied by the user in the data:
 *     - test_raw_v4_sock_send_proto_wildcard
 *     - test_raw_v6_sock_send_proto_wildcard
 *     - test_raw_v4_sock_sendto
 *     - test_raw_v6_sock_sendto
 *     - test_raw_v4_sock_sendmsg
 *     - test_raw_v6_sock_sendmsg
 *
 *   * (AF_INET/6, SOCK_RAW, <protocol>) - Construct IP header according to the protocol number if
 *     `IP_HDRINCL` socket option is not set. Otherwise, IP header should be provided by the user.
 *     Currently `IP_HDRINCL` is always considered set:
 *     - test_raw_v4_sock_send_proto_match
 *     - test_raw_v4_sock_send_proto_mismatch
 *     - test_raw_v6_sock_send_proto_match
 *     - test_raw_v6_sock_send_proto_mismatch
 *
 *   * (AF_INET/6, SOCK_RAW, IPPROTO_RAW) - The IP header needs to be supplied by the user in the
 *     data:
 *     - test_raw_v4_sock_send_proto_ipproto_raw
 *     - test_raw_v6_sock_send_proto_ipproto_raw
 *
 * Receiving data (RX)
 *
 *   *  (AF_INET/6, SOCK_RAW, 0) - 0 value is also `IPPROTO_IP` which is "wildcard" value. The
 *      packet is received for any IP protocol. It works in similar way as in FreeBSD:
 *     - test_raw_v4_sock_recv_proto_wildcard
 *     - test_raw_v6_sock_recv_proto_wildcard
 *     - test_raw_v4_sock_recvfrom
 *     - test_raw_v6_sock_recvfrom
 *     - test_raw_v4_sock_recvmsg
 *     - test_raw_v6_sock_recvmsg
 *
 *   *  (AF_INET/6, SOCK_RAW, <protocol>) - All packets matching the protocol number specified for
 *      the raw socket are passed to this socket. http://www.iana.org/assignments/protocol-numbers:
 *     - test_raw_v4_sock_recv_proto_match
 *     - test_raw_v6_sock_recv_proto_match
 *
 *   *  (AF_INET/6, SOCK_RAW, IPPROTO_RAW) - Receiving of all IP protocols via IPPROTO_RAW is not
 *      possible using raw sockets. https://man7.org/linux/man-pages/man7/raw.7.html:
 *     - test_raw_v4_sock_recv_proto_ipproto_raw
 *     - test_raw_v6_sock_recv_proto_ipproto_raw
 *
 * See https://sock-raw.org/papers/sock_raw
 */

static int test_iface_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);

	struct net_pkt *recv_pkt;
	int ret;

	recv_pkt = net_pkt_rx_clone(pkt, K_NO_WAIT);
	if (recv_pkt == NULL) {
		printk("Failed to clone the packet\n");
		return -ENOMEM;
	}

	ret = net_recv_data(net_pkt_iface(recv_pkt), recv_pkt);
	if (ret < 0) {
		net_pkt_unref(recv_pkt);
		return ret;
	}

	return 0;
}

static void test_iface_init(struct net_if *iface)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	uint8_t mac_addr[sizeof(struct net_eth_addr)] = {
		0x00, 0x00, 0x5e, 0x00, 0x53, 0x00
	};

	mac_addr[5] = sys_rand8_get();

	net_if_set_link_addr(iface, mac_addr, sizeof(mac_addr),
			     NET_LINK_ETHERNET);
}

static struct dummy_api test_iface_if_api = {
	.iface_api.init = test_iface_init,
	.send = test_iface_send,
};

NET_DEVICE_INIT(test_iface_1, "test_iface_1", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &test_iface_if_api,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

#define TEST_PORT_OWN 4242
#define TEST_PORT_DST 4243

static struct net_if *test_iface = NET_IF_GET(test_iface_1, 0);
static struct in_addr test_ipv4_1 = { { { 192, 0, 2, 1 } } };
static struct in_addr test_ipv4_2 = { { { 192, 0, 2, 2 } } };
static struct in6_addr test_ipv6_1 = { { {
	0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1
} } };
static struct in6_addr test_ipv6_2 = { { {
	0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2
} } };

static int raw_sock = -1;
static int raw_sock_2 = -1;
static int udp_sock = -1;
static int udp_sock_2 = -1;
static int packet_sock = -1;
static struct sockaddr src_addr;
static struct sockaddr dst_addr;
static socklen_t addrlen;

static uint8_t test_payload[] = "test_payload";
static uint8_t rx_buf[128];
static uint8_t tx_buf[128];

static void prepare_raw_sock_common(int *sock, sa_family_t family,
				    enum net_ip_protocol proto)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	*sock = zsock_socket(family, SOCK_RAW, proto);
	zassert_true(*sock >= 0, "Failed to create RAW socket(%d)", errno);

	ret = zsock_setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	zassert_ok(ret, "setsockopt failed (%d)", errno);
}

static void prepare_raw_sock(sa_family_t family, enum net_ip_protocol proto)
{
	prepare_raw_sock_common(&raw_sock, family, proto);
}

static void prepare_raw_sock_2(sa_family_t family, enum net_ip_protocol proto)
{
	prepare_raw_sock_common(&raw_sock_2, family, proto);
}

static void prepare_udp_sock_common(int *sock, sa_family_t family,
				    struct sockaddr *bind_addr)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	*sock = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "Failed to create UDP socket (%d)", errno);

	ret = zsock_bind(*sock, bind_addr, addrlen);
	zassert_ok(ret, "Binding UDP socket failed (%d)", errno);

	ret = zsock_setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	zassert_ok(ret, "setsockopt failed (%d)", errno);
}

static void prepare_udp_sock(sa_family_t family)
{
	prepare_udp_sock_common(&udp_sock, family, &src_addr);
}

static void prepare_udp_sock_2(sa_family_t family)
{
	prepare_udp_sock_common(&udp_sock_2, family, &dst_addr);
}

static void prepare_packet_sock(uint16_t proto)
{
	struct sockaddr_ll addr = { 0 };
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = net_if_get_by_iface(test_iface);
	addr.sll_family = AF_PACKET;

	packet_sock = zsock_socket(AF_PACKET, SOCK_DGRAM, htons(proto));
	zassert_true(packet_sock >= 0, "Failed to create packet socket (%d)", errno);

	ret = zsock_bind(packet_sock, (struct sockaddr *)&addr, sizeof(addr));
	zassert_ok(ret, "Binding packet socket failed (%d)", errno);

	ret = zsock_setsockopt(packet_sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	zassert_ok(ret, "setsockopt failed (%d)", errno);
}

static void prepare_addr(sa_family_t family)
{
	src_addr.sa_family = family;
	dst_addr.sa_family = family;

	if (family == AF_INET) {
		net_sin(&src_addr)->sin_addr = test_ipv4_1;
		net_sin(&src_addr)->sin_port = htons(TEST_PORT_OWN);
		net_sin(&dst_addr)->sin_addr = test_ipv4_2;
		net_sin(&dst_addr)->sin_port = htons(TEST_PORT_DST);
		addrlen = sizeof(struct sockaddr_in);
	} else {
		net_sin6(&src_addr)->sin6_addr = test_ipv6_1;
		net_sin6(&src_addr)->sin6_port = htons(TEST_PORT_OWN);
		net_sin6(&dst_addr)->sin6_addr = test_ipv6_2;
		net_sin6(&dst_addr)->sin6_port = htons(TEST_PORT_DST);
		addrlen = sizeof(struct sockaddr_in6);
	}
}

static void prepare_raw_and_udp_sock_and_addr(sa_family_t family,
					      enum net_ip_protocol proto)
{
	prepare_addr(family);
	prepare_raw_sock(family, proto);
	prepare_udp_sock(family);
}

static void sock_bind_any(int sock, sa_family_t family)
{
	struct sockaddr anyaddr = { .sa_family = family };
	socklen_t bindaddr_len = (family == AF_INET) ?
		sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);

	zassert_ok(zsock_bind(sock, &anyaddr, bindaddr_len),
		   "Binding RAW socket failed (%d)", errno);
}

static void test_sockets_close(void)
{
	if (raw_sock >= 0) {
		(void)zsock_close(raw_sock);
		raw_sock = -1;
	}

	if (raw_sock_2 >= 0) {
		(void)zsock_close(raw_sock_2);
		raw_sock_2 = -1;
	}

	if (udp_sock >= 0) {
		(void)zsock_close(udp_sock);
		udp_sock = -1;
	}

	if (udp_sock_2 >= 0) {
		(void)zsock_close(udp_sock_2);
		udp_sock_2 = -1;
	}

	if (packet_sock >= 0) {
		(void)zsock_close(packet_sock);
		packet_sock = -1;
	}
}

static void validate_ip_udp_hdr(sa_family_t family, struct sockaddr *src,
				struct sockaddr *dst, uint8_t *data,
				size_t datalen)
{
	if (family == AF_INET) {
		struct net_ipv4_hdr *ipv4 = (struct net_ipv4_hdr *)data;
		struct net_udp_hdr *udp = (struct net_udp_hdr *)(data + NET_IPV4H_LEN);

		zassert_true(datalen >= NET_IPV4UDPH_LEN, "Packet too short");
		zassert_equal(ipv4->vhl & 0xF0, 0x40, "Not an IPv4 packet");
		zassert_mem_equal(ipv4->src, &net_sin(src)->sin_addr, sizeof(ipv4->src),
				  "Invalid source address");
		zassert_mem_equal(ipv4->dst, &net_sin(dst)->sin_addr, sizeof(ipv4->dst),
				  "Invalid destination address");
		zassert_equal(ipv4->proto, IPPROTO_UDP, "Invalid protocol");
		zassert_equal(udp->src_port, net_sin(src)->sin_port,
			      "Invalid source port");
		zassert_equal(udp->dst_port, net_sin(dst)->sin_port,
			      "Invalid destination port");
		zassert_equal(udp->len, htons(datalen - NET_IPV4H_LEN),
			      "Invalid UDP length");
	} else {
		struct net_ipv6_hdr *ipv6 = (struct net_ipv6_hdr *)data;
		struct net_udp_hdr *udp = (struct net_udp_hdr *)(data + NET_IPV6H_LEN);

		zassert_true(datalen >= NET_IPV6UDPH_LEN, "Packet too short");
		zassert_equal(ipv6->vtc & 0xF0, 0x60, "Not an IPv6 packet");
		zassert_mem_equal(ipv6->src, &net_sin6(src)->sin6_addr, sizeof(ipv6->src),
				  "Invalid source address");
		zassert_mem_equal(ipv6->dst, &net_sin6(dst)->sin6_addr, sizeof(ipv6->dst),
				  "Invalid destination address");
		zassert_equal(ipv6->nexthdr, IPPROTO_UDP, "Invalid protocol");
		zassert_equal(udp->src_port, net_sin6(src)->sin6_port,
			      "Invalid source port");
		zassert_equal(udp->dst_port, net_sin6(dst)->sin6_port,
			      "Invalid destination port");
		zassert_equal(udp->len, htons(datalen - NET_IPV6H_LEN),
			      "Invalid UDP length");
	}
}

static void verify_raw_recv_success(int sock, sa_family_t family)
{
	size_t headers_len = (family == AF_INET) ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN;
	size_t expected_len = headers_len + sizeof(test_payload);
	int ret;

	ret = zsock_recv(sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive RAW packet (%d)", errno);
	zassert_equal(ret, expected_len, "Invalid data size received (%d, expected %d)",
		      ret, expected_len);

	validate_ip_udp_hdr(family, &src_addr, &dst_addr, rx_buf, ret);
	zassert_mem_equal(rx_buf + headers_len, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

static void verify_raw_recv_failure(void)
{
	int ret;

	ret = zsock_recv(raw_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(ret, -1, "RAW receive should fail (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);
}

#define TEST_UDP_IVP4_CHKSUM 0x03e4
#define TEST_UDP_IVP6_CHKSUM 0x930c

static void prepare_raw_ip_packet(sa_family_t family, uint8_t *buf, uint16_t *buflen)
{
	uint16_t headers_len = (family == AF_INET) ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN;
	uint16_t packet_len = headers_len + sizeof(test_payload);

	zassert_true(packet_len <= *buflen, "Packet too long");
	*buflen = packet_len;

	if (family == AF_INET) {
		struct net_ipv4_hdr *ipv4 = (struct net_ipv4_hdr *)tx_buf;
		struct net_udp_hdr *udp = (struct net_udp_hdr *)(tx_buf + NET_IPV4H_LEN);

		/* Prepare IPv4 header */
		ipv4->vhl = 0x45;
		ipv4->len = htons(packet_len);
		ipv4->ttl = 64;
		ipv4->proto = IPPROTO_UDP;
		/* UDP socket binds to src_addr, so for this test we swap src/dst */
		memcpy(ipv4->src, &net_sin(&dst_addr)->sin_addr, sizeof(ipv4->src));
		memcpy(ipv4->dst, &net_sin(&src_addr)->sin_addr, sizeof(ipv4->dst));

		/* Prepare UDP header */
		udp->src_port = net_sin(&dst_addr)->sin_port;
		udp->dst_port = net_sin(&src_addr)->sin_port;
		udp->len = htons(sizeof(test_payload) + NET_UDPH_LEN);
		udp->chksum = TEST_UDP_IVP4_CHKSUM;

		memcpy(tx_buf + NET_IPV4UDPH_LEN, test_payload, sizeof(test_payload));
	} else {
		struct net_ipv6_hdr *ipv6 = (struct net_ipv6_hdr *)tx_buf;
		struct net_udp_hdr *udp = (struct net_udp_hdr *)(tx_buf + NET_IPV6H_LEN);

		/* Prepare IPv6 header */
		ipv6->vtc = 0x60;
		ipv6->len = htons(sizeof(test_payload) + NET_UDPH_LEN);
		ipv6->nexthdr = IPPROTO_UDP;
		ipv6->hop_limit = 64;
		/* UDP socket binds to src_addr, so for this test we swap src/dst */
		memcpy(ipv6->src, &net_sin6(&dst_addr)->sin6_addr, sizeof(ipv6->src));
		memcpy(ipv6->dst, &net_sin6(&src_addr)->sin6_addr, sizeof(ipv6->dst));

		/* Prepare UDP header */
		udp->src_port = net_sin6(&dst_addr)->sin6_port;
		udp->dst_port = net_sin6(&src_addr)->sin6_port;
		udp->len = htons(sizeof(test_payload) + NET_UDPH_LEN);
		udp->chksum = TEST_UDP_IVP6_CHKSUM;

		memcpy(tx_buf + NET_IPV6UDPH_LEN, test_payload, sizeof(test_payload));
	}
}

static void test_raw_sock_send(sa_family_t family, enum net_ip_protocol proto)
{
	uint16_t len = sizeof(tx_buf);
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, proto);
	prepare_raw_ip_packet(family, tx_buf, &len);

	ret = zsock_send(raw_sock, tx_buf, len, 0);
	zassert_equal(ret, len, "Failed to send RAW packet (%d)", errno);

	ret = zsock_recv(udp_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
	zassert_equal(ret, sizeof(test_payload),
		     "Invalid data size received (%d, expected %d)",
		      ret, sizeof(test_payload));
	zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_send_proto_wildcard)
{
	test_raw_sock_send(AF_INET, IPPROTO_IP);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_send_proto_wildcard)
{
	test_raw_sock_send(AF_INET6, IPPROTO_IP);
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_send_proto_match)
{
	test_raw_sock_send(AF_INET, IPPROTO_UDP);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_send_proto_match)
{
	test_raw_sock_send(AF_INET6, IPPROTO_UDP);
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_send_proto_ipproto_raw)
{
	test_raw_sock_send(AF_INET, IPPROTO_RAW);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_send_proto_ipproto_raw)
{
	test_raw_sock_send(AF_INET6, IPPROTO_RAW);
}

static void test_raw_sock_sendto(sa_family_t family)
{
	uint16_t len = sizeof(tx_buf);
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_IP);
	prepare_raw_ip_packet(family, tx_buf, &len);

	ret = zsock_sendto(raw_sock, tx_buf, len, 0, &src_addr, addrlen);
	zassert_equal(ret, len, "Failed to send RAW packet (%d)", errno);

	ret = zsock_recv(udp_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
	zassert_equal(ret, sizeof(test_payload),
		     "Invalid data size received (%d, expected %d)",
		      ret, sizeof(test_payload));
	zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_sendto)
{
	test_raw_sock_sendto(AF_INET);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_sendto)
{
	test_raw_sock_sendto(AF_INET6);
}

static void test_raw_sock_sendmsg(sa_family_t family)
{
	uint16_t len = sizeof(tx_buf);
	struct iovec io_vector;
	struct msghdr msg = { 0 };
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_IP);
	prepare_raw_ip_packet(family, tx_buf, &len);

	io_vector.iov_base = tx_buf;
	io_vector.iov_len = len;
	msg.msg_iov = &io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &src_addr;
	msg.msg_namelen = addrlen;

	ret = zsock_sendmsg(raw_sock, &msg, 0);
	zassert_equal(ret, len, "Failed to send RAW packet (%d)", errno);

	ret = zsock_recv(udp_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
	zassert_equal(ret, sizeof(test_payload),
		     "Invalid data size received (%d, expected %d)",
		      ret, sizeof(test_payload));
	zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_sendmsg)
{
	test_raw_sock_sendmsg(AF_INET);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_sendmsg)
{
	test_raw_sock_sendmsg(AF_INET6);
}

static void test_raw_sock_recv(sa_family_t family, enum net_ip_protocol proto)
{

	int ret;

	prepare_raw_and_udp_sock_and_addr(family, proto);
	sock_bind_any(raw_sock, family);

	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	if (proto == IPPROTO_IP || proto == IPPROTO_UDP) {
		verify_raw_recv_success(raw_sock, family);
	} else {
		verify_raw_recv_failure();
	}
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recv_proto_wildcard)
{
	test_raw_sock_recv(AF_INET, IPPROTO_IP);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recv_proto_wildcard)
{
	test_raw_sock_recv(AF_INET6, IPPROTO_IP);
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recv_proto_match)
{
	test_raw_sock_recv(AF_INET, IPPROTO_UDP);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recv_proto_match)
{
	test_raw_sock_recv(AF_INET6, IPPROTO_UDP);
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recv_proto_mismatch)
{
	test_raw_sock_recv(AF_INET, IPPROTO_TCP);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recv_proto_mismatch)
{
	test_raw_sock_recv(AF_INET6, IPPROTO_TCP);
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recv_proto_ipproto_raw)
{
	test_raw_sock_recv(AF_INET, IPPROTO_RAW);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recv_proto_ipproto_raw)
{
	test_raw_sock_recv(AF_INET6, IPPROTO_RAW);
}

static void test_raw_sock_recvfrom(sa_family_t family)
{
	size_t headers_len = (family == AF_INET) ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN;
	size_t expected_len = headers_len + sizeof(test_payload);
	struct sockaddr recv_addr = { 0 };
	socklen_t recv_addrlen = sizeof(recv_addr);
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_IP);
	sock_bind_any(raw_sock, family);

	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	ret = zsock_recvfrom(raw_sock, rx_buf, sizeof(rx_buf), 0,
			     &recv_addr, &recv_addrlen);
	zassert_not_equal(ret, -1, "Failed to receive RAW packet (%d)", errno);
	zassert_equal(ret, expected_len, "Invalid data size received (%d, expected %d)",
		      ret, expected_len);
	validate_ip_udp_hdr(family, &src_addr, &dst_addr, rx_buf, ret);
	zassert_mem_equal(rx_buf + headers_len, test_payload, sizeof(test_payload),
			  "Invalid payload received");

	zassert_equal(recv_addrlen, addrlen, "Invalid sender address length");
	zassert_equal(recv_addr.sa_family, family, "Invalid sender address family");
	if (family == AF_INET) {
		zassert_mem_equal(&net_sin(&recv_addr)->sin_addr,
				  &net_sin(&src_addr)->sin_addr,
				  sizeof(struct in_addr), "Invalid sender address");
	} else {
		zassert_mem_equal(&net_sin6(&recv_addr)->sin6_addr,
				  &net_sin6(&src_addr)->sin6_addr,
				  sizeof(struct in6_addr), "Invalid sender address");
	}
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recvfrom)
{
	test_raw_sock_recvfrom(AF_INET);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recvfrom)
{
	test_raw_sock_recvfrom(AF_INET6);
}

static void test_raw_sock_recvmsg(sa_family_t family)
{
	size_t headers_len = (family == AF_INET) ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN;
	size_t expected_len = headers_len + sizeof(test_payload);
	struct sockaddr recv_addr = { 0 };
	struct iovec io_vector;
	struct msghdr msg = { 0 };
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_IP);
	sock_bind_any(raw_sock, family);

	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	io_vector.iov_base = rx_buf;
	io_vector.iov_len = sizeof(rx_buf);
	msg.msg_iov = &io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &recv_addr;
	msg.msg_namelen = sizeof(recv_addr);

	ret = zsock_recvmsg(raw_sock, &msg, 0);
	zassert_not_equal(ret, -1, "Failed to receive RAW packet (%d)", errno);
	zassert_equal(ret, expected_len, "Invalid data size received (%d, expected %d)",
		      ret, expected_len);
	validate_ip_udp_hdr(family, &src_addr, &dst_addr, rx_buf, ret);
	zassert_mem_equal(rx_buf + headers_len, test_payload, sizeof(test_payload),
			  "Invalid payload received");

	zassert_equal(msg.msg_namelen, addrlen, "Invalid sender address length");
	zassert_equal(recv_addr.sa_family, family, "Invalid sender address family");
	if (family == AF_INET) {
		zassert_mem_equal(&net_sin(&recv_addr)->sin_addr,
				  &net_sin(&src_addr)->sin_addr,
				  sizeof(struct in_addr), "Invalid sender address");
	} else {
		zassert_mem_equal(&net_sin6(&recv_addr)->sin6_addr,
				  &net_sin6(&src_addr)->sin6_addr,
				  sizeof(struct in6_addr), "Invalid sender address");
	}
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recvmsg)
{
	test_raw_sock_recvmsg(AF_INET);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recvmsg)
{
	test_raw_sock_recvmsg(AF_INET6);
}

static void test_raw_sock_bind(sa_family_t family, struct sockaddr *bind_addr,
			       bool shall_receive)
{
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_UDP);

	ret = zsock_bind(raw_sock, bind_addr, addrlen);
	zassert_ok(ret, "Binding RAW socket failed (%d)", errno);

	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	if (shall_receive) {
		verify_raw_recv_success(raw_sock, family);
	} else {
		verify_raw_recv_failure();
	}
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_bind)
{
	struct sockaddr_in bind_addr = {
		.sin_family = AF_INET,
		.sin_addr = test_ipv4_2
	};

	test_raw_sock_bind(AF_INET, (struct sockaddr *)&bind_addr, true);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_bind)
{
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = test_ipv6_2
	};

	test_raw_sock_bind(AF_INET6, (struct sockaddr *)&bind_addr, true);
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_bind_other_addr)
{
	struct sockaddr_in bind_addr = {
		.sin_family = AF_INET,
		.sin_addr = test_ipv4_1
	};

	test_raw_sock_bind(AF_INET, (struct sockaddr *)&bind_addr, false);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_bind_other_addr)
{
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = test_ipv6_1
	};

	test_raw_sock_bind(AF_INET6, (struct sockaddr *)&bind_addr, false);
}

static void test_raw_sock_recv_no_cross_family(sa_family_t family_raw)
{
	sa_family_t family_udp = family_raw == AF_INET ? AF_INET6 : AF_INET;
	int ret;

	prepare_addr(family_udp);
	prepare_raw_sock(family_raw, IPPROTO_UDP);
	prepare_udp_sock(family_udp);
	sock_bind_any(raw_sock, family_raw);

	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	/* RAW socket should not get the packet from a different family (i.e.
	 * no IPv4 packet on IPv6 socket and vice versa).
	 */
	verify_raw_recv_failure();
}

ZTEST(socket_af_inet_raw, test_raw_v4_sock_recv_no_ipv6)
{
	test_raw_sock_recv_no_cross_family(AF_INET);
}

ZTEST(socket_af_inet_raw, test_raw_v6_sock_recv_no_ipv4)
{
	test_raw_sock_recv_no_cross_family(AF_INET6);
}

static void test_two_raw_socks_recv(sa_family_t family)
{
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_UDP);
	prepare_raw_sock_2(family, IPPROTO_IP);
	sock_bind_any(raw_sock, family);
	sock_bind_any(raw_sock_2, family);

	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	/* Both RAW sockets should get the packet. */
	verify_raw_recv_success(raw_sock, family);
	verify_raw_recv_success(raw_sock_2, family);
}

ZTEST(socket_af_inet_raw, test_two_raw_v4_socks_recv)
{
	test_two_raw_socks_recv(AF_INET);
}

ZTEST(socket_af_inet_raw, test_two_raw_v6_socks_recv)
{
	test_two_raw_socks_recv(AF_INET6);
}

static void test_raw_and_udp_socks_recv(sa_family_t family)
{
	size_t headers_len = (family == AF_INET) ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN;
	size_t expected_len = headers_len + sizeof(test_payload);
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_UDP);
	prepare_udp_sock_2(family);
	sock_bind_any(raw_sock, family);

	/* Send to bound UDP endpoint this time */
	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	/* Both RAW and UDP sockets should receive the packet. */
	ret = zsock_recv(raw_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive RAW packet (%d)", errno);
	zassert_equal(ret, expected_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, expected_len);

	validate_ip_udp_hdr(family, &src_addr, &dst_addr, rx_buf, ret);
	zassert_mem_equal(rx_buf + headers_len, test_payload, sizeof(test_payload),
			  "Invalid payload received");

	ret = zsock_recv(udp_sock_2, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
	zassert_equal(ret, sizeof(test_payload),
		     "Invalid data size received (%d, expected %d)",
		      ret, sizeof(test_payload));
	zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

ZTEST(socket_af_inet_raw, test_raw_and_udp_v4_socks_recv)
{
	test_raw_and_udp_socks_recv(AF_INET);
}

ZTEST(socket_af_inet_raw, test_raw_and_udp_v6_socks_recv)
{
	test_raw_and_udp_socks_recv(AF_INET6);
}

static void test_packet_and_raw_socks_recv(sa_family_t family, uint16_t packet_proto)
{
	size_t headers_len = (family == AF_INET) ? NET_IPV4UDPH_LEN : NET_IPV6UDPH_LEN;
	size_t expected_len = headers_len + sizeof(test_payload);
	int ret;

	prepare_raw_and_udp_sock_and_addr(family, IPPROTO_UDP);
	prepare_packet_sock(packet_proto);
	sock_bind_any(raw_sock, family);

	/* Send to bound UDP endpoint this time */
	ret = zsock_sendto(udp_sock, test_payload, sizeof(test_payload), 0,
			   &dst_addr, addrlen);
	zassert_equal(ret, sizeof(test_payload), "Failed to send UDP packet (%d)",
		      errno);

	/* Both packet and RAW IP sockets should receive the packet. */
	ret = zsock_recv(packet_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive packet (%d)", errno);
	zassert_equal(ret, expected_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, expected_len);

	validate_ip_udp_hdr(family, &src_addr, &dst_addr, rx_buf, ret);
	zassert_mem_equal(rx_buf + headers_len, test_payload, sizeof(test_payload),
			  "Invalid payload received");

	memset(rx_buf, 0, sizeof(rx_buf));

	ret = zsock_recv(raw_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive RAW packet (%d)", errno);
	zassert_equal(ret, expected_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, expected_len);

	validate_ip_udp_hdr(family, &src_addr, &dst_addr, rx_buf, ret);
	zassert_mem_equal(rx_buf + headers_len, test_payload, sizeof(test_payload),
			  "Invalid payload received");


}

ZTEST(socket_af_inet_raw, test_packet_and_raw_v4_socks_recv_wildcard)
{
	if (!IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
		ztest_test_skip();
	}

	test_packet_and_raw_socks_recv(AF_INET, ETH_P_ALL);
}

ZTEST(socket_af_inet_raw, test_packet_and_raw_v6_socks_recv_wildcard)
{
	if (!IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
		ztest_test_skip();
	}

	test_packet_and_raw_socks_recv(AF_INET6, ETH_P_ALL);
}

ZTEST(socket_af_inet_raw, test_packet_and_raw_v4_socks_recv_proto_match)
{
	if (!IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
		ztest_test_skip();
	}

	test_packet_and_raw_socks_recv(AF_INET, ETH_P_IP);
}

ZTEST(socket_af_inet_raw, test_packet_and_raw_v6_socks_recv_proto_match)
{
	if (!IS_ENABLED(CONFIG_NET_SOCKETS_PACKET)) {
		ztest_test_skip();
	}

	test_packet_and_raw_socks_recv(AF_INET6, ETH_P_IPV6);
}

static void test_after(void *arg)
{
	ARG_UNUSED(arg);

	memset(&rx_buf, 0, sizeof(rx_buf));
	memset(&tx_buf, 0, sizeof(tx_buf));
	memset(&src_addr, 0, sizeof(src_addr));
	memset(&dst_addr, 0, sizeof(dst_addr));
	addrlen = 0;

	test_sockets_close();
}

static void *test_setup(void)
{
	(void)net_if_ipv4_addr_add(test_iface, &test_ipv4_1, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv4_addr_add(test_iface, &test_ipv4_2, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv6_addr_add(test_iface, &test_ipv6_1, NET_ADDR_MANUAL, 0);
	(void)net_if_ipv6_addr_add(test_iface, &test_ipv6_2, NET_ADDR_MANUAL, 0);
	(void)net_if_up(test_iface);

	return NULL;
}

ZTEST_SUITE(socket_af_inet_raw, NULL, test_setup, NULL, test_after, NULL);
