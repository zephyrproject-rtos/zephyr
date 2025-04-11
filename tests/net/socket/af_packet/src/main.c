/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/sys/mutex.h>
#include <zephyr/ztest_assert.h>

#include <zephyr/posix/fcntl.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_ip.h>

/* This test suite verifies that AF_PACKET sockets behave according to well known behaviors.
 * Note, that this is not well standardized and relies on behaviors known from Linux or FreeBSD.
 *
 * Sending data (TX)
 *
 *   * (AF_PACKET, SOCK_RAW, 0) - The packet contains already a valid L2 header:
 *     - test_raw_sock_sendto_no_proto_bound
 *     - test_raw_sock_sendto_no_proto_unbound
 *     - test_raw_sock_sendto_no_proto_unbound_no_iface
 *     - test_raw_sock_sendto_no_proto_unbound_no_addr
 *     - test_raw_sock_sendmsg_no_proto
 *
 *   * (AF_PACKET, SOCK_DGRAM, 0) - User needs to supply `struct sockaddr_ll` with
 *     all the relevant fields filled so that L2 header can be constructed:
 *     - test_dgram_sock_sendto_no_proto_bound
 *     - test_dgram_sock_sendto_no_proto_unbound
 *     - test_dgram_sock_sendto_no_proto_unbound_no_iface
 *     - test_dgram_sock_sendto_no_proto_unbound_no_addr
 *     - test_dgram_sock_sendmsg_no_proto
 *
 *   * (AF_PACKET, SOCK_RAW, <protocol>) - The packet contains already a valid
 *     L2 header. Not mentioned in packet(7) but as the L2 header needs to be
 *     supplied by the user, the protocol value is ignored:
 *     - test_raw_sock_sendto_proto_wildcard
 *     - test_raw_sock_sendmsg_proto_wildcard
 *
 *   * (AF_PACKET, SOCK_DGRAM, <protocol>) -  L2 header is constructed according
 *     to protocol and `struct sockaddr_ll`:
 *     - test_dgram_sock_sendto_proto_wildcard
 *     - test_dgram_sock_sendto_proto_match
 *     - test_dgram_sock_sendmsg_proto_wildcard
 *     - test_dgram_sock_sendmsg_proto_match
 *
 * Receiving data (RX)
 *
 *   * (AF_PACKET, SOCK_RAW, 0) - The packet is dropped when received by this socket.
 *     See https://man7.org/linux/man-pages/man7/packet.7.html:
 *     - test_raw_sock_recv_no_proto
 *
 *   * (AF_PACKET, SOCK_DGRAM, 0) - The packet is dropped when received by this socket.
 *     See https://man7.org/linux/man-pages/man7/packet.7.html
 *     - test_dgram_sock_recv_no_proto
 *
 *   * (AF_PACKET, SOCK_RAW, <protocol>) - The packet is received for a given protocol
 *     only. The L2 header is not removed from the data:
 *     - NOT SUPPORTED
 *
 *   * (AF_PACKET, SOCK_DGRAM, <protocol>) - The packet is received for a given protocol
 *     only. The L2 header is removed from the data:
 *     - test_dgram_sock_recv_proto_match
 *     - test_dgram_sock_recv_proto_mismatch
 *
 *   * (AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)) - The packet is received for all protocols.
 *     The L2 header is not removed from the data:
 *     - test_raw_sock_recv_proto_wildcard
 *     - test_raw_sock_recvfrom_proto_wildcard
 *
 *   * (AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL)) - The packet is received for all protocols.
 *     The L2 header is removed from the data:
 *     - test_dgram_sock_recv_proto_wildcard
 *     - test_dgram_sock_recvfrom_proto_wildcard
 */

#if defined(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) NET_DBG(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define IPV4_ADDR "192.0.2.1"

static int packet_sock_1 = -1;
static int packet_sock_2 = -1;
static int packet_sock_3 = -1;
static int udp_sock_1 = -1;
static int udp_sock_2 = -1;

