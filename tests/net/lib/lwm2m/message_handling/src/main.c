/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "lwm2m_engine.h"
#include "lwm2m_message_handling.h"
#include "lwm2m_obj_gateway.h"

#include <zephyr/kernel.h>
#include <zephyr/net/coap.h>
#include <zephyr/net/lwm2m.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define SERVER_PORT     5683
#define RECV_TIMEOUT_MS 500
/* No packet in this suite exceeds 64 bytes; small buffers keep the test
 * thread's stack use low on every platform.
 */
#define PKT_BUF_SIZE    128
#define TIMEZONE_MAX    64
#define BLOCK1_SIZE     16
#define SHORT_TIMEZONE  "UTC+1"
/* Long enough for the Read response not to fit the deduplication cache. */
#define LONG_TIMEZONE   "America/Argentina/Buenos_Aires"
/* Prefix the Gateway object gives its first instance. */
#define GATEWAY_PREFIX  "n0"

#define CACHE_SIZE CONFIG_LWM2M_COAP_DEDUP_CACHE_SIZE
BUILD_ASSERT(CACHE_SIZE == 2, "The eviction test expects a cache of two entries");

struct msg_handling_fixture {
	struct lwm2m_ctx ctx;
	struct net_sockaddr_in server_addr;
	int server_sock;
	char timezone[TIMEZONE_MAX];
};

static struct msg_handling_fixture fixture;

/* Callback bookkeeping shared with the resource callbacks. */
static uint32_t write_cb_calls;
static uint32_t exec_cb_calls;
static uint32_t read_cb_calls;
static uint32_t gateway_cb_calls;
static bool write_cb_last_block;
static bool acknowledge_in_write_cb;
static int write_cb_result;

static int timezone_post_write_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
				  uint8_t *data, uint16_t data_len, bool last_block,
				  size_t total_size, size_t offset)
{
	write_cb_calls++;
	write_cb_last_block = last_block;

	if (acknowledge_in_write_cb) {
		lwm2m_acknowledge(&fixture.ctx);
	}

	return write_cb_result;
}

static void *timezone_read_cb(uint16_t obj_inst_id, uint16_t res_id, uint16_t res_inst_id,
			      size_t *data_len)
{
	read_cb_calls++;
	*data_len = strlen(fixture.timezone);

	return fixture.timezone;
}

static int reboot_exec_cb(uint16_t obj_inst_id, uint8_t *args, uint16_t args_len)
{
	exec_cb_calls++;

	return 0;
}

/* Answers every prefixed request with a payload that does not fit the cache. */
static int gateway_cb(struct lwm2m_message *msg)
{
	int ret;

	gateway_cb_calls++;

	msg->code = COAP_RESPONSE_CODE_CHANGED;
	ret = lwm2m_init_message(msg);
	zassert_ok(ret, "Failed to init gateway response");
	ret = coap_packet_append_payload_marker(&msg->cpkt);
	zassert_ok(ret, "Failed to append payload marker");
	ret = coap_packet_append_payload(&msg->cpkt, LONG_TIMEZONE, strlen(LONG_TIMEZONE));
	zassert_ok(ret, "Failed to append gateway payload");

	return 0;
}

/* Build a CON request for /[prefix/]3/0/<res>. A Block1 option is added when
 * block_size_bytes is non-zero.
 */
