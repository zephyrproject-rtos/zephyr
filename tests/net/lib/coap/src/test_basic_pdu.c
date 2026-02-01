/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


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