static const uint8_t test_payload[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
static uint8_t rx_buf[64];
static uint8_t tx_buf[64];
static struct in_addr fake_src = { { { 192, 0, 2, 2 } } };

static uint8_t lladdr1[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
static uint8_t lladdr2[] = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };

struct eth_fake_context {
	struct net_if *iface;
	uint8_t *mac_address;
	char *ip_address;
};

static struct eth_fake_context eth_fake_data1 = {
	.mac_address = lladdr1,
	.ip_address = IPV4_ADDR,
};
static struct eth_fake_context eth_fake_data2 = {
	.mac_address = lladdr2
};

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *recv_pkt;
	int ret;
	struct net_if *target_iface;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	DBG("Sending data (%d bytes) to iface %d\n",
	    net_pkt_get_len(pkt), net_if_get_by_iface(net_pkt_iface(pkt)));

	recv_pkt = net_pkt_rx_clone(pkt, K_NO_WAIT);

	if (memcmp(pkt->frags->data, lladdr1, sizeof(lladdr1)) == 0) {
		target_iface = eth_fake_data1.iface;
	} else {
		target_iface = eth_fake_data2.iface;
	}

	net_pkt_set_iface(recv_pkt, target_iface);

	k_sleep(K_MSEC(10)); /* Let the receiver run */

	ret = net_recv_data(net_pkt_iface(recv_pkt), recv_pkt);
	zassert_equal(ret, 0, "Cannot receive data (%d)", ret);

	return 0;
}

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address, 6, NET_LINK_ETHERNET);

	if (ctx->ip_address != NULL) {
		struct in_addr addr;

		if (net_addr_pton(AF_INET, ctx->ip_address, &addr) == 0) {
			net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
		}
	}

	ethernet_init(iface);
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(eth_fake1, "eth_fake1", NULL, NULL, &eth_fake_data1, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake2, "eth_fake2", NULL, NULL, &eth_fake_data2, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

static void setup_packet_socket(int *sock, struct net_if *iface, int type,
				int proto)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	*sock = zsock_socket(AF_PACKET, type, proto);
	zassert_true(*sock >= 0, "Cannot create packet socket (%d)", -errno);

	ret = zsock_setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	zassert_ok(ret, "setsockopt failed (%d)", errno);
}

static void bind_packet_socket(int sock, struct net_if *iface)
{
	struct sockaddr_ll addr;
	int ret;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = net_if_get_by_iface(iface);
	addr.sll_family = AF_PACKET;

	ret = zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	zassert_ok(ret, "Cannot bind packet socket (%d)", -errno);
}

static void prepare_packet_socket(int *sock, struct net_if *iface, int type,
				  int proto)
{
	setup_packet_socket(sock, iface, type, proto);
	bind_packet_socket(*sock, iface);
}

struct user_data {
	struct net_if *first;
	struct net_if *second;
} ud;

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *test_data = user_data;
	struct net_linkaddr *link_addr;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	link_addr = net_if_get_link_addr(iface);
	if (memcmp(link_addr->addr, lladdr1, sizeof(lladdr1)) != 0 &&
	    memcmp(link_addr->addr, lladdr2, sizeof(lladdr2)) != 0) {
		return;
	}

	if (test_data->first == NULL) {
		test_data->first = iface;
		return;
	}

	test_data->second = iface;
}

#define SRC_PORT 4240
#define DST_PORT 4242
static void prepare_udp_socket(int *sock, struct sockaddr_in *sockaddr, uint16_t local_port)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	*sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(*sock >= 0, "Cannot create DGRAM (UDP) socket (%d)", sock);

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(local_port);
	ret = zsock_inet_pton(AF_INET, IPV4_ADDR, &sockaddr->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* Bind UDP socket to local port */
	ret = zsock_bind(*sock, (struct sockaddr *) sockaddr, sizeof(*sockaddr));
	zassert_equal(ret, 0, "Cannot bind DGRAM (UDP) socket (%d)", -errno);

	ret = zsock_setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, &optval,
			       sizeof(optval));
	zassert_ok(ret, "setsockopt failed (%d)", errno);
}