static uint16_t build_request(uint8_t *buf, size_t buf_len, uint8_t method, uint16_t mid,
			      const char *prefix, const char *res, const char *payload,
			      size_t block_size_bytes, uint32_t block_num, bool more)
{
	static const uint8_t token[] = {0xde, 0xad, 0xbe, 0xef, 0x01, 0x02, 0x03, 0x04};
	struct coap_packet pkt;
	int ret;

	ret = coap_packet_init(&pkt, buf, buf_len, COAP_VERSION_1, COAP_TYPE_CON, sizeof(token),
			       token, method, mid);
	zassert_ok(ret, "Failed to init request");

	if (prefix != NULL) {
		ret = coap_packet_append_option(&pkt, COAP_OPTION_URI_PATH, prefix, strlen(prefix));
		zassert_ok(ret, "Failed to append URI path");
	}

	ret = coap_packet_append_option(&pkt, COAP_OPTION_URI_PATH, "3", 1);
	zassert_ok(ret, "Failed to append URI path");
	ret = coap_packet_append_option(&pkt, COAP_OPTION_URI_PATH, "0", 1);
	zassert_ok(ret, "Failed to append URI path");
	ret = coap_packet_append_option(&pkt, COAP_OPTION_URI_PATH, res, strlen(res));
	zassert_ok(ret, "Failed to append URI path");

	if (payload != NULL) {
		ret = coap_append_option_int(&pkt, COAP_OPTION_CONTENT_FORMAT,
					     COAP_CONTENT_FORMAT_TEXT_PLAIN);
		zassert_ok(ret, "Failed to append content format");
	}

	if (block_size_bytes > 0) {
		uint32_t block1 = (block_num << 4) | (more ? BIT(3) : 0) |
				  coap_bytes_to_block_size(block_size_bytes);

		ret = coap_append_option_int(&pkt, COAP_OPTION_BLOCK1, block1);
		zassert_ok(ret, "Failed to append Block1 option");
	}

	if (payload != NULL) {
		ret = coap_packet_append_payload_marker(&pkt);
		zassert_ok(ret, "Failed to append payload marker");
		ret = coap_packet_append_payload(&pkt, payload, strlen(payload));
		zassert_ok(ret, "Failed to append payload");
	}

	return pkt.offset;
}

static uint16_t build_write(uint8_t *buf, size_t buf_len, uint16_t mid, const char *value)
{
	return build_request(buf, buf_len, COAP_METHOD_PUT, mid, NULL, "15", value, 0, 0, false);
}

static uint16_t build_block1_write(uint8_t *buf, size_t buf_len, uint16_t mid, uint32_t block_num,
				   bool more, const char *block)
{
	zassert_equal(strlen(block), BLOCK1_SIZE, "Block payload must fill the block");

	return build_request(buf, buf_len, COAP_METHOD_PUT, mid, NULL, "15", block, BLOCK1_SIZE,
			     block_num, more);
}

/* Feed a request to the engine as if it had arrived from the server. */
static void receive(const uint8_t *req, uint16_t req_len)
{
	uint8_t buf[PKT_BUF_SIZE];

	memcpy(buf, req, req_len);
	lwm2m_udp_receive(&fixture.ctx, buf, req_len, (struct net_sockaddr *)&fixture.server_addr);
}

/* Returns the length of the next packet the engine sent, or -EAGAIN. */
static int recv_response(uint8_t *buf, size_t buf_len)
{
	struct zsock_pollfd pfd = {
		.fd = fixture.server_sock,
		.events = ZSOCK_POLLIN,
	};
	int ret;

	ret = zsock_poll(&pfd, 1, RECV_TIMEOUT_MS);
	if (ret < 0) {
		return -errno;
	}

	if (ret == 0) {
		return -EAGAIN;
	}

	ret = zsock_recv(fixture.server_sock, buf, buf_len, 0);
	if (ret < 0) {
		return -errno;
	}

	zassert_true(ret < buf_len, "Packet larger than the test buffer");

	return ret;
}

static void expect_response(uint8_t *buf, size_t buf_len, int *len, uint8_t type, uint8_t code,
			    uint16_t mid)
{
	struct coap_packet pkt;
	int ret;

	*len = recv_response(buf, buf_len);
	zassert_true(*len > 0, "No response received (%d)", *len);

	ret = coap_packet_parse(&pkt, buf, *len, NULL, 0);
	zassert_ok(ret, "Invalid response");
	zassert_equal(coap_header_get_type(&pkt), type, "Unexpected response type");
	zassert_equal(coap_header_get_code(&pkt), code, "Unexpected response code");
	zassert_equal(coap_header_get_id(&pkt), mid, "Unexpected message ID");
}

static void expect_no_response(void)
{
	uint8_t buf[PKT_BUF_SIZE];

	zassert_equal(recv_response(buf, sizeof(buf)), -EAGAIN, "Unexpected packet received");
}

