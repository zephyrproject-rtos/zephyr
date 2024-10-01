/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/sys/sem.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/dummy.h>

#include "net_private.h"

#include "../../socket_helpers.h"

ZTEST_USER(socket_misc_test_suite, test_gethostname)
{
	static ZTEST_BMEM char buf[80];
	int res;

	res = zsock_gethostname(buf, sizeof(buf));
	zassert_equal(res, 0, "");
	printk("%s\n", buf);
	zassert_equal(strcmp(buf, "ztest_hostname"), 0, "");
}

ZTEST_USER(socket_misc_test_suite, test_inet_pton)
{
	int res;
	uint8_t buf[32];

	res = zsock_inet_pton(AF_INET, "127.0.0.1", buf);
	zassert_equal(res, 1, "");

	res = zsock_inet_pton(AF_INET, "127.0.0.1a", buf);
	zassert_equal(res, 0, "");

	res = zsock_inet_pton(AF_INET6, "a:b:c:d:0:1:2:3", buf);
	zassert_equal(res, 1, "");

	res = zsock_inet_pton(AF_INET6, "::1", buf);
	zassert_equal(res, 1, "");

	res = zsock_inet_pton(AF_INET6, "1::", buf);
	zassert_equal(res, 1, "");

	res = zsock_inet_pton(AF_INET6, "a:b:c:d:0:1:2:3z", buf);
	zassert_equal(res, 0, "");
}

#define TEST_MY_IPV4_ADDR "192.0.2.1"
#define TEST_PEER_IPV4_ADDR "192.0.2.2"
#define TEST_MY_IPV6_ADDR "2001:db8::1"
#define TEST_PEER_IPV6_ADDR "2001:db8::2"

static struct in6_addr my_ipv6_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in_addr my_ipv4_addr1 = { { { 192, 0, 2, 1 } } };

static struct in6_addr my_ipv6_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in_addr my_ipv4_addr2 = { { { 192, 0, 2, 2 } } };

static uint8_t lladdr1[] = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
static uint8_t lladdr2[] = { 0x02, 0x02, 0x02, 0x02, 0x02, 0x02 };

struct dummy_context {
	struct net_if *iface;
	uint8_t *mac_address;
	struct in6_addr *ipv6_addr;
	struct in_addr *ipv4_addr;
};

static struct dummy_context dummy_data1 = {
	.mac_address = lladdr1,
	.ipv6_addr = &my_ipv6_addr1,
	.ipv4_addr = &my_ipv4_addr1
};
static struct dummy_context dummy_data2 = {
	.mac_address = lladdr2,
	.ipv6_addr = &my_ipv6_addr2,
	.ipv4_addr = &my_ipv4_addr2
};

static ZTEST_BMEM const struct device *current_dev;
static ZTEST_BMEM struct sys_sem send_sem;

static int dummy_send(const struct device *dev, struct net_pkt *pkt)
{
	struct net_pkt *recv_pkt;
	int ret;

	ARG_UNUSED(dev);

	NET_DBG("Sending data (%zd bytes) to iface %d",
		net_pkt_get_len(pkt), net_if_get_by_iface(net_pkt_iface(pkt)));

	current_dev = dev;

	sys_sem_give(&send_sem);

	/* Report it back to the interface. */
	recv_pkt = net_pkt_clone(pkt, K_NO_WAIT);
	if (recv_pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_recv_data(net_pkt_iface(recv_pkt), recv_pkt);
	zassert_equal(ret, 0, "Cannot receive data (%d)", ret);

	net_pkt_unref(pkt);

	return 0;
}

static void dummy_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct dummy_context *ctx = dev->data;
	struct net_if_addr *ifaddr;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address, 6, NET_LINK_DUMMY);

	ifaddr = net_if_ipv6_addr_add(net_if_lookup_by_dev(dev), ctx->ipv6_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		zassert_not_null(ifaddr, "ipv6 addr");
	}

	ifaddr = net_if_ipv4_addr_add(net_if_lookup_by_dev(dev), ctx->ipv4_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		zassert_not_null(ifaddr, "ipv4 addr");
	}
}

static struct dummy_api dummy_api_funcs = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_send,
};

