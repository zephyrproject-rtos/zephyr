/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* In this websocket test, we create a websocket server which starts
 * to listen connections. Then we start to send data to it and verify that
 * we get proper data back.
 */

#include <ztest.h>

#include <net/net_ip.h>
#include <net/net_app.h>
#include <net/websocket.h>

static struct net_app_ctx app_ctx_v6;
static struct net_app_ctx app_ctx_v4;

#if defined(CONFIG_NET_DEBUG_WEBSOCKET)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#define NET_LOG_ENABLED 1
#else
#define DBG(fmt, ...)
#endif

#include "../../../subsys/net/ip/net_private.h"

/*
 * GET /ws HTTP/1.1
 * Upgrade: websocket
 * Connection: Upgrade
 * Host: 2001:db8::1
 * Origin: http://2001:db8::1
 * Sec-WebSocket-Key: 8VMFeU0j8bImbjyjPVHSQg==
 * Sec-WebSocket-Version: 13
 */
static char http_msg[] = {
	0x47, 0x45, 0x54, 0x20, 0x2f, 0x77, 0x73, 0x20,
	0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31,
	0x0d, 0x0a, 0x55, 0x70, 0x67, 0x72, 0x61, 0x64,
	0x65, 0x3a, 0x20, 0x77, 0x65, 0x62, 0x73, 0x6f,
	0x63, 0x6b, 0x65, 0x74, 0x0d, 0x0a, 0x43, 0x6f,
	0x6e, 0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e,
	0x3a, 0x20, 0x55, 0x70, 0x67, 0x72, 0x61, 0x64,
	0x65, 0x0d, 0x0a, 0x48, 0x6f, 0x73, 0x74, 0x3a,
	0x20, 0x32, 0x30, 0x30, 0x31, 0x3a, 0x64, 0x62,
	0x38, 0x3a, 0x3a, 0x31, 0x0d, 0x0a, 0x4f, 0x72,
	0x69, 0x67, 0x69, 0x6e, 0x3a, 0x20, 0x68, 0x74,
	0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x32, 0x30, 0x30,
	0x31, 0x3a, 0x64, 0x62, 0x38, 0x3a, 0x3a, 0x31,
	0x0d, 0x0a, 0x53, 0x65, 0x63, 0x2d, 0x57, 0x65,
	0x62, 0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x2d,
	0x4b, 0x65, 0x79, 0x3a, 0x20, 0x38, 0x56, 0x4d,
	0x46, 0x65, 0x55, 0x30, 0x6a, 0x38, 0x62, 0x49,
	0x6d, 0x62, 0x6a, 0x79, 0x6a, 0x50, 0x56, 0x48,
	0x53, 0x51, 0x67, 0x3d, 0x3d, 0x0d, 0x0a, 0x53,
	0x65, 0x63, 0x2d, 0x57, 0x65, 0x62, 0x53, 0x6f,
	0x63, 0x6b, 0x65, 0x74, 0x2d, 0x56, 0x65, 0x72,
	0x73, 0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x31, 0x33,
	0x0d, 0x0a, 0x0d, 0x0a,
};

/* WebSocket:
 *   FIN: true
 *   Reserved: 0x00
 *   Opcode: Text (1)
 *   Mask: True
 *   Payload len: 7
 *   Masking key: d1ffa558
 *   Masked payload: 0x99, 0x9a, 0xc9, 0x34, 0xbe, 0xd3, 0x85
 *   Payload: "Hello, "
 */
/* This array is not modified */
static const u8_t ws_test_msg_orig[] = {
	0x81, 0x87, 0xd1, 0xff, 0xa5, 0x58, 0x99, 0x9a,
	0xc9, 0x34, 0xbe, 0xd3, 0x85,
};

/* We are manipulating this array in the test */
static u8_t ws_test_msg[sizeof(ws_test_msg_orig)];

static u32_t mask_value = 0xd1ffa558;
static struct sockaddr server_addr;
static s32_t timeout = K_MSEC(100);
static int bytes_received;
static bool failure;
static struct k_sem wait_data;
static struct k_sem progress;
extern struct http_ctx *ws_ctx;

#define hdr_len 6

static u8_t ws_unmasked_msg[sizeof(ws_test_msg) - hdr_len];
static int total_data_len = sizeof(ws_unmasked_msg);

#define WAIT_TIME K_SECONDS(1)

void test_websocket_init_server(void);
void websocket_cleanup_server(void);

static void ws_mask_payload(u8_t *payload, size_t payload_len,
			    u32_t masking_value)
{
	int i;

	for (i = 0; i < payload_len; i++) {
		payload[i] ^= masking_value >> (8 * (3 - i % 4));
	}
}

