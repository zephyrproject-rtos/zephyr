/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <sys/mutex.h>
#include <ztest_assert.h>

#include <fcntl.h>
#include <net/socket.h>
#include <net/ethernet.h>

#if defined(CONFIG_NET_SOCKETS_LOG_LEVEL_DBG)
#define DBG(fmt, ...) NET_DBG(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static uint8_t lladdr1[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
static uint8_t lladdr2[] = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };

struct eth_fake_context {
	struct net_if *iface;
	uint8_t *mac_address;
};

static struct eth_fake_context eth_fake_data1 = {
	.mac_address = lladdr1
};
static struct eth_fake_context eth_fake_data2 = {
	.mac_address = lladdr2
};

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *recv_pkt;
	int ret;

	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	DBG("Sending data (%d bytes) to iface %d\n",
	    net_pkt_get_len(pkt), net_if_get_by_iface(net_pkt_iface(pkt)));

	recv_pkt = net_pkt_clone(pkt, K_NO_WAIT);

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

	ethernet_init(iface);
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_fake1, "eth_fake1", eth_fake_init,
		    device_pm_control_nop, &eth_fake_data1, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_fake2, "eth_fake2", eth_fake_init,
		    device_pm_control_nop, &eth_fake_data2, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs,
		    NET_ETH_MTU);

static int setup_socket(struct net_if *iface, int type, int proto)
{
	int sock;

	sock = socket(AF_PACKET, type, htons(proto));
	zassert_true(sock >= 0, "Cannot create packet socket (%d)", -errno);

	return sock;
}

static int bind_socket(int sock, struct net_if *iface)
{
	struct sockaddr_ll addr;

	memset(&addr, 0, sizeof(addr));

	addr.sll_ifindex = net_if_get_by_iface(iface);
	addr.sll_family = AF_PACKET;

	return bind(sock, (struct sockaddr *)&addr, sizeof(addr));
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

	fl = fcntl(fd, F_GETFL, 0);
	zassert_not_equal(fl, -1, "Fail to set fcntl");

	if (val) {
		fl &= ~O_NONBLOCK;
	} else {
		fl |= O_NONBLOCK;
	}

	res = fcntl(fd, F_SETFL, fl);
	zassert_not_equal(fl, -1, "Fail to set fcntl");
}

static void test_packet_sockets(void)
{
	struct user_data ud = { 0 };
	int ret, sock1, sock2;

	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	sock1 = setup_socket(ud.first, SOCK_RAW, ETH_P_ALL);
	zassert_true(sock1 >= 0, "Cannot create 1st socket (%d)", sock1);

	sock2 = setup_socket(ud.second, SOCK_RAW, ETH_P_ALL);
	zassert_true(sock2 >= 0, "Cannot create 2nd socket (%d)", sock2);

	ret = bind_socket(sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);
}

static void test_packet_sockets_dgram(void)
{
	uint8_t data_to_send[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	uint8_t data_to_receive[sizeof(data_to_send)];
	socklen_t addrlen = sizeof(struct sockaddr_ll);
	struct user_data ud = { 0 };
	struct sockaddr_ll dst, src;
	int ret, sock1, sock2;

	net_if_foreach(iface_cb, &ud);

	zassert_not_null(ud.first, "1st Ethernet interface not found");
	zassert_not_null(ud.second, "2nd Ethernet interface not found");

	sock1 = setup_socket(ud.first, SOCK_DGRAM, ETH_P_TSN);
	zassert_true(sock1 >= 0, "Cannot create 1st socket (%d)", sock1);

	sock2 = setup_socket(ud.second, SOCK_DGRAM, ETH_P_TSN);
	zassert_true(sock2 >= 0, "Cannot create 2nd socket (%d)", sock2);

	ret = bind_socket(sock1, ud.first);
	zassert_equal(ret, 0, "Cannot bind 1st socket (%d)", -errno);

	ret = bind_socket(sock2, ud.second);
	zassert_equal(ret, 0, "Cannot bind 2nd socket (%d)", -errno);

	memset(&dst, 0, sizeof(dst));
	dst.sll_ifindex = net_if_get_by_iface(ud.first);
	dst.sll_family = AF_PACKET;

	ret = sendto(sock2, data_to_send, sizeof(data_to_send), 0,
		     (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(data_to_send), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10); /* Let the packet enter the system */
	setblocking(sock2, false);
	memset(&src, 0, sizeof(src));

	errno = 0;

	do {
		ret = recvfrom(sock2, data_to_receive, sizeof(data_to_receive),
			       0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
	} while (ret < 0 && errno == EAGAIN);

	zassert_equal(ret, sizeof(data_to_send),
		      "Cannot receive all data (%d vs %zd) (%d)",
		      ret, sizeof(data_to_send), -errno);

	/* Send to socket 2 but read from socket 1. There should not be any
	 * data in socket 1
	 */
	ret = sendto(sock2, data_to_send, sizeof(data_to_send), 0,
		     (const struct sockaddr *)&dst, sizeof(struct sockaddr_ll));
	zassert_equal(ret, sizeof(data_to_send), "Cannot send all data (%d)",
		      -errno);

	k_msleep(10);
	setblocking(sock1, false);
	memset(&src, 0, sizeof(src));

	ret = recvfrom(sock1, data_to_receive, sizeof(data_to_receive), 0,
		       (struct sockaddr *)&src, &addrlen);
	zassert_equal(ret, -1, "Received something (%d)", ret);
	zassert_equal(errno, EAGAIN, "Wrong errno (%d)", errno);

	memset(&src, 0, sizeof(src));

	errno = 0;

	do {
		ret = recvfrom(sock2, data_to_receive, sizeof(data_to_receive),
			       0, (struct sockaddr *)&src, &addrlen);
		k_msleep(10);
	} while (ret < 0 && errno == EAGAIN);

	zassert_equal(ret, sizeof(data_to_send), "Cannot receive all data (%d)",
		      -errno);
}

void test_main(void)
{
	ztest_test_suite(socket_packet,
			 ztest_unit_test(test_packet_sockets),
			 ztest_unit_test(test_packet_sockets_dgram));
	ztest_run_test_suite(socket_packet);
}