#define IP_HDR_SIZE 20
#define UDP_HDR_SIZE 8
#define HDR_SIZE (IP_HDR_SIZE + UDP_HDR_SIZE)
ZTEST(socket_packet, test_raw_packet_sockets_udp_send)
{
	uint8_t data_to_send[] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
	uint8_t data_to_receive[sizeof(data_to_send) + HDR_SIZE];
	struct sockaddr_ll src;
	struct sockaddr_in sockaddr;
	int ret;
	socklen_t addrlen;
	ssize_t sent = 0;

	prepare_packet_socket(&packet_sock_1, ud.first, SOCK_RAW, htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, SOCK_RAW, htons(ETH_P_ALL));

	/* Prepare UDP socket which will read data */
	prepare_udp_socket(&udp_sock_1, &sockaddr, DST_PORT);

	/* Prepare UDP socket from which data are going to be sent */
	prepare_udp_socket(&udp_sock_2, &sockaddr, SRC_PORT);
	/* Properly set destination port for UDP packet */
	sockaddr.sin_port = htons(DST_PORT);

	/*
	 * Send UDP datagram to us - as check_ip_addr() in net_send_data()
	 * returns 1 - the packet is processed immediately in the net stack
	 */
	sent = zsock_sendto(udp_sock_2, data_to_send, sizeof(data_to_send),
			    0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	zassert_equal(sent, sizeof(data_to_send), "sendto failed");

	memset(&data_to_receive, 0, sizeof(data_to_receive));
	errno = 0;

	/* Check if UDP packets can be read after being sent */
	addrlen = sizeof(sockaddr);
	ret = zsock_recvfrom(udp_sock_1, data_to_receive, sizeof(data_to_receive),
			     0, (struct sockaddr *)&sockaddr, &addrlen);
	zassert_equal(ret, sizeof(data_to_send), "Cannot receive all data (%d)",
		      -errno);
	zassert_mem_equal(data_to_receive, data_to_send, sizeof(data_to_send),
			  "Sent and received buffers do not match");

	/* And if the packet has been also passed to RAW socket */
	memset(&data_to_receive, 0, sizeof(data_to_receive));
	memset(&src, 0, sizeof(src));
	addrlen = sizeof(src);
	errno = 0;

	/* The recvfrom reads the whole received packet - including its
	 * IP (20B) and UDP (8) headers. After those we can expect payload,
	 * which have been sent.
	 */
	ret = zsock_recvfrom(packet_sock_1, data_to_receive, sizeof(data_to_receive), 0,
			     (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(data_to_send) + HDR_SIZE,
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(data_to_send), -errno);
	zassert_mem_equal(&data_to_receive[HDR_SIZE], data_to_send,
			  sizeof(data_to_send),
			  "Sent and received buffers do not match");
}

ZTEST(socket_packet, test_packet_sockets)
{
	prepare_packet_socket(&packet_sock_1, ud.first, SOCK_RAW, htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, SOCK_RAW, htons(ETH_P_ALL));
}

ZTEST(socket_packet, test_packet_sockets_dgram)
{
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct sockaddr_ll dst, src;
	int ret;

	prepare_packet_socket(&packet_sock_1, ud.first, SOCK_DGRAM, htons(ETH_P_TSN));
	prepare_packet_socket(&packet_sock_2, ud.second, SOCK_DGRAM, htons(ETH_P_TSN));

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_TSN);
	memcpy(dst.sll_addr, lladdr1, sizeof(lladdr1));

	ret = zsock_sendto(packet_sock_2, test_payload, sizeof(test_payload), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf), 0,
	(struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));
	errno = 0;
	ret = zsock_recvfrom(packet_sock_1, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(test_payload), -errno);

	zassert_equal(addrlen, sizeof(struct sockaddr_ll),
		      "Invalid address length (%d)", addrlen);

	struct sockaddr_ll src_expected = {
		.sll_family = AF_PACKET,
		.sll_protocol = dst.sll_protocol,
		.sll_ifindex = net_if_get_by_iface(ud.first),
		.sll_pkttype = PACKET_OTHERHOST,
		.sll_hatype = ARPHRD_ETHER,
		.sll_halen = sizeof(lladdr2),
		.sll_addr = {0},
	};
	memcpy(&src_expected.sll_addr, lladdr2, ARRAY_SIZE(lladdr2));
	zassert_mem_equal(&src, &src_expected, addrlen, "Invalid source address");

	zassert_mem_equal(test_payload, rx_buf, sizeof(test_payload),
			  "Data mismatch");

	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr2));

	/* Send to socket 2 but read from socket 1. There should not be any
	 * data in socket 1
	 */
	ret = zsock_sendto(packet_sock_2, test_payload, sizeof(test_payload), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	memset(&src, 0, sizeof(src));

	ret = zsock_recvfrom(packet_sock_1, rx_buf, sizeof(rx_buf), 0,
			     (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload), "Cannot receive all data (%d)",
		      -errno);
	zassert_equal(addrlen, sizeof(struct sockaddr_ll),
		      "Invalid address length (%d)", addrlen);

	src_expected = (struct sockaddr_ll){
		.sll_family = AF_PACKET,
		.sll_protocol = dst.sll_protocol,
		.sll_ifindex = net_if_get_by_iface(ud.second),
		.sll_pkttype = PACKET_OTHERHOST,
		.sll_hatype = ARPHRD_ETHER,
		.sll_halen = ARRAY_SIZE(lladdr2),
		.sll_addr = {0},
	};
	memcpy(&src_expected.sll_addr, lladdr2, ARRAY_SIZE(lladdr2));
	zassert_mem_equal(&src, &src_expected, addrlen, "Invalid source address");

	zassert_mem_equal(test_payload, rx_buf, sizeof(test_payload),
			  "Data mismatch");

	/* Send specially crafted payload to mimic IPv4 and IPv6 length field,
	 * to ckeck correct length returned.
	 */
	uint8_t payload_ip_length[64], receive_ip_length[64];

	memset(payload_ip_length, 0, sizeof(payload_ip_length));
	/* Set ipv4 and ipv6 length fields to represent IP payload with the
	 * length of 1 byte.
	 */
	payload_ip_length[3] = 21;
	payload_ip_length[5] = 1;

	ret = zsock_sendto(packet_sock_2, payload_ip_length, sizeof(payload_ip_length), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(payload_ip_length), "Cannot send all data (%d)", -errno);

	memset(&src, 0, sizeof(src));
	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, receive_ip_length, sizeof(receive_ip_length), 0,
			     (struct sockaddr *)&src, &addrlen);

	zassert_equal(ret, ARRAY_SIZE(payload_ip_length), "Cannot receive all data (%d)", -errno);
	zassert_mem_equal(payload_ip_length, receive_ip_length, sizeof(payload_ip_length),
			  "Data mismatch");
}