#if defined(CONFIG_NET_INTERFACE_NAME)
#define DEV1_NAME "dummy0"
#define DEV2_NAME "dummy1"
#else
#define DEV1_NAME "dummy_1"
#define DEV2_NAME "dummy_2"
#endif

NET_DEVICE_INIT(dummy_1, DEV1_NAME, NULL, NULL, &dummy_data1, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_api_funcs,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);


NET_DEVICE_INIT(dummy_2, DEV2_NAME, NULL, NULL, &dummy_data2, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_api_funcs,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

#define DST_PORT 4242
#define BIND_PORT 4240

void test_so_bindtodevice(int sock_c, int sock_s, struct sockaddr *peer_addr,
			  socklen_t peer_addrlen, struct sockaddr *bind_addr,
			  socklen_t bind_addrlen)
{
	int ret;
	struct ifreq ifreq = { 0 };

#if !defined(CONFIG_NET_INTERFACE_NAME)
	const struct device *dev1 = device_get_binding(DEV1_NAME);
	const struct device *dev2 = device_get_binding(DEV2_NAME);
#endif

	uint8_t send_buf[32];
	uint8_t recv_buf[sizeof(send_buf)] = { 0 };

	ret = zsock_bind(sock_s, bind_addr, bind_addrlen);
	zassert_equal(ret, 0, "bind failed, %d", errno);

	/* Bind server socket with interface 2. */

	strcpy(ifreq.ifr_name, DEV2_NAME);
	ret = zsock_setsockopt(sock_s, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed, %d", errno);

	/* Bind client socket with interface 1 and send a packet. */

	current_dev = NULL;
	sys_sem_init(&send_sem, 0, 1);
	strcpy(ifreq.ifr_name, DEV1_NAME);
	strcpy(send_buf, DEV1_NAME);

	ret = zsock_setsockopt(sock_c, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed, %d", errno);

	ret = zsock_sendto(sock_c, send_buf, strlen(send_buf) + 1, 0,
			   peer_addr, peer_addrlen);
	zassert_equal(ret, strlen(send_buf) + 1, "sendto failed, %d", errno);

	ret = sys_sem_take(&send_sem, K_MSEC(100));
	zassert_equal(ret, 0, "iface did not receive packet");

#if !defined(CONFIG_NET_INTERFACE_NAME)
	zassert_equal_ptr(dev1, current_dev, "invalid interface used (%p vs %p)",
			  dev1, current_dev);
#endif

	k_msleep(10);

	/* Bind client socket with interface 2 and send a packet. */

	current_dev = NULL;
	sys_sem_init(&send_sem, 0, 1);
	strcpy(ifreq.ifr_name, DEV2_NAME);
	strcpy(send_buf, DEV2_NAME);

	ret = zsock_setsockopt(sock_c, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed, %d", errno);

	ret = zsock_sendto(sock_c, send_buf, strlen(send_buf) + 1, 0,
			   peer_addr, peer_addrlen);
	zassert_equal(ret, strlen(send_buf) + 1, "sendto failed, %d", errno);

	ret = sys_sem_take(&send_sem, K_MSEC(100));
	zassert_equal(ret, 0, "iface did not receive packet");

#if !defined(CONFIG_NET_INTERFACE_NAME)
	zassert_equal_ptr(dev2, current_dev, "invalid interface used (%p vs %p)",
			  dev2, current_dev);
#endif

	/* Server socket should only receive data from the bound interface. */

	k_msleep(10);

	ret = zsock_recv(sock_s, recv_buf, sizeof(recv_buf), ZSOCK_MSG_DONTWAIT);
	zassert_true(ret > 0, "recv failed, %d", errno);
	zassert_mem_equal(recv_buf, DEV2_NAME, strlen(DEV2_NAME),
			 "received datagram from invalid interface");

	/* Remove the binding from the server socket. */

	strcpy(ifreq.ifr_name, "");
	ret = zsock_setsockopt(sock_s, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed, %d", errno);

	/* Bind client socket with interface 1 again. */

	sys_sem_init(&send_sem, 0, 1);
	strcpy(ifreq.ifr_name, DEV1_NAME);
	strcpy(send_buf, DEV1_NAME);

	ret = zsock_setsockopt(sock_c, SOL_SOCKET, SO_BINDTODEVICE, &ifreq,
			       sizeof(ifreq));
	zassert_equal(ret, 0, "SO_BINDTODEVICE failed, %d", errno);

	ret = zsock_sendto(sock_c, send_buf, strlen(send_buf) + 1, 0,
			   peer_addr, peer_addrlen);
	zassert_equal(ret, strlen(send_buf) + 1, "sendto failed, %d", errno);

	ret = sys_sem_take(&send_sem, K_MSEC(100));
	zassert_equal(ret, 0, "iface did not receive packet");

#if !defined(CONFIG_NET_INTERFACE_NAME)
	zassert_equal_ptr(dev1, current_dev, "invalid interface used");
#endif

	/* Server socket should now receive data from interface 1 as well. */

	k_msleep(10);

	ret = zsock_recv(sock_s, recv_buf, sizeof(recv_buf), ZSOCK_MSG_DONTWAIT);
	zassert_true(ret > 0, "recv failed, %d", errno);
	zassert_mem_equal(recv_buf, DEV1_NAME, strlen(DEV1_NAME),
			 "received datagram from invalid interface");

	ret = zsock_close(sock_c);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(sock_s);
	zassert_equal(ret, 0, "close failed, %d", errno);

	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

void test_ipv4_so_bindtodevice(void)
{
	int ret;
	int sock_c;
	int sock_s;
	struct sockaddr_in peer_addr;
	struct sockaddr_in bind_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(DST_PORT),
		.sin_addr = INADDR_ANY_INIT,
	};

	sock_c = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_c >= 0, "socket open failed");
	sock_s = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_s >= 0, "socket open failed");

	peer_addr.sin_family = AF_INET;
	peer_addr.sin_port = htons(DST_PORT);
	ret = zsock_inet_pton(AF_INET, TEST_PEER_IPV4_ADDR,
			      &peer_addr.sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	test_so_bindtodevice(sock_c, sock_s, (struct sockaddr *)&peer_addr,
			     sizeof(peer_addr), (struct sockaddr *)&bind_addr,
			     sizeof(bind_addr));
}

void test_ipv6_so_bindtodevice(void)
{
	int ret;
	int sock_c;
	int sock_s;
	struct sockaddr_in6 peer_addr;
	struct sockaddr_in6 bind_addr = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(DST_PORT),
		.sin6_addr = IN6ADDR_ANY_INIT,
	};

	sock_c = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_c >= 0, "socket open failed");
	sock_s = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_s >= 0, "socket open failed");

	peer_addr.sin6_family = AF_INET6;
	peer_addr.sin6_port = htons(DST_PORT);
	ret = zsock_inet_pton(AF_INET6, TEST_PEER_IPV6_ADDR,
			      &peer_addr.sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	test_so_bindtodevice(sock_c, sock_s, (struct sockaddr *)&peer_addr,
			     sizeof(peer_addr), (struct sockaddr *)&bind_addr,
			     sizeof(bind_addr));
}

#define ADDR_SIZE(family) ((family == AF_INET) ? \
			   sizeof(struct sockaddr_in) : \
			   sizeof(struct sockaddr_in6))

void test_getpeername(int family)
{
	int ret;
	int sock_c;
	int sock_s;
	struct sockaddr peer_addr;
	socklen_t peer_addr_len;
	struct sockaddr srv_addr = { 0 };

	srv_addr.sa_family = family;
	if (family == AF_INET) {
		net_sin(&srv_addr)->sin_port = htons(DST_PORT);
		ret = zsock_inet_pton(AF_INET, TEST_MY_IPV4_ADDR,
				      &net_sin(&srv_addr)->sin_addr);
	} else {
		net_sin6(&srv_addr)->sin6_port = htons(DST_PORT);
		ret = zsock_inet_pton(AF_INET6, TEST_MY_IPV6_ADDR,
				      &net_sin6(&srv_addr)->sin6_addr);
	}
	zassert_equal(ret, 1, "inet_pton failed");

	/* UDP socket */
	sock_c = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_c >= 0, "socket open failed");

	peer_addr_len = ADDR_SIZE(family);
	ret = zsock_getpeername(sock_c, &peer_addr, &peer_addr_len);
	zassert_equal(ret, -1, "getpeername shouldn've failed");
	zassert_equal(errno, ENOTCONN, "getpeername returned invalid error");

	ret = zsock_connect(sock_c, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "connect failed");

	memset(&peer_addr, 0, sizeof(peer_addr));
	peer_addr_len = ADDR_SIZE(family);
	ret = zsock_getpeername(sock_c, &peer_addr, &peer_addr_len);
	zassert_equal(ret, 0, "getpeername failed");
	zassert_mem_equal(&peer_addr, &srv_addr, ADDR_SIZE(family),
			 "obtained wrong address");

	ret = zsock_close(sock_c);
	zassert_equal(ret, 0, "close failed, %d", errno);

	/* TCP socket */
	sock_c = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_c >= 0, "socket open failed");
	sock_s = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s >= 0, "socket open failed");

	ret = zsock_bind(sock_s, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	ret = zsock_listen(sock_s, 1);
	zassert_equal(ret, 0, "listen failed, %d", errno);

	peer_addr_len = ADDR_SIZE(family);
	ret = zsock_getpeername(sock_c, &peer_addr, &peer_addr_len);
	zassert_equal(ret, -1, "getpeername shouldn've failed");
	zassert_equal(errno, ENOTCONN, "getpeername returned invalid error");

	ret = zsock_connect(sock_c, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "connect failed");

	memset(&peer_addr, 0, sizeof(peer_addr));
	peer_addr_len = ADDR_SIZE(family);
	ret = zsock_getpeername(sock_c, &peer_addr, &peer_addr_len);
	zassert_equal(ret, 0, "getpeername failed");
	zassert_mem_equal(&peer_addr, &srv_addr, ADDR_SIZE(family),
			 "obtained wrong address");

	ret = zsock_close(sock_c);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(sock_s);
	zassert_equal(ret, 0, "close failed, %d", errno);

	k_sleep(K_MSEC(2 * CONFIG_NET_TCP_TIME_WAIT_DELAY));
}