static void recv_cb(struct net_app_ctx *ctx,
		    struct net_pkt *pkt,
		    int status,
		    void *user_data)
{
	int ret;
	size_t len;

	if (!pkt) {
		return;
	}

	len = net_pkt_appdatalen(pkt);

	/* The pkt can contain websocket header because we are bypassing
	 * any websocket message parsing here. So we need to skip the websocket
	 * header here in that case. The return header length is 2 bytes here
	 * only if the returned message < 127 bytes long.
	 */
	if (net_pkt_appdata(pkt)[0] == 0x01) {
		/* If we received packet with websocket header, then skip it */
		net_buf_pull(pkt->frags, 2);
		net_pkt_set_appdata(pkt, net_pkt_appdata(pkt) + 2);

		len -= 2;
		net_pkt_set_appdatalen(pkt, len);
	}

	if (len == 0) {
		goto out;
	}

	DBG("Received %zd bytes\n", len);

	/* Note that we can only use net_pkt_appdata() here because we
	 * know that the data fits in first fragment.
	 */
	ret = memcmp(ws_unmasked_msg + bytes_received,
		     net_pkt_appdata(pkt), len);
	if (ret != 0) {
		net_hexdump("recv", net_pkt_appdata(pkt),
			    net_pkt_appdatalen(pkt));
		net_hexdump("sent", ws_unmasked_msg, sizeof(ws_unmasked_msg));

		failure = true;

		zassert_equal(ret, 0, "Received data does not match");

		goto out;
	}

	bytes_received += len;
	failure = false;

	if (total_data_len == bytes_received) {
		bytes_received = 0;
	}

out:
	k_sem_give(&wait_data);
	k_sem_give(&progress);

	net_pkt_unref(pkt);
}

void test_init(void)
{
	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	k_sem_init(&progress, 0, UINT_MAX);

	memcpy(ws_unmasked_msg, ws_test_msg_orig + hdr_len,
	       sizeof(ws_unmasked_msg));

	ws_mask_payload(ws_unmasked_msg, sizeof(ws_unmasked_msg), mask_value);
}

static void test_connect(struct net_app_ctx *ctx)
{
	int ret;

	ret = net_app_connect(ctx, K_FOREVER);
	zassert_equal(ret, 0, "websocket client connect");
}

static void test_close(struct net_app_ctx *ctx)
{
	int ret;

	ret = net_app_close(ctx);
	zassert_equal(ret, 0, "websocket client close");
}

/* The chunk_size tells how many bytes at a time to send.
 * This is not the same as HTTP chunk!
 */
static void test_send_recv(int chunk_size, struct net_app_ctx *ctx)
{
	int i, j, ret;

	DBG("Sending %d bytes at a time\n", chunk_size);

	for (i = 0; i < sizeof(ws_test_msg); i += chunk_size) {
		for (j = 0;
		     IS_ENABLED(CONFIG_NET_DEBUG_WEBSOCKET) && j < chunk_size;
		     j++) {
			if ((i + chunk_size) >= sizeof(ws_test_msg)) {
				break;
			}

			if ((i + j) < hdr_len) {
				DBG("[%d] = 0x%02x\n", i + j,
				    ws_test_msg[i + j]);
			} else {
				DBG("[%d] = 0x%02x -> \"%c\"\n", i + j,
				    ws_test_msg[i + j],
				    ws_unmasked_msg[i + j - hdr_len]);
			}
		}

		if ((i + chunk_size) >= sizeof(ws_test_msg)) {
			chunk_size = sizeof(ws_test_msg) - i;
		}

		ret = net_app_send_buf(ctx, &ws_test_msg[i],
				       chunk_size,
				       &server_addr,
				       sizeof(struct sockaddr),
				       timeout,
				       NULL);
		if (ret != 0) {
			DBG("Cannot send %d byte(s) (%d)\n", ret, chunk_size);

			zassert_equal(ret, 0, "websocket IPv6 client ws send");
		}

		/* Make sure the receiving side gets the data now */
		k_yield();
	}
}

