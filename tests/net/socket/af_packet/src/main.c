/*
 * Copyright (c) 2020 Intel Corporation
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

#if defined(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) NET_DBG(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define IPV4_ADDR "127.0.0.1"

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

static int setup_socket(struct net_if *iface, int type, int proto)
{
	int sock;

	sock = zsock_socket(AF_PACKET, type, proto);
	zassert_true(sock >= 0, "Cannot create packet socket (%d)", -errno);

	return sock;
}

static int bind_socket(int sock, struct net_if *iface)
{
	struct sockaddr_ll addr;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = net_if_get_by_iface(iface);
	addr.sll_family = AF_PACKET;

	return zsock_bind(sock, (struct sockaddr *)&addr, sizeof(addr));
}

struct user_data {
	struct net_if *first;
	struct net_if *second;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *ud = user_data;
	struct net_linkaddr *link_addr;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	link_addr = net_if_get_link_addr(iface);
	if (memcmp(link_addr->addr, lladdr1, sizeof(lladdr1)) != 0 &&
	    memcmp(link_addr->addr, lladdr2, sizeof(lladdr2)) != 0) {
		return;
	}

	if (ud->first == NULL) {
		ud->first = iface;
		return;
	}

	ud->second = iface;
}

static void setblocking(int fd, bool val)
{
	int fl, res;

	fl = zsock_fcntl(fd, F_GETFL, 0);
	zassert_not_equal(fl, -1, "Fail to set fcntl");

	if (val) {
		fl &= ~O_NONBLOCK;
	} else {
		fl |= O_NONBLOCK;
	}

	res = zsock_fcntl(fd, F_SETFL, fl);
	zassert_not_equal(res, -1, "Fail to set fcntl");
}

#define SRC_PORT 4240
#define DST_PORT 4242
static int prepare_udp_socket(struct sockaddr_in *sockaddr, uint16_t local_port)
{
	int sock, ret;

	sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock >= 0, "Cannot create DGRAM (UDP) socket (%d)", sock);

	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = htons(local_port);
	ret = zsock_inet_pton(AF_INET, IPV4_ADDR, &sockaddr->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* Bind UDP socket to local port */
	ret = zsock_bind(sock, (struct sockaddr *) sockaddr, sizeof(*sockaddr));
	zassert_equal(ret, 0, "Cannot bind DGRAM (UDP) socket (%d)", -errno);

	return sock;
}