void test_ipv4_getpeername(void)
{
	test_getpeername(AF_INET);
}

void test_ipv6_getpeername(void)
{
	test_getpeername(AF_INET6);
}

void test_getsockname_tcp(int family)
{
	int ret;
	int sock_c;
	int sock_s;
	struct sockaddr local_addr = { 0 };
	socklen_t local_addr_len = ADDR_SIZE(family);
	struct sockaddr srv_addr = { 0 };

	srv_addr.sa_family = family;
	if (family == AF_INET) {
		net_sin(&srv_addr)->sin_port = htons(DST_PORT);
		ret = zsock_inet_pton(AF_INET, TEST_MY_IPV4_ADDR,
				      &net_sin(&srv_addr)->sin_addr);
	} else {
		net_sin6(&srv_addr)->sin6_port = htons(DST_PORT);
		ret = zsock_inet_pton(AF_INET6, TEST_MY_IPV6_ADDR,
				      &net_sin6(&srv_addr)->sin6_addr);
	}
	zassert_equal(ret, 1, "inet_pton failed");

	sock_c = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_c >= 0, "socket open failed");
	sock_s = zsock_socket(family, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s >= 0, "socket open failed");

	/* Verify that unbound/unconnected socket has no local address set */
	ret = zsock_getsockname(sock_c, &local_addr, &local_addr_len);
	zassert_equal(ret, -1, "getsockname shouldn've failed");
	zassert_equal(errno, EINVAL, "getsockname returned invalid error");
	ret = zsock_getsockname(sock_s, &local_addr, &local_addr_len);
	zassert_equal(ret, -1, "getsockname shouldn've failed");
	zassert_equal(errno, EINVAL, "getsockname returned invalid error");

	/* Verify that getsockname() can read local address of a bound socket */
	ret = zsock_bind(sock_s, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr_len = ADDR_SIZE(family);
	ret = zsock_getsockname(sock_s, &local_addr, &local_addr_len);
	zassert_equal(ret, 0, "getsockname failed");
	zassert_mem_equal(&local_addr, &srv_addr, ADDR_SIZE(family),
			 "obtained wrong address");

	ret = zsock_listen(sock_s, 1);
	zassert_equal(ret, 0, "listen failed, %d", errno);

	/* Verify that getsockname() can read local address of a connected socket */
	ret = zsock_connect(sock_c, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "connect failed");

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr_len = ADDR_SIZE(family);
	ret = zsock_getsockname(sock_c, &local_addr, &local_addr_len);
	zassert_equal(ret, 0, "getsockname failed");
	zassert_equal(local_addr.sa_family, family, "wrong family");
	/* Can't verify address/port of client socket here reliably as they're
	 * chosen by net stack
	 */

	ret = zsock_close(sock_c);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(sock_s);
	zassert_equal(ret, 0, "close failed, %d", errno);

	k_sleep(K_MSEC(CONFIG_NET_TCP_TIME_WAIT_DELAY));
}