static void *msg_handling_setup(void)
{
	struct lwm2m_ctx *ctx = &fixture.ctx;
	int ret;

	fixture.server_addr.sin_family = NET_AF_INET;
	fixture.server_addr.sin_port = sys_cpu_to_be16(SERVER_PORT);
	ret = net_addr_pton(NET_AF_INET, "127.0.0.1", &fixture.server_addr.sin_addr);
	zassert_ok(ret, "Failed to parse server address");

	fixture.server_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(fixture.server_sock >= 0, "Failed to create server socket");
	ret = zsock_bind(fixture.server_sock, (struct net_sockaddr *)&fixture.server_addr,
			 sizeof(fixture.server_addr));
	zassert_ok(ret, "Failed to bind server socket");

	lwm2m_engine_context_init(ctx);
	memcpy(&ctx->remote_addr_storage, &fixture.server_addr, sizeof(fixture.server_addr));

	ctx->sock_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(ctx->sock_fd >= 0, "Failed to create client socket");
	ret = zsock_connect(ctx->sock_fd, (struct net_sockaddr *)&fixture.server_addr,
			    sizeof(fixture.server_addr));
	zassert_ok(ret, "Failed to connect client socket");

	ret = lwm2m_socket_add(ctx);
	zassert_ok(ret, "Failed to add socket to the engine");

	ret = lwm2m_set_res_buf(&LWM2M_OBJ(3, 0, 15), fixture.timezone, sizeof(fixture.timezone), 0,
				0);
	zassert_ok(ret, "Failed to set resource buffer");
	ret = lwm2m_register_post_write_callback(&LWM2M_OBJ(3, 0, 15), timezone_post_write_cb);
	zassert_ok(ret, "Failed to register post write callback");
	ret = lwm2m_register_read_callback(&LWM2M_OBJ(3, 0, 15), timezone_read_cb);
	zassert_ok(ret, "Failed to register read callback");
	ret = lwm2m_register_exec_callback(&LWM2M_OBJ(3, 0, 4), reboot_exec_cb);
	zassert_ok(ret, "Failed to register execute callback");

	ret = lwm2m_create_object_inst(&LWM2M_OBJ(25, 0));
	zassert_ok(ret, "Failed to create gateway instance");
	ret = lwm2m_register_gw_callback(0, gateway_cb);
	zassert_ok(ret, "Failed to register gateway callback");

	return &fixture;
}

static void msg_handling_before(void *f)
{
	uint8_t buf[PKT_BUF_SIZE];

	ARG_UNUSED(f);

	/* Drop state left by the previous test: messages, pendings, the
	 * deduplication cache and packets still in flight to the server.
	 */
	lwm2m_engine_context_close(&fixture.ctx);
	while (recv_response(buf, sizeof(buf)) > 0) {
	}

	write_cb_calls = 0;
	exec_cb_calls = 0;
	read_cb_calls = 0;
	gateway_cb_calls = 0;
	acknowledge_in_write_cb = false;
	write_cb_result = 0;
	memset(fixture.timezone, 0, sizeof(fixture.timezone));
}

static void msg_handling_teardown(void *f)
{
	ARG_UNUSED(f);

	lwm2m_engine_context_close(&fixture.ctx);
	lwm2m_socket_del(&fixture.ctx);
	zsock_close(fixture.ctx.sock_fd);
	zsock_close(fixture.server_sock);
}

ZTEST(lwm2m_message_handling, test_dedup_write_not_reprocessed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp1[PKT_BUF_SIZE];
	uint8_t rsp2[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp1_len;
	int rsp2_len;

	req_len = build_write(req, sizeof(req), 0x1234, SHORT_TIMEZONE);

	receive(req, req_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x1234);
	zassert_equal(write_cb_calls, 1, "Write callback not called");
	zassert_str_equal(fixture.timezone, SHORT_TIMEZONE, "Resource not written");

	receive(req, req_len);
	expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x1234);
	zassert_equal(write_cb_calls, 1, "Write callback called for the duplicate");
	zassert_equal(rsp1_len, rsp2_len, "Duplicate response has a different length");
	zassert_mem_equal(rsp1, rsp2, rsp1_len, "Duplicate response differs");
}

ZTEST(lwm2m_message_handling, test_dedup_execute_not_reprocessed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp1[PKT_BUF_SIZE];
	uint8_t rsp2[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp1_len;
	int rsp2_len;

	req_len = build_request(req, sizeof(req), COAP_METHOD_POST, 0x2345, NULL, "4", NULL, 0, 0,
				false);

	receive(req, req_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x2345);
	zassert_equal(exec_cb_calls, 1, "Execute callback not called");

	receive(req, req_len);
	expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x2345);
	zassert_equal(exec_cb_calls, 1, "Execute callback called for the duplicate");
	zassert_equal(rsp1_len, rsp2_len, "Duplicate response has a different length");
	zassert_mem_equal(rsp1, rsp2, rsp1_len, "Duplicate response differs");
}