ZTEST(socket_packet, test_raw_and_dgram_socket_exchange)
{
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct sockaddr_ll dst, src;
	int ret;
	const uint8_t expected_payload_raw[] = {
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Dst ll addr */
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};
	const uint8_t send_payload_raw[] = {
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* Dst ll addr */
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};

	prepare_packet_socket(&packet_sock_1, ud.first, SOCK_DGRAM, htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, SOCK_RAW, htons(ETH_P_ALL));

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_IP);
	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr1));

	/* SOCK_DGRAM to SOCK_RAW */

	ret = zsock_sendto(packet_sock_1, test_payload, sizeof(test_payload), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10); /* Let the packet enter the system */
	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(expected_payload_raw), -errno);
	zassert_mem_equal(expected_payload_raw, rx_buf,
			  sizeof(expected_payload_raw), "Data mismatch");

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_IP);

	/* SOCK_RAW to SOCK_DGRAM */

	ret = zsock_sendto(packet_sock_2, send_payload_raw, sizeof(send_payload_raw), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(send_payload_raw), "Cannot send all data (%d)",
		      -errno);

	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_1, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload), "Cannot receive all data (%d)",
		      -errno);
	zassert_mem_equal(test_payload, rx_buf, sizeof(test_payload),
			  "Data mismatch");
}