static void __test_packet_sockets(int *sock1, int *sock2)
{
	struct user_data ud = { 0 };
	int ret;

	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	*sock1 = setup_socket(ud.first, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(*sock1 >= 0, "Cannot create 1st socket (%d)", *sock1);

	*sock2 = setup_socket(ud.second, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(*sock2 >= 0, "Cannot create 2nd socket (%d)", *sock2);

	ret = bind_socket(*sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(*sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);
}

#define IP_HDR_SIZE 20
#define UDP_HDR_SIZE 8
#define HDR_SIZE (IP_HDR_SIZE + UDP_HDR_SIZE)
ZTEST(socket_packet, test_raw_packet_sockets)
{
	uint8_t data_to_send[] = { 10, 11, 12, 13, 14, 15, 16, 17, 18, 19 };
	uint8_t data_to_receive[sizeof(data_to_send) + HDR_SIZE];
	struct sockaddr_ll src;
	struct sockaddr_in sockaddr;
	int ret, sock1, sock2, sock3, sock4;
	socklen_t addrlen;
	ssize_t sent = 0;

	__test_packet_sockets(&sock1, &sock2);

	/* Prepare UDP socket which will read data */
	sock3 = prepare_udp_socket(&sockaddr, DST_PORT);

	/* Prepare UDP socket from which data are going to be sent */
	sock4 = prepare_udp_socket(&sockaddr, SRC_PORT);
	/* Properly set destination port for UDP packet */
	sockaddr.sin_port = htons(DST_PORT);

	/*
	 * Send UDP datagram to us - as check_ip_addr() in net_send_data()
	 * returns 1 - the packet is processed immediately in the net stack
	 */
	sent = zsock_sendto(sock4, data_to_send, sizeof(data_to_send),
			    0, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
	zassert_equal(sent, sizeof(data_to_send), "sendto failed");

	k_msleep(10); /* Let the packet enter the system */

	setblocking(sock3, false);
	memset(&data_to_receive, 0, sizeof(data_to_receive));
	errno = 0;

	/* Check if UDP packets can be read after being sent */
	addrlen = sizeof(sockaddr);
	ret = zsock_recvfrom(sock3, data_to_receive, sizeof(data_to_receive),
			     0, (struct sockaddr *)&sockaddr, &addrlen);
	zassert_equal(ret, sizeof(data_to_send), "Cannot receive all data (%d)",
		      -errno);
	zassert_mem_equal(data_to_receive, data_to_send, sizeof(data_to_send),
			  "Sent and received buffers do not match");

	/* And if the packet has been also passed to RAW socket */
	setblocking(sock1, false);
	memset(&data_to_receive, 0, sizeof(data_to_receive));
	memset(&src, 0, sizeof(src));
	addrlen = sizeof(src);
	errno = 0;

	/* The recvfrom reads the whole received packet - including its
	 * IP (20B) and UDP (8) headers. After those we can expect payload,
	 * which have been sent.
	 */
	ret = zsock_recvfrom(sock1, data_to_receive, sizeof(data_to_receive), 0,
			     (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, sizeof(data_to_send) + HDR_SIZE,
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(data_to_send), -errno);
	zassert_mem_equal(&data_to_receive[HDR_SIZE], data_to_send,
			  sizeof(data_to_send),
			  "Sent and received buffers do not match");

	zsock_close(sock1);
	zsock_close(sock2);
	zsock_close(sock3);
	zsock_close(sock4);
}

ZTEST(socket_packet, test_packet_sockets)
{
	int sock1, sock2;

	__test_packet_sockets(&sock1, &sock2);

	zsock_close(sock1);
	zsock_close(sock2);
}

ZTEST(socket_packet, test_packet_sockets_dgram)
{
	uint8_t data_to_send[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t data_to_receive[32];
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct user_data ud = { 0 };
	struct sockaddr_ll dst, src;
	int ret, sock1, sock2;
	int iter, max_iter = 10;

	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	sock1 = setup_socket(ud.first, SOCK_DGRAM, htons(ETH_P_TSN));
	zassert_true(sock1 >= 0, "Cannot create 1st socket (%d)", sock1);

	sock2 = setup_socket(ud.second, SOCK_DGRAM, htons(ETH_P_TSN));
	zassert_true(sock2 >= 0, "Cannot create 2nd socket (%d)", sock2);

	ret = bind_socket(sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);

	setblocking(sock1, false);
	setblocking(sock2, false);

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_TSN);
	memcpy(dst.sll_addr, lladdr1, sizeof(lladdr1));

	ret = zsock_sendto(sock2, data_to_send, sizeof(data_to_send), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(data_to_send), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10); /* Let the packet enter the system */

	ret = zsock_recvfrom(sock2, data_to_receive, sizeof(data_to_receive), 0,
	(struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));
	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock1, data_to_receive, sizeof(data_to_receive),
				     0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, sizeof(data_to_send),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(data_to_send), -errno);

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

	zassert_mem_equal(data_to_send, data_to_receive, sizeof(data_to_send),
			  "Data mismatch");

	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr2));

	/* Send to socket 2 but read from socket 1. There should not be any
	 * data in socket 1
	 */
	ret = zsock_sendto(sock2, data_to_send, sizeof(data_to_send), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(data_to_send), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10);
	memset(&src, 0, sizeof(src));

	ret = zsock_recvfrom(sock1, data_to_receive, sizeof(data_to_receive), 0,
			     (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));

	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock2, data_to_receive, sizeof(data_to_receive),
				     0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, sizeof(data_to_send), "Cannot receive all data (%d)",
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

	zassert_mem_equal(data_to_send, data_to_receive, sizeof(data_to_send),
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

	ret = zsock_sendto(sock2, payload_ip_length, sizeof(payload_ip_length), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(payload_ip_length), "Cannot send all data (%d)", -errno);

	k_msleep(10);

	memset(&src, 0, sizeof(src));
	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock2, receive_ip_length, sizeof(receive_ip_length), 0,
				     (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, ARRAY_SIZE(payload_ip_length), "Cannot receive all data (%d)", -errno);
	zassert_mem_equal(payload_ip_length, receive_ip_length, sizeof(payload_ip_length),
			  "Data mismatch");

	zsock_close(sock1);
	zsock_close(sock2);
}

ZTEST(socket_packet, test_raw_and_dgram_socket_exchange)
{
	uint8_t data_to_send[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t data_to_receive[32];
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct user_data ud = { 0 };
	struct sockaddr_ll dst, src;
	int ret, sock1, sock2;
	int iter, max_iter = 10;
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


	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	sock1 = setup_socket(ud.first, SOCK_DGRAM, htons(ETH_P_ALL));
	zassert_true(sock1 >= 0, "Cannot create 1st socket (%d)", sock1);

	sock2 = setup_socket(ud.second, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(sock2 >= 0, "Cannot create 2nd socket (%d)", sock2);

	ret = bind_socket(sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);

	setblocking(sock1, false);
	setblocking(sock2, false);

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_IP);
	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr1));

	/* SOCK_DGRAM to SOCK_RAW */

	ret = zsock_sendto(sock1, data_to_send, sizeof(data_to_send), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(data_to_send), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10); /* Let the packet enter the system */
	memset(&src, 0, sizeof(src));

	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock2, data_to_receive, sizeof(data_to_receive),
				     0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(expected_payload_raw), -errno);

	zassert_mem_equal(expected_payload_raw, data_to_receive,
			  sizeof(expected_payload_raw), "Data mismatch");

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_IP);

	/* SOCK_RAW to SOCK_DGRAM */

	ret = zsock_sendto(sock2, send_payload_raw, sizeof(send_payload_raw), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(send_payload_raw), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10);
	memset(&src, 0, sizeof(src));

	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock1, data_to_receive, sizeof(data_to_receive),
				     0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, sizeof(data_to_send), "Cannot receive all data (%d)",
		      -errno);
	zassert_mem_equal(data_to_send, data_to_receive, sizeof(data_to_send),
			  "Data mismatch");

	zsock_close(sock1);
	zsock_close(sock2);
}