static void test_send_multi_msg(struct net_app_ctx *ctx)
{
	u8_t ws_big_msg[sizeof(ws_test_msg) * 2];
	int i, j, ret, chunk_size = 4;

	k_sem_take(&progress, K_FOREVER);

	memcpy(ws_test_msg, ws_test_msg_orig, sizeof(ws_test_msg));
	bytes_received = 0;

	ws_ctx->websocket.data_waiting = 0;
	if (ws_ctx->websocket.pending) {
		net_pkt_unref(ws_ctx->websocket.pending);
		ws_ctx->websocket.pending = NULL;
	}

	memcpy(ws_big_msg, ws_test_msg, sizeof(ws_test_msg));
	memcpy(ws_big_msg + sizeof(ws_test_msg), ws_test_msg,
	       sizeof(ws_test_msg));

	for (i = 0; i < sizeof(ws_big_msg); i += chunk_size) {
		for (j = 0;
		     IS_ENABLED(CONFIG_NET_DEBUG_WEBSOCKET) && j < chunk_size;
		     j++) {
			int first_msg = 0;

			if ((i + chunk_size) >= sizeof(ws_big_msg)) {
				break;
			}

			if (i > sizeof(ws_test_msg)) {
				first_msg = sizeof(ws_test_msg);
			}

			if (i + j + first_msg < hdr_len + first_msg) {
				DBG("[%d] = 0x%02x\n", i + j,
				    ws_big_msg[i + j + first_msg]);
			} else {
				if (i + j + first_msg <
				    sizeof(ws_test_msg) + first_msg) {
					DBG("[%d] = 0x%02x -> \"%c\"\n", i + j,
					    ws_big_msg[i + j + first_msg],
					    ws_unmasked_msg[i + j - hdr_len +
							    first_msg]);
				}
			}
		}

		if ((i + chunk_size) >= sizeof(ws_big_msg)) {
			chunk_size = sizeof(ws_big_msg) - i;
		}

		ret = net_app_send_buf(ctx, &ws_big_msg[i],
				       chunk_size,
				       &server_addr,
				       sizeof(struct sockaddr),
				       timeout,
				       NULL);
		if (ret != 0) {
			DBG("Cannot send %d byte(s) (%d)\n", ret, chunk_size);

			zassert_equal(ret, 0, "websocket client ws send");
		}

		/* Make sure the receiving side gets the data now */
		k_yield();
	}
}

/* Start to send raw data and do not use websocket client API for this so
 * that we can send garbage data if needed.
 */
void test_v6_init(void)
{
	int ret;

	ret = net_ipaddr_parse(CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			       strlen(CONFIG_NET_CONFIG_MY_IPV6_ADDR),
			       &server_addr);
	zassert_equal(ret, 1, "cannot parse server address");

	ret = net_app_init_tcp_client(&app_ctx_v6,
				      NULL,
				      NULL,
				      CONFIG_NET_CONFIG_MY_IPV6_ADDR,
				      80,
				      0,
				      NULL);
	zassert_equal(ret, 0, "websocket IPv6 client init");

	net_app_set_cb(&app_ctx_v6, NULL, recv_cb, NULL, NULL);
}

void test_v6_connect(void)
{
	test_connect(&app_ctx_v6);

	k_sem_give(&progress);
}

void test_v6_close(void)
{
	test_close(&app_ctx_v6);
}

static void test_v6_send_recv(int chunk_size)
{
	static int header_sent;
	int ret;

	if (!header_sent) {
		ret = net_app_send_buf(&app_ctx_v6, http_msg,
				       sizeof(http_msg) - 1,
				       &server_addr,
				       sizeof(struct sockaddr_in6),
				       timeout, NULL);
		if (ret != 0) {
			DBG("Cannot send byte (%d)\n", ret);

			zassert_equal(ret, 0,
				      "websocket IPv6 client http send");
		}

		header_sent = true;
	}

	test_send_recv(chunk_size, &app_ctx_v6);
}

void test_v6_send_recv_n(int chunk_size)
{
	k_sem_take(&progress, K_FOREVER);

	/* Make sure we have a fresh start before running this specific test */
	memcpy(ws_test_msg, ws_test_msg_orig, sizeof(ws_test_msg));
	bytes_received = 0;

	ws_ctx->websocket.data_waiting = 0;
	if (ws_ctx->websocket.pending) {
		net_pkt_unref(ws_ctx->websocket.pending);
		ws_ctx->websocket.pending = NULL;
	}

	test_v6_send_recv(chunk_size);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	zassert_false(failure, "Send test failed");
}

void test_v6_send_recv_1(void)
{
	test_v6_send_recv_n(1);
}

void test_v6_send_recv_2(void)
{
	test_v6_send_recv_n(2);
}

void test_v6_send_recv_3(void)
{
	test_v6_send_recv_n(3);
}

void test_v6_send_recv_4(void)
{
	test_v6_send_recv_n(4);
}

void test_v6_send_recv_5(void)
{
	test_v6_send_recv_n(5);
}

void test_v6_send_recv_6(void)
{
	test_v6_send_recv_n(6);
}

void test_v6_send_recv_7(void)
{
	test_v6_send_recv_n(7);
}

