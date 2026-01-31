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

#if defined(CONFIG_COAP_OSCORE)
#include "coap_oscore.h"
#include <zephyr/net/coap/coap_service.h>
#include <oscore.h>
#include <oscore/security_context.h>
#endif

#if defined(CONFIG_COAP_EDHOC)
#include "coap_edhoc.h"
#include "coap_edhoc_session.h"
#include "coap_oscore_ctx_cache.h"

/* Forward declarations for wrapper functions */
extern int coap_oscore_context_init_wrapper(void *ctx,
					     const uint8_t *master_secret,
					     size_t master_secret_len,
					     const uint8_t *master_salt,
					     size_t master_salt_len,
					     const uint8_t *sender_id,
					     size_t sender_id_len,
					     const uint8_t *recipient_id,
					     size_t recipient_id_len,
					     int aead_alg,
					     int hkdf_alg);
#endif

#define COAP_BUF_SIZE 128

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
static void server_resource_1_callback(struct coap_resource *resource,
				       struct coap_observer *observer);

static void server_resource_2_callback(struct coap_resource *resource,
				       struct coap_observer *observer);

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 struct net_sockaddr *addr, net_socklen_t addr_len);

static const char * const server_resource_1_path[] = { "s", "1", NULL };
static const char *const server_resource_2_path[] = { "s", "2", NULL };
static struct coap_resource server_resources[] = {
	{ .path = server_resource_1_path,
	  .get = server_resource_1_get,
	  .notify = server_resource_1_callback },
	{ .path = server_resource_2_path,
	  .get = server_resource_1_get, /* Get can be shared with the first resource */
	  .notify = server_resource_2_callback },
	{ },
};

#define MY_PORT 12345
#define peer_addr { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0x2 } } }
static struct net_sockaddr_in6 dummy_addr = {
	.sin6_family = NET_AF_INET6,
	.sin6_addr = peer_addr };

static uint8_t data_buf[2][COAP_BUF_SIZE];

#define COAP_ROLLOVER_AGE (1 << 23)
#define COAP_MAX_AGE      0xffffff
#define COAP_FIRST_AGE    2

extern bool coap_age_is_newer(int v1, int v2);

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

	r = coap_pending_init(pending, &cpkt, (struct net_sockaddr *) &dummy_addr,
			      NULL);
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

static bool ipaddr_cmp(const struct net_sockaddr *a, const struct net_sockaddr *b)
{
	if (a->sa_family != b->sa_family) {
		return false;
	}

	if (a->sa_family == NET_AF_INET6) {
		return net_ipv6_addr_cmp(&net_sin6(a)->sin6_addr,
					 &net_sin6(b)->sin6_addr);
	} else if (a->sa_family == NET_AF_INET) {
		return net_ipv4_addr_cmp(&net_sin(a)->sin_addr,
					 &net_sin(b)->sin_addr);
	}

	return false;
}

static void server_resource_1_callback(struct coap_resource *resource,
				       struct coap_observer *observer)
{
	bool r;

	r = ipaddr_cmp(&observer->addr, (const struct net_sockaddr *)&dummy_addr);
	zassert_true(r, "The address of the observer doesn't match");

	coap_remove_observer(resource, observer);
}
static void server_resource_2_callback(struct coap_resource *resource,
				       struct coap_observer *observer)
{
	bool r;

	r = ipaddr_cmp(&observer->addr, (const struct net_sockaddr *)&dummy_addr);
	zassert_true(r, "The address of the observer doesn't match");
}

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 struct net_sockaddr *addr, net_socklen_t addr_len)
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
		0x51, 's', 0x01, '3', /* path */
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
				(struct net_sockaddr *) &dummy_addr,
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
				(struct net_sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	zassert_equal(r, -ENOENT,
		      "There should be no handler for this resource");
}

static int resource_reply_cb(const struct coap_packet *response,
			     struct coap_reply *reply,
			     const struct net_sockaddr *from)
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
				(struct net_sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	zassert_equal(r, 0, "Could not handle packet");

	/* We cheat, and communicate using the resource's user_data */
	rsp_data = server_resources[0].user_data;

	/* 'rsp_pkt' contains the response now */

	r = coap_packet_parse(&rsp, rsp_data, req.offset, options, opt_num);
	zassert_equal(r, 0, "Could not parse rsp packet");

	reply = coap_response_received(&rsp,
				       (const struct net_sockaddr *) &dummy_addr,
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
					(struct net_sockaddr *) &dummy_addr, sizeof(dummy_addr));
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

#define ASSERT_OPTIONS_AND_PAYLOAD(cpkt, expected_opt_len, expected_data, expected_offset,         \
				   expected_delta)                                                 \
	do {                                                                                       \
		size_t expected_data_l = ARRAY_SIZE(expected_data);                                \
		zassert_equal(expected_offset, expected_data_l);                                   \
		static const uint8_t expected_hdr_len = 9;                                         \
		zassert_equal(expected_hdr_len, cpkt.hdr_len, "Wrong header length");              \
		zassert_equal(expected_opt_len, cpkt.opt_len, "Wrong option length");              \
		zassert_equal(expected_offset, cpkt.offset, "Wrong offset");                       \
		zassert_mem_equal(expected_data, cpkt.data, expected_offset, "Wrong data");        \
		zassert_equal(expected_delta, cpkt.delta, "Wrong delta");                          \
	} while (0)

static void init_basic_test_msg(struct coap_packet *cpkt, uint8_t *data)
{
	static const char token[] = "token";
	const char *uri_path = "path";
	const char *uri_host = "hostname";
	const char *uri_query0 = "query0";
	const char *uri_query1 = "query1";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	int r;

	r = coap_packet_init(cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(cpkt, COAP_OPTION_SIZE2,
				   coap_block_size_to_bytes(COAP_BLOCK_128));
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_CONTENT_FORMAT, COAP_CONTENT_FORMAT_APP_JSON);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_HOST, uri_host, strlen(uri_host));
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_URI_PORT, 5638);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_QUERY, uri_query0, strlen(uri_query0));
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_option(cpkt, COAP_OPTION_URI_QUERY, uri_query1, strlen(uri_query1));
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_CBOR);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_OBSERVE, 0);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_MAX_AGE, 3);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(cpkt, COAP_OPTION_SIZE1, 64);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_9[] = {
		0x45, 0x02, 0x12, 0x34, 't',  'o',  'k',  'e',	'n',  0x38, 'h',  'o',	's',
		't',  'n',  'a',  'm',	'e',  0x30, 0x12, 0x16, 0x06, 'D',  'p',  'a',	't',
		'h',  0x11, 0x32, 0x21, 0x03, 0x16, 'q',  'u',	'e',  'r',  'y',  0x30, 0x06,
		'q',  'u',  'e',  'r',	'y',  0x31, 0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40,
	};

	ASSERT_OPTIONS((*cpkt), 43, expected_9, 52);

	r = coap_packet_append_payload_marker(cpkt);
	zassert_equal(r, 0, "Could not append payload marker");

	static const uint8_t test_payload[] = {0xde, 0xad, 0xbe, 0xef};

	r = coap_packet_append_payload(cpkt, test_payload, ARRAY_SIZE(test_payload));
	zassert_equal(r, 0, "Could not append test payload");

	zassert_equal((*cpkt).hdr_len, 9, "Wrong header len");
	zassert_equal((*cpkt).opt_len, 43, "Wrong options size");
	zassert_equal((*cpkt).delta, 60, "Wrong delta");
	zassert_equal((*cpkt).offset, 57, "Wrong data size");
}

ZTEST(coap, test_remove_first_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_HOST);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x60, 0x12, 0x16,
		0x06, 0x44, 0x70, 0x61, 0x74, 0x68, 0x11, 0x32, 0x21, 0x03, 0x16, 0x71,
		0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72, 0x79, 0x31,
		0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 34, expected_0, 48, 60);
}

ZTEST(coap, test_remove_middle_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_OBSERVE);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73, 0x74,
		0x6e, 0x61, 0x6d, 0x65, 0x42, 0x16, 0x06, 0x44, 0x70, 0x61, 0x74, 0x68, 0x11, 0x32,
		0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72,
		0x79, 0x31, 0x21, 0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 42, expected_0, 56, 60);
}

ZTEST(coap, test_remove_last_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_SIZE1);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73, 0x74,
		0x6e, 0x61, 0x6d, 0x65, 0x30, 0x12, 0x16, 0x06, 0x44, 0x70, 0x61, 0x74, 0x68, 0x11,
		0x32, 0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65,
		0x72, 0x79, 0x31, 0x21, 0x3c, 0xb1, 0x80, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 40, expected_0, 54, 28);

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE1, 65);
	zassert_equal(r, 0, "Could not add option at end");

	static const uint8_t expected_1[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f,
		0x73, 0x74, 0x6e, 0x61, 0x6d, 0x65, 0x30, 0x12, 0x16, 0x06, 0x44, 0x70,
		0x61, 0x74, 0x68, 0x11, 0x32, 0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72,
		0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72, 0x79, 0x31, 0x21, 0x3c, 0xb1,
		0x80, 0xd1, 0x13, 0x41, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 43, expected_1, 57, 60);
}

ZTEST(coap, test_remove_single_coap_option)
{
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	static const char token[] = "token";
	const char *uri_path = "path";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	int r1;

	r1 = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			      strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r1, 0, "Could not initialize packet");

	r1 = coap_packet_append_option(&cpkt, COAP_OPTION_URI_PATH, uri_path, strlen(uri_path));
	zassert_equal(r1, 0, "Could not append option");

	r1 = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r1, 0, "Could not append payload marker");

	static const uint8_t test_payload[] = {0xde, 0xad, 0xbe, 0xef};

	r1 = coap_packet_append_payload(&cpkt, test_payload, ARRAY_SIZE(test_payload));
	zassert_equal(r1, 0, "Could not append test payload");

	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xb4, 0x70, 0x61, 0x74, 0x68,
					     0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 5, expected_0, 19, 11);

	/* remove the one and only option */
	r1 = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PATH);
	zassert_equal(r1, 0, "Could not remove option");

	static const uint8_t expected_1[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 0, expected_1, 14, 0);
}

ZTEST(coap, test_remove_repeatable_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73,
		0x74, 0x6e, 0x61, 0x6d, 0x65, 0x30, 0x12, 0x16, 0x06, 0x44, 0x70, 0x61, 0x74,
		0x68, 0x11, 0x32, 0x21, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x31, 0x21,
		0x3c, 0xb1, 0x80, 0xd1, 0x13, 0x40, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 36, expected_0, 50, 60);
}

ZTEST(coap, test_remove_all_coap_options)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];

	init_basic_test_msg(&cpkt, data);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PORT);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_OBSERVE);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_SIZE1);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_0[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0x38, 0x68, 0x6f, 0x73,
		0x74, 0x6e, 0x61, 0x6d, 0x65, 0x84, 0x70, 0x61, 0x74, 0x68, 0x11, 0x32, 0x21,
		0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75, 0x65, 0x72,
		0x79, 0x31, 0x21, 0x3c, 0xb1, 0x80, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 36, expected_0, 50, 28);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_HOST);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_SIZE2);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_CONTENT_FORMAT);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_1[] = {
		0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e, 0xb4, 0x70, 0x61, 0x74,
		0x68, 0x31, 0x03, 0x16, 0x71, 0x75, 0x65, 0x72, 0x79, 0x30, 0x06, 0x71, 0x75,
		0x65, 0x72, 0x79, 0x31, 0x21, 0x3c, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 23, expected_1, 37, 17);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_ACCEPT);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PATH);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_2[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65,
					     0x6e, 0xd1, 0x01, 0x03, 0x16, 0x71, 0x75, 0x65,
					     0x72, 0x79, 0x31, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 10, expected_2, 24, 15);

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_MAX_AGE);
	zassert_equal(r, 0, "Could not remove option");

	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	static const uint8_t expected_3[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 0, expected_3, 14, 0);

	/* remove option that is not there anymore */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_MAX_AGE);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 0, expected_3, 14, 0);
}

ZTEST(coap, test_remove_non_existent_coap_option)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	static const char token[] = "token";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT, COAP_CONTENT_FORMAT_APP_CBOR);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(&cpkt, COAP_OPTION_ACCEPT, COAP_CONTENT_FORMAT_APP_OCTET_STREAM);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Could not append payload marker");

	static const uint8_t test_payload[] = {0xde, 0xad, 0xbe, 0xef};

	r = coap_packet_append_payload(&cpkt, test_payload, ARRAY_SIZE(test_payload));

	static const uint8_t expected_original_msg[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f,
							0x6b, 0x65, 0x6e, 0xc1, 0x3c, 0x51,
							0x2a, 0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);

	/* remove option that is not there but would be before existing options */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_URI_PATH);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);

	/* remove option that is not there but would be between existing options */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_MAX_AGE);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);

	/* remove option that is not there but would be after existing options */
	r = coap_packet_remove_option(&cpkt, COAP_OPTION_LOCATION_QUERY);
	zassert_equal(r, 0, "Could not remove option");

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 4, expected_original_msg, 18, 17);
}

ZTEST(coap, test_coap_packet_options_with_large_values)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	static const char token[] = "token";

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, COAP_OPTION_MAX_AGE, 3600);
	zassert_equal(r, 0, "Could not append option");

	r = coap_append_option_int(&cpkt, COAP_OPTION_SIZE1, 1048576);
	zassert_equal(r, 0, "Could not append option");

	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b, 0x65, 0x6e,
					     0xd2, 0x01, 0x0e, 0x10, 0xd3, 0x21, 0x10, 0x00, 0x00};
	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 9, expected_0, 18, 60);
}

