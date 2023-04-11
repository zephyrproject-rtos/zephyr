/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_DBG);

#include <errno.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/coap.h>

#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include "net_private.h"

#define COAP_BUF_SIZE 128
#define COAP_FIXED_HEADER_SIZE 4

#define NUM_PENDINGS 3
#define NUM_OBSERVERS 3
#define NUM_REPLIES 3

static struct coap_pending pendings[NUM_PENDINGS];
static struct coap_observer observers[NUM_OBSERVERS];
static struct coap_reply replies[NUM_REPLIES];

/* This is exposed for this test in subsys/net/lib/coap/coap_link_format.c */
bool _coap_match_path_uri(const char * const *path,
			  const char *uri, uint16_t len);

/* Some forward declarations */
static void server_notify_callback(struct coap_resource *resource,
				   struct coap_observer *observer);

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 struct sockaddr *addr, socklen_t addr_len);

static const char * const server_resource_1_path[] = { "s", "1", NULL };
static struct coap_resource server_resources[] =  {
	{ .path = server_resource_1_path,
	  .get = server_resource_1_get,
	  .notify = server_notify_callback },
	{ },
};

#define MY_PORT 12345
#define peer_addr { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0x2 } } }
static struct sockaddr_in6 dummy_addr = {
	.sin6_family = AF_INET6,
	.sin6_addr = peer_addr };

static uint8_t data_buf[2][COAP_BUF_SIZE];

ZTEST(coap, test_build_empty_pdu)
{
	uint8_t result_pdu[] = { 0x40, 0x01, 0x0, 0x0 };
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE,
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);

	zassert_equal(r, 0, "Could not initialize packet");
	zassert_equal(cpkt.offset, sizeof(result_pdu),
		     "Different size from the reference packet");
	zassert_equal(cpkt.hdr_len, COAP_FIXED_HEADER_SIZE,
		      "Invalid header length");
	zassert_equal(cpkt.opt_len, 0, "Invalid options length");
	zassert_mem_equal(result_pdu, cpkt.data, cpkt.offset,
			  "Built packet doesn't match reference packet");
}

ZTEST(coap, test_build_simple_pdu)
{
	uint8_t result_pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
				 'n', 0xC0, 0xFF, 'p', 'a', 'y', 'l',
				 'o', 'a', 'd', 0x00 };
	struct coap_packet cpkt;
	const char token[] = "token";
	static const char payload[] = "payload";
	uint8_t *data = data_buf[0];
	const uint8_t *payload_start;
	uint16_t payload_len;
	int r;

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE,
			     COAP_VERSION_1, COAP_TYPE_NON_CON,
			     strlen(token), token,
			     COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED,
			     0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Failed to set the payload marker");

	r = coap_packet_append_payload(&cpkt, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to set the payload");

	zassert_equal(cpkt.offset, sizeof(result_pdu),
		     "Different size from the reference packet");
	zassert_equal(cpkt.hdr_len, COAP_FIXED_HEADER_SIZE + strlen(token),
		      "Invalid header length");
	zassert_equal(cpkt.opt_len, 1, "Invalid options length");
	zassert_mem_equal(result_pdu, cpkt.data, cpkt.offset,
			  "Built packet doesn't match reference packet");

	payload_start = coap_packet_get_payload(&cpkt, &payload_len);

	zassert_equal(payload_len, sizeof(payload), "Invalid payload length");
	zassert_equal_ptr(payload_start, cpkt.data + cpkt.offset - payload_len,
			  "Invalid payload pointer");
}

/* No options, No payload */
ZTEST(coap, test_parse_empty_pdu)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0 };
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	uint8_t ver;
	uint8_t type;
	uint8_t code;
	uint16_t id;
	int r;

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	zassert_equal(r, 0, "Could not parse packet");

	zassert_equal(cpkt.offset, sizeof(pdu),
		     "Different size from the reference packet");
	zassert_equal(cpkt.hdr_len, COAP_FIXED_HEADER_SIZE,
		      "Invalid header length");
	zassert_equal(cpkt.opt_len, 0, "Invalid options length");

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	zassert_equal(ver, 1U, "Invalid version for parsed packet");
	zassert_equal(type, COAP_TYPE_CON,
		      "Packet type doesn't match reference");
	zassert_equal(code, COAP_METHOD_GET,
		      "Packet code doesn't match reference");
	zassert_equal(id, 0U, "Packet id doesn't match reference");
}

/* 1 option, No payload (No payload marker) */
ZTEST(coap, test_parse_empty_pdu_1)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0, 0x40};
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	uint8_t ver;
	uint8_t type;
	uint8_t code;
	uint16_t id;
	int r;

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	zassert_equal(r, 0, "Could not parse packet");

	zassert_equal(cpkt.offset, sizeof(pdu),
		     "Different size from the reference packet");
	zassert_equal(cpkt.hdr_len, COAP_FIXED_HEADER_SIZE,
		      "Invalid header length");
	zassert_equal(cpkt.opt_len, 1, "Invalid options length");

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	zassert_equal(ver, 1U, "Invalid version for parsed packet");
	zassert_equal(type, COAP_TYPE_CON,
		      "Packet type doesn't match reference");
	zassert_equal(code, COAP_METHOD_GET,
		      "Packet code doesn't match reference");
	zassert_equal(id, 0U, "Packet id doesn't match reference");
}

