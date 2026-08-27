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

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ptp_time.h>

/* This test suite verifies that NET_AF_PACKET sockets behave according to well known behaviors.
 * Note, that this is not well standardized and relies on behaviors known from Linux or FreeBSD.
 *
 * Sending data (TX)
 *
 *   * (NET_AF_PACKET, NET_SOCK_RAW, 0) - The packet contains already a valid L2 header:
 *     - test_raw_sock_sendto_no_proto_bound
 *     - test_raw_sock_sendto_no_proto_unbound
 *     - test_raw_sock_sendto_no_proto_unbound_no_iface
 *     - test_raw_sock_sendto_no_proto_unbound_no_addr
 *     - test_raw_sock_sendmsg_no_proto
 *
 *   * (NET_AF_PACKET, NET_SOCK_DGRAM, 0) - User needs to supply `struct net_sockaddr_ll` with
 *     all the relevant fields filled so that L2 header can be constructed:
 *     - test_dgram_sock_sendto_no_proto_bound
 *     - test_dgram_sock_sendto_no_proto_unbound
 *     - test_dgram_sock_sendto_no_proto_unbound_no_iface
 *     - test_dgram_sock_sendto_no_proto_unbound_no_addr
 *     - test_dgram_sock_sendmsg_no_proto
 *
 *   * (NET_AF_PACKET, NET_SOCK_RAW, <protocol>) - The packet contains already a valid
 *     L2 header. Not mentioned in packet(7) but as the L2 header needs to be
 *     supplied by the user, the protocol value is ignored:
 *     - test_raw_sock_sendto_proto_wildcard
 *     - test_raw_sock_sendmsg_proto_wildcard
 *
 *   * (NET_AF_PACKET, NET_SOCK_DGRAM, <protocol>) -  L2 header is constructed according
 *     to protocol and `struct net_sockaddr_ll`:
 *     - test_dgram_sock_sendto_proto_wildcard
 *     - test_dgram_sock_sendto_proto_match
 *     - test_dgram_sock_sendmsg_proto_wildcard
 *     - test_dgram_sock_sendmsg_proto_match
 *
 * Receiving data (RX)
 *
 *   * (NET_AF_PACKET, NET_SOCK_RAW, 0) - The packet is dropped when received by this socket.
 *     See https://man7.org/linux/man-pages/man7/packet.7.html:
 *     - test_raw_sock_recv_no_proto
 *
 *   * (NET_AF_PACKET, NET_SOCK_DGRAM, 0) - The packet is dropped when received by this socket.
 *     See https://man7.org/linux/man-pages/man7/packet.7.html
 *     - test_dgram_sock_recv_no_proto
 *
 *   * (NET_AF_PACKET, NET_SOCK_RAW, <protocol>) - The packet is received for a given protocol
 *     only. The L2 header is not removed from the data:
 *     - NOT SUPPORTED
 *
 *   * (NET_AF_PACKET, NET_SOCK_DGRAM, <protocol>) - The packet is received for a given protocol
 *     only. The L2 header is removed from the data:
 *     - test_dgram_sock_recv_proto_match
 *     - test_dgram_sock_recv_proto_mismatch
 *
 *   * (NET_AF_PACKET, NET_SOCK_RAW, net_htons(ETH_P_ALL)) - The packet is received for all
 *    protocols. The L2 header is not removed from the data:
 *     - test_raw_sock_recv_proto_wildcard
 *     - test_raw_sock_recv_proto_wildcard_bound_other_iface
 *     - test_raw_sock_recvfrom_proto_wildcard
 *     - test_raw_sock_recvfrom_proto_wildcard_unbound
 *
 *   * (NET_AF_PACKET, NET_SOCK_DGRAM, net_htons(ETH_P_ALL)) - The packet is received for all
 *     protocols. The L2 header is removed from the data:
 *     - test_dgram_sock_recv_proto_wildcard
 *     - test_dgram_sock_recv_proto_wildcard_bound_other_iface
 *     - test_dgram_sock_recvfrom_proto_wildcard
 *     - test_dgram_sock_recvfrom_proto_wildcard_unbound
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
static struct net_in_addr fake_src = { { { 192, 0, 2, 2 } } };
static struct net_ptp_time test_rx_timestamp = {
	.second = 1234,
	.nanosecond = 567890123,
};
static bool test_rx_timestamp_marked = true;

static uint8_t lladdr1[] = { 0x02, 0x01, 0x01, 0x01, 0x01, 0x01 };
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

/* Last multicast address the Ethernet L2 programmed to the receive filter
 * of the device.
 */
static struct {
	struct net_if *iface;
	struct net_eth_addr mac_address;
	enum ethernet_filter_type type;
	bool set;
	int count;
} eth_filter_data;

/* Value that the fake device returns for a receive filter request */
static int eth_filter_ret;

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *recv_pkt;
	int ret;
	struct net_if *target_iface;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	DBG("Sending data (%d bytes) to iface %d\n",
	    net_pkt_get_len(pkt), net_if_get_by_iface(net_pkt_iface(pkt)));

	if (memcmp(pkt->frags->data, lladdr1, sizeof(lladdr1)) == 0) {
		target_iface = eth_fake_data1.iface;
	} else if (memcmp(pkt->frags->data, lladdr2, sizeof(lladdr2)) == 0) {
		target_iface = eth_fake_data2.iface;
	} else {
		return 0;
	}

	recv_pkt = net_pkt_rx_clone(pkt, K_NO_WAIT);

	net_pkt_set_iface(recv_pkt, target_iface);
	net_pkt_set_timestamp(recv_pkt, &test_rx_timestamp);
	net_pkt_set_rx_timestamping(recv_pkt, test_rx_timestamp_marked);

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
		struct net_in_addr addr;

		if (net_addr_pton(NET_AF_INET, ctx->ip_address, &addr) == 0) {
			net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
		}
	}

	ethernet_init(iface);
}

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev,
						       struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_HW_FILTERING;
}