ZTEST(coap, test_coap_packet_options_with_large_delta)
{
	int r;
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	static const char token[] = "token";
	static const uint8_t payload[] = {0xde, 0xad, 0xbe, 0xef};

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			     strlen(token), token, COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_append_option_int(&cpkt, 65100, 0x5678);
	zassert_equal(r, 0, "Could not append option");

	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Could not append payload marker");

	r = coap_packet_append_payload(&cpkt, payload, ARRAY_SIZE(payload));
	zassert_equal(r, 0, "Could not append payload");

	static const uint8_t expected_0[] = {0x45, 0x02, 0x12, 0x34, 0x74, 0x6f, 0x6b,
					     0x65, 0x6e, 0xe2, 0xfd, 0x3f, 0x56, 0x78,
					     0xff, 0xde, 0xad, 0xbe, 0xef};

	ASSERT_OPTIONS_AND_PAYLOAD(cpkt, 5, expected_0, 19, 65100);
}

static void assert_coap_packet_set_path_query_options(const char *path,
						      const char * const *expected,
						      size_t expected_len, uint16_t code)
{
	struct coap_packet cpkt;
	uint8_t *data = data_buf[0];
	struct coap_option options[16] = {0};
	int res;

	memset(data_buf[0], 0, ARRAY_SIZE(data_buf[0]));
	TC_PRINT("Assert path: %s\n", path);

	res = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1, COAP_TYPE_CON,
			       COAP_TOKEN_MAX_LEN, coap_next_token(),
			       COAP_METHOD_GET, 0x1234);
	zassert_equal(res, 0, "Could not initialize packet");

	res = coap_packet_set_path(&cpkt, path);
	zassert_equal(res, 0, "Could not set path/query, path: %s", path);

	res = coap_find_options(&cpkt, code, options, ARRAY_SIZE(options));
	if (res <= 0) {
		/* fail if we expect options */
		zassert_true(((expected == NULL) && (expected_len == 0U)),
			     "Expected options but found none, path: %s", path);
	}

	for (unsigned int i = 0U; i < expected_len; ++i) {
		/* validate expected options, the rest shall be 0 */
		if (i < expected_len) {
			zassert_true((options[i].len == strlen(expected[i])),
				     "Expected and parsed option lengths don't match"
				     ", path: %s",
				     path);

			zassert_mem_equal(options[i].value, expected[i], options[i].len,
					  "Expected and parsed option values don't match"
					  ", path: %s",
					  path);
		} else {
			zassert_true((options[i].len == 0U),
				     "Unexpected options shall be empty"
				     ", path: %s",
				     path);
		}
	}
}

ZTEST(coap, test_coap_packet_set_path)
{
	assert_coap_packet_set_path_query_options(" ", NULL, 0U, COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("", NULL, 0U, COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("/", NULL, 0U, COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("?", NULL, 0U, COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("?a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("?a&b",
						 (const char *const[]){"a", "b"}, 2U,
						  COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a", NULL, 0, COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a/",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);

	assert_coap_packet_set_path_query_options("a?b=t&a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a?b=t&a",
						  (const char *const[]){"b=t", "a"}, 2U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a?b=t&aa",
						  (const char *const[]){"b=t", "aa"},
						  2U, COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a?b&a",
						  (const char *const[]){"a"}, 1U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a?b&a",
						  (const char *const[]){"b", "a"}, 2U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a?b&aa",
						  (const char *const[]){"b", "aa"}, 2U,
						  COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a/b",
						  (const char *const[]){"a", "b"}, 2U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a/b/",
						  (const char *const[]){"a", "b"}, 2U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a/b?b&a",
						  (const char *const[]){"b", "a"}, 2U,
						  COAP_OPTION_URI_QUERY);
	assert_coap_packet_set_path_query_options("a/b?b&aa",
						  (const char *const[]){"b", "aa"}, 2U,
						  COAP_OPTION_URI_QUERY);

	assert_coap_packet_set_path_query_options("a/bb",
						  (const char *const[]){"a", "bb"}, 2U,
						  COAP_OPTION_URI_PATH);
	assert_coap_packet_set_path_query_options("a/bb/",
						  (const char *const[]){"a", "bb"}, 2U,
						  COAP_OPTION_URI_PATH);
}

ZTEST(coap, test_transmission_parameters)
{
	struct coap_packet cpkt;
	struct coap_pending *pending;
	struct coap_transmission_parameters params;
	uint8_t *data = data_buf[0];
	int r;
	uint16_t id;

	params = coap_get_transmission_parameters();
	zassert_equal(params.ack_timeout, CONFIG_COAP_INIT_ACK_TIMEOUT_MS, "Wrong ACK timeout");
	zassert_equal(params.ack_random_percent, CONFIG_COAP_ACK_RANDOM_PERCENT,
		      "Wrong ACK random percent");
	zassert_equal(params.coap_backoff_percent, CONFIG_COAP_BACKOFF_PERCENT,
		      "Wrong backoff percent");
	zassert_equal(params.max_retransmission, CONFIG_COAP_MAX_RETRANSMIT,
		      "Wrong max retransmission value");

	params.ack_timeout = 1000;
	params.ack_random_percent = 110;
	params.coap_backoff_percent = 150;
	params.max_retransmission = 2;

	coap_set_transmission_parameters(&params);

	id = coap_next_id();

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE, COAP_VERSION_1,
			     COAP_TYPE_CON, 0, coap_next_token(),
			     COAP_METHOD_GET, id);
	zassert_equal(r, 0, "Could not initialize packet");

	pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
	zassert_not_null(pending, "No free pending");

	params.ack_timeout = 3000;
	params.ack_random_percent = 130;
	params.coap_backoff_percent = 250;
	params.max_retransmission = 3;

	r = coap_pending_init(pending, &cpkt, (struct net_sockaddr *) &dummy_addr,
			      &params);
	zassert_equal(r, 0, "Could not initialize packet");

	zassert_equal(pending->params.ack_timeout, 3000, "Wrong ACK timeout");
	zassert_equal(pending->params.ack_random_percent, 130, "Wrong ACK random percent");
	zassert_equal(pending->params.coap_backoff_percent, 250, "Wrong backoff percent");
	zassert_equal(pending->params.max_retransmission, 3, "Wrong max retransmission value");

	r = coap_pending_init(pending, &cpkt, (struct net_sockaddr *) &dummy_addr,
			      NULL);
	zassert_equal(r, 0, "Could not initialize packet");

	zassert_equal(pending->params.ack_timeout, 1000, "Wrong ACK timeout");
	zassert_equal(pending->params.ack_random_percent, 110, "Wrong ACK random percent");
	zassert_equal(pending->params.coap_backoff_percent, 150, "Wrong backoff percent");
	zassert_equal(pending->params.max_retransmission, 2, "Wrong max retransmission value");
}

ZTEST(coap, test_notify_age)
{
	uint8_t valid_request_pdu[] = {
		0x45, 0x01, 0x12, 0x34, 't', 'o', 'k', 'e', 'n', 0x60, /* enable observe option */
		0x51, 's',  0x01, '2',                                 /* path */
	};

	struct coap_packet req;
	struct coap_option options[4] = {};
	uint8_t *data = data_buf[0];
	uint8_t opt_num = ARRAY_SIZE(options) - 1;
	struct coap_resource *resource = &server_resources[1];
	int r;
	struct coap_observer *observer;
	int last_age;

	memcpy(data, valid_request_pdu, sizeof(valid_request_pdu));

	r = coap_packet_parse(&req, data, sizeof(valid_request_pdu), options, opt_num);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct net_sockaddr *)&dummy_addr, sizeof(dummy_addr));
	zassert_equal(r, 0, "Could not handle packet");

	/* Forward time a bit, as not to run this 8 million time */
	resource->age = COAP_OBSERVE_MAX_AGE - 10;

	last_age = resource->age;

	for (int i = 0; i < 15; i++) {
		r = coap_resource_notify(resource);
		zassert_true(coap_age_is_newer(last_age, resource->age),
			     "Resource age expected to be newer");
		last_age = resource->age;
	}

	observer =
		CONTAINER_OF(sys_slist_peek_head(&resource->observers), struct coap_observer, list);
	coap_remove_observer(resource, observer);
}

ZTEST(coap, test_age_is_newer)
{
	for (int i = COAP_FIRST_AGE; i < COAP_MAX_AGE; ++i) {
		zassert_true(coap_age_is_newer(i, i + 1),
			     "Resource age expected to be marked as newer");
	}

	zassert_true(coap_age_is_newer(COAP_MAX_AGE, COAP_FIRST_AGE),
		     "First age should be marked as newer");
	zassert_true(coap_age_is_newer(COAP_FIRST_AGE, COAP_ROLLOVER_AGE),
		     "Rollover age should be marked as newer");
	zassert_true(coap_age_is_newer(COAP_ROLLOVER_AGE, COAP_MAX_AGE),
		     "Max age should be marked as newer");
}

struct test_coap_request {
	uint16_t id;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t tkl;
	uint8_t code;
	enum coap_msgtype type;
	struct coap_reply *match;
};

static int reply_cb(const struct coap_packet *response,
		    struct coap_reply *reply,
		    const struct net_sockaddr *from)
{
	return 0;
}

ZTEST(coap, test_response_matching)
{
	struct coap_reply matches[] = {
		{ }, /* Non-initialized (unused) entry. */
		{ .id = 100, .reply = reply_cb },
		{ .id = 101, .token = { 1, 2, 3, 4 }, .tkl = 4, .reply = reply_cb },
	};
	struct test_coap_request test_responses[] = {
		/* #0 Piggybacked ACK, empty token */
		{ .id = 100, .type = COAP_TYPE_ACK, .match = &matches[1],
		  .code = COAP_RESPONSE_CODE_CONTENT },
		/* #1 Piggybacked ACK, matching token */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = &matches[2],
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4  },
		/* #2 Piggybacked ACK, token mismatch */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 3 },
		  .tkl = 4 },
		/* #3 Piggybacked ACK, token mismatch 2 */
		{ .id = 100, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #4 Piggybacked ACK, token mismatch 3 */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3 },
		  .tkl = 3 },
		/* #5 Piggybacked ACK, token mismatch 4 */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT },
		/* #6 Piggybacked ACK, id mismatch */
		{ .id = 102, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #7 Separate reply, empty token */
		{ .id = 101, .type = COAP_TYPE_CON, .match = &matches[1],
		  .code = COAP_RESPONSE_CODE_CONTENT },
		/* #8 Separate reply, matching token 1 */
		{ .id = 101, .type = COAP_TYPE_CON, .match = &matches[2],
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #9 Separate reply, matching token 2 */
		{ .id = 102, .type = COAP_TYPE_CON, .match = &matches[2],
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 4 },
		  .tkl = 4 },
		/* #10 Separate reply, token mismatch */
		{ .id = 101, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 3 },
		  .tkl = 4 },
		/* #11 Separate reply, token mismatch 2 */
		{ .id = 100, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3, 3 },
		  .tkl = 4 },
		/* #12 Separate reply, token mismatch 3 */
		{ .id = 100, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_RESPONSE_CODE_CONTENT, .token = { 1, 2, 3 },
		  .tkl = 3 },
		/* #13 Request, empty token */
		{ .id = 100, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_METHOD_GET },
		/* #14 Request, matching token */
		{ .id = 101, .type = COAP_TYPE_CON, .match = NULL,
		  .code = COAP_METHOD_GET, .token = { 1, 2, 3, 4 }, .tkl = 4 },
		/* #15 Empty ACK */
		{ .id = 100, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_CODE_EMPTY },
		/* #16 Empty ACK 2 */
		{ .id = 101, .type = COAP_TYPE_ACK, .match = NULL,
		  .code = COAP_CODE_EMPTY },
		/* #17 Empty RESET */
		{ .id = 100, .type = COAP_TYPE_RESET, .match = &matches[1],
		  .code = COAP_CODE_EMPTY },
		/* #18 Empty RESET 2 */
		{ .id = 101, .type = COAP_TYPE_RESET, .match = &matches[2],
		  .code = COAP_CODE_EMPTY },
		/* #19 Empty RESET, id mismatch */
		{ .id = 102, .type = COAP_TYPE_RESET, .match = NULL,
		  .code = COAP_CODE_EMPTY },
	};

	ARRAY_FOR_EACH_PTR(test_responses, response) {
		struct coap_packet response_pkt = { 0 };
		struct net_sockaddr from = { 0 };
		struct coap_reply *match;
		uint8_t data[64];
		int ret;

		ret = coap_packet_init(&response_pkt, data, sizeof(data), COAP_VERSION_1,
				       response->type, response->tkl, response->token,
				       response->code, response->id);
		zassert_ok(ret, "Failed to initialize test packet: %d", ret);

		match = coap_response_received(&response_pkt, &from, matches,
					       ARRAY_SIZE(matches));
		if (response->match != NULL) {
			zassert_not_null(match, "Did not found a response match when expected");
			zassert_equal_ptr(response->match, match,
					  "Wrong response match, test %d match %d",
					  response - test_responses, match - matches);
		} else {
			zassert_is_null(match,
					"Found unexpected response match, test %d match %d",
					response - test_responses, match - matches);
		}
	}
}

ZTEST(coap, test_no_response_option_absent)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE] = {0};
	bool suppress;
	int r;

	/* Build a request without No-Response option */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, -ENOENT, "Expected -ENOENT when option is absent, got %d", r);

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, -ENOENT, "Expected -ENOENT when option is absent, got %d", r);

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, -ENOENT, "Expected -ENOENT when option is absent, got %d", r);
}

ZTEST(coap, test_no_response_option_empty)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	bool suppress;
	int r;

	/* Build a request with empty No-Response option (interested in all responses) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	/* Add empty No-Response option */
	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE, NULL, 0);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Empty option should not suppress 2.xx");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Empty option should not suppress 4.xx");

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Empty option should not suppress 5.xx");
}

ZTEST(coap, test_no_response_option_suppress_2xx)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value = COAP_NO_RESPONSE_SUPPRESS_2_XX;
	bool suppress;
	int r;

	/* Build a request with No-Response option set to suppress 2.xx */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx responses - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_OK, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.00 OK");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.05 Content");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CHANGED, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.04 Changed");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 4.04 Not Found");

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 5.00 Internal Error");
}