ZTEST(socket_packet, test_raw_and_dgram_socket_recv)
{
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct sockaddr_ll dst, src;
	int ret;
	const uint8_t expected_payload_raw[] = {
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Dst ll addr */
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};

	prepare_packet_socket(&packet_sock_1, ud.first, SOCK_DGRAM, htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, SOCK_RAW, htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_3, ud.second, SOCK_DGRAM, htons(ETH_P_ALL));

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_IP);
	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr1));

	ret = zsock_sendto(packet_sock_1, test_payload, sizeof(test_payload), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	memset(&src, 0, sizeof(src));

	/* Both SOCK_DGRAM to SOCK_RAW sockets should receive packet. */

	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(expected_payload_raw), -errno);

	zassert_mem_equal(expected_payload_raw, rx_buf,
			  sizeof(expected_payload_raw), "Data mismatch");

	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_3, rx_buf, sizeof(rx_buf),
			     0, (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload),
		      "Cannot receive all data (%d)", -errno);
	zassert_mem_equal(test_payload, rx_buf, sizeof(test_payload),
			  "Data mismatch");
}

#define TEST_IPV4_CHKSUM 0xc3f6
#define TEST_UDP_CHKSUM 0x8b46

/* Prepare packet with optional IP/UDP headers and optional ethernet header.  */
static void prepare_test_packet(int sock_type, uint16_t proto,
				uint8_t *ll_src, uint8_t *ll_dst,
				uint16_t *pkt_len)
{
	uint16_t offset = 0;

	if (sock_type == SOCK_RAW) {
		struct net_eth_hdr *eth = (struct net_eth_hdr *)tx_buf;

		offset += sizeof(struct net_eth_hdr);
		zassert_not_null(ll_src, "NULL LL src");
		zassert_not_null(ll_dst, "NULL LL dst");
		zassert_true(offset <= sizeof(tx_buf), "Packet too long");

		memcpy(&eth->dst, ll_dst, sizeof(eth->dst));
		memcpy(&eth->src, ll_src, sizeof(eth->src));
		eth->type = htons(proto);
	}

	if (proto == ETH_P_IP) {
		struct in_addr addr;
		struct net_ipv4_hdr *ipv4 =
			(struct net_ipv4_hdr *)(tx_buf + offset);
		struct net_udp_hdr *udp =
			(struct net_udp_hdr *)(tx_buf + offset + NET_IPV4H_LEN);

		offset += NET_IPV4UDPH_LEN;
		zassert_true(offset <= sizeof(tx_buf), "Packet too long");
		zassert_ok(net_addr_pton(AF_INET, IPV4_ADDR, &addr),
			   "Addres parse failed");

		/* Prepare IPv4 header */
		ipv4->vhl = 0x45;
		ipv4->len = htons(sizeof(test_payload) + NET_IPV4UDPH_LEN);
		ipv4->ttl = 64;
		ipv4->proto = IPPROTO_UDP;
		ipv4->chksum = TEST_IPV4_CHKSUM;
		memcpy(ipv4->src, &fake_src, sizeof(ipv4->src));
		memcpy(ipv4->dst, &addr, sizeof(ipv4->dst));

		/* Prepare UDP header */
		udp->src_port = htons(SRC_PORT);
		udp->dst_port = htons(DST_PORT);
		udp->len = htons(sizeof(test_payload) + NET_UDPH_LEN);
		udp->chksum = TEST_UDP_CHKSUM;
	}

	zassert_true(offset + sizeof(test_payload) <= sizeof(tx_buf), "Packet too long");
	memcpy(tx_buf + offset, test_payload, sizeof(test_payload));

	offset += sizeof(test_payload);
	*pkt_len = offset;
}