static int eth_fake_set_config(const struct device *dev, struct net_if *iface,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	ARG_UNUSED(dev);

	if (type != ETHERNET_CONFIG_TYPE_FILTER) {
		return -ENOTSUP;
	}

	eth_filter_data.iface = iface;
	eth_filter_data.mac_address = config->filter.mac_address;
	eth_filter_data.type = config->filter.type;
	eth_filter_data.set = config->filter.set;
	eth_filter_data.count++;

	return eth_filter_ret;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.get_capabilities = eth_fake_get_capabilities,
	.set_config = eth_fake_set_config,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(eth_fake1, "eth_fake1", NULL, NULL, &eth_fake_data1, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake2, "eth_fake2", NULL, NULL, &eth_fake_data2, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

static void setup_packet_socket(int *sock, int type, int proto)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	*sock = zsock_socket(NET_AF_PACKET, type, proto);
	zassert_true(*sock >= 0, "Cannot create packet socket (%d)", -errno);

	ret = zsock_setsockopt(*sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &optval,
			       sizeof(optval));
	zassert_ok(ret, "setsockopt failed (%d)", errno);
}

static void bind_packet_socket(int sock, struct net_if *iface)
{
	struct net_sockaddr_ll addr;
	int ret;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = (iface == NULL) ? 0 : net_if_get_by_iface(iface);
	addr.sll_family = NET_AF_PACKET;

	ret = zsock_bind(sock, (struct net_sockaddr *)&addr, sizeof(addr));
	zassert_ok(ret, "Cannot bind packet socket (%d)", -errno);
}

static void prepare_packet_socket(int *sock, struct net_if *iface, int type,
				  int proto)
{
	setup_packet_socket(sock, type, proto);
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
static void prepare_udp_socket(int *sock, struct net_sockaddr_in *sockaddr, uint16_t local_port)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	*sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(*sock >= 0, "Cannot create DGRAM (UDP) socket (%d)", *sock);

	sockaddr->sin_family = NET_AF_INET;
	sockaddr->sin_port = net_htons(local_port);
	ret = zsock_inet_pton(NET_AF_INET, IPV4_ADDR, &sockaddr->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* Bind UDP socket to local port */
	ret = zsock_bind(*sock, (struct net_sockaddr *) sockaddr, sizeof(*sockaddr));
	zassert_equal(ret, 0, "Cannot bind DGRAM (UDP) socket (%d)", -errno);

	ret = zsock_setsockopt(*sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &optval,
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
	struct net_sockaddr_ll src;
	struct net_sockaddr_in sockaddr;
	int ret;
	net_socklen_t addrlen;
	ssize_t sent = 0;

	prepare_packet_socket(&packet_sock_1, ud.first, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, NET_SOCK_RAW, net_htons(ETH_P_ALL));

	/* Prepare UDP socket which will read data */
	prepare_udp_socket(&udp_sock_1, &sockaddr, DST_PORT);

	/* Prepare UDP socket from which data are going to be sent */
	prepare_udp_socket(&udp_sock_2, &sockaddr, SRC_PORT);
	/* Properly set destination port for UDP packet */
	sockaddr.sin_port = net_htons(DST_PORT);

	/*
	 * Send UDP datagram to us - as check_ip_addr() in net_send_data()
	 * returns 1 - the packet is processed immediately in the net stack
	 */
	sent = zsock_sendto(udp_sock_2, data_to_send, sizeof(data_to_send),
			    0, (struct net_sockaddr *)&sockaddr, sizeof(sockaddr));
	zassert_equal(sent, sizeof(data_to_send), "sendto failed");

	memset(&data_to_receive, 0, sizeof(data_to_receive));
	errno = 0;

	/* Check if UDP packets can be read after being sent */
	addrlen = sizeof(sockaddr);
	ret = zsock_recvfrom(udp_sock_1, data_to_receive, sizeof(data_to_receive),
			     0, (struct net_sockaddr *)&sockaddr, &addrlen);
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
			     (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(data_to_send) + HDR_SIZE,
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(data_to_send), -errno);
	zassert_mem_equal(&data_to_receive[HDR_SIZE], data_to_send,
			  sizeof(data_to_send),
			  "Sent and received buffers do not match");
}

ZTEST(socket_packet, test_packet_sockets)
{
	prepare_packet_socket(&packet_sock_1, ud.first, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, NET_SOCK_RAW, net_htons(ETH_P_ALL));
}

ZTEST(socket_packet, test_packet_sockets_dgram)
{
	net_socklen_t addrlen = sizeof(struct net_sockaddr_ll);
	struct net_sockaddr_ll dst, src;
	int ret;

	prepare_packet_socket(&packet_sock_1, ud.first, NET_SOCK_DGRAM, net_htons(ETH_P_TSN));
	prepare_packet_socket(&packet_sock_2, ud.second, NET_SOCK_DGRAM, net_htons(ETH_P_TSN));

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = NET_AF_PACKET;
	dst.sll_protocol = net_htons(ETH_P_TSN);
	memcpy(dst.sll_addr, lladdr1, sizeof(lladdr1));

	ret = zsock_sendto(packet_sock_2, test_payload, sizeof(test_payload), 0,
			   (const struct net_sockaddr *)&dst, sizeof(struct net_sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf), 0,
	(struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));
	errno = 0;
	ret = zsock_recvfrom(packet_sock_1, rx_buf, sizeof(rx_buf),
			     0, (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(test_payload), -errno);

	zassert_equal(addrlen, sizeof(struct net_sockaddr_ll),
		      "Invalid address length (%d)", addrlen);

	struct net_sockaddr_ll src_expected = {
		.sll_family = NET_AF_PACKET,
		.sll_protocol = dst.sll_protocol,
		.sll_ifindex = net_if_get_by_iface(ud.first),
		.sll_pkttype = NET_PACKET_OTHERHOST,
		.sll_hatype = NET_ARPHRD_ETHER,
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
			   (const struct net_sockaddr *)&dst, sizeof(struct net_sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	memset(&src, 0, sizeof(src));

	ret = zsock_recvfrom(packet_sock_1, rx_buf, sizeof(rx_buf), 0,
			     (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf),
			     0, (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload), "Cannot receive all data (%d)",
		      -errno);
	zassert_equal(addrlen, sizeof(struct net_sockaddr_ll),
		      "Invalid address length (%d)", addrlen);

	src_expected = (struct net_sockaddr_ll){
		.sll_family = NET_AF_PACKET,
		.sll_protocol = dst.sll_protocol,
		.sll_ifindex = net_if_get_by_iface(ud.second),
		.sll_pkttype = NET_PACKET_OTHERHOST,
		.sll_hatype = NET_ARPHRD_ETHER,
		.sll_halen = ARRAY_SIZE(lladdr2),
		.sll_addr = {0},
	};
	memcpy(&src_expected.sll_addr, lladdr2, ARRAY_SIZE(lladdr2));
	zassert_mem_equal(&src, &src_expected, addrlen, "Invalid source address");

	zassert_mem_equal(test_payload, rx_buf, sizeof(test_payload),
			  "Data mismatch");

	/* Send specially crafted payload to mimic IPv4 and IPv6 length field,
	 * to check correct length returned.
	 */
	uint8_t payload_ip_length[64], receive_ip_length[64];

	memset(payload_ip_length, 0, sizeof(payload_ip_length));
	/* Set ipv4 and ipv6 length fields to represent IP payload with the
	 * length of 1 byte.
	 */
	payload_ip_length[3] = 21;
	payload_ip_length[5] = 1;

	ret = zsock_sendto(packet_sock_2, payload_ip_length, sizeof(payload_ip_length), 0,
			   (const struct net_sockaddr *)&dst, sizeof(struct net_sockaddr_ll));
	zassert_equal(ret, sizeof(payload_ip_length), "Cannot send all data (%d)", -errno);

	memset(&src, 0, sizeof(src));
	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, receive_ip_length, sizeof(receive_ip_length), 0,
			     (struct net_sockaddr *)&src, &addrlen);

	zassert_equal(ret, ARRAY_SIZE(payload_ip_length), "Cannot receive all data (%d)", -errno);
	zassert_mem_equal(payload_ip_length, receive_ip_length, sizeof(payload_ip_length),
			  "Data mismatch");
}

ZTEST(socket_packet, test_raw_and_dgram_socket_exchange)
{
	net_socklen_t addrlen = sizeof(struct net_sockaddr_ll);
	struct net_sockaddr_ll dst, src;
	int ret;
	const uint8_t expected_payload_raw[] = {
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Dst ll addr */
		0x02, 0x01, 0x01, 0x01, 0x01, 0x01, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};
	const uint8_t send_payload_raw[] = {
		0x02, 0x01, 0x01, 0x01, 0x01, 0x01, /* Dst ll addr */
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};

	prepare_packet_socket(&packet_sock_1, ud.first, NET_SOCK_DGRAM, net_htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, NET_SOCK_RAW, net_htons(ETH_P_ALL));

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = NET_AF_PACKET;
	dst.sll_protocol = net_htons(ETH_P_IP);
	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr1));

	/* NET_SOCK_DGRAM to NET_SOCK_RAW */

	ret = zsock_sendto(packet_sock_1, test_payload, sizeof(test_payload), 0,
			   (const struct net_sockaddr *)&dst, sizeof(struct net_sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10); /* Let the packet enter the system */
	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf),
			     0, (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(expected_payload_raw), -errno);
	zassert_mem_equal(expected_payload_raw, rx_buf,
			  sizeof(expected_payload_raw), "Data mismatch");

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = NET_AF_PACKET;
	dst.sll_protocol = net_htons(ETH_P_IP);

	/* NET_SOCK_RAW to NET_SOCK_DGRAM */

	ret = zsock_sendto(packet_sock_2, send_payload_raw, sizeof(send_payload_raw), 0,
			   (const struct net_sockaddr *)&dst, sizeof(struct net_sockaddr_ll));
	zassert_equal(ret, sizeof(send_payload_raw), "Cannot send all data (%d)",
		      -errno);

	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_1, rx_buf, sizeof(rx_buf),
			     0, (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(test_payload), "Cannot receive all data (%d)",
		      -errno);
	zassert_mem_equal(test_payload, rx_buf, sizeof(test_payload),
			  "Data mismatch");
}

ZTEST(socket_packet, test_raw_and_dgram_socket_recv)
{
	net_socklen_t addrlen = sizeof(struct net_sockaddr_ll);
	struct net_sockaddr_ll dst, src;
	int ret;
	const uint8_t expected_payload_raw[] = {
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Dst ll addr */
		0x02, 0x01, 0x01, 0x01, 0x01, 0x01, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};

	prepare_packet_socket(&packet_sock_1, ud.first, NET_SOCK_DGRAM, net_htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_2, ud.second, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	prepare_packet_socket(&packet_sock_3, ud.second, NET_SOCK_DGRAM, net_htons(ETH_P_ALL));

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = NET_AF_PACKET;
	dst.sll_protocol = net_htons(ETH_P_IP);
	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr1));

	ret = zsock_sendto(packet_sock_1, test_payload, sizeof(test_payload), 0,
			   (const struct net_sockaddr *)&dst, sizeof(struct net_sockaddr_ll));
	zassert_equal(ret, sizeof(test_payload), "Cannot send all data (%d)",
		      -errno);

	memset(&src, 0, sizeof(src));

	/* Both NET_SOCK_DGRAM to NET_SOCK_RAW sockets should receive packet. */

	errno = 0;
	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf),
			     0, (struct net_sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(expected_payload_raw), -errno);

	zassert_mem_equal(expected_payload_raw, rx_buf,
			  sizeof(expected_payload_raw), "Data mismatch");

	memset(&src, 0, sizeof(src));

	errno = 0;
	ret = zsock_recvfrom(packet_sock_3, rx_buf, sizeof(rx_buf),
			     0, (struct net_sockaddr *)&src, &addrlen);
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

	if (sock_type == NET_SOCK_RAW) {
		struct net_eth_hdr *eth = (struct net_eth_hdr *)tx_buf;

		offset += sizeof(struct net_eth_hdr);
		zassert_not_null(ll_src, "NULL LL src");
		zassert_not_null(ll_dst, "NULL LL dst");
		zassert_true(offset <= sizeof(tx_buf), "Packet too long");

		memcpy(&eth->dst, ll_dst, sizeof(eth->dst));
		memcpy(&eth->src, ll_src, sizeof(eth->src));
		eth->type = net_htons(proto);
	}

	if (proto == ETH_P_IP) {
		struct net_in_addr addr;
		struct net_ipv4_hdr *ipv4 =
			(struct net_ipv4_hdr *)(tx_buf + offset);
		struct net_udp_hdr *udp =
			(struct net_udp_hdr *)(tx_buf + offset + NET_IPV4H_LEN);

		offset += NET_IPV4UDPH_LEN;
		zassert_true(offset <= sizeof(tx_buf), "Packet too long");
		zassert_ok(net_addr_pton(NET_AF_INET, IPV4_ADDR, &addr), "Address parse failed");

		/* Prepare IPv4 header */
		ipv4->vhl = 0x45;
		ipv4->len = net_htons(sizeof(test_payload) + NET_IPV4UDPH_LEN);
		ipv4->ttl = 64;
		ipv4->proto = NET_IPPROTO_UDP;
		ipv4->chksum = TEST_IPV4_CHKSUM;
		memcpy(ipv4->src, &fake_src, sizeof(ipv4->src));
		memcpy(ipv4->dst, &addr, sizeof(ipv4->dst));

		/* Prepare UDP header */
		udp->src_port = net_htons(SRC_PORT);
		udp->dst_port = net_htons(DST_PORT);
		udp->len = net_htons(sizeof(test_payload) + NET_UDPH_LEN);
		udp->chksum = TEST_UDP_CHKSUM;
	}

	zassert_true(offset + sizeof(test_payload) <= sizeof(tx_buf), "Packet too long");
	memcpy(tx_buf + offset, test_payload, sizeof(test_payload));

	offset += sizeof(test_payload);
	*pkt_len = offset;
}

static void prepare_test_dst_lladdr(struct net_sockaddr_ll *ll_dst, uint16_t proto,
				    uint8_t *ll_addr, struct net_if *iface)
{
	memset(ll_dst, 0, sizeof(struct net_sockaddr_ll));

	ll_dst->sll_family = NET_AF_PACKET;
	ll_dst->sll_protocol = net_htons(proto);
	memcpy(ll_dst->sll_addr, ll_addr, NET_ETH_ADDR_LEN);

	if (iface != NULL) {
		ll_dst->sll_ifindex = net_if_get_by_iface(iface);
	}
}

static void test_sendto_common(int sock_type, int proto, bool do_bind,
			       int custom_dst_iface, bool set_dst_addr,
			       bool success)
{
	struct net_sockaddr_in ip_src;
	struct net_sockaddr_ll ll_dst;
	uint16_t pkt_len;
	int ret;

	setup_packet_socket(&packet_sock_1, sock_type, net_htons(proto));
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
			   set_dst_addr ? (struct net_sockaddr *)&ll_dst : NULL,
			   set_dst_addr ? sizeof(struct net_sockaddr_ll) : 0);
	if (success) {
		zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
		zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

		ret = zsock_recv(udp_sock_1, rx_buf, sizeof(rx_buf), 0);
		zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
		zassert_equal(ret, sizeof(test_payload),
			     "Invalid data size received (%d, expected %zu)",
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
	test_sendto_common(NET_SOCK_RAW, 0, true, 0, true, true);
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_unbound)
{
	test_sendto_common(NET_SOCK_RAW, 0, false, 0, true, true);
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_unbound_no_iface)
{
	test_sendto_common(NET_SOCK_RAW, 0, false, 10, true, false);
}

ZTEST(socket_packet, test_raw_sock_sendto_no_proto_unbound_no_addr)
{
	test_sendto_common(NET_SOCK_RAW, 0, false, 0, false, false);
}

static void test_sendmsg_common(int sock_type, int proto)
{
	struct net_sockaddr_in ip_src;
	struct net_sockaddr_ll ll_dst;
	struct net_iovec io_vector;
	struct net_msghdr msg = { 0 };
	uint16_t pkt_len;
	int ret;

	setup_packet_socket(&packet_sock_1, sock_type, net_htons(proto));
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
		      "Invalid data size received (%d, expected %zu)",
		      ret, sizeof(test_payload));
	zassert_mem_equal(rx_buf, test_payload, sizeof(test_payload),
			  "Invalid payload received");
}

ZTEST(socket_packet, test_raw_sock_sendmsg_no_proto)
{
	test_sendmsg_common(NET_SOCK_RAW, 0);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_bound)
{
	test_sendto_common(NET_SOCK_DGRAM, 0, true, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_unbound)
{
	test_sendto_common(NET_SOCK_DGRAM, 0, false, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_unbound_no_iface)
{
	test_sendto_common(NET_SOCK_DGRAM, 0, false, 10, true, false);
}

ZTEST(socket_packet, test_dgram_sock_sendto_no_proto_unbound_no_addr)
{
	test_sendto_common(NET_SOCK_DGRAM, 0, false, 0, false, false);
}

ZTEST(socket_packet, test_dgram_sock_sendmsg_no_proto)
{
	test_sendmsg_common(NET_SOCK_DGRAM, 0);
}

ZTEST(socket_packet, test_raw_sock_sendto_proto_wildcard)
{
	test_sendto_common(NET_SOCK_RAW, ETH_P_ALL, true, 0, true, true);
}

ZTEST(socket_packet, test_raw_sock_sendmsg_proto_wildcard)
{
	test_sendmsg_common(NET_SOCK_RAW, ETH_P_ALL);
}

ZTEST(socket_packet, test_dgram_sock_sendto_proto_wildcard)
{
	test_sendto_common(NET_SOCK_DGRAM, ETH_P_ALL, true, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendto_proto_match)
{
	test_sendto_common(NET_SOCK_DGRAM, ETH_P_IP, true, 0, true, true);
}

ZTEST(socket_packet, test_dgram_sock_sendmsg_proto_wildcard)
{
	test_sendmsg_common(NET_SOCK_DGRAM, ETH_P_ALL);
}

ZTEST(socket_packet, test_dgram_sock_sendmsg_proto_match)
{
	test_sendmsg_common(NET_SOCK_DGRAM, ETH_P_IP);
}

static void test_recv_common(int sock_type, int proto, bool success)
{
	struct net_sockaddr_ll ll_dst;
	uint16_t offset = 0;
	uint16_t pkt_len;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, 0);
	prepare_test_packet(NET_SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	/* Receiving sock */
	setup_packet_socket(&packet_sock_2, sock_type, net_htons(proto));
	bind_packet_socket(packet_sock_2, ud.first);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	if (sock_type == NET_SOCK_DGRAM) {
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

static void test_recvmsg_timestamping_common(size_t cmsgbuf_len, bool expect_timestamp,
					     bool expect_trunc)
{
	struct net_sockaddr_ll ll_dst;
	struct net_iovec io_vector = {
		.iov_base = rx_buf,
		.iov_len = sizeof(rx_buf),
	};
	struct net_msghdr msg = {
		.msg_iov = &io_vector,
		.msg_iovlen = 1,
	};
	uint8_t timestamping = ZSOCK_SOF_TIMESTAMPING_RX_HARDWARE;
	uint16_t offset = sizeof(struct net_eth_hdr);
	uint16_t pkt_len;
	int ret;
	union {
		struct net_cmsghdr hdr;
		uint8_t buf[NET_CMSG_SPACE(sizeof(struct net_ptp_time))];
	} cmsgbuf = {0};

	__ASSERT_NO_MSG(cmsgbuf_len <= sizeof(cmsgbuf.buf));

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, 0);
	prepare_test_packet(NET_SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);

	setup_packet_socket(&packet_sock_2, NET_SOCK_DGRAM, net_htons(ETH_P_ALL));
	bind_packet_socket(packet_sock_2, ud.first);

	ret = zsock_setsockopt(packet_sock_2, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING,
			       &timestamping, sizeof(timestamping));
	zassert_equal(ret, 0, "timestamping setsockopt failed (%d)", errno);

	msg.msg_control = cmsgbuf.buf;
	msg.msg_controllen = cmsgbuf_len;

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0, (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	pkt_len -= offset;

	ret = zsock_recvmsg(packet_sock_2, &msg, 0);
	zassert_not_equal(ret, -1, "Failed to receive packet (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data size received (%d, expected %d)", ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len, "Invalid payload received");

	if (!expect_timestamp) {
		zassert_false(msg.msg_flags & ZSOCK_MSG_CTRUNC,
			      "Control data should not have been truncated");
		zassert_equal(msg.msg_controllen, 0, "Unexpected control data length %zu",
			      msg.msg_controllen);
		zassert_is_null(NET_CMSG_FIRSTHDR(&msg), "Unexpected control header");
		return;
	}

	if (expect_trunc) {
		zassert_true(msg.msg_flags & ZSOCK_MSG_CTRUNC,
			     "Control data should have been truncated");
		zassert_equal(msg.msg_controllen, 0, "Unexpected control data length %zu",
			      msg.msg_controllen);
		zassert_is_null(NET_CMSG_FIRSTHDR(&msg), "Unexpected control header");
		return;
	}

	struct net_cmsghdr *cmsg = NET_CMSG_FIRSTHDR(&msg);
	struct net_ptp_time timestamp;

	zassert_not_null(cmsg, "Missing timestamp control message");
	zassert_equal(msg.msg_controllen, NET_CMSG_SPACE(sizeof(struct net_ptp_time)),
		      "Unexpected msg_controllen %zu", msg.msg_controllen);
	zassert_equal(cmsg->cmsg_level, ZSOCK_SOL_SOCKET, "Unexpected cmsg level");
	zassert_equal(cmsg->cmsg_type, ZSOCK_SO_TIMESTAMPING, "Unexpected cmsg type");
	zassert_equal(cmsg->cmsg_len, NET_CMSG_LEN(sizeof(struct net_ptp_time)),
		      "Unexpected cmsg length %u", cmsg->cmsg_len);

	memcpy(&timestamp, NET_CMSG_DATA(cmsg), sizeof(timestamp));
	zassert_mem_equal(&timestamp, &test_rx_timestamp, sizeof(timestamp),
			  "Unexpected timestamp payload");
}

ZTEST(socket_packet, test_raw_sock_recv_no_proto)
{
	test_recv_common(NET_SOCK_RAW, 0, false);
}

ZTEST(socket_packet, test_dgram_sock_recv_no_proto)
{
	test_recv_common(NET_SOCK_DGRAM, 0, false);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_match)
{
	test_recv_common(NET_SOCK_DGRAM, ETH_P_IP, true);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_mismatch)
{
	test_recv_common(NET_SOCK_DGRAM, ETH_P_IPV6, false);
}

ZTEST(socket_packet, test_raw_sock_recv_proto_wildcard)
{
	test_recv_common(NET_SOCK_RAW, ETH_P_ALL, true);
}

static void validate_recvfrom_addr(struct net_sockaddr_ll *ll_rx, net_socklen_t addrlen,
				   int iface, uint8_t *lladdr)
{
	zassert_equal(addrlen, sizeof(struct net_sockaddr_ll),
		      "Invalid address length (%u)", addrlen);
	zassert_equal(ll_rx->sll_family, NET_AF_PACKET, "Invalid family");
	zassert_equal(ll_rx->sll_protocol, net_htons(ETH_P_IP), "Invalid protocol");
	zassert_equal(ll_rx->sll_ifindex, iface, "Invalid interface");
	zassert_equal(ll_rx->sll_hatype, NET_ARPHRD_ETHER, "Invalid hardware type");
	zassert_equal(ll_rx->sll_pkttype, NET_PACKET_OTHERHOST, "Invalid packet type");
	zassert_equal(ll_rx->sll_halen, NET_ETH_ADDR_LEN, "Invalid address length");
	zassert_mem_equal(ll_rx->sll_addr, lladdr, NET_ETH_ADDR_LEN, "Invalid address");
}

static bool recvfrom_addr_matches(struct net_sockaddr_ll *ll_rx, net_socklen_t addrlen,
				  int iface, uint8_t *lladdr)
{
	return addrlen == sizeof(struct net_sockaddr_ll) &&
	       ll_rx->sll_family == NET_AF_PACKET &&
	       ll_rx->sll_protocol == net_htons(ETH_P_IP) &&
	       ll_rx->sll_ifindex == iface &&
	       ll_rx->sll_hatype == NET_ARPHRD_ETHER &&
	       ll_rx->sll_pkttype == NET_PACKET_OTHERHOST &&
	       ll_rx->sll_halen == NET_ETH_ADDR_LEN &&
	       memcmp(ll_rx->sll_addr, lladdr, NET_ETH_ADDR_LEN) == 0;
}

static void test_recvfrom_common(int sock_type, int proto)
{
	struct net_sockaddr_ll ll_dst;
	struct net_sockaddr_ll ll_rx = { 0 };
	net_socklen_t addrlen = sizeof(ll_rx);
	uint16_t pkt_len;
	uint16_t offset = 0;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, 0);
	prepare_test_packet(NET_SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	/* Receiving sock */
	setup_packet_socket(&packet_sock_2, sock_type, net_htons(proto));
	bind_packet_socket(packet_sock_2, ud.first);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	if (sock_type == NET_SOCK_DGRAM) {
		offset = sizeof(struct net_eth_hdr);
	}

	pkt_len -= offset;

	ret = zsock_recvfrom(packet_sock_2, rx_buf, sizeof(rx_buf), 0,
			     (struct net_sockaddr *)&ll_rx, &addrlen);
	zassert_not_equal(ret, -1, "Failed to receive packet (%d)", errno);
	zassert_equal(ret, pkt_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len,
			  "Invalid payload received");
	zassert_equal(addrlen, sizeof(struct net_sockaddr_ll),
		      "Invalid address length (%u)", addrlen);
	validate_recvfrom_addr(&ll_rx, addrlen, net_if_get_by_iface(ud.first),
			       lladdr2);
}

ZTEST(socket_packet, test_raw_sock_recvfrom_proto_wildcard)
{
	test_recvfrom_common(NET_SOCK_RAW, ETH_P_ALL);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_wildcard)
{
	test_recv_common(NET_SOCK_DGRAM, ETH_P_ALL, true);
}

ZTEST(socket_packet, test_dgram_sock_recvmsg_timestamping)
{
	test_recvmsg_timestamping_common(NET_CMSG_SPACE(sizeof(struct net_ptp_time)), true, false);
}

ZTEST(socket_packet, test_dgram_sock_recvmsg_timestamping_truncated_control)
{
	test_recvmsg_timestamping_common(NET_CMSG_SPACE(sizeof(struct net_ptp_time)) - 1,
					 true, true);
}

ZTEST(socket_packet, test_dgram_sock_recvmsg_timestamping_unmarked)
{
	test_rx_timestamp_marked = false;
	test_recvmsg_timestamping_common(NET_CMSG_SPACE(sizeof(struct net_ptp_time)), false, false);
	test_rx_timestamp_marked = true;
}

ZTEST(socket_packet, test_dgram_sock_recvfrom_proto_wildcard)
{
	test_recvfrom_common(NET_SOCK_DGRAM, ETH_P_ALL);
}

static void test_recvfrom_unbound_round(int tx_sock, int rx_sock, int sock_type,
					uint8_t *src_addr, struct net_if *src_iface,
					uint8_t *dst_addr, struct net_if *dst_iface)
{
	uint16_t offset = (sock_type == NET_SOCK_DGRAM) ? sizeof(struct net_eth_hdr) : 0;
	struct net_sockaddr_ll ll_dst;
	struct net_sockaddr_ll ll_rx = { 0 };
	net_socklen_t last_addrlen = 0U;
	uint16_t pkt_len;
	int ret;
	int expected_iface = net_if_get_by_iface(dst_iface);

	prepare_test_packet(NET_SOCK_RAW, ETH_P_IP, src_addr, dst_addr, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, dst_addr, src_iface);

	ret = zsock_sendto(tx_sock, tx_buf, pkt_len, 0,
			   (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	zassert_true(pkt_len >= offset, "Packet shorter than expected L2 header");
	pkt_len -= offset;

	for (int attempt = 0; attempt < 4; attempt++) {
		net_socklen_t addrlen = sizeof(ll_rx);

		memset(&ll_rx, 0, sizeof(ll_rx));
		ret = zsock_recvfrom(rx_sock, rx_buf, sizeof(rx_buf), 0,
				     (struct net_sockaddr *)&ll_rx, &addrlen);
		last_addrlen = addrlen;
		zassert_not_equal(ret, -1, "Failed to receive packet (%d)", errno);

		if (ret == pkt_len &&
		    memcmp(rx_buf, tx_buf + offset, pkt_len) == 0 &&
		    recvfrom_addr_matches(&ll_rx, addrlen, expected_iface, src_addr)) {
			validate_recvfrom_addr(&ll_rx, addrlen, expected_iface, src_addr);
			return;
		}
	}

	zassert_equal(ret, pkt_len,
		      "Invalid data size received (%d, expected %d)",
		      ret, pkt_len);
	zassert_mem_equal(rx_buf, tx_buf + offset, pkt_len,
			  "Invalid payload received");
	validate_recvfrom_addr(&ll_rx, last_addrlen, expected_iface, src_addr);
}

static void test_recvfrom_common_unbound(int sock_type, bool bind_iface_0)
{
	struct net_sockaddr_in ip_src;
	struct net_sockaddr_ll ll_dst;
	static uint8_t dummy_lladdr[] = { 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	uint16_t pkt_len;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, 0);
	/* Receiving sock */
	setup_packet_socket(&packet_sock_2, sock_type, net_htons(ETH_P_ALL));

	/* Avoid ICMP destination-unreachable replies polluting the packet socket. */
	prepare_udp_socket(&udp_sock_1, &ip_src, DST_PORT);

	if (bind_iface_0) {
		bind_packet_socket(packet_sock_2, NULL);
	}

	/* Verify we get packet from iface 1 */
	test_recvfrom_unbound_round(packet_sock_1, packet_sock_2, sock_type,
				    lladdr2, ud.second, lladdr1, ud.first);

	/* Verify we get packet from iface 2 */
	test_recvfrom_unbound_round(packet_sock_1, packet_sock_2, sock_type,
				    lladdr1, ud.first, lladdr2, ud.second);

	/* Send some dummy data into the void on the "receiving" socket and make
	 * sure it doesn't get automatically "bound" to the target iface.
	 */
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, dummy_lladdr, ud.second);
	prepare_test_packet(sock_type, ETH_P_IP, lladdr2, dummy_lladdr, &pkt_len);
	ret = zsock_sendto(packet_sock_2, tx_buf, pkt_len, 0,
			   (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	/* And try to receive again. */
	/* Verify we get packet from iface 1 */
	test_recvfrom_unbound_round(packet_sock_1, packet_sock_2, sock_type,
				    lladdr2, ud.second, lladdr1, ud.first);

	/* Verify we get packet from iface 2 */
	test_recvfrom_unbound_round(packet_sock_1, packet_sock_2, sock_type,
				    lladdr1, ud.first, lladdr2, ud.second);
}

ZTEST(socket_packet, test_raw_sock_recvfrom_proto_wildcard_unbound)
{
	test_recvfrom_common_unbound(NET_SOCK_RAW, false);
}

ZTEST(socket_packet, test_dgram_sock_recvfrom_proto_wildcard_unbound)
{
	test_recvfrom_common_unbound(NET_SOCK_DGRAM, false);
}

ZTEST(socket_packet, test_raw_sock_recvfrom_proto_wildcard_bound_iface_0)
{
	test_recvfrom_common_unbound(NET_SOCK_RAW, true);
}

ZTEST(socket_packet, test_dgram_sock_recvfrom_proto_wildcard_bound_iface_0)
{
	test_recvfrom_common_unbound(NET_SOCK_DGRAM, true);
}

static void test_recv_common_bound_other_iface(int sock_type)
{
	struct net_sockaddr_ll ll_dst;
	struct net_sockaddr_in ip_src;
	uint16_t pkt_len;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, 0);
	prepare_test_packet(NET_SOCK_RAW, ETH_P_IP, lladdr1, lladdr2, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr2, ud.first);
	/* Receiving sock */
	setup_packet_socket(&packet_sock_2, sock_type, net_htons(ETH_P_ALL));
	bind_packet_socket(packet_sock_2, ud.first);
	prepare_udp_socket(&udp_sock_1, &ip_src, DST_PORT);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
	zassert_not_equal(ret, -1, "Failed to send (%d)", errno);
	zassert_equal(ret, pkt_len, "Invalid data length sent (%d/%d)", ret, pkt_len);

	/* Packet socket should not get the packet due to different binding */
	ret = zsock_recv(packet_sock_2, rx_buf, sizeof(rx_buf), 0);
	zassert_equal(ret, -1, "Recv should fail");
	zassert_equal(errno, EAGAIN, "Wrong errno");

	/* But UDP socket should get the packet just fine. */
	ret = zsock_recv(udp_sock_1, rx_buf, sizeof(rx_buf), 0);
	zassert_not_equal(ret, -1, "Failed to receive UDP packet (%d)", errno);
}

ZTEST(socket_packet, test_raw_sock_recv_proto_wildcard_bound_other_iface)
{
	test_recv_common_bound_other_iface(NET_SOCK_RAW);
}

ZTEST(socket_packet, test_dgram_sock_recv_proto_wildcard_bound_other_iface)
{
	test_recv_common_bound_other_iface(NET_SOCK_DGRAM);
}

ZTEST(socket_packet, test_raw_dgram_udp_socks_recv)
{
	struct net_sockaddr_in ip_src;
	struct net_sockaddr_ll ll_dst;
	uint8_t offset = 0;
	uint16_t pkt_len;
	int ret;

	/* Transmitting sock */
	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, 0);
	prepare_test_packet(NET_SOCK_RAW, ETH_P_IP, lladdr2, lladdr1, &pkt_len);
	prepare_test_dst_lladdr(&ll_dst, ETH_P_IP, lladdr1, ud.second);
	/* Receiving sockets */
	setup_packet_socket(&packet_sock_2, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	bind_packet_socket(packet_sock_2, ud.first);
	setup_packet_socket(&packet_sock_3, NET_SOCK_DGRAM, net_htons(ETH_P_ALL));
	bind_packet_socket(packet_sock_3, ud.first);
	prepare_udp_socket(&udp_sock_1, &ip_src, DST_PORT);

	ret = zsock_sendto(packet_sock_1, tx_buf, pkt_len, 0,
			   (struct net_sockaddr *)&ll_dst,
			   sizeof(struct net_sockaddr_ll));
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

/* Multicast group membership handling, i.e. the ZSOCK_PACKET_ADD_MEMBERSHIP
 * and ZSOCK_PACKET_DROP_MEMBERSHIP socket options.
 */

/* IPv4 all hosts (224.0.0.1) mapped to an Ethernet multicast address */
static const uint8_t mcast_lladdr[] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x01 };

/* Fill in a request for the mcast_lladdr group with the last address byte
 * replaced by group, so that tests can use several distinct groups.
 */
static void mcast_mreq_group(struct net_packet_mreq *mreq, struct net_if *iface,
			     uint8_t group)
{
	memset(mreq, 0, sizeof(*mreq));

	mreq->mr_ifindex = net_if_get_by_iface(iface);
	mreq->mr_type = NET_PACKET_MR_MULTICAST;
	mreq->mr_alen = sizeof(mcast_lladdr);
	memcpy(mreq->mr_address, mcast_lladdr, sizeof(mcast_lladdr));
	mreq->mr_address[sizeof(mcast_lladdr) - 1] = group;
}

/* What the device was told about its receive filter is how the tests see
 * that a membership change reached the L2 and the driver.
 */
static void mcast_mreq_init(struct net_packet_mreq *mreq, struct net_if *iface)
{
	mcast_mreq_group(mreq, iface, mcast_lladdr[sizeof(mcast_lladdr) - 1]);

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));
}

static int mcast_membership(int sock, int optname,
			    const struct net_packet_mreq *mreq)
{
	return zsock_setsockopt(sock, ZSOCK_SOL_PACKET, optname, mreq,
				sizeof(*mreq));
}

struct mcast_foreach_data {
	struct net_if *iface;
	uint8_t last_byte;
	int count;
};

static void mcast_foreach_cb(struct net_if *iface,
			     const struct net_eth_mcast_addr *addr,
			     void *user_data)
{
	struct mcast_foreach_data *data = user_data;

	data->iface = iface;
	data->last_byte = addr->addr.addr[sizeof(mcast_lladdr) - 1];
	data->count++;
}

/* How many L2 multicast addresses the interface is listening to */
static int mcast_addr_count(struct net_if *iface)
{
	struct mcast_foreach_data data = { 0 };

	net_eth_mcast_addr_foreach(iface, mcast_foreach_cb, &data);

	return data.count;
}

ZTEST(socket_packet, test_packet_sock_add_drop_membership)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_ok(ret, "Cannot add membership (%d)", errno);

	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_equal(eth_filter_data.iface, ud.first,
		      "Filter programmed for a wrong interface");
	zassert_true(eth_filter_data.set, "Filter not enabled");
	zassert_equal(eth_filter_data.type, ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS,
		      "Invalid filter type (%d)", eth_filter_data.type);
	zassert_mem_equal(eth_filter_data.mac_address.addr, mcast_lladdr,
			  sizeof(mcast_lladdr), "Wrong address filtered");

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_ok(ret, "Cannot drop membership (%d)", errno);

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
	zassert_mem_equal(eth_filter_data.mac_address.addr, mcast_lladdr,
			  sizeof(mcast_lladdr), "Wrong address filtered");
}

/* A group is joined on the interface that the request names, and not on any
 * other one.
 */
ZTEST(socket_packet, test_packet_sock_membership_other_iface)
{
	struct net_packet_mreq mreq;
	int first_count;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.second);

	first_count = mcast_addr_count(ud.first);

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);

	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_equal(eth_filter_data.iface, ud.second,
		      "Filter programmed for a wrong interface");
	zassert_equal(mcast_addr_count(ud.first), first_count,
		      "Group tracked by the wrong interface");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_equal(eth_filter_data.iface, ud.second,
		      "Filter removed from a wrong interface");
	zassert_equal(mcast_addr_count(ud.first), first_count,
		      "Group tracked by the wrong interface");
}

ZTEST(socket_packet, test_packet_sock_membership_close)
{
	struct net_packet_mreq mreq;
	int count;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	count = mcast_addr_count(ud.first);

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");

	/* A second group so that the close has more than one to drop */
	mcast_mreq_group(&mreq, ud.first, 2);

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add second membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 2, "Filter not programmed");
	zassert_equal(mcast_addr_count(ud.first), count + 2,
		      "Groups not tracked by the interface");

	/* Closing the socket must drop every membership it still holds */
	(void)zsock_close(packet_sock_1);
	packet_sock_1 = -1;

	zassert_equal(eth_filter_data.count, 4, "Memberships not dropped on close");
	zassert_false(eth_filter_data.set, "Filter not disabled");
	zassert_equal(mcast_addr_count(ud.first), count,
		      "Groups still tracked after the socket was closed");
}

ZTEST(socket_packet, test_packet_sock_membership_two_sockets)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	setup_packet_socket(&packet_sock_2, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	/* Every socket takes a reference of its own... */
	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");

	ret = mcast_membership(packet_sock_2, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);

	/* ...but the device is only told about the group once, as the L2
	 * counts how many users it has.
	 */
	zassert_equal(eth_filter_data.count, 1,
		      "Filter programmed again for an already joined group");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1,
		      "Filter changed while the group is still in use");
	zassert_true(eth_filter_data.set,
		     "Filter disabled while the group is still in use");

	(void)zsock_close(packet_sock_2);
	packet_sock_2 = -1;

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
}

ZTEST(socket_packet, test_packet_sock_membership_duplicate)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	/* The same socket joining twice takes one reference and needs two
	 * leaves.
	 */
	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);
	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership again (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Second join reached the device");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Group dropped too early");
	zassert_true(eth_filter_data.set, "Filter disabled too early");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 2, "Group not dropped");
	zassert_false(eth_filter_data.set, "Filter not disabled");

	/* Dropping a group that is not joined must fail */
	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_equal(ret, -1, "Dropping an unknown group succeeded");
	zassert_equal(errno, EADDRNOTAVAIL, "Invalid errno (%d)", errno);
}