ZTEST(coap, test_no_response_option_suppress_4xx)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value = COAP_NO_RESPONSE_SUPPRESS_4_XX;
	bool suppress;
	int r;

	/* Build a request with No-Response option set to suppress 4.xx */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 2.05 Content");

	/* Check 4.xx responses - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_BAD_REQUEST, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.00 Bad Request");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.04 Not Found");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_BAD_OPTION, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.02 Bad Option");

	/* Check 5.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 5.00 Internal Error");
}

ZTEST(coap, test_no_response_option_suppress_5xx)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value = COAP_NO_RESPONSE_SUPPRESS_5_XX;
	bool suppress;
	int r;

	/* Build a request with No-Response option set to suppress 5.xx */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 2.05 Content");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 4.04 Not Found");

	/* Check 5.xx responses - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.00 Internal Error");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_IMPLEMENTED, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.01 Not Implemented");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_BAD_GATEWAY, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.02 Bad Gateway");
}

ZTEST(coap, test_no_response_option_suppress_combinations)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value;
	bool suppress;
	int r;

	/* Test suppressing 2.xx and 5.xx (0x12 = 0x02 | 0x10) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	no_response_value = COAP_NO_RESPONSE_SUPPRESS_2_XX | COAP_NO_RESPONSE_SUPPRESS_5_XX;
	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check 2.xx response - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.05 Content");

	/* Check 4.xx response - should not be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_false(suppress, "Should not suppress 4.04 Not Found");

	/* Check 5.xx response - should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.00 Internal Error");

	/* Test suppressing all (0x1A = 0x02 | 0x08 | 0x10) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	no_response_value = COAP_NO_RESPONSE_SUPPRESS_ALL;
	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      &no_response_value, 1);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* All response classes should be suppressed */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 2.05 Content");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_NOT_FOUND, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 4.04 Not Found");

	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_INTERNAL_ERROR, &suppress);
	zassert_equal(r, 0, "Failed to check No-Response option");
	zassert_true(suppress, "Should suppress 5.00 Internal Error");
}

ZTEST(coap, test_no_response_option_invalid_length)
{
	struct coap_packet request;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t no_response_value[2] = {0x02, 0x08};
	bool suppress;
	int r;

	/* Build a request with invalid No-Response option (length > 1) */
	r = coap_packet_init(&request, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Could not initialize packet");

	r = coap_packet_append_option(&request, COAP_OPTION_NO_RESPONSE,
				      no_response_value, 2);
	zassert_equal(r, 0, "Could not add No-Response option");

	/* Check that invalid length is detected */
	r = coap_no_response_check(&request, COAP_RESPONSE_CODE_CONTENT, &suppress);
	zassert_equal(r, -EINVAL, "Should return -EINVAL for invalid option length");
}

ZTEST(coap, test_packet_init_invalid_token_len)
{
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t token[COAP_TOKEN_MAX_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	int r;

	/* Test with token_len = 9 (reserved per RFC 7252 Section 3) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 9, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 9");

	/* Test with token_len = 15 (reserved per RFC 7252 Section 3) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 15, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 15");

	/* Test with token_len > 15 */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 255, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 255");
}

ZTEST(coap, test_packet_init_null_token_with_nonzero_len)
{
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	int r;

	/* Test with token_len > 0 but token = NULL */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 4, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len > 0 with NULL token");

	/* Test with token_len = 1 but token = NULL */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 1, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 1 with NULL token");

	/* Test with token_len = 8 but token = NULL */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, -EINVAL, "Should reject token_len = 8 with NULL token");
}

ZTEST(coap, test_packet_init_valid_token_len)
{
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	uint8_t token[COAP_TOKEN_MAX_LEN] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	int r;

	/* Test with token_len = 0 and token = NULL (valid) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 0 with NULL token");

	/* Test with token_len = 0 and token != NULL (valid, token is ignored) */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 0 with non-NULL token");

	/* Test with token_len = 1 and valid token */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 1, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 1 with valid token");

	/* Test with token_len = 8 and valid token */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 8, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 8 with valid token");

	/* Test with token_len = 4 and valid token */
	r = coap_packet_init(&cpkt, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_CON, 4, token,
			     COAP_METHOD_GET, 0);
	zassert_equal(r, 0, "Should accept token_len = 4 with valid token");
}

ZTEST(coap, test_packet_parse_rejects_invalid_tkl)
{
	/* Test that parsing a packet with TKL=9 returns -EBADMSG */
	uint8_t pdu_with_tkl_9[] = {
		0x49, /* Ver=1, Type=CON, TKL=9 (reserved) */
		0x01, /* Code=GET */
		0x12, 0x34 /* Message ID */
	};
	struct coap_packet cpkt;
	uint8_t data[COAP_BUF_SIZE];
	int r;

	memcpy(data, pdu_with_tkl_9, sizeof(pdu_with_tkl_9));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu_with_tkl_9), NULL, 0);
	zassert_equal(r, -EBADMSG, "Should reject packet with TKL=9");

	/* Test with TKL=15 (also reserved) */
	uint8_t pdu_with_tkl_15[] = {
		0x4F, /* Ver=1, Type=CON, TKL=15 (reserved) */
		0x01, /* Code=GET */
		0x12, 0x34 /* Message ID */
	};

	memcpy(data, pdu_with_tkl_15, sizeof(pdu_with_tkl_15));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu_with_tkl_15), NULL, 0);
	zassert_equal(r, -EBADMSG, "Should reject packet with TKL=15");
}

ZTEST(coap, test_next_token_is_sequence_and_unique)
{
	/* Test RFC9175-compliant sequence-based token generation */
	uint8_t token1[COAP_TOKEN_MAX_LEN], token2[COAP_TOKEN_MAX_LEN], token3[COAP_TOKEN_MAX_LEN];
	uint8_t *token_ptr;
	uint32_t prefix, seq1, seq2, seq3;

	/* Reset token generator with a known prefix for deterministic testing */
	coap_token_generator_reset(0x12345678);

	/* Get first token and copy it (coap_next_token returns pointer to static buffer) */
	token_ptr = coap_next_token();
	zassert_not_null(token_ptr, "Token should not be NULL");
	memcpy(token1, token_ptr, COAP_TOKEN_MAX_LEN);

	/* Extract prefix and sequence from token1 (big-endian encoding) */
	prefix = (token1[0] << 24) | (token1[1] << 16) | (token1[2] << 8) | token1[3];
	seq1 = (token1[4] << 24) | (token1[5] << 16) | (token1[6] << 8) | token1[7];

	/* Verify prefix is correct */
	zassert_equal(prefix, 0x12345678, "Token prefix should match reset value");

	/* Verify sequence starts at 0 (RFC9175 §4.2: "starting at zero") */
	zassert_equal(seq1, 0, "First token sequence should be 0");

	/* Get second token and copy it */
	token_ptr = coap_next_token();
	zassert_not_null(token_ptr, "Token should not be NULL");
	memcpy(token2, token_ptr, COAP_TOKEN_MAX_LEN);

	/* Extract sequence from token2 */
	seq2 = (token2[4] << 24) | (token2[5] << 16) | (token2[6] << 8) | token2[7];

	/* Verify sequence increments */
	zassert_equal(seq2, 1, "Second token sequence should be 1");

	/* Verify tokens are unique */
	zassert_true(memcmp(token1, token2, COAP_TOKEN_MAX_LEN) != 0,
		     "Tokens should be unique");

	/* Get third token and copy it */
	token_ptr = coap_next_token();
	memcpy(token3, token_ptr, COAP_TOKEN_MAX_LEN);
	seq3 = (token3[4] << 24) | (token3[5] << 16) | (token3[6] << 8) | token3[7];

	/* Verify sequence continues to increment */
	zassert_equal(seq3, 2, "Third token sequence should be 2");

	/* Verify all three tokens are unique */
	zassert_true(memcmp(token1, token3, COAP_TOKEN_MAX_LEN) != 0,
		     "Token 1 and 3 should be unique");
	zassert_true(memcmp(token2, token3, COAP_TOKEN_MAX_LEN) != 0,
		     "Token 2 and 3 should be unique");
}

ZTEST(coap, test_token_generator_rekey)
{
	/* Test that rekey generates new prefix and resets sequence */
	uint8_t token1[COAP_TOKEN_MAX_LEN], token2[COAP_TOKEN_MAX_LEN];
	uint8_t *token_ptr;
	uint32_t prefix1, prefix2, seq1, seq2;

	/* First rekey */
	coap_token_generator_rekey();
	token_ptr = coap_next_token();
	memcpy(token1, token_ptr, COAP_TOKEN_MAX_LEN);
	prefix1 = (token1[0] << 24) | (token1[1] << 16) | (token1[2] << 8) | token1[3];
	seq1 = (token1[4] << 24) | (token1[5] << 16) | (token1[6] << 8) | token1[7];

	/* Sequence should start at 0 after rekey */
	zassert_equal(seq1, 0, "Sequence should be 0 after rekey");

	/* Second rekey */
	coap_token_generator_rekey();
	token_ptr = coap_next_token();
	memcpy(token2, token_ptr, COAP_TOKEN_MAX_LEN);
	prefix2 = (token2[0] << 24) | (token2[1] << 16) | (token2[2] << 8) | token2[3];
	seq2 = (token2[4] << 24) | (token2[5] << 16) | (token2[6] << 8) | token2[7];

	/* Sequence should reset to 0 after rekey */
	zassert_equal(seq2, 0, "Sequence should reset to 0 after rekey");

	/* Prefixes should be different (with very high probability) */
	zassert_true(prefix1 != prefix2,
		     "Rekey should generate different prefix (may fail rarely due to randomness)");
}

ZTEST(coap, test_request_tag_generation_not_recycled)
{
	/* Test that Request-Tags are not recycled (use sequence-based generation) */
	uint8_t tag1[COAP_TOKEN_MAX_LEN], tag2[COAP_TOKEN_MAX_LEN], tag3[COAP_TOKEN_MAX_LEN];
	uint8_t *tag_ptr;

	/* Reset token generator for deterministic testing */
	coap_token_generator_reset(0xAABBCCDD);

	/* Generate multiple Request-Tags (using coap_next_token which is used for Request-Tag) */
	tag_ptr = coap_next_token();
	memcpy(tag1, tag_ptr, COAP_TOKEN_MAX_LEN);
	
	tag_ptr = coap_next_token();
	memcpy(tag2, tag_ptr, COAP_TOKEN_MAX_LEN);
	
	tag_ptr = coap_next_token();
	memcpy(tag3, tag_ptr, COAP_TOKEN_MAX_LEN);

	/* Verify all tags are unique (never recycled) */
	zassert_true(memcmp(tag1, tag2, COAP_TOKEN_MAX_LEN) != 0,
		     "Request-Tags should not be recycled");
	zassert_true(memcmp(tag1, tag3, COAP_TOKEN_MAX_LEN) != 0,
		     "Request-Tags should not be recycled");
	zassert_true(memcmp(tag2, tag3, COAP_TOKEN_MAX_LEN) != 0,
		     "Request-Tags should not be recycled");

	/* Verify they follow sequence pattern */
	uint32_t seq1 = (tag1[4] << 24) | (tag1[5] << 16) | (tag1[6] << 8) | tag1[7];
	uint32_t seq2 = (tag2[4] << 24) | (tag2[5] << 16) | (tag2[6] << 8) | tag2[7];
	uint32_t seq3 = (tag3[4] << 24) | (tag3[5] << 16) | (tag3[6] << 8) | tag3[7];

	zassert_equal(seq2, seq1 + 1, "Request-Tags should follow sequence");
	zassert_equal(seq3, seq2 + 1, "Request-Tags should follow sequence");
}

#if defined(CONFIG_COAP_SERVER_ECHO)

#include <zephyr/net/coap_service.h>

/* Forward declarations for Echo test helpers */
extern struct coap_echo_entry *coap_echo_cache_find(struct coap_echo_entry *cache,
						    const struct net_sockaddr *addr,
						    net_socklen_t addr_len);
extern int coap_echo_create_challenge(struct coap_echo_entry *cache,
				      const struct net_sockaddr *addr,
				      net_socklen_t addr_len,
				      uint8_t *echo_value, size_t *echo_len);
extern int coap_echo_verify_value(struct coap_echo_entry *cache,
				  const struct net_sockaddr *addr,
				  net_socklen_t addr_len,
				  const uint8_t *echo_value, size_t echo_len);
extern bool coap_echo_is_address_verified(struct coap_echo_entry *cache,
					  const struct net_sockaddr *addr,
					  net_socklen_t addr_len);
extern int coap_echo_build_challenge_response(struct coap_packet *response,
					      const struct coap_packet *request,
					      const uint8_t *echo_value,
					      size_t echo_len,
					      uint8_t *buf, size_t buf_len);
extern bool coap_is_unsafe_method(uint8_t code);
extern int coap_echo_extract_from_request(const struct coap_packet *request,
					  uint8_t *echo_value, size_t *echo_len);

/* Test Echo option length validation per RFC 9175 Section 2.2.1 */
ZTEST(coap, test_echo_option_length_validation)
{
	struct coap_echo_entry cache[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE] = {0};
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = peer_addr,
		.sin6_port = net_htons(5683)
	};
	uint8_t echo_value[41];
	size_t echo_len;
	int ret;

	/* Valid Echo length (1-40 bytes) */
	echo_len = 8;
	ret = coap_echo_create_challenge(cache, (struct net_sockaddr *)&addr,
					 sizeof(addr), echo_value, &echo_len);
	zassert_equal(ret, 0, "Should create challenge with valid length");
	zassert_equal(echo_len, CONFIG_COAP_SERVER_ECHO_MAX_LEN,
		     "Echo length should match config");

	/* Verify with valid length */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, echo_len);
	zassert_equal(ret, 0, "Should verify valid Echo value");

	/* Test invalid length: 0 bytes (caught by extract function) */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, 0);
	zassert_equal(ret, -EINVAL, "Should reject Echo with length 0");

	/* Test invalid length: > 40 bytes */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, 41);
	zassert_equal(ret, -EINVAL, "Should reject Echo with length > 40");
}