ZTEST(lwm2m_message_handling, test_dedup_other_mid_processed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp_len;

	req_len = build_write(req, sizeof(req), 0x3456, SHORT_TIMEZONE);
	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x3456);
	zassert_equal(write_cb_calls, 1, "Write callback not called");

	req_len = build_write(req, sizeof(req), 0x3457, SHORT_TIMEZONE);
	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x3457);
	zassert_equal(write_cb_calls, 2, "Request with a new message ID not processed");
}

ZTEST(lwm2m_message_handling, test_dedup_acknowledged_gets_empty_ack)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t ack1[PKT_BUF_SIZE];
	uint8_t ack2[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	struct coap_packet pkt;
	uint16_t req_len;
	int ack1_len;
	int ack2_len;
	int rsp_len;
	int ret;

	acknowledge_in_write_cb = true;
	req_len = build_write(req, sizeof(req), 0x4567, SHORT_TIMEZONE);

	receive(req, req_len);
	expect_response(ack1, sizeof(ack1), &ack1_len, COAP_TYPE_ACK, COAP_CODE_EMPTY, 0x4567);
	zassert_equal(write_cb_calls, 1, "Write callback not called");

	/* The separate response is a CON with a message ID of its own. */
	rsp_len = recv_response(rsp, sizeof(rsp));
	zassert_true(rsp_len > 0, "No separate response received (%d)", rsp_len);
	ret = coap_packet_parse(&pkt, rsp, rsp_len, NULL, 0);
	zassert_ok(ret, "Invalid separate response");
	zassert_equal(coap_header_get_type(&pkt), COAP_TYPE_CON, "Separate response is not CON");
	zassert_equal(coap_header_get_code(&pkt), COAP_RESPONSE_CODE_CHANGED,
		      "Unexpected separate response code");
	zassert_not_equal(coap_header_get_id(&pkt), 0x4567, "Separate response reuses request ID");

	receive(req, req_len);
	expect_response(ack2, sizeof(ack2), &ack2_len, COAP_TYPE_ACK, COAP_CODE_EMPTY, 0x4567);
	zassert_equal(write_cb_calls, 1, "Write callback called for the duplicate");
	zassert_equal(ack1_len, ack2_len, "Duplicate ACK has a different length");
	zassert_mem_equal(ack1, ack2, ack1_len, "Duplicate ACK differs");
	expect_no_response();
}

/* Receive an acknowledged Write and read the empty ACK and, when expected, the
 * separate CON response.
 */
static void acknowledged_write(uint16_t mid, bool expect_separate_response)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	struct coap_packet pkt;
	uint16_t req_len;
	int rsp_len;
	int ret;

	req_len = build_write(req, sizeof(req), mid, SHORT_TIMEZONE);
	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_CODE_EMPTY, mid);

	if (!expect_separate_response) {
		expect_no_response();
		return;
	}

	rsp_len = recv_response(rsp, sizeof(rsp));
	zassert_true(rsp_len > 0, "No separate response received (%d)", rsp_len);
	ret = coap_packet_parse(&pkt, rsp, rsp_len, NULL, 0);
	zassert_ok(ret, "Invalid separate response");
	zassert_equal(coap_header_get_type(&pkt), COAP_TYPE_CON, "Separate response is not CON");
}

ZTEST(lwm2m_message_handling, test_dedup_failed_separate_response_replayed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp_len;

	acknowledge_in_write_cb = true;

	/* Occupy every pending with a separate response. */
	acknowledged_write(0x5c00, true);
	acknowledged_write(0x5c01, true);
	zassert_equal(write_cb_calls, 2, "Write callbacks not called");

	/* No pending is left, so this separate response cannot be sent. */
	acknowledged_write(0x5c02, false);
	zassert_equal(write_cb_calls, 3, "Write callback not called");

	/* A retransmission gets the response piggybacked instead. */
	req_len = build_write(req, sizeof(req), 0x5c02, SHORT_TIMEZONE);
	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5c02);
	zassert_equal(write_cb_calls, 3, "Write callback called for the duplicate");
}