ZTEST(socket_packet, test_packet_sock_membership_no_buffers)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	for (int i = 0; i < CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP_COUNT; i++) {
		mcast_mreq_group(&mreq, ud.first, i + 1);

		ret = mcast_membership(packet_sock_1,
				       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
		zassert_ok(ret, "Cannot add membership %d (%d)", i, errno);
	}

	mcast_mreq_group(&mreq, ud.first,
			 CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP_COUNT + 1);

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_equal(ret, -1, "Membership added when out of entries");
	zassert_equal(errno, ENOBUFS, "Invalid errno (%d)", errno);

	/* Closing the socket must release all of the entries so that the
	 * next test can allocate them again.
	 */
	(void)zsock_close(packet_sock_1);
	packet_sock_1 = -1;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_group(&mreq, ud.first, 1);

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Entries not released on close (%d)", errno);
}

ZTEST(socket_packet, test_packet_sock_membership_invalid)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));

	mcast_mreq_init(&mreq, ud.first);
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET, 0xff, &mreq,
			       sizeof(mreq));
	zassert_equal(ret, -1, "Unknown option accepted");
	zassert_equal(errno, ENOPROTOOPT, "Invalid errno (%d)", errno);

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, NULL, sizeof(mreq));
	zassert_equal(ret, -1, "NULL option value accepted");
	zassert_equal(errno, EINVAL, "Invalid errno (%d)", errno);

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq,
			       sizeof(mreq) - 1);
	zassert_equal(ret, -1, "Invalid option length accepted");
	zassert_equal(errno, EINVAL, "Invalid errno (%d)", errno);

	mcast_mreq_init(&mreq, ud.first);
	mreq.mr_ifindex = 0;
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_equal(ret, -1, "Zero interface index accepted");
	zassert_equal(errno, ENODEV, "Invalid errno (%d)", errno);

	mreq.mr_ifindex = -1;
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_equal(ret, -1, "Negative interface index accepted");
	zassert_equal(errno, ENODEV, "Invalid errno (%d)", errno);

	mreq.mr_ifindex = 255;
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_equal(ret, -1, "Out of range interface index accepted");
	zassert_equal(errno, ENODEV, "Invalid errno (%d)", errno);

	mcast_mreq_init(&mreq, ud.first);
	mreq.mr_alen = sizeof(mcast_lladdr) - 1;
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_equal(ret, -1, "Invalid address length accepted");
	zassert_equal(errno, EINVAL, "Invalid errno (%d)", errno);

	/* Neither promiscuous mode nor all-multicast is implemented */
	mcast_mreq_init(&mreq, ud.first);
	mreq.mr_type = NET_PACKET_MR_PROMISC;
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_equal(ret, -1, "NET_PACKET_MR_PROMISC accepted");
	zassert_equal(errno, ENOTSUP, "Invalid errno (%d)", errno);

	mreq.mr_type = NET_PACKET_MR_ALLMULTI;
	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_equal(ret, -1, "NET_PACKET_MR_ALLMULTI accepted");
	zassert_equal(errno, ENOTSUP, "Invalid errno (%d)", errno);

	zassert_equal(eth_filter_data.count, 0,
		      "A rejected request reached the device");
}