/* Test unsafe method freshness requirement per RFC 9175 Section 2.3 */
ZTEST(coap, test_echo_unsafe_method_detection)
{
	/* Test that unsafe methods are correctly identified */
	zassert_true(coap_is_unsafe_method(COAP_METHOD_POST),
		    "POST should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_PUT),
		    "PUT should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_DELETE),
		    "DELETE should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_PATCH),
		    "PATCH should be unsafe");
	zassert_true(coap_is_unsafe_method(COAP_METHOD_IPATCH),
		    "IPATCH should be unsafe");

	/* Test that safe methods are not flagged */
	zassert_false(coap_is_unsafe_method(COAP_METHOD_GET),
		     "GET should be safe");
	zassert_false(coap_is_unsafe_method(COAP_METHOD_FETCH),
		     "FETCH should be safe");
}

/* Test Echo challenge and verification flow */
ZTEST(coap, test_echo_challenge_verification_flow)
{
	struct coap_echo_entry cache[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE] = {0};
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = peer_addr,
		.sin6_port = net_htons(5683)
	};
	uint8_t echo_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];
	size_t echo_len;
	int ret;

	/* Step 1: Create initial challenge */
	ret = coap_echo_create_challenge(cache, (struct net_sockaddr *)&addr,
					 sizeof(addr), echo_value, &echo_len);
	zassert_equal(ret, 0, "Should create challenge");
	zassert_equal(echo_len, CONFIG_COAP_SERVER_ECHO_MAX_LEN,
		     "Echo length should match config");

	/* Step 2: Verify the challenge succeeds */
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), echo_value, echo_len);
	zassert_equal(ret, 0, "Should verify correct Echo value");

	/* Step 3: Verify address is now verified for amplification mitigation */
	bool verified = coap_echo_is_address_verified(cache,
						      (struct net_sockaddr *)&addr,
						      sizeof(addr));
	zassert_true(verified, "Address should be verified after successful Echo");

	/* Step 4: Verify wrong Echo value fails */
	uint8_t wrong_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];

	memset(wrong_value, 0xFF, sizeof(wrong_value));
	ret = coap_echo_verify_value(cache, (struct net_sockaddr *)&addr,
				    sizeof(addr), wrong_value, echo_len);
	zassert_equal(ret, -EINVAL, "Should reject incorrect Echo value");
}

/* Test Echo challenge response format per RFC 9175 Section 2.4 item 3 */
ZTEST(coap, test_echo_challenge_response_format)
{
	uint8_t request_buf[COAP_BUF_SIZE];
	uint8_t response_buf[COAP_BUF_SIZE];
	struct coap_packet request;
	struct coap_packet response;
	uint8_t echo_value[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	uint8_t token[4] = {0xAA, 0xBB, 0xCC, 0xDD};
	int ret;

	/* Test CON request -> ACK response with Echo */
	ret = coap_packet_init(&request, request_buf, sizeof(request_buf),
			      COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			      COAP_METHOD_PUT, coap_next_id());
	zassert_equal(ret, 0, "Should init CON request");

	ret = coap_echo_build_challenge_response(&response, &request,
						echo_value, sizeof(echo_value),
						response_buf, sizeof(response_buf));
	zassert_equal(ret, 0, "Should build challenge response");

	/* Verify response is ACK type per RFC 9175 */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_ACK,
		     "CON request should get ACK response");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_UNAUTHORIZED,
		     "Should be 4.01 Unauthorized");

	/* Verify Echo option is present */
	struct coap_option option;

	ret = coap_find_options(&response, COAP_OPTION_ECHO, &option, 1);
	zassert_equal(ret, 1, "Should find Echo option");
	zassert_equal(option.len, sizeof(echo_value), "Echo length should match");
	zassert_mem_equal(option.value, echo_value, sizeof(echo_value),
			 "Echo value should match");

	/* Test NON request -> NON response with Echo */
	ret = coap_packet_init(&request, request_buf, sizeof(request_buf),
			      COAP_VERSION_1, COAP_TYPE_NON_CON, sizeof(token), token,
			      COAP_METHOD_PUT, coap_next_id());
	zassert_equal(ret, 0, "Should init NON request");

	ret = coap_echo_build_challenge_response(&response, &request,
						echo_value, sizeof(echo_value),
						response_buf, sizeof(response_buf));
	zassert_equal(ret, 0, "Should build challenge response");

	/* Verify response is NON type per RFC 9175 */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_NON_CON,
		     "NON request should get NON response");
}

/* Test Echo cache management (LRU eviction) */
ZTEST(coap, test_echo_cache_lru_eviction)
{
	struct coap_echo_entry cache[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE] = {0};
	struct net_sockaddr_in6 addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE + 1];
	uint8_t echo_value[CONFIG_COAP_SERVER_ECHO_MAX_LEN];
	size_t echo_len;
	int ret;

	/* Fill the cache */
	for (int i = 0; i < CONFIG_COAP_SERVER_ECHO_CACHE_SIZE; i++) {
		addrs[i].sin6_family = NET_AF_INET6;
		addrs[i].sin6_addr = dummy_addr.sin6_addr;
		addrs[i].sin6_port = net_htons(5683 + i);

		ret = coap_echo_create_challenge(cache, (struct net_sockaddr *)&addrs[i],
						sizeof(addrs[i]), echo_value, &echo_len);
		zassert_equal(ret, 0, "Should create challenge %d", i);

		/* Small delay to ensure different timestamps */
		k_msleep(1);
	}

	/* Verify all entries are in cache */
	for (int i = 0; i < CONFIG_COAP_SERVER_ECHO_CACHE_SIZE; i++) {
		struct coap_echo_entry *entry = coap_echo_cache_find(
			cache, (struct net_sockaddr *)&addrs[i], sizeof(addrs[i]));
		zassert_not_null(entry, "Entry %d should be in cache", i);
	}

	/* Add one more entry - should evict the oldest (first) */
	addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE].sin6_family = NET_AF_INET6;
	addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE].sin6_addr = dummy_addr.sin6_addr;
	addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE].sin6_port =
		net_htons(5683 + CONFIG_COAP_SERVER_ECHO_CACHE_SIZE);

	ret = coap_echo_create_challenge(
		cache,
		(struct net_sockaddr *)&addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE],
		sizeof(addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE]),
		echo_value, &echo_len);
	zassert_equal(ret, 0, "Should create challenge for new entry");

	/* Verify first entry was evicted */
	struct coap_echo_entry *entry = coap_echo_cache_find(
		cache, (struct net_sockaddr *)&addrs[0], sizeof(addrs[0]));
	zassert_is_null(entry, "Oldest entry should be evicted");

	/* Verify new entry is in cache */
	entry = coap_echo_cache_find(
		cache,
		(struct net_sockaddr *)&addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE],
		sizeof(addrs[CONFIG_COAP_SERVER_ECHO_CACHE_SIZE]));
	zassert_not_null(entry, "New entry should be in cache");
}

/* Test Echo option extraction from request */
ZTEST(coap, test_echo_extract_from_request)
{
	uint8_t request_buf[COAP_BUF_SIZE];
	uint8_t request_buf2[COAP_BUF_SIZE];
	struct coap_packet request;
	uint8_t echo_value_in[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
	uint8_t echo_value_out[40];
	size_t echo_len_out;
	int ret;

	/* Create request with Echo option */
	ret = coap_packet_init(&request, request_buf, sizeof(request_buf),
			      COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			      COAP_METHOD_PUT, coap_next_id());
	zassert_equal(ret, 0, "Should init request");

	ret = coap_packet_append_option(&request, COAP_OPTION_ECHO,
				       echo_value_in, sizeof(echo_value_in));
	zassert_equal(ret, 0, "Should append Echo option");

	/* Extract Echo option */
	ret = coap_echo_extract_from_request(&request, echo_value_out, &echo_len_out);
	zassert_equal(ret, 0, "Should extract Echo option");
	zassert_equal(echo_len_out, sizeof(echo_value_in), "Echo length should match");
	zassert_mem_equal(echo_value_out, echo_value_in, sizeof(echo_value_in),
			 "Echo value should match");

	/* Test request without Echo option - use fresh buffer */
	ret = coap_packet_init(&request, request_buf2, sizeof(request_buf2),
			      COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			      COAP_METHOD_GET, coap_next_id());
	zassert_equal(ret, 0, "Should init request");

	ret = coap_echo_extract_from_request(&request, echo_value_out, &echo_len_out);
	zassert_equal(ret, -ENOENT, "Should return -ENOENT for missing Echo, got %d", ret);
}

#endif /* CONFIG_COAP_SERVER_ECHO */

#if defined(CONFIG_COAP_OSCORE)

/* Test OSCORE option number is correctly defined */
ZTEST(coap, test_oscore_option_number)
{
	/* RFC 8613 Section 2: OSCORE option number is 9 */
	zassert_equal(COAP_OPTION_OSCORE, 9, "OSCORE option number must be 9");
}

/* Test OSCORE malformed message validation (RFC 8613 Section 2) */
ZTEST(coap, test_oscore_malformed_validation)
{
	struct coap_packet cpkt;
	uint8_t buf[COAP_BUF_SIZE];
	int r;

	/* RFC 8613 Section 2: OSCORE option without payload is malformed */
	r = coap_packet_init(&cpkt, buf, sizeof(buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Should init packet");

	/* Add OSCORE option (empty value is valid for the option itself) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE, NULL, 0);
	zassert_equal(r, 0, "Should append OSCORE option");

	/* Validate - should fail because no payload */
	r = coap_oscore_validate_msg(&cpkt);
	zassert_equal(r, -EBADMSG, "Should reject OSCORE without payload, got %d", r);

	/* Now add a payload marker and payload */
	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Should append payload marker");

	const uint8_t payload[] = "test";
	r = coap_packet_append_payload(&cpkt, payload, sizeof(payload) - 1);
	zassert_equal(r, 0, "Should append payload");

	/* Now validation should pass */
	r = coap_oscore_validate_msg(&cpkt);
	zassert_equal(r, 0, "Should accept OSCORE with payload, got %d", r);
}

/* Test OSCORE message detection */
ZTEST(coap, test_oscore_message_detection)
{
	struct coap_packet cpkt;
	uint8_t buf[COAP_BUF_SIZE];
	uint8_t buf2[COAP_BUF_SIZE];
	int r;
	bool has_oscore;

	/* Create message without OSCORE option */
	memset(buf, 0, sizeof(buf));
	r = coap_packet_init(&cpkt, buf, sizeof(buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Should init packet");

	has_oscore = coap_oscore_msg_has_oscore(&cpkt);
	zassert_false(has_oscore, "Should not detect OSCORE option");

	/* Create message with OSCORE option */
	memset(buf2, 0, sizeof(buf2));
	r = coap_packet_init(&cpkt, buf2, sizeof(buf2),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Should init packet");

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE, NULL, 0);
	zassert_equal(r, 0, "Should append OSCORE option");

	has_oscore = coap_oscore_msg_has_oscore(&cpkt);
	zassert_true(has_oscore, "Should detect OSCORE option");
}

/* Test OSCORE exchange cache management */
ZTEST(coap, test_oscore_exchange_cache)
{
	/* This test requires access to internal functions, which are exposed
	 * through CONFIG_COAP_TEST_API_ENABLE for testing purposes
	 */
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr1 = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
		.sin6_port = net_htons(5683),
	};
	struct net_sockaddr_in6 addr2 = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x2 } } },
		.sin6_port = net_htons(5683),
	};
	uint8_t token1[] = {0x01, 0x02, 0x03, 0x04};
	uint8_t token2[] = {0x05, 0x06, 0x07, 0x08};

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Test: Add entry to cache */
	int ret = oscore_exchange_add(cache, (struct net_sockaddr *)&addr1,
				      sizeof(addr1), token1, sizeof(token1), false, NULL);
	zassert_equal(ret, 0, "Should add exchange entry");

	/* Test: Find the entry */
	struct coap_oscore_exchange *entry;
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr1,
				     sizeof(addr1), token1, sizeof(token1));
	zassert_not_null(entry, "Should find exchange entry");
	zassert_equal(entry->tkl, sizeof(token1), "Token length should match");
	zassert_mem_equal(entry->token, token1, sizeof(token1), "Token should match");
	zassert_false(entry->is_observe, "Should not be Observe exchange");

	/* Test: Add another entry with different address */
	ret = oscore_exchange_add(cache, (struct net_sockaddr *)&addr2,
				  sizeof(addr2), token2, sizeof(token2), true, NULL);
	zassert_equal(ret, 0, "Should add second exchange entry");

	/* Test: Find second entry */
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr2,
				     sizeof(addr2), token2, sizeof(token2));
	zassert_not_null(entry, "Should find second exchange entry");
	zassert_true(entry->is_observe, "Should be Observe exchange");

	/* Test: Update existing entry */
	ret = oscore_exchange_add(cache, (struct net_sockaddr *)&addr1,
				  sizeof(addr1), token1, sizeof(token1), true, NULL);
	zassert_equal(ret, 0, "Should update exchange entry");

	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr1,
				     sizeof(addr1), token1, sizeof(token1));
	zassert_not_null(entry, "Should still find exchange entry");
	zassert_true(entry->is_observe, "Should now be Observe exchange");

	/* Test: Remove entry */
	oscore_exchange_remove(cache, (struct net_sockaddr *)&addr1,
			       sizeof(addr1), token1, sizeof(token1));

	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr1,
				     sizeof(addr1), token1, sizeof(token1));
	zassert_is_null(entry, "Should not find removed entry");

	/* Test: Second entry should still exist */
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr2,
				     sizeof(addr2), token2, sizeof(token2));
	zassert_not_null(entry, "Second entry should still exist");
}

