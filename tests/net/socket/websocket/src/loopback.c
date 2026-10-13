/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/* Websocket client tests over a real TCP connection on the loopback
 * interface. The frame tests in main.c feed data straight into the parser
 * with CONFIG_NET_TEST, which stubs out the socket I/O of the websocket
 * library; the cases here need that I/O, so they build only in the scenario
 * that turns CONFIG_NET_TEST off. A server thread completes the handshake and
 * then drives the connection.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_WEBSOCKET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/websocket.h>
#include <zephyr/sys/base64.h>

#include <psa/crypto.h>

#include "websocket_internal.h"

#define SERVER_ADDR "::1"

/* base64 of a SHA-1 digest */
#define ACCEPT_KEY_LEN 28

/* The client always offers a base64 encoded 16 byte key */
#define CLIENT_KEY_LEN 24

#define SRV_STACK_SIZE 4096
#define SRV_DONE_TIMEOUT K_SECONDS(5)
#define SRV_REPLY_TIMEOUT_MS 1000
#define CONNECT_TIMEOUT_MS 2000
#define RECV_TIMEOUT_MS 2000
#define SEND_TIMEOUT_MS 500
#define STALL_RELEASE_TIMEOUT K_SECONDS(10)

/* Enough rounds of this to fill the receive window the scenario configures,
 * with margin for whatever the stack buffers on top of it.
 */
#define SEND_CHUNK_LEN 256
#define SEND_CHUNKS_MAX 64

#define TMP_BUF_LEN 256
#define RECV_BUF_LEN 64

static const char rsp_fmt[] =
	"HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n"
	"Connection: Upgrade\r\n"
	"Sec-WebSocket-Accept: %s\r\n\r\n";

/* What the server does once the handshake is complete */
enum srv_mode {
	SRV_SILENT,		/* nothing, wait for the client to go away */
	SRV_CLOSE,		/* send an empty CLOSE frame */
	SRV_CLOSE_FRAGMENTED,	/* send a CLOSE frame with the FIN bit clear */
	SRV_STALL,		/* stop reading until released, then drain */
};

/* FIN + CLOSE, no payload, unmasked */
static const uint8_t close_frame[] = { 0x88, 0x00 };

/* CLOSE without FIN. RFC 6455 5.5 forbids fragmenting control frames, but a
 * peer may send one anyway and the client must not come apart over it.
 */
static const uint8_t close_frame_fragmented[] = { 0x08, 0x00 };

static enum srv_mode srv_mode;

static K_SEM_DEFINE(srv_done, 0, 1);
static K_SEM_DEFINE(srv_release, 0, 1);
static K_THREAD_STACK_DEFINE(srv_stack, SRV_STACK_SIZE);
static struct k_thread srv_thread;

static int srv_listen_sock = -1;
static int client_sock = -1;

/* How many CLOSE frames the server read back from the client */
static int srv_close_frames;

/* How many bytes the server read back from the client in all */
static size_t srv_drained;

/* Each test uses its own port so a lingering connection from the previous one
 * cannot make bind() fail.
 */
static uint16_t srv_port = 9101;

static uint8_t tmp_buf[TMP_BUF_LEN];
static uint8_t recv_buf[RECV_BUF_LEN];
static uint8_t send_buf[SEND_CHUNK_LEN];

/* Derive the Sec-WebSocket-Accept value from the key the client offered, the
 * way RFC 6455 section 4.2.2 describes it.
 */
static int compute_accept(const char *request, char *out, size_t out_len)
{
	static const char key_hdr[] = "Sec-WebSocket-Key: ";
	char buf[CLIENT_KEY_LEN + sizeof(WS_MAGIC) - 1];
	uint8_t digest[WS_SHA1_OUTPUT_LEN];
	size_t digest_len;
	size_t olen;
	const char *key;

	key = strstr(request, key_hdr);
	if (key == NULL) {
		return -ENOENT;
	}

	key += sizeof(key_hdr) - 1;

	memcpy(buf, key, CLIENT_KEY_LEN);
	memcpy(buf + CLIENT_KEY_LEN, WS_MAGIC, sizeof(WS_MAGIC) - 1);

	if (psa_hash_compute(PSA_ALG_SHA_1, (const uint8_t *)buf, sizeof(buf),
			     digest, sizeof(digest),
			     &digest_len) != PSA_SUCCESS) {
		return -EIO;
	}

	if (base64_encode(out, out_len, &olen, digest, digest_len) != 0) {
		return -ENOMEM;
	}

	return 0;
}

