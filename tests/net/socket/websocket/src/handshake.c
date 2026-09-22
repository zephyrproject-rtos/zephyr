/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* Tests for the WebSocket client handshake.
 *
 * RFC 6455 section 4.1 says the client must fail the connection unless the
 * Sec-WebSocket-Accept value is exactly the base64 of the SHA-1 of the key it
 * offered concatenated with the magic GUID. Unlike the frame tests in main.c,
 * these need the real handshake, so they run a server on the loopback
 * interface that returns that value either intact or corrupted.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, CONFIG_NET_WEBSOCKET_LOG_LEVEL);

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

#define HS_STACK_SIZE 4096
#define HS_DONE_TIMEOUT K_SECONDS(5)
#define SEGMENT_GAP_MS 50
#define CONNECT_TIMEOUT_MS 2000

/* The receive buffer the client parses the response in. One of the cases below
 * deliberately lands a header value on its last byte.
 */
#define TMP_BUF_LEN 256

static const char rsp_head[] =
	"HTTP/1.1 101 Switching Protocols\r\n"
	"Upgrade: websocket\r\n"
	"Connection: Upgrade\r\n";

static const char accept_hdr[] = "Sec-WebSocket-Accept: ";

/* How the server fills in the Sec-WebSocket-Accept header */
enum accept_mode {
	ACCEPT_CORRECT,		/* the value RFC 6455 asks for */
	ACCEPT_PREFIX,		/* only the first few characters of it */
	ACCEPT_EMPTY,		/* no value at all */
	ACCEPT_WRONG,		/* right length, one character changed */
	ACCEPT_SPLIT,		/* correct, but split across two segments */
	ACCEPT_WRONG_AT_END,	/* wrong, and ending on the last byte of tmp_buf */
};

static enum accept_mode hs_mode;

static K_SEM_DEFINE(hs_done, 0, 1);
static K_THREAD_STACK_DEFINE(hs_stack, HS_STACK_SIZE);
static struct k_thread hs_thread;

static int hs_listen_sock = -1;
static int hs_client_sock = -1;
static bool hs_saw_request;

/* Each test uses its own port so a lingering connection from the previous one
 * cannot make bind() fail.
 */
static uint16_t hs_port = 9001;

static uint8_t tmp_buf[TMP_BUF_LEN];

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

/* Assemble the 101 response, corrupting the accept value as the mode asks.
 * Returns the length, and reports where a split should fall.
 */
static size_t build_response(char *rsp, size_t rsp_len, const char *accept,
			     size_t *split_at)
{
	char value[ACCEPT_KEY_LEN + 1];
	size_t pad = 0;
	size_t len;

	strncpy(value, accept, sizeof(value) - 1);
	value[sizeof(value) - 1] = '\0';

	switch (hs_mode) {
	case ACCEPT_PREFIX:
		value[8] = '\0';
		break;
	case ACCEPT_EMPTY:
		value[0] = '\0';
		break;
	case ACCEPT_WRONG:
	case ACCEPT_WRONG_AT_END:
		value[ACCEPT_KEY_LEN - 1] =
			(value[ACCEPT_KEY_LEN - 1] == 'A') ? 'B' : 'A';
		break;
	default:
		break;
	}

	if (hs_mode == ACCEPT_WRONG_AT_END) {
		/* Pad the response so the last character of the accept value
		 * lands on the last byte of the client's receive buffer, and
		 * make the whole thing longer than that buffer so the first
		 * read fills it exactly. The parser then hands the value over
		 * as a slice that reaches the end of the buffer, with no
		 * terminator behind it.
		 */
		pad = TMP_BUF_LEN - (sizeof(rsp_head) - 1) -
		      strlen("X-Pad: \r\n") - (sizeof(accept_hdr) - 1) -
		      ACCEPT_KEY_LEN;
	}

	len = snprintk(rsp, rsp_len, "%s", rsp_head);

	if (pad > 0) {
		len += snprintk(rsp + len, rsp_len - len, "X-Pad: ");
		memset(rsp + len, 'x', pad);
		len += pad;
		len += snprintk(rsp + len, rsp_len - len, "\r\n");
	}

	len += snprintk(rsp + len, rsp_len - len, "%s%s\r\n\r\n", accept_hdr,
			value);

	/* Split in the middle of the accept value, so the client has to match
	 * it across two calls of its header value callback.
	 */
	*split_at = sizeof(rsp_head) - 1 + sizeof(accept_hdr) - 1 +
		    (ACCEPT_KEY_LEN / 2);

	return len;
}

static void hs_server_thread(void *p1, void *p2, void *p3)
{
	char request[512];
	char response[TMP_BUF_LEN + 128];
	char accept[ACCEPT_KEY_LEN + 1];
	size_t split_at;
	size_t rsp_len;
	int received = 0;
	int sock;
	int ret;

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	sock = zsock_accept(hs_listen_sock, NULL, NULL);
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

	hs_saw_request = true;

	if (compute_accept(request, accept, sizeof(accept)) < 0) {
		goto close;
	}

	rsp_len = build_response(response, sizeof(response), accept, &split_at);

	if (hs_mode == ACCEPT_SPLIT) {
		int nodelay = 1;

		(void)zsock_setsockopt(sock, NET_IPPROTO_TCP,
				       ZSOCK_TCP_NODELAY, &nodelay,
				       sizeof(nodelay));
		(void)zsock_send(sock, response, split_at, 0);
		k_msleep(SEGMENT_GAP_MS);
		(void)zsock_send(sock, response + split_at,
				 rsp_len - split_at, 0);
	} else {
		(void)zsock_send(sock, response, rsp_len, 0);
	}

	/* Let the client finish the handshake before the connection goes away
	 * underneath it.
	 */
	k_msleep(SEGMENT_GAP_MS * 4);

close:
	(void)zsock_close(sock);
out:
	k_sem_give(&hs_done);
}