void test_getsockname_udp(int family)
{
	int ret;
	int sock_c;
	int sock_s;
	struct sockaddr local_addr = { 0 };
	socklen_t local_addr_len = ADDR_SIZE(family);
	struct sockaddr srv_addr = { 0 };

	srv_addr.sa_family = family;
	if (family == AF_INET) {
		net_sin(&srv_addr)->sin_port = htons(DST_PORT);
		ret = zsock_inet_pton(AF_INET, TEST_MY_IPV4_ADDR,
				      &net_sin(&srv_addr)->sin_addr);
	} else {
		net_sin6(&srv_addr)->sin6_port = htons(DST_PORT);
		ret = zsock_inet_pton(AF_INET6, TEST_MY_IPV6_ADDR,
				      &net_sin6(&srv_addr)->sin6_addr);
	}
	zassert_equal(ret, 1, "inet_pton failed");

	sock_c = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_c >= 0, "socket open failed");
	sock_s = zsock_socket(family, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_s >= 0, "socket open failed");

	/* Verify that unbound/unconnected socket has no local address set */
	ret = zsock_getsockname(sock_c, &local_addr, &local_addr_len);
	zassert_equal(ret, -1, "getsockname shouldn've failed");
	zassert_equal(errno, EINVAL, "getsockname returned invalid error");
	ret = zsock_getsockname(sock_s, &local_addr, &local_addr_len);
	zassert_equal(ret, -1, "getsockname shouldn've failed");
	zassert_equal(errno, EINVAL, "getsockname returned invalid error");

	/* Verify that getsockname() can read local address of a bound socket */
	ret = zsock_bind(sock_s, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr_len = ADDR_SIZE(family);
	ret = zsock_getsockname(sock_s, &local_addr, &local_addr_len);
	zassert_equal(ret, 0, "getsockname failed");
	zassert_mem_equal(&local_addr, &srv_addr, ADDR_SIZE(family),
			 "obtained wrong address");

	/* Verify that getsockname() can read local address of a connected socket */
	ret = zsock_connect(sock_c, &srv_addr, ADDR_SIZE(family));
	zassert_equal(ret, 0, "connect failed");

	memset(&local_addr, 0, sizeof(local_addr));
	local_addr_len = ADDR_SIZE(family);
	ret = zsock_getsockname(sock_c, &local_addr, &local_addr_len);
	zassert_equal(ret, 0, "getsockname failed");
	zassert_equal(local_addr.sa_family, family, "wrong family");
	/* Can't verify address/port of client socket here reliably as they're
	 * chosen by net stack
	 */

	ret = zsock_close(sock_c);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(sock_s);
	zassert_equal(ret, 0, "close failed, %d", errno);
}