/* Count the CLOSE frames in a byte run of client frames. Client frames are
 * masked, so each carries four mask bytes after the length.
 */
static int count_close_frames(const uint8_t *buf, size_t len)
{
	size_t pos = 0;
	int count = 0;

	while (pos + 2 <= len) {
		uint8_t opcode = buf[pos] & 0x0f;
		size_t plen = buf[pos + 1] & 0x7f;
		size_t hdr = 2 + ((buf[pos + 1] & 0x80) ? 4 : 0);

		if (plen >= 126) {
			/* Not expected from the cases here */
			break;
		}

		if (opcode == WEBSOCKET_OPCODE_CLOSE) {
			count++;
		}

		pos += hdr + plen;
	}

	return count;
}

/* Read whatever the client sends until it closes the connection or goes
 * quiet, and count the CLOSE frames in it.
 */
static void srv_read_reply(int sock)
{
	struct zsock_timeval tv = {
		.tv_sec = SRV_REPLY_TIMEOUT_MS / 1000,
		.tv_usec = (SRV_REPLY_TIMEOUT_MS % 1000) * 1000,
	};
	uint8_t buf[64];
	size_t received = 0;
	int ret;

	(void)zsock_setsockopt(sock, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv,
			       sizeof(tv));

	while (true) {
		if (received < sizeof(buf)) {
			ret = zsock_recv(sock, buf + received,
					 sizeof(buf) - received, 0);
		} else {
			/* Keep draining so the client is never left with a
			 * full window, but there is nothing left to inspect.
			 */
			ret = zsock_recv(sock, send_buf, sizeof(send_buf), 0);
		}

		if (ret <= 0) {
			break;
		}

		srv_drained += ret;
		received = MIN(received + ret, sizeof(buf));
	}

	srv_close_frames = count_close_frames(buf, received);
}

static void srv_thread_fn(void *p1, void *p2, void *p3)
{
	char request[512];
	char response[256];
	char accept[ACCEPT_KEY_LEN + 1];
	int received = 0;
	int sock;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sock = zsock_accept(srv_listen_sock, NULL, NULL);
	if (sock < 0) {
		goto out;
	}

	/* Read until the end of the request headers */
	while (received < (int)sizeof(request) - 1) {
		ret = zsock_recv(sock, request + received,
				 sizeof(request) - 1 - received, 0);
		if (ret <= 0) {
			goto close;
		}

		received += ret;
		request[received] = '\0';

		if (strstr(request, "\r\n\r\n") != NULL) {
			break;
		}
	}

	if (compute_accept(request, accept, sizeof(accept)) < 0) {
		goto close;
	}

	ret = snprintk(response, sizeof(response), rsp_fmt, accept);
	(void)zsock_send(sock, response, ret, 0);

	switch (srv_mode) {
	case SRV_CLOSE:
		(void)zsock_send(sock, close_frame, sizeof(close_frame), 0);
		break;
	case SRV_CLOSE_FRAGMENTED:
		(void)zsock_send(sock, close_frame_fragmented,
				 sizeof(close_frame_fragmented), 0);
		break;
	case SRV_STALL:
		/* Leave everything the client sends in the receive window */
		(void)k_sem_take(&srv_release, STALL_RELEASE_TIMEOUT);
		break;
	default:
		break;
	}

	srv_read_reply(sock);

close:
	(void)zsock_close(sock);
out:
	k_sem_give(&srv_done);
}