ZTEST(socket_packet, test_packet_sock_setsockopt_other_level)
{
	struct timeval optval = {
		.tv_usec = 100000,
	};
	int ret;

	/* Options that are not ZSOCK_SOL_PACKET must still reach the
	 * generic socket option handling.
	 */
	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_SOCKET,
			       ZSOCK_SO_RCVTIMEO, &optval, sizeof(optval));
	zassert_ok(ret, "Cannot set receive timeout (%d)", errno);
}

/* A packet socket membership must not stop the IP level joins from reaching
 * the receive filter.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_ip)
{
	/* 224.0.0.251 maps to 01:00:5e:00:00:fb */
	static const uint8_t mcast_ip_lladdr[] = {
		0x01, 0x00, 0x5e, 0x00, 0x00, 0xfb
	};
	struct net_addr addr = {
		.family = NET_AF_INET,
		.in_addr = { { { 224, 0, 0, 251 } } },
	};

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	net_if_mcast_monitor(ud.first, &addr, true);

	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_equal(eth_filter_data.iface, ud.first,
		      "Filter programmed for a wrong interface");
	zassert_true(eth_filter_data.set, "Filter not enabled");
	zassert_mem_equal(eth_filter_data.mac_address.addr, mcast_ip_lladdr,
			  sizeof(mcast_ip_lladdr), "Wrong address filtered");

	net_if_mcast_monitor(ud.first, &addr, false);

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
}

