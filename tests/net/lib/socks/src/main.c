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
#include "tcp_internal.h"

#define PROXY_IPV4_ADDR "127.0.0.1"
#define PROXY_IPV6_ADDR "::1"

/* Documentation ranges, nothing ever connects to these. The fake proxy only
 * reports success, so the addresses just have to be recognisable on the wire.
 */
#define DEST_IPV4_ADDR "192.0.2.1"
#define DEST_IPV6_ADDR "2001:db8::1"

#define DEST_PORT 4242

/* What the destination "sends" once the proxy has relayed the connection. The
 * client must see exactly this and nothing else, whatever the reply looked
 * like on the wire.
 */
#define TEST_PAYLOAD "socks5-payload"
#define TEST_PAYLOAD_LEN (sizeof(TEST_PAYLOAD) - 1)
#define TEST_PAYLOAD_SPLIT 7

#define PROXY_STACK_SIZE 2048
#define PROXY_RECV_TIMEOUT_US 300000
#define PROXY_DONE_TIMEOUT K_SECONDS(2)
#define THREAD_SETTLE_MS 50

/* Long enough for the client to give up on the proxy before it answers */
#define PROXY_LATE_MS (CONFIG_NET_SOCKETS_CONNECT_TIMEOUT + 200)

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

/* How the proxy puts the CONNECT reply on the wire. The reply carries a bound
 * address whose length depends on its address type, and a proxy is free to
 * split it across segments or to pack relayed data in behind it, so the client
 * has to consume exactly the reply and no more.
 */
enum proxy_reply_mode {
	PROXY_REPLY_IPV4,	/* 10 bytes, one write */
	PROXY_REPLY_IPV6,	/* 22 bytes, one write */
	PROXY_REPLY_DOMAIN,	/* variable length bound address */
	PROXY_REPLY_SPLIT,	/* header first, bound address in a later segment */
	PROXY_REPLY_COALESCED,	/* reply and relayed data in one write */
};

static enum proxy_reply_mode proxy_reply_mode;

/* Send TEST_PAYLOAD after the reply, as the destination would */
static bool proxy_send_payload;

/* Send TEST_PAYLOAD straight after the reply, as it is when the destination
 * speaks first, rather than after a pause. It goes out in two writes, so that
 * more than one segment follows the reply.
 */
static bool proxy_payload_now;

/* Make the proxy answer one step of the handshake only after the client has
 * stopped waiting for it.
 */
enum proxy_late_mode {
	PROXY_LATE_NONE,
	PROXY_LATE_METHOD,	/* method selection reply */
	PROXY_LATE_REPLY,	/* CONNECT reply */
};

static enum proxy_late_mode proxy_late;

/* Each test uses its own proxy port so that a lingering TCP connection from
 * the previous test cannot make bind() fail.
 */
static uint16_t proxy_port = 1080;

/* Build and send the CONNECT reply the way the current mode asks for. The
 * bound address is all zeroes; only its length and framing matter here.
 */
static void proxy_send_reply(int sock)
{
	uint8_t rsp[4 + 1 + 8 + 2 + TEST_PAYLOAD_LEN];
	size_t len = 4;
	int nodelay = 1;

	rsp[0] = SOCKS5_PKT_MAGIC;
	rsp[1] = SOCKS5_CMD_RESP_SUCCESS;
	rsp[2] = SOCKS5_PKT_RSV;

	switch (proxy_reply_mode) {
	case PROXY_REPLY_IPV6:
		rsp[3] = SOCKS5_ATYP_IPV6;
		len += 16 + 2;
		break;
	case PROXY_REPLY_DOMAIN:
		rsp[3] = SOCKS5_ATYP_DOMAINNAME;
		rsp[4] = 8;
		len += 1 + 8 + 2;
		break;
	default:
		rsp[3] = SOCKS5_ATYP_IPV4;
		len += 4 + 2;
		break;
	}

	if (proxy_reply_mode != PROXY_REPLY_DOMAIN) {
		memset(&rsp[4], 0, len - 4);
	} else {
		memset(&rsp[5], 0, len - 5);
	}

	if (proxy_reply_mode == PROXY_REPLY_COALESCED) {
		memcpy(&rsp[len], TEST_PAYLOAD, TEST_PAYLOAD_LEN);
		(void)zsock_send(sock, rsp, len + TEST_PAYLOAD_LEN, 0);
		return;
	}

	if (proxy_reply_mode == PROXY_REPLY_SPLIT || proxy_payload_now) {
		/* Put each write in a segment of its own, which is what a
		 * proxy that writes them separately produces.
		 */
		(void)zsock_setsockopt(sock, NET_IPPROTO_TCP,
				       ZSOCK_TCP_NODELAY, &nodelay,
				       sizeof(nodelay));
	}

	if (proxy_reply_mode == PROXY_REPLY_SPLIT) {
		(void)zsock_send(sock, rsp, 4, 0);
		k_msleep(THREAD_SETTLE_MS);
		(void)zsock_send(sock, &rsp[4], len - 4, 0);
	} else {
		(void)zsock_send(sock, rsp, len, 0);
	}

	if (proxy_send_payload && proxy_payload_now) {
		(void)zsock_send(sock, TEST_PAYLOAD, TEST_PAYLOAD_SPLIT, 0);
		(void)zsock_send(sock, &TEST_PAYLOAD[TEST_PAYLOAD_SPLIT],
				 TEST_PAYLOAD_LEN - TEST_PAYLOAD_SPLIT, 0);
	} else if (proxy_send_payload) {
		k_msleep(THREAD_SETTLE_MS);
		(void)zsock_send(sock, TEST_PAYLOAD, TEST_PAYLOAD_LEN, 0);
	}
}