ZTEST(coap, test_parse_simple_pdu)
{
	uint8_t pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y', 'l', 'o',
		       'a', 'd', 0x00 };
	struct coap_packet cpkt;
	struct coap_option options[16] = {};
	const uint8_t token[8];
	const uint8_t payload[] = "payload";
	uint8_t *data = data_buf[0];
	uint8_t ver;
	uint8_t type;
	uint8_t code;
	uint8_t tkl;
	uint16_t id;
	const uint8_t *payload_start;
	uint16_t payload_len;
	int r, count = ARRAY_SIZE(options) - 1;

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	zassert_equal(r, 0, "Could not parse packet");

	zassert_equal(cpkt.offset, sizeof(pdu),
		     "Different size from the reference packet");
	zassert_equal(cpkt.hdr_len, COAP_FIXED_HEADER_SIZE + strlen("token"),
		      "Invalid header length");
	zassert_equal(cpkt.opt_len, 3, "Invalid options length");

	payload_start = coap_packet_get_payload(&cpkt, &payload_len);

	zassert_equal(payload_len, sizeof(payload), "Invalid payload length");
	zassert_equal_ptr(payload_start, cpkt.data + cpkt.offset - payload_len,
			  "Invalid payload pointer");

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	zassert_equal(ver, 1U, "Invalid version for parsed packet");
	zassert_equal(type, COAP_TYPE_NON_CON,
		      "Packet type doesn't match reference");
	zassert_equal(code, COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED,
		      "Packet code doesn't match reference");
	zassert_equal(id, 0x1234, "Packet id doesn't match reference");

	tkl = coap_header_get_token(&cpkt, (uint8_t *)token);

	zassert_equal(tkl, 5U, "Token length doesn't match reference");
	zassert_mem_equal(token, "token", tkl,
			  "Token value doesn't match the reference");

	count = coap_find_options(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   options, count);

	zassert_equal(count, 1, "Unexpected number of options in the packet");
	zassert_equal(options[0].len, 1U,
		     "Option length doesn't match the reference");
	zassert_equal(((uint8_t *)options[0].value)[0],
		      COAP_CONTENT_FORMAT_TEXT_PLAIN,
		      "Option value doesn't match the reference");

	/* Not existent */
	count = coap_find_options(&cpkt, COAP_OPTION_ETAG, options, count);

	zassert_equal(count, 0,
		      "There shouldn't be any ETAG option in the packet");
}

ZTEST(coap, test_parse_malformed_pkt)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12 };

	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	r = coap_packet_parse(&cpkt, NULL, sizeof(opt), NULL, 0);
	zassert_equal(r, -EINVAL, "Should've failed to parse a packet");

	r = coap_packet_parse(&cpkt, data, 0, NULL, 0);
	zassert_equal(r, -EINVAL, "Should've failed to parse a packet");

	memcpy(data, opt, sizeof(opt));
	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	zassert_equal(r, -EINVAL, "Should've failed to parse a packet");
}

ZTEST(coap, test_parse_malformed_coap_hdr)
{
	uint8_t opt[] = { 0x55, 0x24, 0x49, 0x55, 0xff, 0x66, 0x77, 0x99};

	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	memcpy(data, opt, sizeof(opt));
	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	zassert_equal(r, -EBADMSG, "Should've failed to parse a packet");
}

ZTEST(coap, test_parse_malformed_opt)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xD0 };
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	zassert_equal(r, -EILSEQ, "Should've failed to parse a packet");
}

ZTEST(coap, test_parse_malformed_opt_len)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xC1 };
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	zassert_equal(r, -EILSEQ, "Should've failed to parse a packet");
}

ZTEST(coap, test_parse_malformed_opt_ext)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xE0, 0x01 };
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	zassert_equal(r, -EILSEQ, "Should've failed to parse a packet");
}

ZTEST(coap, test_parse_malformed_opt_len_ext)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xEE, 0x01, 0x02, 0x01};
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	zassert_equal(r, -EILSEQ, "Should've failed to parse a packet");
}

/* 1 option, No payload (with payload marker) */
ZTEST(coap, test_parse_malformed_marker)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0, 0x40, 0xFF};
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	int r;

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	zassert_not_equal(r, 0, "Should've failed to parse a packet");
}

