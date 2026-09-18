/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests for the SOCKS5 client in subsys/net/lib/socks.
 *
 * The SOCKS5 request carries the destination address independently of the
 * address family used to reach the proxy itself, so all four combinations of
 * proxy and destination family have to produce the right request. These tests
 * run a minimal SOCKS5 proxy in a helper thread, let the client connect
 * through it, and inspect the bytes that actually reached the proxy.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_SOCKS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>

#include "socks_internal.h"

#define PROXY_IPV4_ADDR "127.0.0.1"
#define PROXY_IPV6_ADDR "::1"

/* Documentation ranges, nothing ever connects to these. The fake proxy only
 * reports success, so the addresses just have to be recognisable on the wire.
 */
#define DEST_IPV4_ADDR "192.0.2.1"
#define DEST_IPV6_ADDR "2001:db8::1"

#define DEST_PORT 4242

#define PROXY_STACK_SIZE 2048
#define PROXY_RECV_TIMEOUT_US 300000
#define PROXY_DONE_TIMEOUT K_SECONDS(2)

/* VER + CMD + RSV + ATYP, then the address and the port */
#define REQUEST_IPV4_SIZE (4 + 4 + 2)
#define REQUEST_IPV6_SIZE (4 + 16 + 2)

/* What the fake proxy saw. Filled in by the proxy thread, read by the test
 * thread once the proxy is done, so that a mismatch is reported with the bytes
 * that were actually received.
 */
struct proxy_record {
	uint8_t method_req[8];
	int method_req_len;
	uint8_t cmd_req[32];
	int cmd_req_len;
	bool accepted;
};

static struct proxy_record proxy;
static K_SEM_DEFINE(proxy_done, 0, 1);
static K_THREAD_STACK_DEFINE(proxy_stack, PROXY_STACK_SIZE);
static struct k_thread proxy_thread;

static int listen_sock = -1;
static int client_sock = -1;

/* Make the proxy answer the negotiation with NO ACCEPTABLE METHODS, which
 * takes the client down its negotiation failure path.
 */
static bool proxy_reject_auth;

/* Each test uses its own proxy port so that a lingering TCP connection from
 * the previous test cannot make bind() fail.
 */
static uint16_t proxy_port = 1080;

static void proxy_thread_fn(void *p1, void *p2, void *p3)
{
	static const uint8_t method_rsp[] = {
		SOCKS5_PKT_MAGIC, SOCKS5_AUTH_METHOD_NOAUTH
	};
	static const uint8_t method_rsp_reject[] = {
		SOCKS5_PKT_MAGIC, SOCKS5_AUTH_METHOD_NONEG
	};
	static const uint8_t cmd_rsp[] = {
		SOCKS5_PKT_MAGIC, SOCKS5_CMD_RESP_SUCCESS, SOCKS5_PKT_RSV,
		SOCKS5_ATYP_IPV4, 0, 0, 0, 0, 0, 0
	};
	struct timeval timeo = {
		.tv_sec = 0,
		.tv_usec = PROXY_RECV_TIMEOUT_US,
	};
	struct net_sockaddr addr;
	net_socklen_t addrlen = sizeof(addr);
	int sock;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sock = zsock_accept(listen_sock, &addr, &addrlen);
	if (sock < 0) {
		goto out;
	}

	proxy.accepted = true;

	(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO,
			       &timeo, sizeof(timeo));

	ret = zsock_recv(sock, proxy.method_req, sizeof(proxy.method_req), 0);
	if (ret <= 0) {
		goto close;
	}

	proxy.method_req_len = ret;

	if (proxy_reject_auth) {
		ret = zsock_send(sock, method_rsp_reject,
				 sizeof(method_rsp_reject), 0);
	} else {
		ret = zsock_send(sock, method_rsp, sizeof(method_rsp), 0);
	}

	if (ret < 0) {
		goto close;
	}

	/* A client that rejects the destination must not send anything here,
	 * so a timeout is a valid and expected outcome.
	 */
	ret = zsock_recv(sock, proxy.cmd_req, sizeof(proxy.cmd_req), 0);
	if (ret > 0) {
		proxy.cmd_req_len = ret;
		(void)zsock_send(sock, cmd_rsp, sizeof(cmd_rsp), 0);
	}

close:
	(void)zsock_close(sock);
out:
	k_sem_give(&proxy_done);
}

/* Bring up the fake proxy on the loopback interface and create a client socket
 * that is configured to reach it. The proxy address family has to match the
 * client socket family, the SOCKS5 option enforces that.
 */