static void proxy_thread_fn(void *p1, void *p2, void *p3)
{
	static const uint8_t method_rsp[] = {
		SOCKS5_PKT_MAGIC, SOCKS5_AUTH_METHOD_NOAUTH
	};
	static const uint8_t method_rsp_reject[] = {
		SOCKS5_PKT_MAGIC, SOCKS5_AUTH_METHOD_NONEG
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

	if (proxy_late == PROXY_LATE_METHOD) {
		k_msleep(PROXY_LATE_MS);
	}

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

		if (proxy_late == PROXY_LATE_REPLY) {
			k_msleep(PROXY_LATE_MS);
		}

		proxy_send_reply(sock);
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
 * an error path that leaves socks5_tcp_connect() early, with the handshake
 * callback still installed. It has to be uninstalled before the function
 * returns, otherwise closing the socket later still calls into the abandoned
 * handshake. Run under CONFIG_ASAN to catch a stray access.
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

/* After a successful CONNECT the socket carries the relayed stream and nothing
 * else. Whatever the bound address in the reply looked like must not show up
 * here, and data the proxy sent must not have been dropped with it.
 */
static void expect_payload_only(void)
{
	struct timeval timeo = {
		.tv_sec = 0,
		.tv_usec = PROXY_RECV_TIMEOUT_US,
	};
	uint8_t buf[64];
	size_t got = 0;
	int ret;

	(void)zsock_setsockopt(client_sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO,
			       &timeo, sizeof(timeo));

	/* The data may have been sent, and so be received, in pieces */
	while (got < TEST_PAYLOAD_LEN) {
		ret = zsock_recv(client_sock, &buf[got], sizeof(buf) - got, 0);
		if (ret <= 0) {
			break;
		}

		got += ret;
	}

	zassert_equal(got, TEST_PAYLOAD_LEN,
		      "received %zu bytes, expected %zu", got,
		      TEST_PAYLOAD_LEN);
	zexpect_mem_equal(buf, TEST_PAYLOAD, TEST_PAYLOAD_LEN,
			  "relayed data does not match");
}

static void connect_through_proxy_v4(void)
{
	struct net_sockaddr_in dest;
	int ret;

	proxy_setup_v4();
	make_dest_v4(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zassert_equal(ret, 0, "connect failed (%d)", errno);
}

/* Hand each TCP segment to the network stack at once, as TCP does for a peer
 * on another host. TCP defers segments for a local address to its work queue
 * instead, which does not run before the sender blocks, so a reply could never
 * get ahead of a sender that is still in net_context_sendto().
 */
static int send_now(struct net_pkt *pkt)
{
	if (net_send_data(pkt) < 0) {
		net_pkt_unref(pkt);
	}

	return 0;
}

/* The proxy answers each request before the client has run again after
 * sending it, as a fast proxy does on SMP or when the client thread has a
 * lower priority than the RX thread and the proxy. Each answer must still
 * reach the step of the handshake that it belongs to.
 */
ZTEST(net_socks5, test_answer_before_client_runs)
{
	int prio = k_thread_priority_get(k_current_get());
	struct net_sockaddr_in dest;
	int ret;

	proxy_setup_v4();
	make_dest_v4(&dest);

	/* Switch to the RX thread and the proxy as soon as a request goes
	 * out, while the client is still in the middle of sending it.
	 */
	tcp_send_cb = send_now;
	k_thread_priority_set(k_current_get(), K_LOWEST_APPLICATION_THREAD_PRIO);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));

	k_thread_priority_set(k_current_get(), prio);
	tcp_send_cb = NULL;

	zexpect_equal(ret, 0, "connect failed (%d)", errno);

	proxy_wait();
	check_ipv4_request();
}

/* The reply header and its bound address arrive in separate segments. The
 * bound address bytes belong to the reply, not to the stream.
 */
ZTEST(net_socks5, test_reply_split)
{
	proxy_reply_mode = PROXY_REPLY_SPLIT;
	proxy_send_payload = true;

	connect_through_proxy_v4();
	expect_payload_only();
	proxy_wait();
}

/* The proxy packs the relayed data in behind the reply. Consuming the reply
 * must not take the data with it.
 */
ZTEST(net_socks5, test_reply_coalesced)
{
	proxy_reply_mode = PROXY_REPLY_COALESCED;

	connect_through_proxy_v4();
	expect_payload_only();
	proxy_wait();
}

/* An IPv6 bound address makes the reply 22 bytes rather than 10 */
ZTEST(net_socks5, test_reply_bnd_ipv6)
{
	proxy_reply_mode = PROXY_REPLY_IPV6;
	proxy_send_payload = true;

	connect_through_proxy_v4();
	expect_payload_only();
	proxy_wait();
}

/* RFC 1928 allows a domain name as the bound address, so its length is only
 * known after the length byte has been read.
 */
ZTEST(net_socks5, test_reply_bnd_domain)
{
	proxy_reply_mode = PROXY_REPLY_DOMAIN;
	proxy_send_payload = true;

	connect_through_proxy_v4();
	expect_payload_only();
	proxy_wait();
}

/* The destination speaks first, so its data follows the reply with no pause,
 * in segments of its own. They reach the client while connect() is still
 * waiting to hear about the reply, or while the socket is taking the
 * connection over from the handshake, and all of them must still end up in
 * the socket, in order and ahead of the end of the stream.
 */
ZTEST(net_socks5, test_reply_then_data)
{
	uint8_t buf[8];
	int ret;

	proxy_send_payload = true;
	proxy_payload_now = true;

	connect_through_proxy_v4();
	expect_payload_only();

	/* The proxy closes the connection right after the data */
	ret = zsock_recv(client_sock, buf, sizeof(buf), 0);
	zexpect_equal(ret, 0, "expected the end of the stream, got %d (%d)",
		      ret, errno);

	proxy_wait();
}

/* Give up on a proxy that is too slow to answer, then let its answer arrive
 * while the socket is still open. None of it may reach the socket, and the
 * socket must still close normally.
 */
static void connect_timeout_then_late_answer(void)
{
	struct net_sockaddr_in dest;
	uint8_t buf[64];
	int ret;

	proxy_setup_v4();
	make_dest_v4(&dest);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&dest,
			    sizeof(dest));
	zassert_equal(ret, -1, "connect should have failed");
	zassert_equal(errno, ETIMEDOUT, "unexpected errno %d", errno);

	/* The proxy is done once it has sent its late answer and closed its
	 * end. Give the receive path time to deliver both before the client
	 * socket goes away.
	 */
	proxy_wait();
	k_msleep(THREAD_SETTLE_MS);

	ret = zsock_recv(client_sock, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);
	zexpect_true(ret <= 0, "%d bytes of a failed handshake reached the "
		     "socket", ret);

	ret = zsock_close(client_sock);
	client_sock = -1;
	zexpect_equal(ret, 0, "close failed (%d)", errno);
}