#define MAPPING_PORT 4244

void test_ipv4_mapped_to_ipv6_disabled(void)
{
	int ret;
	int sock_s4, sock_s6;
	struct sockaddr srv_addr4 = { 0 };
	struct sockaddr srv_addr6 = { 0 };

	if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
		return;
	}

	srv_addr4.sa_family = AF_INET;
	net_sin(&srv_addr4)->sin_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET, TEST_MY_IPV4_ADDR,
			      &net_sin(&srv_addr4)->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	srv_addr6.sa_family = AF_INET6;
	net_sin6(&srv_addr6)->sin6_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET6, TEST_MY_IPV6_ADDR,
			      &net_sin6(&srv_addr6)->sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	sock_s4 = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s4 >= 0, "socket open failed");

	ret = zsock_bind(sock_s4, &srv_addr4, ADDR_SIZE(AF_INET));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	ret = zsock_listen(sock_s4, 1);
	zassert_equal(ret, 0, "listen failed, %d", errno);

	sock_s6 = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s6 >= 0, "socket open failed");

	ret = zsock_bind(sock_s6, &srv_addr6, ADDR_SIZE(AF_INET6));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	ret = zsock_close(sock_s4);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(sock_s6);
	zassert_equal(ret, 0, "close failed, %d", errno);
}