/* The IP level and the packet sockets can need the same link layer address,
 * so the device must be told to stop listening to it only after the last one
 * has left the group.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_shared)
{
	/* 224.0.0.1 maps to mcast_lladdr */
	struct net_addr addr = {
		.family = NET_AF_INET,
		.in_addr = { { { 224, 0, 0, 1 } } },
	};
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	net_if_mcast_monitor(ud.first, &addr, true);

	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_true(eth_filter_data.set, "Filter not enabled");
	zassert_mem_equal(eth_filter_data.mac_address.addr, mcast_lladdr,
			  sizeof(mcast_lladdr), "Wrong address filtered");

	/* The device already listens to the address so it must not be told
	 * about it a second time.
	 */
	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);

	zassert_equal(eth_filter_data.count, 1,
		      "Filter programmed again for an already joined group");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);

	zassert_equal(eth_filter_data.count, 1,
		      "Filter changed while the IP level still needs the group");
	zassert_true(eth_filter_data.set,
		     "Filter disabled while the IP level still needs the group");

	net_if_mcast_monitor(ud.first, &addr, false);

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
	zassert_mem_equal(eth_filter_data.mac_address.addr, mcast_lladdr,
			  sizeof(mcast_lladdr), "Wrong address filtered");
}

/* IPv4 multicast addresses map 32:1 to link layer addresses, so leaving one
 * group must not stop the device from listening to another group that shares
 * the link layer address.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_alias)
{
	/* Both of these map to mcast_lladdr */
	struct net_addr first = {
		.family = NET_AF_INET,
		.in_addr = { { { 224, 0, 0, 1 } } },
	};
	struct net_addr second = {
		.family = NET_AF_INET,
		.in_addr = { { { 225, 0, 0, 1 } } },
	};

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	net_if_mcast_monitor(ud.first, &first, true);
	net_if_mcast_monitor(ud.first, &second, true);

	zassert_equal(eth_filter_data.count, 1,
		      "Filter programmed per IP address instead of per group");

	net_if_mcast_monitor(ud.first, &first, false);

	zassert_equal(eth_filter_data.count, 1,
		      "Filter changed while the address is still in use");
	zassert_true(eth_filter_data.set,
		     "Filter disabled while the address is still in use");

	net_if_mcast_monitor(ud.first, &second, false);

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
}