ZTEST(lwm2m_message_handling, test_dedup_large_gateway_response)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp_len;

	/* A Write whose response did not fit is not run again. */
	req_len = build_request(req, sizeof(req), COAP_METHOD_PUT, 0x5d00, GATEWAY_PREFIX, "15",
				SHORT_TIMEZONE, 0, 0, false);

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5d00);
	zassert_equal(gateway_cb_calls, 1, "Gateway callback not called");
	zassert_true(rsp_len > LWM2M_COAP_DEDUP_RESPONSE_SIZE, "Response fits the cache");

	receive(req, req_len);
	expect_no_response();
	zassert_equal(gateway_cb_calls, 1, "Gateway callback called for the duplicate Write");

	/* A Read in the same situation is processed again. */
	req_len = build_request(req, sizeof(req), COAP_METHOD_GET, 0x5d01, GATEWAY_PREFIX, "15",
				NULL, 0, 0, false);

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5d01);
	zassert_equal(gateway_cb_calls, 2, "Gateway callback not called for the Read");

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5d01);
	zassert_equal(gateway_cb_calls, 3, "Duplicate Read not processed again");
}

ZTEST(lwm2m_message_handling, test_dedup_large_read_reprocessed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp1[PKT_BUF_SIZE];
	uint8_t rsp2[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp1_len;
	int rsp2_len;

	strcpy(fixture.timezone, LONG_TIMEZONE);
	req_len = build_request(req, sizeof(req), COAP_METHOD_GET, 0x5678, NULL, "15", NULL, 0, 0,
				false);

	receive(req, req_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CONTENT,
			0x5678);
	zassert_equal(read_cb_calls, 1, "Read callback not called");
	zassert_true(rsp1_len > LWM2M_COAP_DEDUP_RESPONSE_SIZE, "Response fits the cache");

	receive(req, req_len);
	expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CONTENT,
			0x5678);
	zassert_equal(read_cb_calls, 2, "Duplicate Read not processed again");
	zassert_equal(rsp1_len, rsp2_len, "Duplicate response has a different length");
	zassert_mem_equal(rsp1, rsp2, rsp1_len, "Duplicate response differs");
}

ZTEST(lwm2m_message_handling, test_dedup_large_reads_do_not_evict)
{
	uint8_t write_req[PKT_BUF_SIZE];
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp1[PKT_BUF_SIZE];
	uint8_t rsp2[PKT_BUF_SIZE];
	uint16_t write_len;
	uint16_t req_len;
	int rsp1_len;
	int rsp2_len;

	write_len = build_write(write_req, sizeof(write_req), 0x5e00, SHORT_TIMEZONE);
	receive(write_req, write_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5e00);
	zassert_equal(write_cb_calls, 1, "Write callback not called");

	/* Reads whose responses do not fit the cache take no entries. */
	strcpy(fixture.timezone, LONG_TIMEZONE);
	for (uint16_t i = 0; i < CACHE_SIZE; i++) {
		req_len = build_request(req, sizeof(req), COAP_METHOD_GET, 0x5e10 + i, NULL, "15",
					NULL, 0, 0, false);
		receive(req, req_len);
		expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK,
				COAP_RESPONSE_CODE_CONTENT, 0x5e10 + i);
		zassert_true(rsp2_len > LWM2M_COAP_DEDUP_RESPONSE_SIZE, "Response fits the cache");
	}
	zassert_equal(read_cb_calls, CACHE_SIZE, "Read callback not called for each Read");

	/* The Write is still cached. */
	receive(write_req, write_len);
	expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5e00);
	zassert_equal(write_cb_calls, 1, "Write callback called for the duplicate");
	zassert_equal(rsp1_len, rsp2_len, "Duplicate response has a different length");
	zassert_mem_equal(rsp1, rsp2, rsp1_len, "Duplicate response differs");
}

ZTEST(lwm2m_message_handling, test_dedup_block1_block_not_reprocessed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp1[PKT_BUF_SIZE];
	uint8_t rsp2[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp1_len;
	int rsp2_len;

	req_len = build_block1_write(req, sizeof(req), 0x5a00, 0, true, "Europe/Helsinki/");

	receive(req, req_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CONTINUE,
			0x5a00);
	zassert_equal(write_cb_calls, 1, "Write callback not called for the first block");
	zassert_false(write_cb_last_block, "First block reported as the last one");
	zassert_true(rsp1_len <= LWM2M_COAP_DEDUP_RESPONSE_SIZE, "Response does not fit the cache");

	receive(req, req_len);
	expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CONTINUE,
			0x5a00);
	zassert_equal(write_cb_calls, 1, "Write callback called for the duplicate block");
	zassert_equal(rsp1_len, rsp2_len, "Duplicate response has a different length");
	zassert_mem_equal(rsp1, rsp2, rsp1_len, "Duplicate response differs");

	req_len = build_block1_write(req, sizeof(req), 0x5a01, 1, false, "Second/block/___");

	receive(req, req_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x5a01);
	zassert_equal(write_cb_calls, 2, "Write callback not called for the last block");
	zassert_true(write_cb_last_block, "Last block not reported as such");
}