static void hs_server_start(struct net_sockaddr_in6 *addr)
{
	int ret;

	memset(addr, 0, sizeof(*addr));
	addr->sin6_family = NET_AF_INET6;
	addr->sin6_port = net_htons(hs_port);
	zassert_equal(zsock_inet_pton(NET_AF_INET6, SERVER_ADDR,
				      &addr->sin6_addr), 1,
		      "inet_pton failed");

	hs_listen_sock = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM,
				      NET_IPPROTO_TCP);
	zassert_true(hs_listen_sock >= 0, "server socket failed (%d)", errno);

	ret = zsock_bind(hs_listen_sock, (struct net_sockaddr *)addr,
			 sizeof(*addr));
	zassert_equal(ret, 0, "server bind failed (%d)", errno);

	ret = zsock_listen(hs_listen_sock, 1);
	zassert_equal(ret, 0, "server listen failed (%d)", errno);

	(void)k_thread_create(&hs_thread, hs_stack,
			      K_THREAD_STACK_SIZEOF(hs_stack),
			      hs_server_thread, NULL, NULL, NULL,
			      k_thread_priority_get(k_current_get()), 0,
			      K_NO_WAIT);
}

/* Run one handshake and report what websocket_connect() made of it */
static int do_handshake(enum accept_mode mode)
{
	struct websocket_request wreq = { 0 };
	struct net_sockaddr_in6 addr;
	int ret;

	hs_mode = mode;
	hs_server_start(&addr);

	hs_client_sock = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM,
				      NET_IPPROTO_TCP);
	zassert_true(hs_client_sock >= 0, "client socket failed (%d)", errno);

	ret = zsock_connect(hs_client_sock, (struct net_sockaddr *)&addr,
			    sizeof(addr));
	zassert_equal(ret, 0, "client connect failed (%d)", errno);

	wreq.host = SERVER_ADDR;
	wreq.url = "/ws";
	wreq.tmp_buf = tmp_buf;
	wreq.tmp_buf_len = sizeof(tmp_buf);

	ret = websocket_connect(hs_client_sock, &wreq, CONNECT_TIMEOUT_MS,
				NULL);

	zassert_equal(k_sem_take(&hs_done, HS_DONE_TIMEOUT), 0,
		      "server thread did not finish");
	zexpect_true(hs_saw_request, "server never saw the request");

	return ret;
}

static void expect_rejected(int ws, const char *what)
{
	zexpect_true(ws < 0, "handshake accepted %s (%d)", what, ws);

	if (ws >= 0) {
		(void)websocket_disconnect(ws);
	}
}

ZTEST(net_websocket_handshake, test_accept_correct)
{
	int ws = do_handshake(ACCEPT_CORRECT);

	zassert_true(ws >= 0, "handshake rejected a correct key (%d)", ws);

	(void)websocket_disconnect(ws);
}

/* The value is only a prefix of the expected one. Matching just the bytes the
 * server chose to send would let this through.
 */
ZTEST(net_websocket_handshake, test_accept_prefix_rejected)
{
	expect_rejected(do_handshake(ACCEPT_PREFIX), "a truncated key");
}

/* An empty value means nothing was compared at all */
ZTEST(net_websocket_handshake, test_accept_empty_rejected)
{
	expect_rejected(do_handshake(ACCEPT_EMPTY), "an empty key");
}

ZTEST(net_websocket_handshake, test_accept_wrong_rejected)
{
	expect_rejected(do_handshake(ACCEPT_WRONG), "a wrong key");
}

/* The parser hands a header value over in as many pieces as the segment
 * boundaries dictate, so a correct value that arrives split must still be
 * accepted.
 */
ZTEST(net_websocket_handshake, test_accept_split_across_segments)
{
	int ws = do_handshake(ACCEPT_SPLIT);

	zassert_true(ws >= 0, "handshake rejected a split correct key (%d)",
		     ws);

	(void)websocket_disconnect(ws);
}

/* A mismatching value that reaches the end of the receive buffer. The parser
 * gives the client a pointer and a length with no terminator behind it, so
 * logging the mismatch reads past the buffer unless the length is respected.
 * Only an allocation checker such as CONFIG_ASAN sees the difference.
 */
ZTEST(net_websocket_handshake, test_accept_wrong_at_buffer_end)
{
	expect_rejected(do_handshake(ACCEPT_WRONG_AT_END),
			"a wrong key at the buffer end");
}

static void hs_before(void *arg)
{
	ARG_UNUSED(arg);

	k_sem_reset(&hs_done);
	hs_saw_request = false;
	hs_mode = ACCEPT_CORRECT;
	hs_port++;

	/* Keep the cases independent of what the previous one left behind */
	memset(tmp_buf, 0, sizeof(tmp_buf));
}

static void hs_after(void *arg)
{
	ARG_UNUSED(arg);

	/* websocket_disconnect() releases the websocket fd only, the socket it
	 * was handed stays with us.
	 */
	if (hs_client_sock >= 0) {
		(void)zsock_close(hs_client_sock);
		hs_client_sock = -1;
	}

	if (hs_listen_sock >= 0) {
		(void)zsock_close(hs_listen_sock);
		hs_listen_sock = -1;
	}

	(void)k_thread_join(&hs_thread, K_SECONDS(1));
}

static void *hs_setup(void)
{
	k_thread_system_pool_assign(k_current_get());

	zassert_equal(psa_crypto_init(), PSA_SUCCESS, "psa_crypto_init failed");

	return NULL;
}

ZTEST_SUITE(net_websocket_handshake, NULL, hs_setup, hs_before, hs_after, NULL);