/* An interface can only track so many addresses, and a group that does not
 * fit is not joined at all rather than joined without the reference
 * counting.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_full)
{
	struct net_eth_addr addr;
	int ret;

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));
	memcpy(addr.addr, mcast_lladdr, sizeof(mcast_lladdr));

	for (int i = 0; i < NET_ETH_MCAST_FILTER_COUNT; i++) {
		addr.addr[sizeof(mcast_lladdr) - 1] = 0x10 + i;

		ret = net_eth_mcast_addr_add(ud.first, &addr);
		zassert_ok(ret, "Cannot add address %d (%d)", i, ret);
	}

	zassert_equal(eth_filter_data.count, NET_ETH_MCAST_FILTER_COUNT,
		      "Not all the addresses were programmed");

	addr.addr[sizeof(mcast_lladdr) - 1] = 0x20;

	ret = net_eth_mcast_addr_add(ud.first, &addr);
	zassert_equal(ret, -ENOMEM, "Address added to a full list (%d)", ret);
	zassert_equal(eth_filter_data.count, NET_ETH_MCAST_FILTER_COUNT,
		      "Untracked address programmed");

	ret = net_eth_mcast_addr_rm(ud.first, &addr);
	zassert_equal(ret, -ENOENT, "Address that was not joined was left (%d)",
		      ret);
	zassert_equal(eth_filter_data.count, NET_ETH_MCAST_FILTER_COUNT,
		      "Address that was not joined reached the device");

	for (int i = 0; i < NET_ETH_MCAST_FILTER_COUNT; i++) {
		addr.addr[sizeof(mcast_lladdr) - 1] = 0x10 + i;

		ret = net_eth_mcast_addr_rm(ud.first, &addr);
		zassert_ok(ret, "Cannot remove address %d (%d)", i, ret);
	}
}

/* An interface that cannot track another multicast address must say so to the
 * application instead of letting the join look like it succeeded.
 */