/* The method selection reply arrives after the client has stopped waiting */
ZTEST(net_socks5, test_late_method_rsp)
{
	proxy_late = PROXY_LATE_METHOD;

	connect_timeout_then_late_answer();

	zexpect_equal(proxy.cmd_req_len, 0,
		      "%d bytes of CONNECT request were sent after the "
		      "negotiation timed out", proxy.cmd_req_len);
}

/* The CONNECT reply, with relayed data behind it, arrives after the client
 * has stopped waiting, so the connection has already been reported as failed.
 */
ZTEST(net_socks5, test_late_reply)
{
	proxy_late = PROXY_LATE_REPLY;
	proxy_reply_mode = PROXY_REPLY_COALESCED;

	connect_timeout_then_late_answer();

	zexpect_equal(proxy.cmd_req_len, REQUEST_IPV4_SIZE,
		      "request was %d bytes, expected %d", proxy.cmd_req_len,
		      REQUEST_IPV4_SIZE);
}

static void before(void *arg)
{
	ARG_UNUSED(arg);

	memset(&proxy, 0, sizeof(proxy));
	k_sem_reset(&proxy_done);
	proxy_reject_auth = false;
	proxy_reply_mode = PROXY_REPLY_IPV4;
	proxy_send_payload = false;
	proxy_payload_now = false;
	proxy_late = PROXY_LATE_NONE;

	proxy_port++;
}

static void after(void *arg)
{
	ARG_UNUSED(arg);

	tcp_send_cb = NULL;

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