void test_v6_send_multi_msg(void)
{
	test_send_multi_msg(&app_ctx_v6);
}

/* Start to send raw data and do not use websocket client API for this so
 * that we can send garbage data if needed.
 */
void test_v4_init(void)
{
	int ret;

	ret = net_ipaddr_parse(CONFIG_NET_CONFIG_MY_IPV4_ADDR,
			       strlen(CONFIG_NET_CONFIG_MY_IPV4_ADDR),
			       &server_addr);
	zassert_equal(ret, 1, "cannot parse server address");

	ret = net_app_init_tcp_client(&app_ctx_v4,
				      NULL,
				      NULL,
				      CONFIG_NET_CONFIG_MY_IPV4_ADDR,
				      80,
				      0,
				      NULL);
	zassert_equal(ret, 0, "websocket IPv4 client init");

	net_app_set_cb(&app_ctx_v4, NULL, recv_cb, NULL, NULL);
}

void test_v4_connect(void)
{
	test_connect(&app_ctx_v4);

	k_sem_give(&progress);
}

void test_v4_close(void)
{
	test_close(&app_ctx_v4);
}

static void test_v4_send_recv(int chunk_size)
{
	static int header_sent;
	int ret;

	if (!header_sent) {
		ret = net_app_send_buf(&app_ctx_v4, http_msg,
				       sizeof(http_msg) - 1,
				       &server_addr,
				       sizeof(struct sockaddr_in),
				       timeout, NULL);
		if (ret != 0) {
			DBG("Cannot send byte (%d)\n", ret);

			zassert_equal(ret, 0,
				      "websocket IPv4 client http send");
		}

		header_sent = true;
	}

	test_send_recv(chunk_size, &app_ctx_v4);
}

void test_v4_send_recv_n(int chunk_size)
{
	k_sem_take(&progress, K_FOREVER);

	/* Make sure we have a fresh start before running this specific test */
	memcpy(ws_test_msg, ws_test_msg_orig, sizeof(ws_test_msg));
	bytes_received = 0;

	ws_ctx->websocket.data_waiting = 0;
	if (ws_ctx->websocket.pending) {
		net_pkt_unref(ws_ctx->websocket.pending);
		ws_ctx->websocket.pending = NULL;
	}

	test_v4_send_recv(chunk_size);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		zassert_true(false, "Timeout while waiting data");
	}

	zassert_false(failure, "Send test failed");
}

void test_v4_send_recv_1(void)
{
	test_v4_send_recv_n(1);
}

void test_v4_send_recv_2(void)
{
	test_v4_send_recv_n(2);
}

void test_v4_send_recv_3(void)
{
	test_v4_send_recv_n(3);
}

void test_v4_send_recv_4(void)
{
	test_v4_send_recv_n(4);
}

void test_v4_send_recv_5(void)
{
	test_v4_send_recv_n(5);
}

void test_v4_send_recv_6(void)
{
	test_v4_send_recv_n(6);
}

void test_v4_send_recv_7(void)
{
	test_v4_send_recv_n(7);
}

void test_v4_send_multi_msg(void)
{
	test_send_multi_msg(&app_ctx_v4);
}

void test_main(void)
{
	ztest_test_suite(websocket,
			 ztest_unit_test(test_websocket_init_server),
			 ztest_unit_test(test_init),
			 ztest_unit_test(test_v6_init),
			 ztest_unit_test(test_v6_connect),
			 ztest_unit_test(test_v6_send_recv_1),
			 ztest_unit_test(test_v6_send_recv_2),
			 ztest_unit_test(test_v6_send_recv_3),
			 ztest_unit_test(test_v6_send_recv_4),
			 ztest_unit_test(test_v6_send_recv_5),
			 ztest_unit_test(test_v6_send_recv_6),
			 ztest_unit_test(test_v6_send_recv_7),
			 ztest_unit_test(test_v6_send_multi_msg),
			 ztest_unit_test(test_v6_close),
			 ztest_unit_test(test_websocket_init_server),
			 ztest_unit_test(test_v4_init),
			 ztest_unit_test(test_v4_connect),
			 ztest_unit_test(test_v4_send_recv_1),
			 ztest_unit_test(test_v4_send_recv_2),
			 ztest_unit_test(test_v4_send_recv_3),
			 ztest_unit_test(test_v4_send_recv_4),
			 ztest_unit_test(test_v4_send_recv_5),
			 ztest_unit_test(test_v4_send_recv_6),
			 ztest_unit_test(test_v4_send_recv_7),
			 ztest_unit_test(test_v4_send_multi_msg),
			 ztest_unit_test(test_v4_close)
			 );

	ztest_run_test_suite(websocket);
}