void test_ipv4_mapped_to_ipv6_enabled(void)
{
	socklen_t optlen = sizeof(int);
	int off = 0;
	int ret;
	int sock_s4, sock_s6;
	struct sockaddr srv_addr4 = { 0 };
	struct sockaddr srv_addr6 = { 0 };

	if (!IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
		return;
	}

	/* We must have bound to ANY address so that the
	 * v4-mapping-to-v6 works as expected.
	 */
	srv_addr4.sa_family = AF_INET;
	net_sin(&srv_addr4)->sin_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET, "0.0.0.0",
			      &net_sin(&srv_addr4)->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	srv_addr6.sa_family = AF_INET6;
	net_sin6(&srv_addr6)->sin6_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET6, "::",
			      &net_sin6(&srv_addr6)->sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* First create IPv6 socket */
	sock_s6 = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s6 >= 0, "socket open failed");

	ret = zsock_bind(sock_s6, &srv_addr6, ADDR_SIZE(AF_INET6));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	ret = zsock_listen(sock_s6, 1);
	zassert_equal(ret, 0, "listen failed, %d", errno);

	/* Then try to create IPv4 socket to the same port */
	sock_s4 = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s4 >= 0, "socket open failed");

	/* Initially the IPV6_V6ONLY is set so the next bind is ok */
	ret = zsock_bind(sock_s4, &srv_addr4, ADDR_SIZE(AF_INET));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	ret = zsock_close(sock_s4);
	zassert_equal(ret, 0, "close failed, %d", errno);

	/* Then we turn off IPV6_V6ONLY which means that IPv4 and IPv6
	 * will have same port space and the next bind should fail.
	 */
	ret = zsock_setsockopt(sock_s6, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
	zassert_equal(ret, 0, "setsockopt failed, %d", errno);

	ret = zsock_getsockopt(sock_s6, IPPROTO_IPV6, IPV6_V6ONLY, &off, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", errno);
	zassert_equal(off, 0, "IPV6_V6ONLY option setting failed");

	sock_s4 = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s4 >= 0, "socket open failed");

	/* Now v4 bind should fail */
	ret = zsock_bind(sock_s4, &srv_addr4, ADDR_SIZE(AF_INET));
	zassert_equal(ret, -1, "bind failed, %d", errno);
	zassert_equal(errno, EADDRINUSE, "bind failed");

	ret = zsock_close(sock_s4);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(sock_s6);
	zassert_equal(ret, 0, "close failed, %d", errno);
}