static void proxy_setup(int family, struct net_sockaddr *proxy_addr,
			net_socklen_t proxy_len)
{
	int ret;

	listen_sock = zsock_socket(family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(listen_sock >= 0, "proxy socket failed (%d)", errno);

	ret = zsock_bind(listen_sock, proxy_addr, proxy_len);
	zassert_equal(ret, 0, "proxy bind failed (%d)", errno);

	ret = zsock_listen(listen_sock, 1);
	zassert_equal(ret, 0, "proxy listen failed (%d)", errno);

	client_sock = zsock_socket(family, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	zassert_true(client_sock >= 0, "client socket failed (%d)", errno);

	ret = zsock_setsockopt(client_sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_SOCKS5,
			       proxy_addr, proxy_len);
	zassert_equal(ret, 0, "SO_SOCKS5 failed (%d)", errno);

	(void)k_thread_create(&proxy_thread, proxy_stack,
			      K_THREAD_STACK_SIZEOF(proxy_stack),
			      proxy_thread_fn, NULL, NULL, NULL,
			      k_thread_priority_get(k_current_get()), 0,
			      K_NO_WAIT);
}

static void proxy_setup_v4(void)
{
	struct net_sockaddr_in addr = { 0 };

	addr.sin_family = NET_AF_INET;
	addr.sin_port = net_htons(proxy_port);
	zassert_equal(zsock_inet_pton(NET_AF_INET, PROXY_IPV4_ADDR,
				      &addr.sin_addr), 1, "inet_pton failed");

	proxy_setup(NET_AF_INET, (struct net_sockaddr *)&addr, sizeof(addr));
}

static void proxy_setup_v6(void)
{
	struct net_sockaddr_in6 addr = { 0 };

	addr.sin6_family = NET_AF_INET6;
	addr.sin6_port = net_htons(proxy_port);
	zassert_equal(zsock_inet_pton(NET_AF_INET6, PROXY_IPV6_ADDR,
				      &addr.sin6_addr), 1, "inet_pton failed");

	proxy_setup(NET_AF_INET6, (struct net_sockaddr *)&addr, sizeof(addr));
}

static void proxy_wait(void)
{
	zassert_equal(k_sem_take(&proxy_done, PROXY_DONE_TIMEOUT), 0,
		      "proxy thread did not finish");

	zexpect_true(proxy.accepted, "proxy did not accept the connection");
	zexpect_equal(proxy.method_req_len, 3,
		      "method request was %d bytes, expected 3",
		      proxy.method_req_len);
}

static void check_common_header(void)
{
	zexpect_equal(proxy.cmd_req[0], SOCKS5_PKT_MAGIC,
		      "bad version 0x%02x", proxy.cmd_req[0]);
	zexpect_equal(proxy.cmd_req[1], SOCKS5_CMD_CONNECT,
		      "bad command 0x%02x", proxy.cmd_req[1]);
	zexpect_equal(proxy.cmd_req[2], SOCKS5_PKT_RSV,
		      "bad reserved 0x%02x", proxy.cmd_req[2]);
}

/* The request must carry the destination as IPv4, whatever the proxy family */
static void check_ipv4_request(void)
{
	struct net_in_addr expected;
	uint16_t port;

	zassert_equal(proxy.cmd_req_len, REQUEST_IPV4_SIZE,
		      "request was %d bytes, expected %d (atyp 0x%02x)",
		      proxy.cmd_req_len, REQUEST_IPV4_SIZE, proxy.cmd_req[3]);

	check_common_header();

	zexpect_equal(proxy.cmd_req[3], SOCKS5_ATYP_IPV4,
		      "atyp was 0x%02x, expected 0x%02x", proxy.cmd_req[3],
		      SOCKS5_ATYP_IPV4);

	zassert_equal(zsock_inet_pton(NET_AF_INET, DEST_IPV4_ADDR, &expected),
		      1, "inet_pton failed");
	zexpect_mem_equal(&proxy.cmd_req[4], &expected, sizeof(expected),
			  "destination address mismatch");

	memcpy(&port, &proxy.cmd_req[8], sizeof(port));
	zexpect_equal(port, net_htons(DEST_PORT), "destination port mismatch");
}

/* The request must carry the destination as IPv6, whatever the proxy family */
static void check_ipv6_request(void)
{
	struct net_in6_addr expected;
	uint16_t port;

	zassert_equal(proxy.cmd_req_len, REQUEST_IPV6_SIZE,
		      "request was %d bytes, expected %d (atyp 0x%02x)",
		      proxy.cmd_req_len, REQUEST_IPV6_SIZE, proxy.cmd_req[3]);

	check_common_header();

	zexpect_equal(proxy.cmd_req[3], SOCKS5_ATYP_IPV6,
		      "atyp was 0x%02x, expected 0x%02x", proxy.cmd_req[3],
		      SOCKS5_ATYP_IPV6);

	zassert_equal(zsock_inet_pton(NET_AF_INET6, DEST_IPV6_ADDR, &expected),
		      1, "inet_pton failed");
	zexpect_mem_equal(&proxy.cmd_req[4], &expected, sizeof(expected),
			  "destination address mismatch");

	memcpy(&port, &proxy.cmd_req[20], sizeof(port));
	zexpect_equal(port, net_htons(DEST_PORT), "destination port mismatch");
}

static void make_dest_v4(struct net_sockaddr_in *dest)
{
	memset(dest, 0, sizeof(*dest));
	dest->sin_family = NET_AF_INET;
	dest->sin_port = net_htons(DEST_PORT);
	zassert_equal(zsock_inet_pton(NET_AF_INET, DEST_IPV4_ADDR,
				      &dest->sin_addr), 1, "inet_pton failed");
}

static void make_dest_v6(struct net_sockaddr_in6 *dest)
{
	memset(dest, 0, sizeof(*dest));
	dest->sin6_family = NET_AF_INET6;
	dest->sin6_port = net_htons(DEST_PORT);
	zassert_equal(zsock_inet_pton(NET_AF_INET6, DEST_IPV6_ADDR,
				      &dest->sin6_addr), 1, "inet_pton failed");
}

/* An IPv4 destination reached through an IPv6 proxy. Before the fix the
 * destination was decoded using the proxy family, so 16 bytes were read out of
 * an 8 byte struct net_sockaddr_in and sent as an IPv6 address.
 */
ZTEST(net_socks5, test_v6_proxy_v4_dest)
{
	struct net_sockaddr_in dest;
	int ret;

	proxy_setup_v6();
	make_dest_v4(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zexpect_equal(ret, 0, "connect failed (%d)", errno);

	proxy_wait();
	check_ipv4_request();
}

/* An IPv6 destination reached through an IPv4 proxy, which is the whole point
 * of using a proxy on an IPv4 only node. Before the fix this was encoded as an
 * IPv4 address built from the first four bytes of the IPv6 address.
 */
ZTEST(net_socks5, test_v4_proxy_v6_dest)
{
	struct net_sockaddr_in6 dest;
	int ret;

	proxy_setup_v4();
	make_dest_v6(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zexpect_equal(ret, 0, "connect failed (%d)", errno);

	proxy_wait();
	check_ipv6_request();
}

ZTEST(net_socks5, test_v4_proxy_v4_dest)
{
	struct net_sockaddr_in dest;
	int ret;

	proxy_setup_v4();
	make_dest_v4(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zexpect_equal(ret, 0, "connect failed (%d)", errno);

	proxy_wait();
	check_ipv4_request();
}

ZTEST(net_socks5, test_v6_proxy_v6_dest)
{
	struct net_sockaddr_in6 dest;
	int ret;

	proxy_setup_v6();
	make_dest_v6(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zexpect_equal(ret, 0, "connect failed (%d)", errno);

	proxy_wait();
	check_ipv6_request();
}

/* A destination the SOCKS5 request cannot express must be refused before
 * anything is put on the wire. Before the fix neither branch was taken, so the
 * request was sent with an uninitialised address type and a stale length.
 */
ZTEST(net_socks5, test_unsupported_dest_family)
{
	struct net_sockaddr_in6 dest;
	int ret;

	proxy_setup_v6();
	make_dest_v6(&dest);
	dest.sin6_family = NET_AF_UNSPEC;

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zexpect_equal(ret, -1, "connect should have failed");
	zexpect_equal(errno, EAFNOSUPPORT, "unexpected errno %d", errno);

	proxy_wait();
	zexpect_equal(proxy.cmd_req_len, 0,
		      "%d bytes of CONNECT request were sent for an "
		      "unsupported destination family", proxy.cmd_req_len);
}

/* A destination that is too short for the family it claims must be refused
 * rather than read past its end.
 */
ZTEST(net_socks5, test_short_dest_len)
{
	struct net_sockaddr_in6 dest;
	int ret;

	proxy_setup_v6();
	make_dest_v6(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(struct net_sockaddr_in));
	zexpect_equal(ret, -1, "connect should have failed");
	zexpect_equal(errno, EINVAL, "unexpected errno %d", errno);

	proxy_wait();
	zexpect_equal(proxy.cmd_req_len, 0,
		      "%d bytes of CONNECT request were sent for a truncated "
		      "destination address", proxy.cmd_req_len);
}

/* A proxy that refuses the offered authentication method takes the client down
 * an error path that leaves socks5_tcp_connect() early. The response callbacks
 * point at that function's stack frame, so they have to be uninstalled before
 * it returns, otherwise closing the socket makes the TCP stack call into a
 * frame that no longer exists. Run under CONFIG_ASAN to catch that.
 */
ZTEST(net_socks5, test_proxy_rejects_auth)
{
	struct net_sockaddr_in6 dest;
	int ret;

	proxy_reject_auth = true;

	proxy_setup_v6();
	make_dest_v6(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zexpect_equal(ret, -1, "connect should have failed");
	zexpect_equal(errno, ENOTSUP, "unexpected errno %d", errno);

	proxy_wait();
	zexpect_equal(proxy.cmd_req_len, 0,
		      "%d bytes of CONNECT request were sent after a failed "
		      "negotiation", proxy.cmd_req_len);
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	memset(&proxy, 0, sizeof(proxy));
	k_sem_reset(&proxy_done);
	proxy_reject_auth = false;

	proxy_port++;
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	if (client_sock >= 0) {
		(void)zsock_close(client_sock);
		client_sock = -1;
	}

	if (listen_sock >= 0) {
		(void)zsock_close(listen_sock);
		listen_sock = -1;
	}

	/* Make sure a thread blocked in accept() cannot outlive the test */
	(void)k_thread_join(&proxy_thread, K_SECONDS(1));
}

ZTEST_SUITE(net_socks5, NULL, NULL, before, after, NULL);