ZTEST(lwm2m_message_handling, test_dedup_error_response_replayed)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp1[PKT_BUF_SIZE];
	uint8_t rsp2[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp1_len;
	int rsp2_len;

	write_cb_result = -ECANCELED;
	req_len = build_write(req, sizeof(req), 0x5b00, SHORT_TIMEZONE);

	receive(req, req_len);
	expect_response(rsp1, sizeof(rsp1), &rsp1_len, COAP_TYPE_ACK,
			COAP_RESPONSE_CODE_NOT_ACCEPTABLE, 0x5b00);
	zassert_equal(write_cb_calls, 1, "Write callback not called");

	receive(req, req_len);
	expect_response(rsp2, sizeof(rsp2), &rsp2_len, COAP_TYPE_ACK,
			COAP_RESPONSE_CODE_NOT_ACCEPTABLE, 0x5b00);
	zassert_equal(write_cb_calls, 1, "Write callback called for the duplicate");
	zassert_equal(rsp1_len, rsp2_len, "Duplicate response has a different length");
	zassert_mem_equal(rsp1, rsp2, rsp1_len, "Duplicate response differs");
}

ZTEST(lwm2m_message_handling, test_dedup_entry_expires)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp_len;

	req_len = build_write(req, sizeof(req), 0x6789, SHORT_TIMEZONE);

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x6789);
	zassert_equal(write_cb_calls, 1, "Write callback not called");

	k_sleep(K_SECONDS(CONFIG_LWM2M_COAP_DEDUP_LIFETIME + 1));

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x6789);
	zassert_equal(write_cb_calls, 2, "Expired request not processed again");
}

ZTEST(lwm2m_message_handling, test_dedup_oldest_entry_evicted)
{
	uint8_t req[CACHE_SIZE + 1][PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	uint16_t req_len[CACHE_SIZE + 1];
	int rsp_len;

	for (uint16_t i = 0; i < CACHE_SIZE + 1; i++) {
		req_len[i] = build_write(req[i], sizeof(req[i]), 0x7000 + i, SHORT_TIMEZONE);
		receive(req[i], req_len[i]);
		expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK,
				COAP_RESPONSE_CODE_CHANGED, 0x7000 + i);
		/* Keep the timestamps apart so the eviction order is defined. */
		k_sleep(K_MSEC(10));
	}
	zassert_equal(write_cb_calls, CACHE_SIZE + 1, "Write callback not called for each request");

	/* The first request has been evicted, the last one is still cached. */
	receive(req[0], req_len[0]);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x7000);
	zassert_equal(write_cb_calls, CACHE_SIZE + 2, "Evicted request not processed again");

	receive(req[CACHE_SIZE], req_len[CACHE_SIZE]);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x7000 + CACHE_SIZE);
	zassert_equal(write_cb_calls, CACHE_SIZE + 2, "Cached request processed again");
}

ZTEST(lwm2m_message_handling, test_dedup_cleared_on_context_close)
{
	uint8_t req[PKT_BUF_SIZE];
	uint8_t rsp[PKT_BUF_SIZE];
	uint16_t req_len;
	int rsp_len;

	req_len = build_write(req, sizeof(req), 0x789a, SHORT_TIMEZONE);

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x789a);
	zassert_equal(write_cb_calls, 1, "Write callback not called");

	lwm2m_engine_context_close(&fixture.ctx);

	receive(req, req_len);
	expect_response(rsp, sizeof(rsp), &rsp_len, COAP_TYPE_ACK, COAP_RESPONSE_CODE_CHANGED,
			0x789a);
	zassert_equal(write_cb_calls, 2, "Request not processed again after context close");
}

ZTEST_SUITE(lwm2m_message_handling, NULL, msg_handling_setup, msg_handling_before, NULL,
	    msg_handling_teardown);