ZTEST(socket_packet, test_packet_sock_membership_l2_full)
{
	struct net_packet_mreq mreq;
	struct net_eth_addr addr;
	int filled;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	memcpy(addr.addr, mcast_lladdr, sizeof(mcast_lladdr));

	/* Take every address that the interface can track. The group that the
	 * request asks for is left out so that the socket has to join a new
	 * one.
	 */
	for (filled = 0; filled < NET_ETH_MCAST_FILTER_COUNT; filled++) {
		addr.addr[sizeof(mcast_lladdr) - 1] = 0x10 + filled;

		ret = net_eth_mcast_addr_add(ud.first, &addr);
		zassert_ok(ret, "Cannot add address %d (%d)", filled, ret);
	}

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_equal(ret, -1, "Membership added to a full interface");
	zassert_equal(errno, ENOMEM, "Invalid errno (%d)", errno);
	zassert_equal(eth_filter_data.count, 0, "Untracked address programmed");

	/* Give one address back and try again. This also tells that the failed
	 * join did not leave an entry behind, as the socket can still take one.
	 */
	addr.addr[sizeof(mcast_lladdr) - 1] = 0x10;
	ret = net_eth_mcast_addr_rm(ud.first, &addr);
	zassert_ok(ret, "Cannot remove address (%d)", ret);

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);

	for (int i = 1; i < filled; i++) {
		addr.addr[sizeof(mcast_lladdr) - 1] = 0x10 + i;

		ret = net_eth_mcast_addr_rm(ud.first, &addr);
		zassert_ok(ret, "Cannot remove address %d (%d)", i, ret);
	}
}

/* A device that refuses the group must not leave the membership behind, at
 * either level.
 */
ZTEST(socket_packet, test_packet_sock_membership_device_error)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	eth_filter_ret = -EIO;

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_equal(ret, -1, "Membership added by a device that refused it");
	zassert_equal(errno, EIO, "Invalid errno (%d)", errno);

	eth_filter_ret = 0;

	/* The socket did not join, so it has nothing to drop */
	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_equal(ret, -1, "Dropping a group that was not joined passed");
	zassert_equal(errno, EADDRNOTAVAIL, "Invalid errno (%d)", errno);

	/* The L2 reference was given back as well, so the group is programmed
	 * again now that the device accepts it.
	 */
	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_true(eth_filter_data.set, "Filter not enabled");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);
}