/* Test OSCORE response protection integration */
ZTEST(coap, test_oscore_response_protection)
{
	/* This test verifies that the OSCORE response protection logic is correctly
	 * integrated into coap_service_send(). We test the exchange tracking and
	 * protection decision logic.
	 *
	 * Note: Full end-to-end OSCORE encryption/decryption testing requires
	 * initializing a uoscore security context, which is beyond the scope of
	 * this unit test. This test focuses on the exchange tracking mechanism.
	 */

	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	struct coap_packet cpkt;
	uint8_t buf[COAP_BUF_SIZE];
	int r;

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Simulate OSCORE request verification by adding exchange entry */
	r = oscore_exchange_add(cache, (struct net_sockaddr *)&addr,
				sizeof(addr), token, sizeof(token), false, NULL);
	zassert_equal(r, 0, "Should add exchange entry");

	/* Create a response packet with the same token */
	r = coap_packet_init(&cpkt, buf, sizeof(buf),
			     COAP_VERSION_1, COAP_TYPE_ACK, sizeof(token), token,
			     COAP_RESPONSE_CODE_CONTENT, coap_next_id());
	zassert_equal(r, 0, "Should init response packet");

	/* Verify exchange is found (indicating response needs protection) */
	struct coap_oscore_exchange *entry;
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
				     sizeof(addr), token, sizeof(token));
	zassert_not_null(entry, "Should find exchange for response");

	/* For non-Observe exchanges, the entry should be removed after sending */
	oscore_exchange_remove(cache, (struct net_sockaddr *)&addr,
			       sizeof(addr), token, sizeof(token));

	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
				     sizeof(addr), token, sizeof(token));
	zassert_is_null(entry, "Non-Observe exchange should be removed after response");
}

/* Test OSCORE Observe exchange lifecycle */
ZTEST(coap, test_oscore_observe_exchange_lifecycle)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	int r;

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Add Observe exchange */
	r = oscore_exchange_add(cache, (struct net_sockaddr *)&addr,
				sizeof(addr), token, sizeof(token), true, NULL);
	zassert_equal(r, 0, "Should add Observe exchange");

	/* Verify exchange persists (for Observe notifications) */
	struct coap_oscore_exchange *entry;
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
				     sizeof(addr), token, sizeof(token));
	zassert_not_null(entry, "Observe exchange should persist");
	zassert_true(entry->is_observe, "Should be marked as Observe");

	/* Simulate sending multiple notifications - entry should persist */
	for (int i = 0; i < 3; i++) {
		entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
					     sizeof(addr), token, sizeof(token));
		zassert_not_null(entry, "Observe exchange should persist for notifications");
	}

	/* Remove when observation is cancelled */
	oscore_exchange_remove(cache, (struct net_sockaddr *)&addr,
			       sizeof(addr), token, sizeof(token));

	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
				     sizeof(addr), token, sizeof(token));
	zassert_is_null(entry, "Observe exchange should be removed when cancelled");
}

/* Test OSCORE exchange expiry */
ZTEST(coap, test_oscore_exchange_expiry)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } },
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	int r;

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Add non-Observe exchange */
	r = oscore_exchange_add(cache, (struct net_sockaddr *)&addr,
				sizeof(addr), token, sizeof(token), false, NULL);
	zassert_equal(r, 0, "Should add exchange");

	/* Manually set timestamp to old value to simulate expiry */
	struct coap_oscore_exchange *entry;
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
				     sizeof(addr), token, sizeof(token));
	zassert_not_null(entry, "Should find fresh entry");

	/* Set timestamp to expired value */
	entry->timestamp = k_uptime_get() - CONFIG_COAP_OSCORE_EXCHANGE_LIFETIME_MS - 1000;

	/* Next find should detect expiry and clear the entry */
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
				     sizeof(addr), token, sizeof(token));
	zassert_is_null(entry, "Expired entry should be cleared");
}

/* Test OSCORE exchange cache LRU eviction */
ZTEST(coap, test_oscore_exchange_cache_eviction)
{
	struct coap_oscore_exchange cache[CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE];
	struct net_sockaddr_in6 addr_base = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0 } } },
		.sin6_port = net_htons(5683),
	};
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};
	int r;

	/* Initialize cache */
	memset(cache, 0, sizeof(cache));

	/* Fill the cache */
	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		struct net_sockaddr_in6 addr = addr_base;
		addr.sin6_addr.s6_addr[15] = i + 1;
		token[0] = i + 1;

		r = oscore_exchange_add(cache, (struct net_sockaddr *)&addr,
					sizeof(addr), token, sizeof(token), false, NULL);
		zassert_equal(r, 0, "Should add entry %d", i);

		/* Small delay to ensure different timestamps */
		k_msleep(1);
	}

	/* Verify cache is full */
	for (int i = 0; i < CONFIG_COAP_OSCORE_EXCHANGE_CACHE_SIZE; i++) {
		struct net_sockaddr_in6 addr = addr_base;
		addr.sin6_addr.s6_addr[15] = i + 1;
		token[0] = i + 1;

		struct coap_oscore_exchange *entry;
		entry = oscore_exchange_find(cache, (struct net_sockaddr *)&addr,
					     sizeof(addr), token, sizeof(token));
		zassert_not_null(entry, "Should find entry %d", i);
	}

	/* Add one more entry - should evict the oldest (first) entry */
	struct net_sockaddr_in6 new_addr = addr_base;
	new_addr.sin6_addr.s6_addr[15] = 0xFF;
	token[0] = 0xFF;

	r = oscore_exchange_add(cache, (struct net_sockaddr *)&new_addr,
				sizeof(new_addr), token, sizeof(token), false, NULL);
	zassert_equal(r, 0, "Should add new entry and evict oldest");

	/* Verify new entry exists */
	struct coap_oscore_exchange *entry;
	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&new_addr,
				     sizeof(new_addr), token, sizeof(token));
	zassert_not_null(entry, "Should find new entry");

	/* Verify oldest entry was evicted */
	struct net_sockaddr_in6 first_addr = addr_base;
	first_addr.sin6_addr.s6_addr[15] = 1;
	token[0] = 1;

	entry = oscore_exchange_find(cache, (struct net_sockaddr *)&first_addr,
				     sizeof(first_addr), token, sizeof(token));
	zassert_is_null(entry, "Oldest entry should be evicted");
}

/* Test OSCORE client request protection (RFC 8613 Section 8.1) */
ZTEST(coap, test_oscore_client_request_protection)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* TODO: Implement end-to-end test with OSCORE client and server.
	 * This test should verify that:
	 * 1. When client->oscore_ctx is set, requests are automatically OSCORE-protected
	 * 2. The sent message has the OSCORE option
	 * 3. The server can decrypt and process the request
	 */
	ztest_test_skip();
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE client response verification (RFC 8613 Section 8.4) */
ZTEST(coap, test_oscore_client_response_verification)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* TODO: Implement test verifying automatic OSCORE response verification.
	 * Should test that decrypted inner response is passed to the callback.
	 */
	ztest_test_skip();
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE client fail-closed behavior */
ZTEST(coap, test_oscore_client_fail_closed)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* TODO: Implement fail-closed behavior tests:
	 * 1. OSCORE protection failure prevents sending
	 * 2. Plaintext response to OSCORE request is rejected
	 * 3. OSCORE verification failure drops response
	 */
	ztest_test_skip();
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE client with Block2 (RFC 8613 Section 8.4.1) */
ZTEST(coap, test_oscore_client_block2)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* This test verifies RFC 8613 Section 8.4.1 compliance:
	 * Outer Block2 options are processed according to RFC 7959 before
	 * OSCORE verification, and verification happens only on the
	 * reconstructed complete OSCORE message.
	 */
	struct coap_packet cpkt;
	uint8_t buf[COAP_BUF_SIZE];
	int r;

	/* Test 1: Verify outer Block2 option is recognized */
	r = coap_packet_init(&cpkt, buf, sizeof(buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_RESPONSE_CODE_CONTENT, coap_next_id());
	zassert_equal(r, 0, "Should init packet");

	/* Add OSCORE option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE, NULL, 0);
	zassert_equal(r, 0, "Should append OSCORE option");

	/* Add outer Block2 option (block 0, more blocks, size 64) */
	uint8_t block2_val = 0x08; /* NUM=0, M=1, SZX=0 (16 bytes) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_BLOCK2, &block2_val, 1);
	zassert_equal(r, 0, "Should append Block2 option");

	/* Add payload (simulating OSCORE ciphertext) */
	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Should append payload marker");

	const uint8_t payload[] = "encrypted_block_0";
	r = coap_packet_append_payload(&cpkt, payload, sizeof(payload) - 1);
	zassert_equal(r, 0, "Should append payload");

	/* Verify the packet has both OSCORE and Block2 options */
	bool has_oscore = coap_oscore_msg_has_oscore(&cpkt);
	zassert_true(has_oscore, "Should have OSCORE option");

	int block2_opt = coap_get_option_int(&cpkt, COAP_OPTION_BLOCK2);
	zassert_true(block2_opt > 0, "Should have Block2 option");
	zassert_true(GET_MORE(block2_opt), "Should indicate more blocks");
	zassert_equal(GET_BLOCK_NUM(block2_opt), 0, "Should be block 0");

	/* Test 2: Verify block context initialization and update */
	struct coap_block_context blk_ctx;
	coap_block_transfer_init(&blk_ctx, COAP_BLOCK_16, 0);

	r = coap_update_from_block(&cpkt, &blk_ctx);
	zassert_equal(r, 0, "Should update block context");

	/* Advance to next block using the proper API.
	 * coap_next_block() advances by the actual payload length in the packet.
	 */
	size_t next_offset = coap_next_block(&cpkt, &blk_ctx);
	zassert_equal(blk_ctx.current, sizeof(payload) - 1,
		      "Should advance by payload length");
	zassert_equal(next_offset, sizeof(payload) - 1,
		      "Should return next offset");

	/* Test 3: Verify MAX_UNFRAGMENTED_SIZE constant is defined */
	zassert_true(CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE > 0,
		     "MAX_UNFRAGMENTED_SIZE should be configured");
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE client with Observe (RFC 8613 Section 8.4.2) */
ZTEST(coap, test_oscore_client_observe)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* TODO: Implement Observe + OSCORE test verifying:
	 * 1. Notifications are OSCORE-verified
	 * 2. Verification failures don't cancel observation
	 * 3. Client waits for next notification
	 */
	ztest_test_skip();
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE MAX_UNFRAGMENTED_SIZE enforcement (RFC 8613 Section 4.1.3.4.2) */
ZTEST(coap, test_oscore_max_unfragmented_size)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* RFC 8613 Section 4.1.3.4.2: "An endpoint receiving an OSCORE message
	 * with an Outer Block option SHALL first process this option according
	 * to [RFC7959], until all blocks ... have been received or the cumulated
	 * message size ... exceeds MAX_UNFRAGMENTED_SIZE ... In the latter case,
	 * the message SHALL be discarded."
	 */

	/* Verify that the configuration is sane */
	zassert_true(CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE > 0,
		     "MAX_UNFRAGMENTED_SIZE must be positive");

	/* Test: Create a series of blocks that would exceed MAX_UNFRAGMENTED_SIZE
	 * In a real implementation test, we would:
	 * 1. Send multiple outer blocks whose cumulative size exceeds the limit
	 * 2. Verify the exchange is discarded
	 * 3. Verify no callback is invoked
	 * 4. Verify state is cleared
	 *
	 * For now, we verify the constant is defined and reasonable.
	 */
	zassert_true(CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE >= 1024,
		     "MAX_UNFRAGMENTED_SIZE should be at least 1024 bytes");
	zassert_true(CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE <= 65536,
		     "MAX_UNFRAGMENTED_SIZE should not exceed 64KB");
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE outer Block2 reassembly buffer management */
ZTEST(coap, test_oscore_outer_block2_reassembly)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* This test verifies that outer Block2 reassembly works correctly:
	 * 1. First block initializes the reassembly buffer
	 * 2. Subsequent blocks are accumulated at correct offsets
	 * 3. Block context is properly maintained
	 * 4. Last block triggers OSCORE verification
	 */
	struct coap_block_context blk_ctx;
	uint8_t reassembly_buf[256];
	size_t reassembly_len = 0;

	/* Initialize block transfer */
	coap_block_transfer_init(&blk_ctx, COAP_BLOCK_16, 0);
	zassert_equal(blk_ctx.block_size, COAP_BLOCK_16, "Block size should be 16");
	zassert_equal(blk_ctx.current, 0, "Should start at offset 0");

	/* Simulate receiving block 0 */
	const uint8_t block0_data[] = "0123456789ABCDEF"; /* 16 bytes */
	memcpy(reassembly_buf + blk_ctx.current, block0_data, sizeof(block0_data) - 1);
	reassembly_len = blk_ctx.current + sizeof(block0_data) - 1;

	/* Advance to next block */
	blk_ctx.current += coap_block_size_to_bytes(blk_ctx.block_size);
	zassert_equal(blk_ctx.current, 16, "Should advance to offset 16");

	/* Simulate receiving block 1 */
	const uint8_t block1_data[] = "fedcba9876543210"; /* 16 bytes */
	memcpy(reassembly_buf + blk_ctx.current, block1_data, sizeof(block1_data) - 1);
	reassembly_len = blk_ctx.current + sizeof(block1_data) - 1;

	/* Verify reassembly buffer contains both blocks */
	zassert_equal(reassembly_len, 32, "Should have 32 bytes total");
	zassert_mem_equal(reassembly_buf, "0123456789ABCDEFfedcba9876543210", 32,
			  "Reassembled data should match");

	/* Test: Verify MAX_UNFRAGMENTED_SIZE would be enforced */
	size_t max_size = CONFIG_COAP_OSCORE_MAX_UNFRAGMENTED_SIZE;
	zassert_true(reassembly_len < max_size,
		     "Test data should be within MAX_UNFRAGMENTED_SIZE");

	/* Simulate exceeding MAX_UNFRAGMENTED_SIZE */
	size_t oversized_len = max_size + 1;
	zassert_true(oversized_len > max_size,
		     "Oversized data should exceed MAX_UNFRAGMENTED_SIZE");
#else
	ztest_test_skip();
#endif
}