ZTEST(coap, test_parse_req_build_ack)
{
	uint8_t pdu[] = { 0x45, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y', 'l', 'o',
		       'a', 'd', 0x00 };
	uint8_t ack_pdu[] = { 0x65, 0x80, 0x12, 0x34, 't', 'o', 'k', 'e', 'n' };
	struct coap_packet cpkt;
	struct coap_packet ack_cpkt;
	uint8_t *data = data_buf[0];
	uint8_t *ack_data = data_buf[1];
	int r;

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	zassert_equal(r, 0, "Could not parse packet");

	r = coap_ack_init(&ack_cpkt, &cpkt, ack_data, COAP_BUF_SIZE,
			  COAP_RESPONSE_CODE_BAD_REQUEST);
	zassert_equal(r, 0, "Could not initialize ACK packet");

	zassert_equal(ack_cpkt.offset, sizeof(ack_pdu),
		     "Different size from the reference packet");
	zassert_mem_equal(ack_pdu, ack_cpkt.data, ack_cpkt.offset,
			  "Built packet doesn't match reference packet");
}

ZTEST(coap, test_parse_req_build_empty_ack)
{
	uint8_t pdu[] = { 0x45, 0xA5, 0xDE, 0xAD, 't', 'o', 'k', 'e', 'n',
		       0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y', 'l', 'o',
		       'a', 'd', 0x00 };
	uint8_t ack_pdu[] = { 0x60, 0x00, 0xDE, 0xAD };
	struct coap_packet cpkt;
	struct coap_packet ack_cpkt;
	uint8_t *data = data_buf[0];
	uint8_t *ack_data = data_buf[1];
	int r;

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	zassert_equal(r, 0, "Could not parse packet");

	r = coap_ack_init(&ack_cpkt, &cpkt, ack_data, COAP_BUF_SIZE,
			  COAP_CODE_EMPTY);
	zassert_equal(r, 0, "Could not initialize ACK packet");

	zassert_equal(ack_cpkt.offset, sizeof(ack_pdu),
		      "Different size from the reference packet");
	zassert_mem_equal(ack_pdu, ack_cpkt.data, ack_cpkt.offset,
			  "Built packet doesn't match reference packet");
}

ZTEST(coap, test_match_path_uri)
{
	const char * const resource_path[] = {
		"s",
		"1",
		"foobar",
		"foobar3a",
		"foobar3",
		"devnull",
		NULL
	};
	const char *uri;
	int r;

	uri = "/k";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_false(r, "Matching %s failed", uri);

	uri = "/s";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_true(r, "Matching %s failed", uri);

	uri = "/foobar";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_true(r, "Matching %s failed", uri);

	uri = "/foobar2";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_false(r, "Matching %s failed", uri);

	uri = "/foobar*";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_true(r, "Matching %s failed", uri);

	uri = "/foobar3*";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_true(r, "Matching %s failed", uri);

	uri = "/devnull*";
	r = _coap_match_path_uri(resource_path, uri, strlen(uri));
	zassert_false(r, "Matching %s failed", uri);
}

#define BLOCK_WISE_TRANSFER_SIZE_GET 150