static void prepare_test_dst_lladdr(struct sockaddr_ll *ll_dst, uint16_t proto,
				    uint8_t *ll_addr, struct net_if *iface)
{
	memset(ll_dst, 0, sizeof(struct sockaddr_ll));

	ll_dst->sll_family = AF_PACKET;
	ll_dst->sll_protocol = htons(proto);
	memcpy(ll_dst->sll_addr, ll_addr, NET_ETH_ADDR_LEN);

	if (iface != NULL) {
		ll_dst->sll_ifindex = net_if_get_by_iface(iface);
	}
}

static void test_sendto_common(int sock_type, int proto, bool do_bind,
			       int custom_dst_iface, bool set_dst_addr,
			       bool success)
{
	struct sockaddr_in ip_src;
	struct sockaddr_ll ll_dst;
	uint16_t pkt_len;
	int ret;

	setup_packet_socket(&packet_sock_1, ud.second, sock_type, htons(proto));
	if (do_bind) {
		bind_packet_socket(packet_sock_1, ud.second);
	}
	prepare_udp_socket(&udp_sock_1, &ip_src, DST_PORT);
	prepare_test_packet(sock_type, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	if (custom_dst_iface != 0) {
		ll_dst.sll_ifindex = custom_dst_iface;
	}

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   set_dst_addr ? (struct sockaddr *)&ll_dst : NULL,
			   set_dst_addr ? sizeof(struct sockaddr_ll) : 0);
	if (success) {
		zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
		zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

		ret = zsock_recv(udp_sock_1, rx_buf, sizeof(rx_buf), 0);
		zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
		zassert_equal(ret, sizeof(test_payload),
			     "Invalid data size received (%d, expected %d)",
			      ret, sizeof(test_payload));
		zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
				  "Invalid payload received");
	} else {
		zassert_equal(ret, -1, "Send should fail");
		zassert_equal(errno, EDESTADDRREQ, "Wrong errno");
	}
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_bound)
{
	test_sendto_common(SOCK_RAW, 0, true, 0, true, true);
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_unbound)
{
	test_sendto_common(SOCK_RAW, 0, false, 0, true, true);
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_unbound_no_iface)
{
	test_sendto_common(SOCK_RAW, 0, false, 10, true, false);
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_unbound_no_addr)
{
	test_sendto_common(SOCK_RAW, 0, false, 0, false, false);
}