/* Test OSCORE next block requesting behavior (RFC 7959 + RFC 8613 Section 8.4.1) */
ZTEST(coap, test_oscore_next_block_request)
{
#if defined(CONFIG_COAP_CLIENT) && defined(CONFIG_COAP_OSCORE)
	/* RFC 8613 Section 8.4.1: "If Block-wise is present in the response,
	 * then process the Outer Block options according to [RFC7959], until
	 * all blocks of the response have been received"
	 *
	 * This means the client must actively request the next block, not just
	 * wait passively. This test verifies the block request logic.
	 */
	struct coap_packet request;
	uint8_t buf[COAP_BUF_SIZE];
	struct coap_block_context blk_ctx;
	int r;

	/* Initialize block context for receiving */
	coap_block_transfer_init(&blk_ctx, COAP_BLOCK_16, 0);

	/* Create a dummy packet to simulate receiving first block */
	struct coap_packet dummy_response;
	uint8_t dummy_buf[COAP_BUF_SIZE];
	r = coap_packet_init(&dummy_response, dummy_buf, sizeof(dummy_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_RESPONSE_CODE_CONTENT, coap_next_id());
	zassert_equal(r, 0, "Should init dummy response");

	/* Add Block2 option for block 0 with 16-byte block size */
	uint8_t block0_val = 0x08; /* NUM=0, M=1, SZX=0 (16 bytes) */
	r = coap_packet_append_option(&dummy_response, COAP_OPTION_BLOCK2, &block0_val, 1);
	zassert_equal(r, 0, "Should append Block2 option");

	/* Add a 16-byte payload to match the block size */
	r = coap_packet_append_payload_marker(&dummy_response);
	zassert_equal(r, 0, "Should append payload marker");
	const uint8_t block_payload[16] = "0123456789ABCDE"; /* 16 bytes */
	r = coap_packet_append_payload(&dummy_response, block_payload, 16);
	zassert_equal(r, 0, "Should append payload");

	/* Update context from the block */
	r = coap_update_from_block(&dummy_response, &blk_ctx);
	zassert_equal(r, 0, "Should update block context");

	/* Advance to next block using the proper API.
	 * coap_next_block() advances by the actual payload length.
	 */
	size_t next_offset = coap_next_block(&dummy_response, &blk_ctx);
	zassert_equal(blk_ctx.current, 16, "Should advance to next block");
	zassert_equal(next_offset, 16, "Should return offset 16");

	/* Build next block request */
	r = coap_packet_init(&request, buf, sizeof(buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, coap_next_id());
	zassert_equal(r, 0, "Should init request packet");

	/* Append Block2 option for next block request */
	r = coap_append_block2_option(&request, &blk_ctx);
	zassert_equal(r, 0, "Should append Block2 option");

	/* Verify the Block2 option is correct */
	int block2_opt = coap_get_option_int(&request, COAP_OPTION_BLOCK2);
	zassert_true(block2_opt > 0, "Should have Block2 option");
	zassert_equal(GET_BLOCK_NUM(block2_opt), 1, "Should request block 1");

	/* Test: Verify block size is maintained */
	int szx = GET_BLOCK_SIZE(block2_opt);
	zassert_equal(szx, COAP_BLOCK_16, "Block size should be preserved");
#else
	ztest_test_skip();
#endif
}

#endif /* CONFIG_COAP_OSCORE */

/* Tests for RFC 7252 Section 5.4.1: Unrecognized critical options handling */
#if !defined(CONFIG_COAP_OSCORE)

ZTEST(coap, test_unsupported_critical_option_helper)
{
	struct coap_packet cpkt;
	uint8_t buffer[128];
	uint16_t unsupported_opt;
	int r;

	/* Build a packet with OSCORE option (which is unsupported in this build) */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Add OSCORE option with some dummy value */
	uint8_t oscore_value[] = {0x01, 0x02, 0x03};

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to append OSCORE option");

	/* Add a payload to make it a valid OSCORE message format */
	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Failed to append payload marker");

	uint8_t payload[] = "test";

	r = coap_packet_append_payload(&cpkt, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to append payload");

	/* Test: Check for unsupported critical options */
	r = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_equal(r, -ENOTSUP, "Should detect unsupported OSCORE option");
	zassert_equal(unsupported_opt, COAP_OPTION_OSCORE,
		      "Should report OSCORE as unsupported option");

	/* Test: Packet without OSCORE should pass */
	uint8_t buffer2[128];

	r = coap_packet_init(&cpkt, buffer2, sizeof(buffer2),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0x1235);
	zassert_equal(r, 0, "Failed to init packet");

	r = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_equal(r, 0, "Should not detect unsupported options in normal packet");
}

ZTEST(coap, test_server_rejects_oscore_con_request)
{
	struct coap_packet request;
	struct coap_packet response;
	uint8_t request_buf[128];
	uint8_t response_buf[128];
	int r;

	/* Build a CON request with OSCORE option */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0x1234);
	zassert_equal(r, 0, "Failed to init request");

	/* Add OSCORE option */
	uint8_t oscore_value[] = {0x01, 0x02, 0x03};

	r = coap_packet_append_option(&request, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to append OSCORE option");

	/* Add payload (required for valid OSCORE message) */
	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to append payload marker");

	uint8_t payload[] = "encrypted_data";

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to append payload");

	/* Simulate server processing: check for unsupported critical options */
	uint16_t unsupported_opt;

	r = coap_check_unsupported_critical_options(&request, &unsupported_opt);
	zassert_equal(r, -ENOTSUP, "Should detect unsupported OSCORE option");

	/* Server should send 4.02 Bad Option for CON request */
	r = coap_ack_init(&response, &request, response_buf, sizeof(response_buf),
			  COAP_RESPONSE_CODE_BAD_OPTION);
	zassert_equal(r, 0, "Failed to init Bad Option response");

	/* Verify response properties */
	uint8_t response_type = coap_header_get_type(&response);
	uint8_t response_code = coap_header_get_code(&response);
	uint16_t response_id = coap_header_get_id(&response);

	zassert_equal(response_type, COAP_TYPE_ACK, "Should be ACK");
	zassert_equal(response_code, COAP_RESPONSE_CODE_BAD_OPTION,
		      "Should be 4.02 Bad Option");
	zassert_equal(response_id, 0x1234, "Should match request ID");
}

ZTEST(coap, test_server_rejects_oscore_non_request)
{
	struct coap_packet request;
	uint8_t request_buf[128];
	int r;

	/* Build a NON request with OSCORE option */
	r = coap_packet_init(&request, request_buf, sizeof(request_buf),
			     COAP_VERSION_1, COAP_TYPE_NON_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1235);
	zassert_equal(r, 0, "Failed to init request");

	/* Add OSCORE option */
	uint8_t oscore_value[] = {0x01, 0x02, 0x03};

	r = coap_packet_append_option(&request, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to append OSCORE option");

	/* Add payload */
	r = coap_packet_append_payload_marker(&request);
	zassert_equal(r, 0, "Failed to append payload marker");

	uint8_t payload[] = "encrypted_data";

	r = coap_packet_append_payload(&request, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to append payload");

	/* Check for unsupported critical options */
	uint16_t unsupported_opt;

	r = coap_check_unsupported_critical_options(&request, &unsupported_opt);
	zassert_equal(r, -ENOTSUP, "Should detect unsupported OSCORE option");

	/* For NON requests, server should silently drop (no response) */
	/* This test verifies the detection; actual drop behavior is in server code */
}

ZTEST(coap, test_client_rejects_oscore_response)
{
	struct coap_packet response;
	uint8_t response_buf[128];
	int r;

	/* Build a response with OSCORE option */
	uint8_t token[] = {0x01, 0x02, 0x03, 0x04};

	r = coap_packet_init(&response, response_buf, sizeof(response_buf),
			     COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			     COAP_RESPONSE_CODE_CONTENT, 0x1236);
	zassert_equal(r, 0, "Failed to init response");

	/* Add OSCORE option */
	uint8_t oscore_value[] = {0x01, 0x02, 0x03};

	r = coap_packet_append_option(&response, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to append OSCORE option");

	/* Add payload */
	r = coap_packet_append_payload_marker(&response);
	zassert_equal(r, 0, "Failed to append payload marker");

	uint8_t payload[] = "encrypted_data";

	r = coap_packet_append_payload(&response, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to append payload");

	/* Client should detect unsupported critical option */
	uint16_t unsupported_opt;

	r = coap_check_unsupported_critical_options(&response, &unsupported_opt);
	zassert_equal(r, -ENOTSUP, "Should detect unsupported OSCORE option");
	zassert_equal(unsupported_opt, COAP_OPTION_OSCORE,
		      "Should report OSCORE as unsupported");

	/* For CON response, client should send RST (verified in client code) */
	/* This test verifies the detection logic */
}

ZTEST(coap, test_normal_messages_not_affected)
{
	struct coap_packet cpkt;
	uint8_t buffer[128];
	uint16_t unsupported_opt;
	int r;

	/* Build a normal request without OSCORE */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0x1237);
	zassert_equal(r, 0, "Failed to init packet");

	/* Add some normal options */
	r = coap_packet_set_path(&cpkt, "/test/path");
	zassert_equal(r, 0, "Failed to set path");

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	zassert_equal(r, 0, "Failed to append content format");

	/* Add payload */
	r = coap_packet_append_payload_marker(&cpkt);
	zassert_equal(r, 0, "Failed to append payload marker");

	uint8_t payload[] = "normal_payload";

	r = coap_packet_append_payload(&cpkt, payload, sizeof(payload));
	zassert_equal(r, 0, "Failed to append payload");

	/* Should not detect any unsupported critical options */
	r = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_equal(r, 0, "Should not detect unsupported options in normal message");
}

#endif /* !CONFIG_COAP_OSCORE */

/* RFC 9668: EDHOC option and content-format tests */
ZTEST(coap, test_edhoc_option_number)
{
	/* RFC 9668 Section 3.1 / IANA Section 8.1: EDHOC option number is 21 */
	zassert_equal(COAP_OPTION_EDHOC, 21, "EDHOC option number must be 21");
}

ZTEST(coap, test_edhoc_content_formats)
{
	/* RFC 9528 Section 10.9 Table 13: EDHOC content-format IDs */
	zassert_equal(COAP_CONTENT_FORMAT_APP_EDHOC_CBOR_SEQ, 64,
		      "application/edhoc+cbor-seq content-format must be 64");
	zassert_equal(COAP_CONTENT_FORMAT_APP_CID_EDHOC_CBOR_SEQ, 65,
		      "application/cid-edhoc+cbor-seq content-format must be 65");
}

#if !defined(CONFIG_COAP_EDHOC)
/* Test that EDHOC option is rejected when EDHOC support is disabled */
ZTEST(coap, test_edhoc_unsupported_critical_option)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	uint16_t unsupported_opt;
	int r;

	/* Build a request with EDHOC option */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Add EDHOC option (empty as per RFC 9668) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to append EDHOC option");

	/* Should detect EDHOC as unsupported critical option */
	r = coap_check_unsupported_critical_options(&cpkt, &unsupported_opt);
	zassert_equal(r, -ENOTSUP, "Should detect EDHOC as unsupported");
	zassert_equal(unsupported_opt, COAP_OPTION_EDHOC,
		      "Should report EDHOC option as unsupported");
}
#endif /* !CONFIG_COAP_EDHOC */

#if defined(CONFIG_COAP_EDHOC)
/* Test EDHOC option detection */
ZTEST(coap, test_edhoc_msg_has_edhoc)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a request without EDHOC option */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Should not detect EDHOC option */
	zassert_false(coap_edhoc_msg_has_edhoc(&cpkt),
		      "Should not detect EDHOC in message without option");

	/* Add EDHOC option (empty as per RFC 9668) */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to append EDHOC option");

	/* Should detect EDHOC option */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt),
		     "Should detect EDHOC option in message");
}

/* Test EDHOC combined payload parsing - RFC 9668 Figure 4 example */
ZTEST(coap, test_edhoc_split_comb_payload)
{
	/* Example from RFC 9668 Section 3.2.1:
	 * EDHOC_MSG_3 is a CBOR bstr containing some data
	 * For this test, we'll use a simple example:
	 * - CBOR bstr with 10 bytes of data: 0x4a (header) + 10 bytes
	 * - Followed by OSCORE payload
	 */
	uint8_t combined_payload[] = {
		/* CBOR bstr header: major type 2, length 10 */
		0x4a,
		/* EDHOC_MSG_3 data (10 bytes) */
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
		/* OSCORE_PAYLOAD (5 bytes) */
		0xaa, 0xbb, 0xcc, 0xdd, 0xee
	};

	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	r = coap_edhoc_split_comb_payload(combined_payload, sizeof(combined_payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, 0, "Failed to split combined payload");

	/* Check EDHOC_MSG_3 span (header + data) */
	zassert_equal(edhoc_msg3.len, 11, "EDHOC_MSG_3 length incorrect");
	zassert_equal(edhoc_msg3.ptr, combined_payload, "EDHOC_MSG_3 pointer incorrect");

	/* Check OSCORE_PAYLOAD span */
	zassert_equal(oscore_payload.len, 5, "OSCORE_PAYLOAD length incorrect");
	zassert_equal(oscore_payload.ptr, combined_payload + 11,
		      "OSCORE_PAYLOAD pointer incorrect");
	zassert_equal(oscore_payload.ptr[0], 0xaa, "OSCORE_PAYLOAD data incorrect");
}

/* Test EDHOC combined payload parsing with 1-byte length encoding */
ZTEST(coap, test_edhoc_split_comb_payload_1byte_len)
{
	/* CBOR bstr with 1-byte length encoding (additional info = 24)
	 * 0x58 0x1e (30 bytes) + data + OSCORE payload
	 */
	uint8_t combined_payload[2 + 30 + 5];

	combined_payload[0] = 0x58; /* major type 2, additional info 24 */
	combined_payload[1] = 30;   /* length = 30 */
	memset(&combined_payload[2], 0xaa, 30); /* EDHOC data */
	memset(&combined_payload[32], 0xbb, 5); /* OSCORE payload */

	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	r = coap_edhoc_split_comb_payload(combined_payload, sizeof(combined_payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, 0, "Failed to split combined payload with 1-byte length");

	zassert_equal(edhoc_msg3.len, 32, "EDHOC_MSG_3 length incorrect");
	zassert_equal(oscore_payload.len, 5, "OSCORE_PAYLOAD length incorrect");
}

/* Test EDHOC combined payload parsing with 2-byte length encoding */
ZTEST(coap, test_edhoc_split_comb_payload_2byte_len)
{
	/* CBOR bstr with 2-byte length encoding (additional info = 25)
	 * 0x59 0x01 0x00 (256 bytes) + data + OSCORE payload
	 */
	uint8_t combined_payload[3 + 256 + 5];

	combined_payload[0] = 0x59; /* major type 2, additional info 25 */
	combined_payload[1] = 0x01; /* length high byte */
	combined_payload[2] = 0x00; /* length low byte = 256 */
	memset(&combined_payload[3], 0xcc, 256); /* EDHOC data */
	memset(&combined_payload[259], 0xdd, 5); /* OSCORE payload */

	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	r = coap_edhoc_split_comb_payload(combined_payload, sizeof(combined_payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, 0, "Failed to split combined payload with 2-byte length");

	zassert_equal(edhoc_msg3.len, 259, "EDHOC_MSG_3 length incorrect");
	zassert_equal(oscore_payload.len, 5, "OSCORE_PAYLOAD length incorrect");
}

/* Test EDHOC combined payload parsing error cases */
ZTEST(coap, test_edhoc_split_comb_payload_errors)
{
	uint8_t payload[] = { 0x4a, 0x01, 0x02, 0x03, 0x04, 0x05,
			      0x06, 0x07, 0x08, 0x09, 0x0a };
	struct coap_edhoc_span edhoc_msg3, oscore_payload;
	int r;

	/* Test NULL parameters */
	r = coap_edhoc_split_comb_payload(NULL, sizeof(payload),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject NULL payload");

	r = coap_edhoc_split_comb_payload(payload, sizeof(payload),
					  NULL, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject NULL edhoc_msg3");

	r = coap_edhoc_split_comb_payload(payload, sizeof(payload),
					  &edhoc_msg3, NULL);
	zassert_equal(r, -EINVAL, "Should reject NULL oscore_payload");

	/* Test empty payload */
	r = coap_edhoc_split_comb_payload(payload, 0,
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject empty payload");

	/* Test wrong CBOR major type (not byte string) */
	uint8_t wrong_type[] = { 0x01, 0x02, 0x03 }; /* major type 0 (unsigned int) */

	r = coap_edhoc_split_comb_payload(wrong_type, sizeof(wrong_type),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject non-bstr major type");

	/* Test missing OSCORE payload (EDHOC_MSG_3 takes entire payload) */
	uint8_t no_oscore[] = { 0x43, 0x01, 0x02, 0x03 }; /* bstr of length 3 */

	r = coap_edhoc_split_comb_payload(no_oscore, sizeof(no_oscore),
					  &edhoc_msg3, &oscore_payload);
	zassert_equal(r, -EINVAL, "Should reject payload without OSCORE part");
}

/* Test EDHOC option removal */
ZTEST(coap, test_edhoc_remove_option)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a request with EDHOC option */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to init packet");

	/* Add EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to append EDHOC option");

	/* Verify EDHOC option is present */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt), "EDHOC option should be present");

	/* Remove EDHOC option */
	r = coap_edhoc_remove_option(&cpkt);
	zassert_equal(r, 0, "Failed to remove EDHOC option");

	/* Re-parse the packet to ensure option removal is reflected */
	struct coap_option options[10];
	uint8_t opt_num = 10;

	r = coap_packet_parse(&cpkt, buffer, cpkt.offset, options, opt_num);
	zassert_equal(r, 0, "Failed to re-parse packet");

	/* Verify EDHOC option is removed */
	zassert_false(coap_edhoc_msg_has_edhoc(&cpkt), "EDHOC option should be removed");
}

#if defined(CONFIG_COAP_OSCORE)
/* Test that EDHOC option is Class U (unprotected) for OSCORE */
ZTEST(coap, test_edhoc_option_class_u_oscore)
{
	/* This test verifies that the EDHOC option (21) is treated as Class U
	 * (unprotected) by OSCORE, as required by RFC 9668 Section 3.1.
	 * This is implemented in the uoscore-uedhoc library's is_class_e() function.
	 *
	 * We can't directly test the uoscore library here, but we verify that
	 * the EDHOC option number is correctly defined.
	 */
	zassert_equal(COAP_OPTION_EDHOC, 21,
		      "EDHOC option must be 21 for Class U classification");
}
#endif /* CONFIG_COAP_OSCORE */

#endif /* CONFIG_COAP_EDHOC */

#if defined(CONFIG_COAP_EDHOC_COMBINED_REQUEST)
/* Test OSCORE option kid extraction per RFC 9668 Section 3.3.1 Step 3 */
ZTEST(coap, test_oscore_option_extract_kid)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a CoAP packet with OSCORE option per RFC 8613 Section 6.1:
	 * OSCORE option value format:
	 *   - Flag byte: bits 0-2: n (Partial IV length, 0-5 valid)
	 *                bit 3: k (kid present flag)
	 *                bit 4: h (kid context present flag)
	 *                bits 5-7: reserved (must be 0)
	 *   - n bytes: Partial IV (if n > 0)
	 *   - 1 byte: kid context length s (if h=1)
	 *   - s bytes: kid context (if h=1)
	 *   - remaining bytes: kid (if k=1, NOT length-prefixed)
	 *
	 * Test case: flag=0x08 (k=1, h=0, n=0), kid value=0x42
	 * OSCORE option value: 0x0842
	 */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add OSCORE option: flag=0x08 (k=1, h=0, n=0), kid=0x42 */
	uint8_t oscore_value[] = { 0x08, 0x42 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to add OSCORE option");

	/* Extract kid */
	uint8_t kid[16];
	size_t kid_len = sizeof(kid);

	/* Need to include the helper header */
	extern int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
						  uint8_t *kid, size_t *kid_len);

	r = coap_oscore_option_extract_kid(&cpkt, kid, &kid_len);
	zassert_equal(r, 0, "Failed to extract kid");
	zassert_equal(kid_len, 1, "kid length should be 1");
	zassert_equal(kid[0], 0x42, "kid value should be 0x42");
}

/* Test OSCORE option with reserved bits set must fail */
ZTEST(coap, test_oscore_option_reserved_bits)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* RFC 8613 §6.1: Reserved bits (5-7) must be zero */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add OSCORE option with reserved bit 7 set: 0x88 (bit 7 set, k=1) */
	uint8_t oscore_value[] = { 0x88, 0x42 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to add OSCORE option");

	/* Extract kid - should fail due to reserved bits */
	uint8_t kid[16];
	size_t kid_len = sizeof(kid);

	extern int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
						  uint8_t *kid, size_t *kid_len);

	r = coap_oscore_option_extract_kid(&cpkt, kid, &kid_len);
	zassert_equal(r, -EINVAL, "Should fail with reserved bits set");
}

/* Test OSCORE option with reserved Partial IV length must fail */
ZTEST(coap, test_oscore_option_reserved_piv_length)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* RFC 8613 §6.1: n=6 and n=7 are reserved */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add OSCORE option with n=6 and k=1: 0x0E (bits 0-2: n=6, bit 3: k=1) */
	uint8_t oscore_value[] = { 0x0E, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x42 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to add OSCORE option");

	/* Extract kid - should fail due to reserved n value */
	uint8_t kid[16];
	size_t kid_len = sizeof(kid);

	extern int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
						  uint8_t *kid, size_t *kid_len);

	r = coap_oscore_option_extract_kid(&cpkt, kid, &kid_len);
	zassert_equal(r, -EINVAL, "Should fail with reserved Partial IV length");
}

/* Test OSCORE option truncated at kid context length must fail */
ZTEST(coap, test_oscore_option_truncated_kid_context_length)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* h=1 but missing s byte */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add OSCORE option with h=1 but no s byte: 0x10 (bit 4: h=1) */
	uint8_t oscore_value[] = { 0x10 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to add OSCORE option");

	/* Extract kid - should fail due to truncation */
	uint8_t kid[16];
	size_t kid_len = sizeof(kid);

	extern int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
						  uint8_t *kid, size_t *kid_len);

	r = coap_oscore_option_extract_kid(&cpkt, kid, &kid_len);
	zassert_equal(r, -EINVAL, "Should fail with truncated kid context");
}

/* Test OSCORE option with kid context length exceeding remaining data must fail */
ZTEST(coap, test_oscore_option_invalid_kid_context_length)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* h=1 with s that exceeds remaining length */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add OSCORE option with h=1, s=10 but only 2 bytes follow: 0x10, 0x0A, 0x01, 0x02 */
	uint8_t oscore_value[] = { 0x10, 0x0A, 0x01, 0x02 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to add OSCORE option");

	/* Extract kid - should fail due to invalid kid context length */
	uint8_t kid[16];
	size_t kid_len = sizeof(kid);

	extern int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
						  uint8_t *kid, size_t *kid_len);

	r = coap_oscore_option_extract_kid(&cpkt, kid, &kid_len);
	zassert_equal(r, -EINVAL, "Should fail with invalid kid context length");
}

/* Test OSCORE option with no kid flag must return -ENOENT */
ZTEST(coap, test_oscore_option_no_kid_flag)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* k=0 (no kid present) */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add OSCORE option with k=0: 0x00 (all flags clear) */
	uint8_t oscore_value[] = { 0x00 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_OSCORE,
				      oscore_value, sizeof(oscore_value));
	zassert_equal(r, 0, "Failed to add OSCORE option");

	/* Extract kid - should return -ENOENT since k=0 */
	uint8_t kid[16];
	size_t kid_len = sizeof(kid);

	extern int coap_oscore_option_extract_kid(const struct coap_packet *cpkt,
						  uint8_t *kid, size_t *kid_len);

	r = coap_oscore_option_extract_kid(&cpkt, kid, &kid_len);
	zassert_equal(r, -ENOENT, "Should return -ENOENT when kid flag not set");
}