static void srv_start(struct net_sockaddr_in6 *addr)
{
	int ret;

	memset(addr, 0, sizeof(*addr));
	addr->sin6_family = NET_AF_INET6;
	addr->sin6_port = net_htons(srv_port);
	zassert_equal(zsock_inet_pton(NET_AF_INET6, SERVER_ADDR,
				      &addr->sin6_addr), 1,
		      "inet_pton failed");

	srv_listen_sock = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM,
				       NET_IPPROTO_TCP);
	zassert_true(srv_listen_sock >= 0, "server socket failed (%d)", errno);

	ret = zsock_bind(srv_listen_sock, (struct net_sockaddr *)addr,
			 sizeof(*addr));
	zassert_equal(ret, 0, "server bind failed (%d)", errno);

	ret = zsock_listen(srv_listen_sock, 1);
	zassert_equal(ret, 0, "server listen failed (%d)", errno);

	k_sem_reset(&srv_done);
	k_sem_reset(&srv_release);
	srv_close_frames = 0;
	srv_drained = 0;

	(void)k_thread_create(&srv_thread, srv_stack,
			      K_THREAD_STACK_SIZEOF(srv_stack),
			      srv_thread_fn, NULL, NULL, NULL,
			      k_thread_priority_get(k_current_get()), 0,
			      K_NO_WAIT);
}

static void srv_stop(void)
{
	zassert_equal(k_sem_take(&srv_done, SRV_DONE_TIMEOUT), 0,
		      "server thread did not finish");

	if (srv_listen_sock >= 0) {
		(void)zsock_close(srv_listen_sock);
		srv_listen_sock = -1;
	}

	(void)k_thread_join(&srv_thread, K_SECONDS(1));

	/* Let the connection leave TIME_WAIT so the rounds do not pile up
	 * TCP contexts.
	 */
	k_msleep(CONFIG_NET_TCP_TIME_WAIT_DELAY + 50);
}

/* Start a server in the given mode and complete a handshake against it.
 * Returns the websocket fd.
 */
static int ws_connect(enum srv_mode mode)
{
	struct websocket_request wreq = { 0 };
	struct net_sockaddr_in6 addr;
	int ws;
	int ret;

	srv_mode = mode;
	srv_port++;
	srv_start(&addr);

	client_sock = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM,
				   NET_IPPROTO_TCP);
	zassert_true(client_sock >= 0, "client socket failed (%d)", errno);

	ret = zsock_connect(client_sock, (struct net_sockaddr *)&addr,
			    sizeof(addr));
	zassert_equal(ret, 0, "client connect failed (%d)", errno);

	wreq.host = SERVER_ADDR;
	wreq.url = "/ws";
	wreq.tmp_buf = tmp_buf;
	wreq.tmp_buf_len = sizeof(tmp_buf);

	ws = websocket_connect(client_sock, &wreq, CONNECT_TIMEOUT_MS, NULL);

	return ws;
}

static void client_close(void)
{
	/* websocket_disconnect() releases the websocket fd only, the socket it
	 * was handed stays with us.
	 */
	if (client_sock >= 0) {
		(void)zsock_close(client_sock);
		client_sock = -1;
	}
}

/* Receive the peer's CLOSE, hang up, and check that the client answered it
 * with exactly one CLOSE frame of its own.
 */
static void peer_close_round(enum srv_mode mode)
{
	uint32_t msg_type = 0;
	uint64_t remaining = 0;
	int ws;
	int ret;

	ws = ws_connect(mode);
	zassert_true(ws >= 0, "websocket_connect() failed (%d)", ws);

	ret = websocket_recv_msg(ws, recv_buf, sizeof(recv_buf), &msg_type,
				 &remaining, RECV_TIMEOUT_MS);
	zassert_equal(ret, 0, "expected an empty CLOSE, got %d", ret);
	zassert_true((msg_type & WEBSOCKET_FLAG_CLOSE) != 0,
		     "message type 0x%x is not CLOSE", msg_type);

	ret = websocket_disconnect(ws);
	zassert_equal(ret, 0, "websocket_disconnect() failed (%d)", ret);

	client_close();
	srv_stop();

	zassert_equal(srv_close_frames, 1,
		      "client sent %d CLOSE frames, expected 1",
		      srv_close_frames);
}

/* Every connection must hand its context back, so running more rounds than
 * there are contexts fails on a leaked slot.
 */
#define ROUNDS (CONFIG_WEBSOCKET_MAX_CONTEXTS + 1)

ZTEST(net_websocket_loopback, test_peer_close_releases_context)
{
	for (int i = 0; i < ROUNDS; i++) {
		peer_close_round(SRV_CLOSE);
	}
}