/* A driver can ask the interface what it should be listening to instead of
 * keeping track of the addresses itself.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_foreach)
{
	struct mcast_foreach_data data = { 0 };
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	net_eth_mcast_addr_foreach(ud.first, mcast_foreach_cb, &data);
	zassert_equal(data.count, 0, "Interface has stale multicast addresses");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);

	net_eth_mcast_addr_foreach(ud.first, mcast_foreach_cb, &data);
	zassert_equal(data.count, 1, "Joined group not listed");
	zassert_equal(data.iface, ud.first, "Wrong interface");
	zassert_equal(data.last_byte, mcast_lladdr[sizeof(mcast_lladdr) - 1],
		      "Wrong address listed");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);

	data.count = 0;
	net_eth_mcast_addr_foreach(ud.first, mcast_foreach_cb, &data);
	zassert_equal(data.count, 0, "Group still listed after leaving it");
}

/* A filter that an application sets itself is counted together with the
 * groups joined by the IP level and by the packet sockets, so that neither
 * of them can take the address away from the other.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_mgmt)
{
	/* 224.0.0.1 maps to mcast_lladdr */
	struct net_addr addr = {
		.family = NET_AF_INET,
		.in_addr = { { { 224, 0, 0, 1 } } },
	};
	struct net_eth_addr mac;
	int ret;

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));
	memcpy(mac.addr, mcast_lladdr, sizeof(mcast_lladdr));

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, true);
	zassert_ok(ret, "Cannot set the filter (%d)", ret);

	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_true(eth_filter_data.set, "Filter not enabled");
	zassert_mem_equal(eth_filter_data.mac_address.addr, mcast_lladdr,
			  sizeof(mcast_lladdr), "Wrong address filtered");

	/* The device already listens to the address so it must not be told
	 * about it a second time.
	 */
	net_if_mcast_monitor(ud.first, &addr, true);

	zassert_equal(eth_filter_data.count, 1,
		      "Filter programmed again for an already filtered address");

	net_if_mcast_monitor(ud.first, &addr, false);

	zassert_equal(eth_filter_data.count, 1,
		      "Filter removed while the application still needs it");
	zassert_true(eth_filter_data.set,
		     "Filter disabled while the application still needs it");

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, false);
	zassert_ok(ret, "Cannot unset the filter (%d)", ret);

	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
}

/* Only the multicast destination addresses are counted, the rest are given
 * to the device as they are.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_mgmt_other)
{
	struct mcast_foreach_data data = { 0 };
	struct net_eth_addr mac;
	int ret;

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));

	/* A source address is not a group that the interface listens to */
	memcpy(mac.addr, mcast_lladdr, sizeof(mcast_lladdr));

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_SRC_MAC_ADDRESS, true);
	zassert_ok(ret, "Cannot set the filter (%d)", ret);
	zassert_equal(eth_filter_data.count, 1, "Filter not programmed");
	zassert_equal(eth_filter_data.type,
		      ETHERNET_FILTER_TYPE_SRC_MAC_ADDRESS,
		      "Wrong filter type programmed");

	net_eth_mcast_addr_foreach(ud.first, mcast_foreach_cb, &data);
	zassert_equal(data.count, 0, "Source address tracked as a group");

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_SRC_MAC_ADDRESS, false);
	zassert_ok(ret, "Cannot unset the filter (%d)", ret);
	zassert_equal(eth_filter_data.count, 2, "Filter not removed");

	/* A unicast destination address is not a group either */
	memcpy(mac.addr, lladdr2, sizeof(lladdr2));

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, true);
	zassert_ok(ret, "Cannot set the filter (%d)", ret);
	zassert_equal(eth_filter_data.count, 3, "Filter not programmed");

	data.count = 0;
	net_eth_mcast_addr_foreach(ud.first, mcast_foreach_cb, &data);
	zassert_equal(data.count, 0, "Unicast address tracked as a group");

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, false);
	zassert_ok(ret, "Cannot unset the filter (%d)", ret);
	zassert_equal(eth_filter_data.count, 4, "Filter not removed");
}

/* Unsetting a filter that was never set is refused, and an error from the
 * device is told to the caller.
 */
ZTEST(socket_packet, test_packet_sock_membership_eth_filter_mgmt_errors)
{
	struct net_eth_addr mac;
	int ret;

	memset(&eth_filter_data, 0, sizeof(eth_filter_data));
	memcpy(mac.addr, mcast_lladdr, sizeof(mcast_lladdr));

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, false);
	zassert_equal(ret, -ENOENT, "Unsetting an unset filter passed (%d)",
		      ret);
	zassert_equal(eth_filter_data.count, 0, "Device told to unset");

	eth_filter_ret = -EIO;

	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, true);
	zassert_equal(ret, -EIO, "Device error not reported (%d)", ret);

	eth_filter_ret = 0;

	/* The address was taken into use even though the device refused it,
	 * so it can be given back.
	 */
	ret = net_eth_mac_filter(ud.first, &mac,
				 ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS, false);
	zassert_ok(ret, "Cannot unset the filter (%d)", ret);
	zassert_equal(eth_filter_data.count, 2, "Filter not removed");
	zassert_false(eth_filter_data.set, "Filter not disabled");
}

/* A device that cannot filter receives the group anyway, so the join must
 * pass and the address must be tracked for a filter that is programmed
 * later.
 */
ZTEST(socket_packet, test_packet_sock_membership_no_device_filter)
{
	struct net_packet_mreq mreq;
	int count;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	count = mcast_addr_count(ud.first);

	eth_filter_ret = -ENOTSUP;

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot add membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 1, "Device not told about the group");
	zassert_equal(mcast_addr_count(ud.first), count + 1,
		      "Group not tracked by the interface");

	ret = mcast_membership(packet_sock_1, ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq);
	zassert_ok(ret, "Cannot drop membership (%d)", errno);
	zassert_equal(eth_filter_data.count, 2, "Device not told about the leave");
	zassert_equal(mcast_addr_count(ud.first), count,
		      "Group still tracked by the interface");
}

#if defined(CONFIG_NET_MGMT_EVENT_INFO)
static struct net_mgmt_event_callback mcast_mgmt_cb;
static K_SEM_DEFINE(mcast_mgmt_sem, 0, 1);
static struct net_event_packet_mcast mcast_mgmt_info;
static uint64_t mcast_mgmt_event;

static void mcast_mgmt_handler(struct net_mgmt_event_callback *cb,
			       uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(iface);

	if (cb->info == NULL || cb->info_length != sizeof(mcast_mgmt_info)) {
		return;
	}

	memcpy(&mcast_mgmt_info, cb->info, sizeof(mcast_mgmt_info));
	mcast_mgmt_event = mgmt_event;

	k_sem_give(&mcast_mgmt_sem);
}

ZTEST(socket_packet, test_packet_sock_membership_mgmt_event)
{
	struct net_packet_mreq mreq;
	int ret;

	setup_packet_socket(&packet_sock_1, NET_SOCK_RAW, net_htons(ETH_P_ALL));
	mcast_mreq_init(&mreq, ud.first);

	memset(&mcast_mgmt_info, 0, sizeof(mcast_mgmt_info));
	mcast_mgmt_event = 0;
	k_sem_reset(&mcast_mgmt_sem);

	net_mgmt_init_event_callback(&mcast_mgmt_cb, mcast_mgmt_handler,
				     NET_EVENT_PACKET_MCAST_MEMBERSHIP_ADD |
				     NET_EVENT_PACKET_MCAST_MEMBERSHIP_DROP);
	net_mgmt_add_event_callback(&mcast_mgmt_cb);

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_ok(ret, "Cannot add membership (%d)", errno);

	zassert_ok(k_sem_take(&mcast_mgmt_sem, K_SECONDS(1)),
		   "Timeout while waiting for the add event");
	zassert_equal(mcast_mgmt_event, NET_EVENT_PACKET_MCAST_MEMBERSHIP_ADD,
		      "Wrong event received");
	zassert_equal(mcast_mgmt_info.type, NET_PACKET_MR_MULTICAST,
		      "Wrong membership type (%d)", mcast_mgmt_info.type);
	zassert_equal(mcast_mgmt_info.addr.len, sizeof(mcast_lladdr),
		      "Wrong link address length (%d)", mcast_mgmt_info.addr.len);
	zassert_mem_equal(mcast_mgmt_info.addr.addr, mcast_lladdr,
			  sizeof(mcast_lladdr), "Wrong link address");

	ret = zsock_setsockopt(packet_sock_1, ZSOCK_SOL_PACKET,
			       ZSOCK_PACKET_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
	zassert_ok(ret, "Cannot drop membership (%d)", errno);

	zassert_ok(k_sem_take(&mcast_mgmt_sem, K_SECONDS(1)),
		   "Timeout while waiting for the drop event");
	zassert_equal(mcast_mgmt_event, NET_EVENT_PACKET_MCAST_MEMBERSHIP_DROP,
		      "Wrong event received");

	net_mgmt_del_event_callback(&mcast_mgmt_cb);
}
#endif /* CONFIG_NET_MGMT_EVENT_INFO */

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

	eth_filter_ret = 0;

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