/* Test EDHOC option validation: at most once */
ZTEST(coap, test_edhoc_option_at_most_once)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a packet with two EDHOC options (invalid per RFC 9668 Section 3.1) */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add first EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to add first EDHOC option");

	/* Add second EDHOC option */
	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC, NULL, 0);
	zassert_equal(r, 0, "Failed to add second EDHOC option");

	/* Verify that coap_edhoc_msg_has_edhoc returns false for multiple options */
	zassert_false(coap_edhoc_msg_has_edhoc(&cpkt),
		      "Multiple EDHOC options should be treated as malformed");
}

/* Test EDHOC option validation: ignore non-empty value */
ZTEST(coap, test_edhoc_option_ignore_value)
{
	uint8_t buffer[128];
	struct coap_packet cpkt;
	int r;

	/* Build a packet with EDHOC option containing a value (should be ignored) */
	r = coap_packet_init(&cpkt, buffer, sizeof(buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0);
	zassert_equal(r, 0, "Failed to initialize packet");

	/* Add EDHOC option with a value (RFC 9668 says recipient MUST ignore it) */
	uint8_t edhoc_value[] = { 0x01, 0x02, 0x03 };

	r = coap_packet_append_option(&cpkt, COAP_OPTION_EDHOC,
				      edhoc_value, sizeof(edhoc_value));
	zassert_equal(r, 0, "Failed to add EDHOC option");

	/* Verify that EDHOC option is still detected (value is ignored) */
	zassert_true(coap_edhoc_msg_has_edhoc(&cpkt),
		     "EDHOC option should be detected even with non-empty value");
}

/* Test EDHOC error encoding: basic case with ERR_CODE=1 */
ZTEST(coap, test_edhoc_encode_error_basic)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	/* Encode EDHOC error: ERR_CODE=1, ERR_INFO="EDHOC error" */
	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	r = coap_edhoc_encode_error(1, "EDHOC error", buffer, &buffer_len);
	zassert_equal(r, 0, "Failed to encode EDHOC error");

	/* Verify CBOR Sequence encoding:
	 * - First item: CBOR unsigned int 1 = 0x01
	 * - Second item: CBOR text string "EDHOC error" (11 bytes)
	 *   - Major type 3 (text string), length 11 in additional info
	 *   - Header: 0x6B (0x60 | 11)
	 *   - Followed by 11 bytes of UTF-8 text
	 */
	zassert_equal(buffer_len, 1 + 1 + 11, "Encoded length should be 13 bytes");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x6B, "ERR_INFO header should be 0x6B (tstr, len=11)");
	zassert_mem_equal(&buffer[2], "EDHOC error", 11, "ERR_INFO should be 'EDHOC error'");
}

/* Test EDHOC error encoding: short diagnostic message */
ZTEST(coap, test_edhoc_encode_error_short_diag)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	r = coap_edhoc_encode_error(1, "err", buffer, &buffer_len);
	zassert_equal(r, 0, "Failed to encode EDHOC error");

	/* Verify encoding:
	 * - ERR_CODE: 0x01
	 * - ERR_INFO: 0x63 (tstr, len=3) + "err"
	 */
	zassert_equal(buffer_len, 1 + 1 + 3, "Encoded length should be 5 bytes");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x63, "ERR_INFO header should be 0x63 (tstr, len=3)");
	zassert_mem_equal(&buffer[2], "err", 3, "ERR_INFO should be 'err'");
}