static void prepare_block1_request(struct coap_packet *req,
				   struct coap_block_context *req_ctx,
				   int *more)
{
	const char token[] = "token";
	uint8_t payload[32] = { 0 };
	uint8_t *data = data_buf[0];
	bool first;
	int r;
	int block_size = coap_block_size_to_bytes(COAP_BLOCK_32);
	int payload_len;

	/* Request Context */
	if (req_ctx->total_size == 0) {
		first = true;
		coap_block_transfer_init(req_ctx, COAP_BLOCK_32,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	} else {
		first = false;
	}

	r = coap_packet_init(req, data, COAP_BUF_SIZE, COAP_VERSION_1,
			     COAP_TYPE_CON, strlen(token),
			     token, COAP_METHOD_POST,
			     coap_next_id());
	zassert_equal(r, 0, "Unable to initialize request");

	r = coap_append_block1_option(req, req_ctx);
	zassert_equal(r, 0, "Unable to append block1 option");

	if (first) {
		r = coap_append_size1_option(req, req_ctx);
		zassert_equal(r, 0, "Unable to append size1 option");
	}

	r = coap_packet_append_payload_marker(req);
	zassert_equal(r, 0, "Unable to append payload marker");

	payload_len = req_ctx->total_size - req_ctx->current;
	if (payload_len > block_size) {
		payload_len = block_size;
	}

	r = coap_packet_append_payload(req, payload, payload_len);
	zassert_equal(r, 0, "Unable to append payload");

	*more = coap_next_block(req, req_ctx);
}

static void prepare_block1_response(struct coap_packet *rsp,
				    struct coap_block_context *rsp_ctx,
				    struct coap_packet *req)
{
	uint8_t token[8];
	uint8_t *data = data_buf[1];
	uint16_t id;
	uint8_t tkl;
	int r;

	if (rsp_ctx->total_size == 0) {
		coap_block_transfer_init(rsp_ctx, COAP_BLOCK_32,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	r = coap_update_from_block(req, rsp_ctx);
	zassert_equal(r, 0, "Failed to read block option");

	id = coap_header_get_id(req);
	tkl = coap_header_get_token(req, token);

	r = coap_packet_init(rsp, data, COAP_BUF_SIZE, COAP_VERSION_1,
			     COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CREATED, id);
	zassert_equal(r, 0, "Unable to initialize request");

	r = coap_append_block1_option(rsp, rsp_ctx);
	zassert_equal(r, 0, "Unable to append block1 option");
}

#define ITER_COUNT(len, block_len) DIV_ROUND_UP(len, block_len)

static void verify_block1_request(struct coap_block_context *req_ctx,
				  uint8_t iter)
{
	int block_size = coap_block_size_to_bytes(COAP_BLOCK_32);
	int iter_max = ITER_COUNT(BLOCK_WISE_TRANSFER_SIZE_GET, block_size);

	zassert_equal(req_ctx->block_size, COAP_BLOCK_32,
		      "req:%d,Couldn't get block size", iter);

	/* In last iteration "current" must match "total_size" */
	if (iter < iter_max) {
		zassert_equal(
			req_ctx->current, block_size * iter,
			"req:%d,Couldn't get the current block position", iter);
	} else {
		zassert_equal(
			req_ctx->current, req_ctx->total_size,
			"req:%d,Couldn't get the current block position", iter);
	}

	zassert_equal(req_ctx->total_size, BLOCK_WISE_TRANSFER_SIZE_GET,
		      "req:%d,Couldn't packet total size", iter);
}

static void verify_block1_response(struct coap_block_context *rsp_ctx,
				   uint8_t iter)
{
	zassert_equal(rsp_ctx->block_size, COAP_BLOCK_32,
		      "rsp:%d,Couldn't get block size", iter);
	zassert_equal(rsp_ctx->current,
		      coap_block_size_to_bytes(COAP_BLOCK_32) * (iter - 1U),
		      "rsp:%d, Couldn't get the current block position", iter);
	zassert_equal(rsp_ctx->total_size, BLOCK_WISE_TRANSFER_SIZE_GET,
		      "rsp:%d, Couldn't packet total size", iter);
}

ZTEST(coap, test_block1_size)
{
	struct coap_block_context req_ctx;
	struct coap_block_context rsp_ctx;
	struct coap_packet req;
	struct coap_packet rsp;
	int more;
	uint8_t i;

	i = 0U;
	more = 1;
	memset(&req_ctx, 0, sizeof(req_ctx));
	memset(&rsp_ctx, 0, sizeof(rsp_ctx));

	while (more) {
		prepare_block1_request(&req, &req_ctx, &more);
		prepare_block1_response(&rsp, &rsp_ctx, &req);

		i++;

		verify_block1_request(&req_ctx, i);
		verify_block1_response(&rsp_ctx, i);
	}
}

#define BLOCK2_WISE_TRANSFER_SIZE_GET 300

static void prepare_block2_request(struct coap_packet *req,
				   struct coap_block_context *req_ctx,
				   struct coap_packet *rsp)
{
	const char token[] = "token";
	uint8_t *data = data_buf[0];
	int r;

	/* Request Context */
	if (req_ctx->total_size == 0) {
		coap_block_transfer_init(req_ctx, COAP_BLOCK_64,
					 BLOCK2_WISE_TRANSFER_SIZE_GET);
	} else {
		coap_next_block(rsp, req_ctx);
	}

	r = coap_packet_init(req, data, COAP_BUF_SIZE, COAP_VERSION_1,
			     COAP_TYPE_CON, strlen(token),
			     token, COAP_METHOD_GET,
			     coap_next_id());
	zassert_equal(r, 0, "Unable to initialize request");

	r = coap_append_block2_option(req, req_ctx);
	zassert_equal(r, 0, "Unable to append block2 option");
}

static void prepare_block2_response(struct coap_packet *rsp,
				   struct coap_block_context *rsp_ctx,
				   struct coap_packet *req,
				   int *more)
{
	uint8_t payload[64];
	uint8_t token[8];
	uint8_t *data = data_buf[1];
	uint16_t id;
	uint8_t tkl;
	bool first;
	int r;
	int block_size = coap_block_size_to_bytes(COAP_BLOCK_64);
	int payload_len;

	if (rsp_ctx->total_size == 0) {
		first = true;
		coap_block_transfer_init(rsp_ctx, COAP_BLOCK_64,
					 BLOCK2_WISE_TRANSFER_SIZE_GET);
	} else {
		first = false;
	}

	id = coap_header_get_id(req);
	tkl = coap_header_get_token(req, token);

	r = coap_packet_init(rsp, data, COAP_BUF_SIZE, COAP_VERSION_1,
			     COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	zassert_equal(r, 0, "Unable to initialize request");

	r = coap_append_block2_option(rsp, rsp_ctx);
	zassert_equal(r, 0, "Unable to append block2 option");

	if (first) {
		r = coap_append_size2_option(rsp, rsp_ctx);
		zassert_equal(r, 0, "Unable to append size2 option");
	}

	r = coap_packet_append_payload_marker(rsp);
	zassert_equal(r, 0, "Unable to append payload marker");

	payload_len = rsp_ctx->total_size - rsp_ctx->current;
	if (payload_len > block_size) {
		payload_len = block_size;
	}

	r = coap_packet_append_payload(rsp, payload, payload_len);
	zassert_equal(r, 0, "Unable to append payload");

	*more = coap_next_block(rsp, rsp_ctx);
}

static void verify_block2_request(struct coap_block_context *req_ctx,
				 uint8_t iter)
{
	zassert_equal(req_ctx->block_size, COAP_BLOCK_64,
		      "req:%d,Couldn't get block size", iter);
	zassert_equal(req_ctx->current,
		      coap_block_size_to_bytes(COAP_BLOCK_64) * (iter - 1U),
		      "req:%d, Couldn't get the current block position", iter);
	zassert_equal(req_ctx->total_size, BLOCK2_WISE_TRANSFER_SIZE_GET,
		      "req:%d,Couldn't packet total size", iter);
}

static void verify_block2_response(struct coap_block_context *rsp_ctx,
				  uint8_t iter)
{
	int block_size = coap_block_size_to_bytes(COAP_BLOCK_64);
	int iter_max = ITER_COUNT(BLOCK2_WISE_TRANSFER_SIZE_GET, block_size);

	zassert_equal(rsp_ctx->block_size, COAP_BLOCK_64,
		      "rsp:%d,Couldn't get block size", iter);

	/* In last iteration "current" must match "total_size" */
	if (iter < iter_max) {
		zassert_equal(
			rsp_ctx->current, block_size * iter,
			"req:%d,Couldn't get the current block position", iter);
	} else {
		zassert_equal(
			rsp_ctx->current, rsp_ctx->total_size,
			"req:%d,Current block position does not match total size", iter);
	}

	zassert_equal(rsp_ctx->total_size, BLOCK2_WISE_TRANSFER_SIZE_GET,
		      "rsp:%d, Couldn't packet total size", iter);
}

ZTEST(coap, test_block2_size)
{
	struct coap_block_context req_ctx;
	struct coap_block_context rsp_ctx;
	struct coap_packet req;
	struct coap_packet rsp;
	int more;
	uint8_t i;

	i = 0U;
	more = 1;
	memset(&req_ctx, 0, sizeof(req_ctx));
	memset(&rsp_ctx, 0, sizeof(rsp_ctx));

	while (more) {
		prepare_block2_request(&req, &req_ctx, &rsp);
		prepare_block2_response(&rsp, &rsp_ctx, &req, &more);

		i++;

		verify_block2_request(&req_ctx, i);
		verify_block2_response(&rsp_ctx, i);
	}
}

ZTEST(coap, test_retransmit_second_round)
{
	struct coap_packet cpkt;
	struct coap_packet rsp;
	struct coap_pending *pending;
	struct coap_pending *rsp_pending;
	uint8_t *data = data_buf[0];
	uint8_t *rsp_data = data_buf[1];
	int r;
	uint16_t id;

	id = coap_next_id();

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1,
			     COAP_TYPE_CON, 0, coap_next_token(),
			     COAP_METHOD_GET, id);
	zassert_equal(r, 0, "Could not initialize packet");

	pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
	zassert_not_null(pending, "No free pending");

	r = coap_pending_init(pending, &cpkt, (struct sockaddr *) &dummy_addr,
			      CONFIG_COAP_MAX_RETRANSMIT);
	zassert_equal(r, 0, "Could not initialize packet");

	/* We "send" the packet the first time here */
	zassert_true(coap_pending_cycle(pending), "Pending expired too early");

	/* We simulate that the first transmission got lost */
	zassert_true(coap_pending_cycle(pending), "Pending expired too early");

	r = coap_packet_init(&rsp, rsp_data, COAP_BUF_SIZE,
			     COAP_VERSION_1, COAP_TYPE_ACK, 0, NULL,
			     COAP_METHOD_GET, id);
	zassert_equal(r, 0, "Could not initialize packet");

	/* Now we get the ack from the remote side */
	rsp_pending = coap_pending_received(&rsp, pendings, NUM_PENDINGS);
	zassert_equal_ptr(pending, rsp_pending,
			  "Invalid pending %p should be %p",
			  rsp_pending, pending);

	coap_pending_clear(rsp_pending);

	rsp_pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	zassert_is_null(rsp_pending, "There should be no active pendings");
}

static bool ipaddr_cmp(const struct sockaddr *a, const struct sockaddr *b)
{
	if (a->sa_family != b->sa_family) {
		return false;
	}

	if (a->sa_family == AF_INET6) {
		return net_ipv6_addr_cmp(&net_sin6(a)->sin6_addr,
					 &net_sin6(b)->sin6_addr);
	} else if (a->sa_family == AF_INET) {
		return net_ipv4_addr_cmp(&net_sin(a)->sin_addr,
					 &net_sin(b)->sin_addr);
	}

	return false;
}

static void server_notify_callback(struct coap_resource *resource,
				   struct coap_observer *observer)
{
	bool r;

	r = ipaddr_cmp(&observer->addr, (const struct sockaddr *)&dummy_addr);
	zassert_true(r, "The address of the observer doesn't match");

	coap_remove_observer(resource, observer);
}

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	struct coap_observer *observer;
	uint8_t *data = data_buf[1];
	char payload[] = "This is the payload";
	uint8_t token[8];
	uint8_t tkl;
	uint16_t id;
	int r;

	zassert_true(coap_request_is_observe(request),
		     "The request should enable observing");

	observer = coap_observer_next_unused(observers, NUM_OBSERVERS);
	zassert_not_null(observer, "There should be an available observer");

	tkl = coap_header_get_token(request, (uint8_t *) token);
	id = coap_header_get_id(request);

	coap_observer_init(observer, request, addr);
	coap_register_observer(resource, observer);

	r = coap_packet_init(&response, data, COAP_BUF_SIZE,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_OK, id);
	zassert_equal(r, 0, "Unable to initialize packet");

	r = coap_append_option_int(&response, COAP_OPTION_OBSERVE,
				   resource->age);
	zassert_equal(r, 0, "Failed to append observe option");

	r = coap_packet_append_payload_marker(&response);
	zassert_equal(r, 0, "Failed to set the payload marker");

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	zassert_equal(r, 0, "Unable to append payload");

	resource->user_data = data;

	return 0;
}

ZTEST(coap, test_observer_server)
{
	uint8_t valid_request_pdu[] = {
		0x45, 0x01, 0x12, 0x34,
		't', 'o', 'k', 'e', 'n',
		0x60, /* enable observe option */
		0x51, 's', 0x01, '1', /* path */
	};
	uint8_t not_found_request_pdu[] = {
		0x45, 0x01, 0x12, 0x34,
		't', 'o', 'k', 'e', 'n',
		0x60, /* enable observe option */
		0x51, 's', 0x01, '2', /* path */
	};
	struct coap_packet req;
	struct coap_option options[4] = {};
	uint8_t *data = data_buf[0];
	uint8_t opt_num = ARRAY_SIZE(options) - 1;
	int r;

	memcpy(data, valid_request_pdu, sizeof(valid_request_pdu));

	r = coap_packet_parse(&req, data, sizeof(valid_request_pdu),
			      options, opt_num);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	zassert_equal(r, 0, "Could not handle packet");

	/* Suppose some time passes */
	r = coap_resource_notify(&server_resources[0]);
	zassert_equal(r, 0, "Could not notify resource");

	memcpy(data, not_found_request_pdu, sizeof(not_found_request_pdu));

	r = coap_packet_parse(&req, data, sizeof(not_found_request_pdu),
			      options, opt_num);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	zassert_equal(r, -ENOENT,
		      "There should be no handler for this resource");
}

static int resource_reply_cb(const struct coap_packet *response,
			     struct coap_reply *reply,
			     const struct sockaddr *from)
{
	TC_PRINT("You should see this");

	return 0;
}

ZTEST(coap, test_observer_client)
{
	struct coap_packet req;
	struct coap_packet rsp;
	struct coap_reply *reply;
	struct coap_option options[4] = {};
	const char token[] = "token";
	const char * const *p;
	uint8_t *data = data_buf[0];
	uint8_t *rsp_data = data_buf[1];
	uint8_t opt_num = ARRAY_SIZE(options) - 1;
	int observe = 0;
	int r;

	r = coap_packet_init(&req, data, COAP_BUF_SIZE,
			     COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token,
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Unable to initialize request");

	/* Enable observing the resource. */
	r = coap_append_option_int(&req, COAP_OPTION_OBSERVE, observe);
	zassert_equal(r, 0, "Unable to add option to request int");

	for (p = server_resource_1_path; p && *p; p++) {
		r = coap_packet_append_option(&req, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		zassert_equal(r, 0, "Unable to add option to request");
	}

	reply = coap_reply_next_unused(replies, NUM_REPLIES);
	zassert_not_null(reply, "No resources for waiting for replies");

	coap_reply_init(reply, &req);
	reply->reply = resource_reply_cb;

	/* Server side, not interesting for this test */
	r = coap_packet_parse(&req, data, req.offset, options, opt_num);
	zassert_equal(r, 0, "Could not parse req packet");

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	zassert_equal(r, 0, "Could not handle packet");

	/* We cheat, and communicate using the resource's user_data */
	rsp_data = server_resources[0].user_data;

	/* 'rsp_pkt' contains the response now */

	r = coap_packet_parse(&rsp, rsp_data, req.offset, options, opt_num);
	zassert_equal(r, 0, "Could not parse rsp packet");

	reply = coap_response_received(&rsp,
				       (const struct sockaddr *) &dummy_addr,
				       replies, NUM_REPLIES);
	zassert_not_null(reply, "Couldn't find a matching waiting reply");
}

ZTEST(coap, test_handle_invalid_coap_req)
{
	struct coap_packet pkt;
	uint8_t *data = data_buf[0];
	struct coap_option options[4] = {};
	uint8_t opt_num = 4;
	int r;
	const char *const *p;

	r = coap_packet_init(&pkt, data, COAP_BUF_SIZE, COAP_VERSION_1,
					COAP_TYPE_CON, 0, NULL, 0xFF, coap_next_id());

	zassert_equal(r, 0, "Unable to init req");

	for (p = server_resource_1_path; p && *p; p++) {
		r = coap_packet_append_option(&pkt, COAP_OPTION_URI_PATH,
					*p, strlen(*p));
		zassert_equal(r, 0, "Unable to append option");
	}

	r = coap_packet_parse(&pkt, data, pkt.offset, options, opt_num);
	zassert_equal(r, 0, "Could not parse req packet");

	r = coap_handle_request(&pkt, server_resources, options, opt_num,
					(struct sockaddr *) &dummy_addr, sizeof(dummy_addr));
	zassert_equal(r, -ENOTSUP, "Request handling should fail with -ENOTSUP");
}

ZTEST(coap, test_build_options_out_of_order_0)
{
	uint8_t result[] = {0x45, 0x02, 0x12, 0x34, 't', 'o', 'k',  'e', 'n', 0xC0, 0xB1, 0x19,
			    0xC5, 'p',	'r',  'o',  'x', 'y', 0x44, 'c', 'o', 'a',  'p'};
	struct coap_packet cpkt;
	static const char token[] = "token";
	uint8_t *data = data_buf[0];
	int r;

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_options_0 = 0xc0; /* content format */

	zassert_mem_equal(&expected_options_0, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	const char *proxy_uri = "proxy";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_PROXY_URI, proxy_uri, strlen(proxy_uri));
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_options_1[] = {
		0xc0,				    /* content format */
		0xd5, 0x0a, 'p', 'r', 'o', 'x', 'y' /* proxy url */
	};
	zassert_mem_equal(expected_options_1, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	const char *proxy_scheme = "coap";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_PROXY_SCHEME, proxy_scheme,
				      strlen(proxy_scheme));
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_options_2[] = {
		0xc0,				     /*  content format */
		0xd5, 0x0a, 'p', 'r', 'o', 'x', 'y', /*  proxy url */
		0x44, 'c',  'o', 'a', 'p'	     /*  proxy scheme */
	};
	zassert_mem_equal(expected_options_2, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	/*  option out of order */
	const uint8_t block_option = 0b11001;

	r = coap_append_option_int(&cpkt, COAP_OPTION_BLOCK2, block_option);
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_options_3[] = {
		0xc0,				/*  content format */
		0xb1, 0x19,			/*  block2 */
		0xc5, 'p',  'r', 'o', 'x', 'y', /*  proxy url */
		0x44, 'c',  'o', 'a', 'p'	/*  proxy scheme */
	};
	zassert_mem_equal(expected_options_3, &cpkt.data[cpkt.hdr_len], cpkt.opt_len);

	/*  look for options */
	struct coap_option opt;

	r = coap_find_options(&cpkt, COAP_OPTION_CONTENT_FORMAT, &opt, 1);
	zassert_equal(r, 1, "Could not find option");

	r = coap_find_options(&cpkt, COAP_OPTION_PROXY_URI, &opt, 1);
	zassert_equal(r, 1, "Could not find option");
	zassert_equal(opt.len, strlen(proxy_uri), "Wrong option len");
	zassert_mem_equal(opt.value, proxy_uri, strlen(proxy_uri), "Wrong option content");

	r = coap_find_options(&cpkt, COAP_OPTION_PROXY_SCHEME, &opt, 1);
	zassert_equal(r, 1, "Could not find option");
	zassert_equal(opt.len, strlen(proxy_scheme), "Wrong option len");
	zassert_mem_equal(opt.value, proxy_scheme, strlen(proxy_scheme), "Wrong option content");

	r = coap_find_options(&cpkt, COAP_OPTION_BLOCK2, &opt, 1);
	zassert_equal(r, 1, "Could not find option");
	zassert_equal(opt.len, 1, "Wrong option len");
	zassert_equal(*opt.value, block_option, "Wrong option content");

	zassert_equal(cpkt.hdr_len, 9, "Wrong header len");
	zassert_equal(cpkt.opt_len, 14, "Wrong options size");
	zassert_equal(cpkt.delta, 39, "Wrong delta");

	zassert_equal(cpkt.offset, 23, "Wrong data size");

	zassert_mem_equal(result, cpkt.data, cpkt.offset,
			  "Built packet doesn't match reference packet");
}

#define ASSERT_OPTIONS(cpkt, expected_opt_len, expected_data, expected_data_len)                   \
	do {                                                                                       \
		static const uint8_t expected_hdr_len = 9;                                         \
		zassert_equal(expected_hdr_len, cpkt.hdr_len, "Wrong header length");              \
		zassert_equal(expected_opt_len, cpkt.opt_len, "Wrong option length");              \
		zassert_equal(expected_hdr_len + expected_opt_len, cpkt.offset, "Wrong offset");   \
		zassert_equal(expected_data_len, cpkt.offset, "Wrong offset");                     \
		zassert_mem_equal(expected_data, cpkt.data, expected_data_len, "Wrong data");      \
	} while (0)

ZTEST(coap, test_build_options_out_of_order_1)
{
	struct coap_packet cpkt;

	static const char token[] = "token";

	uint8_t *data = data_buf[0];

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	int r;

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE2,
				   coap_block_size_to_bytes(COAP_BLOCK_128));
	zassert_equal(r, 0, "Could not append option");
	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 't',  'o',
					     'k',  'e',	 'n',  0xd1, 0x0f, 0x80};
	ASSERT_OPTIONS(cpkt, 3, expected_0, 12);

	const char *uri_path = "path";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_1[] = {
		0x45, 0x02, 0x12, 0x34, 't', 'o',  'k',	 'e',  'n',
		0xb4, 'p',  'a',  't',	'h', 0xd1, 0x04, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 8, expected_1, 17);

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_2[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0xb4,
		'p',  'a',  't',  'h',	0x11, 0x32, 0xd1, 0x03, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 10, expected_2, 19);

	const char *uri_host = "hostname";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_HOST, uri_host, strlen(uri_host));
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_3[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o', 'k', 'e', 'n', 0x38, 'h',  'o',  's',  't',
		'n',  'a',  'm',  'e',	0x84, 'p', 'a', 't', 'h', 0x11, 0x32, 0xd1, 0x03, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 19, expected_3, 28);

	r = coap_append_option_int(&cpkt, COAP_OPTION_URI_PORT, 5638);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_4[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',
		'o',  's',  't',  'n',	'a',  'm',  'e',  'B',	0x16, 0x06, 'D',
		'p',  'a',  't',  'h',	0x11, 0x32, 0xd1, 0x03, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 22, expected_4, 31);

	const char *uri_query0 = "query0";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_QUERY, uri_query0, strlen(uri_query0));
	zassert_equal(r, 0, "Could not append option");

	const char *uri_query1 = "query1";

	r = coap_packet_append_option(&cpkt, COAP_OPTION_URI_QUERY, uri_query1, strlen(uri_query1));
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_5[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',
		's',  't',  'n',  'a',	'm',  'e',  'B',  0x16, 0x06, 'D',  'p',  'a',
		't',  'h',  0x11, 0x32, 0x36, 'q',  'u',  'e',	'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0xd1, 0x00, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 36, expected_5, 45);

	r = coap_append_option_int(&cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_CBOR);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_6[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',
		's',  't',  'n',  'a',	'm',  'e',  'B',  0x16, 0x06, 'D',  'p',  'a',
		't',  'h',  0x11, 0x32, 0x36, 'q',  'u',  'e',	'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 37, expected_6, 46);

	r = coap_append_option_int(&cpkt, COAP_OPTION_OBSERVE, 0);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_7[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',
		's',  't',  'n',  'a',	'm',  'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p',
		'a',  't',  'h',  0x11, 0x32, 0x36, 'q',  'u',	'e',  'r',  'y',  0x30,
		0x06, 'q',  'u',  'e',	'r',  'y',  0x31, 0x21, 0x3c, 0xb1, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 38, expected_7, 47);

	r = coap_append_option_int(&cpkt, COAP_OPTION_MAX_AGE, 3);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_8[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h', 'o',  's',
		't',  'n',  'a',  'm',	'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p', 'a',  't',
		'h',  0x11, 0x32, 0x21, 0x03, 0x16, 'q',  'u',	'e',  'r',  'y', 0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80,
	};

	ASSERT_OPTIONS(cpkt, 40, expected_8, 49);

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE1, 64);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_9[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',	's',
		't',  'n',  'a',  'm',	'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p',  'a',	't',
		'h',  0x11, 0x32, 0x21, 0x03, 0x16, 'q',  'u',	'e',  'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40,
	};

	ASSERT_OPTIONS(cpkt, 43, expected_9, 52);

	zassert_equal(cpkt.hdr_len, 9, "Wrong header len");
	zassert_equal(cpkt.opt_len, 43, "Wrong options size");
	zassert_equal(cpkt.delta, 60, "Wrong delta");
	zassert_equal(cpkt.offset, 52, "Wrong data size");
}

ZTEST_SUITE(coap, NULL, NULL, NULL, NULL, NULL);