void test_ipv4_mapped_to_ipv6_server(void)
{
	socklen_t optlen = sizeof(int);
	int off;
	int ret, len;
	int sock_c4, sock_c6, sock_s6, new_sock;
	struct sockaddr srv_addr6 = { 0 };
	struct sockaddr srv_addr = { 0 };
	struct sockaddr connect_addr6 = { 0 };
	struct sockaddr addr = { 0 };
	socklen_t addrlen = sizeof(addr);
	struct sockaddr_in6 mapped;

#define MAX_BUF_SIZE 16
	char buf[MAX_BUF_SIZE];

	if (!IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
		return;
	}

	len = strlen("foobar");

	/* Create a server socket and try to connect IPv4
	 * socket to it. If v4-mapping-to-v6 is enabled,
	 * this will work and we should be bound internally
	 * to ::ffff:a.b.c.d IPv6 address.
	 */
	srv_addr.sa_family = AF_INET;
	net_sin(&srv_addr)->sin_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET, "192.0.2.1",
			      &net_sin(&srv_addr)->sin_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	connect_addr6.sa_family = AF_INET6;
	net_sin6(&connect_addr6)->sin6_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET6, TEST_PEER_IPV6_ADDR,
			      &net_sin6(&connect_addr6)->sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	net_ipv6_addr_create_v4_mapped(&net_sin(&srv_addr)->sin_addr,
				       &mapped.sin6_addr);

	/* We must have bound to ANY address so that the
	 * v4-mapping-to-v6 works as expected.
	 */
	srv_addr6.sa_family = AF_INET6;
	net_sin6(&srv_addr6)->sin6_port = htons(MAPPING_PORT);
	ret = zsock_inet_pton(AF_INET6, "::",
			      &net_sin6(&srv_addr6)->sin6_addr);
	zassert_equal(ret, 1, "inet_pton failed");

	/* First create IPv6 socket */
	sock_s6 = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_s6 >= 0, "socket open failed");

	/* Verify that by default the IPV6_V6ONLY option is set */
	ret = zsock_getsockopt(sock_s6, IPPROTO_IPV6, IPV6_V6ONLY, &off, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", errno);
	zassert_not_equal(off, 0, "IPV6_V6ONLY option setting failed");

	/* Then we turn off IPV6_V6ONLY which means that IPv4 and IPv6
	 * will have same port space.
	 */
	off = 0;
	ret = zsock_setsockopt(sock_s6, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
	zassert_equal(ret, 0, "setsockopt failed, %d", errno);

	ret = zsock_getsockopt(sock_s6, IPPROTO_IPV6, IPV6_V6ONLY, &off, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", errno);
	zassert_equal(off, 0, "IPV6_V6ONLY option setting failed, %d", off);

	ret = zsock_bind(sock_s6, &srv_addr6, ADDR_SIZE(AF_INET6));
	zassert_equal(ret, 0, "bind failed, %d", errno);

	ret = zsock_listen(sock_s6, 1);
	zassert_equal(ret, 0, "listen failed, %d", errno);

	/* Then create IPv4 socket and connect to the IPv6 port */
	sock_c4 = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_c4 >= 0, "socket open failed");

	ret = zsock_connect(sock_c4, &srv_addr, ADDR_SIZE(AF_INET));
	zassert_equal(ret, 0, "connect failed");

	new_sock = zsock_accept(sock_s6, &addr, &addrlen);
	zassert_true(new_sock >= 0, "accept failed, %d", errno);

	/* Note that we should get IPv6 address here (mapped from IPv4) */
	zassert_equal(addr.sa_family, AF_INET6, "wrong family");
	zassert_equal(addrlen, sizeof(struct sockaddr_in6),
		      "wrong addrlen (%d, expecting %d)", addrlen,
		      sizeof(struct sockaddr_in6));
	zassert_mem_equal(&mapped.sin6_addr, &net_sin6(&addr)->sin6_addr,
			  sizeof(struct in6_addr),
			  "wrong address (%s, expecting %s)",
			  net_sprint_ipv6_addr(&mapped.sin6_addr),
			  net_sprint_ipv6_addr(&net_sin6(&addr)->sin6_addr));

	/* Send data back to IPv4 client from IPv6 socket */
	ret = zsock_send(new_sock, "foobar", len, 0);
	zassert_equal(ret, len, "cannot send (%d vs %d), errno %d", ret, len, errno);

	addrlen = sizeof(struct sockaddr_in);
	ret = zsock_recv(sock_c4, buf, sizeof(buf), 0);
	zassert_equal(ret, strlen("foobar"), "cannot recv");

	ret = zsock_close(sock_c4);
	zassert_equal(ret, 0, "close failed, %d", errno);

	(void)zsock_close(new_sock);

	/* Let the system stabilize and cleanup itself */
	k_sleep(K_MSEC(200));

	/* Then create IPv6 socket and make sure it works ok too */
	sock_c6 = zsock_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_c6 >= 0, "socket open failed");

	ret = zsock_connect(sock_c6, &connect_addr6, ADDR_SIZE(AF_INET6));
	zassert_equal(ret, 0, "connect failed, %d", errno);

	new_sock = zsock_accept(sock_s6, &addr, &addrlen);
	zassert_true(new_sock >= 0, "accept failed, %d", errno);

	ret = zsock_send(new_sock, "foobar", len, 0);
	zassert_equal(ret, len, "cannot send (%d vs %d), errno %d", ret, len, errno);

	addrlen = sizeof(struct sockaddr_in);
	ret = zsock_recv(sock_c6, buf, sizeof(buf), 0);
	zassert_equal(ret, strlen("foobar"), "cannot recv");

	ret = zsock_close(sock_c6);
	zassert_equal(ret, 0, "close failed, %d", errno);

	ret = zsock_close(sock_s6);
	zassert_equal(ret, 0, "close failed, %d", errno);
	ret = zsock_close(new_sock);
	zassert_equal(ret, 0, "close failed, %d", errno);
}