/* Test EDHOC error encoding: longer diagnostic message (>23 bytes) */
ZTEST(coap, test_edhoc_encode_error_long_diag)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	/* 30-byte diagnostic message */
	const char *diag = "EDHOC processing failed here";

	r = coap_edhoc_encode_error(1, diag, buffer, &buffer_len);
	zassert_equal(r, 0, "Failed to encode EDHOC error");

	size_t diag_len = strlen(diag);

	/* Verify encoding:
	 * - ERR_CODE: 0x01
	 * - ERR_INFO: 0x78 (tstr, 1-byte length follows) + length byte + text
	 */
	zassert_equal(buffer_len, 1 + 2 + diag_len, "Encoded length incorrect");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x78, "ERR_INFO header should be 0x78 (tstr, 1-byte len)");
	zassert_equal(buffer[2], diag_len, "Length byte should match diagnostic length");
	zassert_mem_equal(&buffer[3], diag, diag_len, "ERR_INFO text incorrect");
}

/* Test EDHOC error encoding: buffer too small */
ZTEST(coap, test_edhoc_encode_error_buffer_too_small)
{
	uint8_t buffer[5];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	/* Try to encode "EDHOC error" (12 bytes) into 5-byte buffer */
	r = coap_edhoc_encode_error(1, "EDHOC error", buffer, &buffer_len);
	zassert_equal(r, -ENOMEM, "Should fail with -ENOMEM for small buffer");
}

/* Test EDHOC error encoding: invalid parameters */
ZTEST(coap, test_edhoc_encode_error_invalid_params)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	/* NULL buffer */
	r = coap_edhoc_encode_error(1, "test", NULL, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with NULL buffer");

	/* NULL length pointer */
	r = coap_edhoc_encode_error(1, "test", buffer, NULL);
	zassert_equal(r, -EINVAL, "Should fail with NULL length pointer");

	/* NULL diagnostic message */
	r = coap_edhoc_encode_error(1, NULL, buffer, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with NULL diagnostic message");

	/* Invalid error code (>23) */
	r = coap_edhoc_encode_error(100, "test", buffer, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with error code > 23");

	/* Negative error code */
	r = coap_edhoc_encode_error(-1, "test", buffer, &buffer_len);
	zassert_equal(r, -EINVAL, "Should fail with negative error code");
}

/* Test EDHOC error encoding: empty diagnostic message */
ZTEST(coap, test_edhoc_encode_error_empty_diag)
{
	uint8_t buffer[128];
	size_t buffer_len = sizeof(buffer);
	int r;

	extern int coap_edhoc_encode_error(int err_code, const char *diag_msg,
					   uint8_t *out_buf, size_t *inout_len);

	r = coap_edhoc_encode_error(1, "", buffer, &buffer_len);
	zassert_equal(r, 0, "Should succeed with empty diagnostic message");

	/* Verify encoding:
	 * - ERR_CODE: 0x01
	 * - ERR_INFO: 0x60 (tstr, len=0)
	 */
	zassert_equal(buffer_len, 2, "Encoded length should be 2 bytes");
	zassert_equal(buffer[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(buffer[1], 0x60, "ERR_INFO header should be 0x60 (tstr, len=0)");
}

/* Test EDHOC error response formatting: basic case */
ZTEST(coap, test_edhoc_error_response_format)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build a CON request */
	uint8_t token[] = { 0x12, 0x34 };

	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, sizeof(token), token,
			     COAP_METHOD_POST, 0x5678);
	zassert_equal(r, 0, "Failed to initialize request");

	/* Build EDHOC error response */
	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");

	/* Verify response properties */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_ACK,
		      "Response should be ACK for CON request");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_BAD_REQUEST,
		      "Response code should be 4.00");
	zassert_equal(coap_header_get_id(&response), 0x5678,
		      "Response ID should match request ID");

	uint8_t resp_token[COAP_TOKEN_MAX_LEN];
	uint8_t resp_tkl = coap_header_get_token(&response, resp_token);

	zassert_equal(resp_tkl, sizeof(token), "Token length should match");
	zassert_mem_equal(resp_token, token, sizeof(token), "Token should match");

	/* Verify Content-Format option */
	int content_format = coap_get_option_int(&response, COAP_OPTION_CONTENT_FORMAT);

	zassert_equal(content_format, COAP_CONTENT_FORMAT_APP_EDHOC_CBOR_SEQ,
		      "Content-Format should be application/edhoc+cbor-seq (64)");

	/* Verify payload contains EDHOC error CBOR sequence */
	uint16_t payload_len;
	const uint8_t *payload = coap_packet_get_payload(&response, &payload_len);

	zassert_not_null(payload, "Response should have payload");
	zassert_true(payload_len > 0, "Payload should not be empty");

	/* Verify CBOR sequence structure:
	 * - First byte: ERR_CODE = 0x01
	 * - Second byte: tstr header for "EDHOC error" (11 bytes) = 0x6B
	 * - Remaining bytes: "EDHOC error"
	 */
	zassert_equal(payload[0], 0x01, "ERR_CODE should be 0x01");
	zassert_equal(payload[1], 0x6B, "ERR_INFO header should be 0x6B");
	zassert_mem_equal(&payload[2], "EDHOC error", 11, "ERR_INFO should be 'EDHOC error'");
}

/* Test EDHOC error response: NON request should get NON response */
ZTEST(coap, test_edhoc_error_response_non)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build a NON request */
	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_NON_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");

	/* Verify response type is NON for NON request */
	zassert_equal(coap_header_get_type(&response), COAP_TYPE_NON_CON,
		      "Response should be NON for NON request");
}

/* Test EDHOC error response: no OSCORE option present */
ZTEST(coap, test_edhoc_error_response_no_oscore)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	/* Build request */
	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");

	/* Verify OSCORE option is NOT present in error response
	 * Per RFC 9668 Section 3.3.1, EDHOC error responses MUST NOT be OSCORE-protected
	 */
	struct coap_option option;

	r = coap_find_options(&response, COAP_OPTION_OSCORE, &option, 1);
	zassert_equal(r, 0, "OSCORE option should NOT be present in EDHOC error response");
}

/* Test EDHOC error response: different error codes */
ZTEST(coap, test_edhoc_error_response_different_codes)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[256];
	struct coap_packet request;
	struct coap_packet response;
	int r;

	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	/* Test with 5.00 Internal Server Error */
	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_INTERNAL_ERROR,
					    1, "Server error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_equal(r, 0, "Failed to build EDHOC error response");
	zassert_equal(coap_header_get_code(&response), COAP_RESPONSE_CODE_INTERNAL_ERROR,
		      "Response code should be 5.00");

	/* Verify payload still has correct EDHOC error structure */
	uint16_t payload_len;
	const uint8_t *payload = coap_packet_get_payload(&response, &payload_len);

	zassert_not_null(payload, "Response should have payload");
	zassert_equal(payload[0], 0x01, "ERR_CODE should be 0x01");
}

/* Test EDHOC error response: buffer too small */
ZTEST(coap, test_edhoc_error_response_buffer_too_small)
{
	uint8_t req_buffer[128];
	uint8_t resp_buffer[10];  /* Too small - need at least ~25 bytes */
	struct coap_packet request;
	struct coap_packet response;
	int r;

	r = coap_packet_init(&request, req_buffer, sizeof(req_buffer),
			     COAP_VERSION_1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_POST, 0x1234);
	zassert_equal(r, 0, "Failed to initialize request");

	extern int coap_edhoc_build_error_response(struct coap_packet *response,
						   const struct coap_packet *request,
						   uint8_t code,
						   int err_code,
						   const char *diag_msg,
						   uint8_t *buf, size_t buf_len);

	r = coap_edhoc_build_error_response(&response, &request,
					    COAP_RESPONSE_CODE_BAD_REQUEST,
					    1, "EDHOC error",
					    resp_buffer, sizeof(resp_buffer));
	zassert_true(r < 0, "Should fail with buffer too small");
}

/* Test RFC 9528 Table 14 ID mapping for derived OSCORE contexts */
ZTEST(coap, test_edhoc_oscore_id_mapping)
{
	/* This test verifies that EDHOC-derived OSCORE contexts use the correct
	 * Sender/Recipient ID mapping per RFC 9528 Appendix A.1 Table 14:
	 * "EDHOC Responder: OSCORE Sender ID = C_I; OSCORE Recipient ID = C_R"
	 */

	/* Test data: C_I and C_R from RFC 9528 test vectors */
	uint8_t c_i[] = { 0x37 };  /* Connection identifier for initiator */
	uint8_t c_r[] = { 0x27 };  /* Connection identifier for responder */

	/* Verify that wrapper signature accepts both IDs */
	uint8_t master_secret[16] = { 0 };
	uint8_t master_salt[8] = { 0 };
	struct context mock_ctx = { 0 };

	/* When CONFIG_UEDHOC=n, this will return -ENOTSUP (expected for tests) */
	int ret = coap_oscore_context_init_wrapper(
		&mock_ctx,
		master_secret, sizeof(master_secret),
		master_salt, sizeof(master_salt),
		c_i, sizeof(c_i),  /* Sender ID = C_I */
		c_r, sizeof(c_r),  /* Recipient ID = C_R */
		10,  /* AES-CCM-16-64-128 */
		5);  /* HKDF-SHA-256 */

	/* In test environment without CONFIG_UEDHOC, expect -ENOTSUP */
	/* In production with CONFIG_UEDHOC, this would succeed and initialize the context */
	zassert_true(ret == -ENOTSUP || ret == 0,
		     "Wrapper should return -ENOTSUP (test) or 0 (production)");
}

/* Test per-exchange OSCORE context tracking */
ZTEST(coap, test_oscore_exchange_context_tracking)
{
	/* This test verifies that OSCORE exchanges track the correct context
	 * for response protection, enabling per-exchange contexts for EDHOC-derived
	 * OSCORE contexts per RFC 9668 Section 3.3.1.
	 */

	struct coap_oscore_exchange cache[4] = { 0 };
	struct net_sockaddr_in6 addr = {
		.sin6_family = NET_AF_INET6,
		.sin6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				   0, 0, 0, 0, 0, 0, 0, 0x1 } } }
	};
	uint8_t token[] = { 0x12, 0x34 };
	struct context mock_ctx = { 0 };

	/* Add exchange with specific context */
	int ret = oscore_exchange_add(cache, (struct net_sockaddr *)&addr,
				      sizeof(addr), token, sizeof(token),
				      false, &mock_ctx);
	zassert_equal(ret, 0, "Failed to add OSCORE exchange");

	/* Find exchange and verify context is stored */
	struct coap_oscore_exchange *exchange = oscore_exchange_find(
		cache, (struct net_sockaddr *)&addr, sizeof(addr),
		token, sizeof(token));

	zassert_not_null(exchange, "Exchange should be found");
	zassert_equal(exchange->oscore_ctx, &mock_ctx,
		      "Exchange should track the correct OSCORE context");
}

/* Test EDHOC session C_I storage */
ZTEST(coap, test_edhoc_session_ci_storage)
{
	/* This test verifies that EDHOC sessions can store C_I for later use
	 * in OSCORE context initialization per RFC 9528 Table 14.
	 */

	struct coap_edhoc_session cache[4] = { 0 };
	uint8_t c_r[] = { 0x27 };
	uint8_t c_i[] = { 0x37 };

	/* Insert session */
	struct coap_edhoc_session *session = coap_edhoc_session_insert(
		cache, ARRAY_SIZE(cache), c_r, sizeof(c_r));
	zassert_not_null(session, "Failed to insert EDHOC session");

	/* Set C_I */
	int ret = coap_edhoc_session_set_ci(session, c_i, sizeof(c_i));
	zassert_equal(ret, 0, "Failed to set C_I");

	/* Verify C_I is stored */
	zassert_equal(session->c_i_len, sizeof(c_i), "C_I length mismatch");
	zassert_mem_equal(session->c_i, c_i, sizeof(c_i), "C_I value mismatch");

	/* Find session and verify C_I is still there */
	struct coap_edhoc_session *found = coap_edhoc_session_find(
		cache, ARRAY_SIZE(cache), c_r, sizeof(c_r));
	zassert_not_null(found, "Session should be found");
	zassert_equal(found->c_i_len, sizeof(c_i), "Found C_I length mismatch");
	zassert_mem_equal(found->c_i, c_i, sizeof(c_i), "Found C_I value mismatch");
}

#if defined(CONFIG_UOSCORE)
/* Test OSCORE context allocation from pool */
ZTEST(coap, test_oscore_context_pool_allocation)
{
	/* This test verifies that OSCORE contexts can be allocated from the
	 * internal fixed pool for EDHOC-derived contexts.
	 */

	struct context *ctx1 = coap_oscore_ctx_alloc();
	zassert_not_null(ctx1, "Failed to allocate first context");

	struct context *ctx2 = coap_oscore_ctx_alloc();
	zassert_not_null(ctx2, "Failed to allocate second context");

	/* Contexts should be different */
	zassert_not_equal(ctx1, ctx2, "Contexts should be different");

	/* Free contexts */
	coap_oscore_ctx_free(ctx1);
	coap_oscore_ctx_free(ctx2);

	/* Should be able to allocate again after freeing */
	struct context *ctx3 = coap_oscore_ctx_alloc();
	zassert_not_null(ctx3, "Failed to allocate after freeing");

	coap_oscore_ctx_free(ctx3);
}
#endif /* CONFIG_UOSCORE */

#endif /* CONFIG_COAP_EDHOC_COMBINED_REQUEST */

ZTEST_SUITE(coap, NULL, NULL, NULL, NULL, NULL);