ZTEST(socket_packet, test_raw_and_dgram_socket_recv)
{
	uint8_t data_to_send[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t data_to_receive[32];
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct user_data ud = { 0 };
	struct sockaddr_ll dst, src;
	int ret, sock1, sock2, sock3;
	int iter, max_iter = 10;
	const uint8_t expected_payload_raw[] = {
		0x02, 0x02, 0x02, 0x02, 0x02, 0x02, /* Dst ll addr */
		0x01, 0x01, 0x01, 0x01, 0x01, 0x01, /* Src ll addr */
		ETH_P_IP >> 8, ETH_P_IP & 0xFF, /* EtherType */
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9 /* Payload */
	};

	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	sock1 = setup_socket(ud.first, SOCK_DGRAM, htons(ETH_P_ALL));
	zassert_true(sock1 >= 0, "Cannot create 1st socket (%d)", sock1);

	sock2 = setup_socket(ud.second, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(sock2 >= 0, "Cannot create 2nd socket (%d)", sock2);

	sock3 = setup_socket(ud.second, SOCK_RAW, htons(ETH_P_ALL));
	zassert_true(sock3 >= 0, "Cannot create 2nd socket (%d)", sock3);

	ret = bind_socket(sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);

	ret = bind_socket(sock3, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);

	setblocking(sock1, false);
	setblocking(sock2, false);
	setblocking(sock3, false);

	memset(&dst, 0, sizeof(dst));
	dst.sll_family = AF_PACKET;
	dst.sll_protocol = htons(ETH_P_IP);
	memcpy(dst.sll_addr, lladdr2, sizeof(lladdr1));

	ret = zsock_sendto(sock1, data_to_send, sizeof(data_to_send), 0,
			   (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(data_to_send), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10); /* Let the packet enter the system */
	memset(&src, 0, sizeof(src));

	/* Both SOCK_DGRAM to SOCK_RAW sockets should receive packet. */

	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock2, data_to_receive, sizeof(data_to_receive),
				     0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(expected_payload_raw), -errno);

	zassert_mem_equal(expected_payload_raw, data_to_receive,
			  sizeof(expected_payload_raw), "Data mismatch");

	memset(&src, 0, sizeof(src));

	errno = 0;
	iter = 0;
	do {
		ret = zsock_recvfrom(sock3, data_to_receive, sizeof(data_to_receive),
				     0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
		iter++;
	} while (ret < 0 && errno == EAGAIN && iter < max_iter);

	zassert_equal(ret, sizeof(expected_payload_raw),
		      "Cannot receive all data (%d)", -errno);
	zassert_mem_equal(expected_payload_raw, data_to_receive,
			  sizeof(expected_payload_raw), "Data mismatch");

	zsock_close(sock1);
	zsock_close(sock2);
	zsock_close(sock3);
}

ZTEST_SUITE(socket_packet, NULL, NULL, NULL, NULL, NULL);