ZTEST(net_websocket_loopback, test_peer_fragmented_close_releases_context)
{
	for (int i = 0; i < ROUNDS; i++) {
		peer_close_round(SRV_CLOSE_FRAGMENTED);
	}
}

/* A masked frame carrying SEND_CHUNK_LEN bytes: 2 byte header, 4 byte mask */
#define SEND_FRAME_LEN (2 + 4 + SEND_CHUNK_LEN)

/* The timeout passed to websocket_send_msg() bounds the call: the server
 * stops reading, the client fills the receive window, and the send that no
 * longer fits has to give up with -ETIMEDOUT when that timeout expires.
 *
 * An expired timeout leaves the peer with whatever part of the frame went
 * out, here the head of it since the window is not a multiple of the frame
 * length. From then on the connection must refuse to send anything, including
 * the CLOSE frame, or the peer would read it as the rest of that frame.
 */
ZTEST(net_websocket_loopback, test_send_timeout_wins_over_socket)
{
	int64_t start = 0;
	int64_t elapsed = 0;
	int sent;
	int ws;
	int ret = 0;

	ws = ws_connect(SRV_STALL);
	zassert_true(ws >= 0, "websocket_connect() failed (%d)", ws);

	for (sent = 0; sent < SEND_CHUNKS_MAX; sent++) {
		start = k_uptime_get();
		ret = websocket_send_msg(ws, send_buf, sizeof(send_buf),
					 WEBSOCKET_OPCODE_DATA_BINARY, true,
					 true, SEND_TIMEOUT_MS);
		elapsed = k_uptime_delta(&start);

		if (ret == -ETIMEDOUT) {
			break;
		}

		zassert_equal(ret, sizeof(send_buf), "send %d failed (%d)",
			      sent, ret);
	}

	zassert_equal(ret, -ETIMEDOUT, "no timeout after %d chunks (%d)",
		      sent, ret);
	zassert_true(elapsed >= SEND_TIMEOUT_MS, "gave up after %lld ms",
		     elapsed);
	zassert_true(elapsed < SEND_TIMEOUT_MS * 4, "gave up after %lld ms",
		     elapsed);

	/* The timeout expired mid-stream, so the connection is no good for
	 * sending.
	 */
	ret = websocket_send_msg(ws, send_buf, sizeof(send_buf),
				 WEBSOCKET_OPCODE_DATA_BINARY, true, true,
				 SEND_TIMEOUT_MS);
	zassert_equal(ret, -EPIPE, "send on a torn connection returned %d", ret);

	k_sem_give(&srv_release);

	ret = websocket_disconnect(ws);
	zassert_equal(ret, 0, "websocket_disconnect() failed (%d)", ret);

	client_close();
	srv_stop();

	/* TCP fills the window exactly, so the server got the complete frames
	 * and the head of the torn one, and nothing after it: a CLOSE frame
	 * would show up as six bytes more.
	 */
	zassert_equal(srv_drained, CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE,
		      "server drained %zu bytes, expected %d", srv_drained,
		      CONFIG_NET_TCP_MAX_RECV_WINDOW_SIZE);
}

static void lb_before(void *arg)
{
	ARG_UNUSED(arg);

	memset(tmp_buf, 0, sizeof(tmp_buf));
	memset(recv_buf, 0, sizeof(recv_buf));
}

static void lb_after(void *arg)
{
	ARG_UNUSED(arg);

	client_close();

	if (srv_listen_sock >= 0) {
		(void)zsock_close(srv_listen_sock);
		srv_listen_sock = -1;
	}

	/* A failed test may have left the server stalled; release it and
	 * outwait the stall so the next test gets a thread that is done.
	 */
	k_sem_give(&srv_release);
	(void)k_thread_join(&srv_thread, K_SECONDS(15));
}

static void *lb_setup(void)
{
	k_thread_system_pool_assign(k_current_get());

	zassert_equal(psa_crypto_init(), PSA_SUCCESS, "psa_crypto_init failed");

	return NULL;
}

ZTEST_SUITE(net_websocket_loopback, NULL, lb_setup, lb_before, lb_after, NULL);