ZTEST_USER(socket_misc_test_suite, test_ipv4_getsockname_tcp)
{
	test_getsockname_tcp(AF_INET);
}

ZTEST_USER(socket_misc_test_suite, test_ipv4_getsockname_udp)
{
	test_getsockname_udp(AF_INET);
}

ZTEST_USER(socket_misc_test_suite, test_ipv6_getsockname_tcp)
{
	test_getsockname_tcp(AF_INET6);
}

ZTEST_USER(socket_misc_test_suite, test_ipv6_getsockname_udp)
{
	test_getsockname_udp(AF_INET6);
}

static void *setup(void)
{
	k_thread_system_pool_assign(k_current_get());
	return NULL;
}

ZTEST_USER(socket_misc_test_suite, test_ipv4)
{
	test_ipv4_so_bindtodevice();
	test_ipv4_getpeername();
}

ZTEST_USER(socket_misc_test_suite, test_ipv6)
{
	test_ipv6_so_bindtodevice();
	test_ipv6_getpeername();
}

ZTEST_USER(socket_misc_test_suite, test_ipv4_mapped_to_ipv6_disabled)
{
	test_ipv4_mapped_to_ipv6_disabled();
}

ZTEST_USER(socket_misc_test_suite, test_ipv4_mapped_to_ipv6_enabled)
{
	test_ipv4_mapped_to_ipv6_enabled();
}

ZTEST(socket_misc_test_suite, test_ipv4_mapped_to_ipv6_server)
{
	test_ipv4_mapped_to_ipv6_server();
}

ZTEST(socket_misc_test_suite, test_so_domain_socket_option)
{
	int ret;
	int sock_u;
	int sock_t;
	socklen_t optlen = sizeof(int);
	int domain;

	sock_t = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	zassert_true(sock_t >= 0, "TCP socket open failed");
	sock_u = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	zassert_true(sock_u >= 0, "UDP socket open failed");

	ret = zsock_getsockopt(sock_t, SOL_SOCKET, SO_DOMAIN, &domain, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", -errno);
	zassert_equal(domain, AF_INET, "Mismatch domain value %d vs %d",
		      AF_INET, domain);

	ret = zsock_getsockopt(sock_u, SOL_SOCKET, SO_DOMAIN, &domain, &optlen);
	zassert_equal(ret, 0, "getsockopt failed, %d", -errno);
	zassert_equal(domain, AF_INET6, "Mismatch domain value %d vs %d",
		      AF_INET6, domain);

	/* setsockopt() is not supported for this option */
	domain = AF_INET;
	ret = zsock_setsockopt(sock_u, SOL_SOCKET, SO_DOMAIN, &domain, optlen);
	zassert_equal(ret, -1, "setsockopt succeed");
	zassert_equal(errno, ENOPROTOOPT, "Invalid errno %d", errno);

	ret = zsock_close(sock_t);
	zassert_equal(ret, 0, "close failed, %d", -errno);
	ret = zsock_close(sock_u);
	zassert_equal(ret, 0, "close failed, %d", -errno);
}

ZTEST_SUITE(socket_misc_test_suite, NULL, setup, NULL, NULL, NULL);