static void test_sendmsg_common(int sock_type, int proto)
{
	struct sockaddr_in ip_src;
	struct sockaddr_ll ll_dst;
	struct iovec io_vector;
	struct msghdr msg = { 0 };
	uint16_t pkt_len;
	int ret;

	setup_packet_socket(&packet_sock_1, ud.second, sock_type, htons(proto));
	prepare_udp_socket(&udp_sock_1, &ip_src, DST_PORT);
	prepare_test_packet(sock_type, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);

	io_vector.iov_base = tx_buf;
	io_vector.iov_len = pkt_len;
	msg.msg_iov = &io_vector;
	msg.msg_iovlen = 1;
	msg.msg_name = &ll_dst;
	msg.msg_namelen = sizeof(ll_dst);

	ret = zsock_sendmsg(packet_sock_1, &msg, 0);
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	ret = zsock_recv(udp_sock_1, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
	zassert_equal(ret, sizeof(test_payload),
		      "Invalid data size received (%d, expected %d)",
		      ret, sizeof(test_payload));
	zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

ZTEST(socket_packet, test_raw_sock_sendmsg_no_proto)
{
	test_sendmsg_common(SOCK_RAW, 0);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_bound)
{
	test_sendto_common(SOCK_DGRAM, 0, true, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_unbound)
{
	test_sendto_common(SOCK_DGRAM, 0, false, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_unbound_no_iface)
{
	test_sendto_common(SOCK_DGRAM, 0, false, 10, true, false);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_unbound_no_addr)
{
	test_sendto_common(SOCK_DGRAM, 0, false, 0, false, false);
}

ZTEST(socket_packet, test_dgram_sock_sendmsg_no_proto)
{
	test_sendmsg_common(SOCK_DGRAM, 0);
}

ZTEST(socket_packet, test_raw_sock_sendto_proto_wildcard)
{
	test_sendto_common(SOCK_RAW, ETH_P_ALL, true, 0, true, true);
}

ZTEST(socket_packet, test_raw_sock_sendmsg_proto_wildcard)
{
	test_sendmsg_common(SOCK_RAW, ETH_P_ALL);
}

ZTEST(socket_packet, test_dgram_sock_sendto_proto_wildcard)
{
	test_sendto_common(SOCK_DGRAM, ETH_P_ALL, true, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendto_proto_match)
{
	test_sendto_common(SOCK_DGRAM, ETH_P_IP, true, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendmsg_proto_wildcard)
{
	test_sendmsg_common(SOCK_DGRAM, ETH_P_ALL);
}

ZTEST(socket_packet, test_dgram_sock_sendmsg_proto_match)
{
	test_sendmsg_common(SOCK_DGRAM, ETH_P_IP);
}

static void test_recv_common(int sock_type, int proto, bool success)
{
	struct sockaddr_ll ll_dst;
	uint16_t offset = 0;
	uint16_t pkt_len;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, ud.second, SOCK_RAW, 0);
	prepare_test_packet(SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	/* Receiving sock */
	setup_packet_socket(&packet_sock_2, ud.first, sock_type, htons(proto));
	bind_packet_socket(packet_sock_2, ud.first);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct sockaddr *)&ll_dst,
			   sizeof(struct sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	if (sock_type == SOCK_DGRAM) {
		offset = sizeof(struct net_eth_hdr);
		pkt_len -= sizeof(struct net_eth_hdr);
	}

	ret = zsock_recv(packet_sock_2, rx_buf, sizeof(rx_buf), 0);
	if (success) {
		zassert_not_equal(ret, -1, "Failed to receive packet (%d)", errno);
		zassert_equal(ret, pkt_len,
			     "Invalid data size received (%d, expected %d)",
			      ret, pkt_len);
		zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len,
				  "Invalid payload received");
	} else {
		zassert_equal(ret, -1, "Recv should fail");
		zassert_equal(errno, EAGAIN, "Wrong errno");
	}
}

ZTEST(socket_packet, test_raw_sock_recv_no_proto)
{
	test_recv_common(SOCK_RAW, 0, false);
}

ZTEST(socket_packet, test_dgram_sock_recv_no_proto)
{
	test_recv_common(SOCK_DGRAM, 0, false);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_match)
{
	test_recv_common(SOCK_DGRAM, ETH_P_IP, true);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_mismatch)
{
	test_recv_common(SOCK_DGRAM, ETH_P_IPV6, false);
}

ZTEST(socket_packet, test_raw_sock_recv_proto_wildcard)
{
	test_recv_common(SOCK_RAW, ETH_P_ALL, true);
}

static void test_recvfrom_common(int sock_type, int proto)
{
	struct sockaddr_ll ll_dst;
	struct sockaddr_ll ll_rx = { 0 };
	socklen_t addrlen = sizeof(ll_rx);
	uint16_t pkt_len;
	uint16_t offset = 0;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, ud.second, SOCK_RAW, 0);
	prepare_test_packet(SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	/* Receiving sock */
	setup_packet_socket(&packet_sock_2, ud.first, sock_type, htons(proto));
	bind_packet_socket(packet_sock_2, ud.first);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct sockaddr *)&ll_dst,
			   sizeof(struct sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	if (sock_type == SOCK_DGRAM) {
		offset = sizeof(struct net_eth_hdr);
	}

	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf), 0,
			     (struct sockaddr *)&ll_rx, &addrlen);
	zassert_not_equal(ret, -1, "Failed to receive packet (%d)", errno);
	zassert_equal(ret, pkt_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len,
			  "Invalid payload received");
	zassert_equal(addrlen, sizeof(struct sockaddr_ll),
		      "Invalid address length (%u)", addrlen);
	zassert_equal(ll_rx.sll_family, AF_PACKET, "Invalid family");
	zassert_equal(ll_rx.sll_protocol, htons(ETH_P_IP), "Invalid protocol");
	zassert_equal(ll_rx.sll_ifindex, net_if_get_by_iface(ud.first),
		      "Invalid interface");
	zassert_equal(ll_rx.sll_hatype, ARPHRD_ETHER, "Invalid hardware type");
	zassert_equal(ll_rx.sll_pkttype, PACKET_OTHERHOST, "Invalid packet type");
	zassert_equal(ll_rx.sll_halen, NET_ETH_ADDR_LEN, "Invalid address length");
	zassert_mem_equal(ll_rx.sll_addr, &lladdr2, NET_ETH_ADDR_LEN,
			  "Invalid address");
}

ZTEST(socket_packet, test_raw_sock_recvfrom_proto_wildcard)
{
	test_recvfrom_common(SOCK_RAW, ETH_P_ALL);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_wildcard)
{
	test_recv_common(SOCK_DGRAM, ETH_P_ALL, true);
}

ZTEST(socket_packet, test_dgram_sock_recvfrom_proto_wildcard)
{
	test_recvfrom_common(SOCK_RAW, ETH_P_ALL);
}

ZTEST(socket_packet, test_raw_dgram_udp_socks_recv)
{
	struct sockaddr_in ip_src;
	struct sockaddr_ll ll_dst;
	uint8_t offset = 0;
	uint16_t pkt_len;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, ud.second, SOCK_RAW, 0);
	prepare_test_packet(SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	/* Receiving sockets */
	setup_packet_socket(&packet_sock_2, ud.first, SOCK_RAW, htons(ETH_P_ALL));
	bind_packet_socket(packet_sock_2, ud.first);
	setup_packet_socket(&packet_sock_3, ud.first, SOCK_DGRAM, htons(ETH_P_ALL));
	bind_packet_socket(packet_sock_3, ud.first);
	prepare_udp_socket(&udp_sock_1, &ip_src, DST_PORT);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct sockaddr *)&ll_dst,
			   sizeof(struct sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	/* All 3 sockets should get their packets */
	ret = zsock_recv(packet_sock_2, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive RAW packet (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data size received (%d, expected %d)",
		      ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf, pkt_len, "Invalid payload received");

	offset += sizeof(struct net_eth_hdr);
	pkt_len -= sizeof(struct net_eth_hdr);

	ret = zsock_recv(packet_sock_3, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive DGRAM packet (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data size received (%d, expected %d)",
		      ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len, "Invalid payload received");

	offset += NET_IPV4UDPH_LEN;
	pkt_len -= NET_IPV4UDPH_LEN;

	ret = zsock_recv(udp_sock_1, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
	zassert_equal(ret, pkt_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len, "Invalid payload received");
}

static void test_sockets_close(void)
{
	if (packet_sock_1 >= 0) {
		(void)zsock_close(packet_sock_1);
		packet_sock_1 = -1;
	}

	if (packet_sock_2 >= 0) {
		(void)zsock_close(packet_sock_2);
		packet_sock_2 = -1;
	}

	if (packet_sock_3 >= 0) {
		(void)zsock_close(packet_sock_3);
		packet_sock_3 = -1;
	}

	if (udp_sock_1 >= 0) {
		(void)zsock_close(udp_sock_1);
		udp_sock_1 = -1;
	}

	if (udp_sock_2 >= 0) {
		(void)zsock_close(udp_sock_2);
		udp_sock_2 = -1;
	}
}

static void test_after(void *arg)
{
	ARG_UNUSED(arg);

	memset(&rx_buf, 0, sizeof(rx_buf));
	memset(&tx_buf, 0, sizeof(tx_buf));

	test_sockets_close();
}

static void *test_setup(void)
{
	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	return NULL;
}

ZTEST_SUITE(socket_packet, NULL, test_setup, NULL, test_after, NULL);
