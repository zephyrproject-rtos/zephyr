/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


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



/* Helper to build a Block2 response with optional ETag */
static int build_block2_response(struct coap_packet *response, uint8_t *buf, size_t buf_len,
				 const uint8_t *token, uint8_t tkl, uint16_t id,
				 int block_num, bool more, const uint8_t *etag,
				 uint8_t etag_len, const uint8_t *payload, size_t payload_len)
{
	int ret;

	ret = coap_packet_init(response, buf, buf_len, COAP_VERSION_1, COAP_TYPE_ACK,
			       tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (ret < 0) {
		return ret;
	}

	if (etag != NULL && etag_len > 0) {
		ret = coap_packet_append_option(response, COAP_OPTION_ETAG, etag, etag_len);
		if (ret < 0) {
			return ret;
		}
	}

	ret = coap_append_option_int(response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (ret < 0) {
		return ret;
	}

	int block_opt = (block_num << 4) | (more ? 0x08 : 0x00) | COAP_BLOCK_64;
	ret = coap_append_option_int(response, COAP_OPTION_BLOCK2, block_opt);
	if (ret < 0) {
		return ret;
	}

	if (payload != NULL && payload_len > 0) {
		ret = coap_packet_append_payload_marker(response);
		if (ret < 0) {
			return ret;
		}
		ret = coap_packet_append_payload(response, payload, payload_len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}


/* Helper to set up test request state after block 0 */
static void setup_block_state(struct coap_client_internal_request *req,
			      const uint8_t *token, size_t tkl,
			      const uint8_t *etag, size_t etag_len)
{
	req->request_ongoing = true;
	req->last_response_id = -1;
	memcpy(req->request_token, token, tkl);
	req->request_tkl = tkl;
	if (etag && etag_len > 0) {
		memcpy(req->block2_etag, etag, etag_len);
		req->block2_etag_len = etag_len;
	}
	req->recv_blk_ctx.current = 64;
	req->recv_blk_ctx.block_size = COAP_BLOCK_64;
}

/* Test: ETag mismatch aborts Block2 transfer */
ZTEST(coap, test_block2_etag_mismatch_aborts)
{
	struct coap_client client = {0};
	uint8_t token[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
	uint8_t etag_a[] = {0x01, 0x02, 0x03, 0x04};
	uint8_t etag_b[] = {0x05, 0x06, 0x07, 0x08};
	uint8_t payload[] = "Test payload data";
	uint8_t response_buf[256];
	struct coap_packet response;
	int ret;

	k_mutex_init(&client.lock);
	client.fd = 1;

	struct coap_client_internal_request *req = &client.requests[0];
	memset(req, 0, sizeof(*req));
	memcpy(req->request_token, token, sizeof(token));
	req->request_tkl = sizeof(token);
	req->request_ongoing = true;
	req->last_response_id = -1;

	/* Inject Block 0 with ETag A */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1234,
				    0, true, etag_a, sizeof(etag_a),
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 0");

	coap_client_test_inject_response(&client, response.data, response.offset);

	/* Restore state after block 0 (send will fail without real socket) */
	setup_block_state(req, token, sizeof(token), etag_a, sizeof(etag_a));

	/* Inject Block 1 with ETag B (mismatch) */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1235,
				    1, false, etag_b, sizeof(etag_b),
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 1");

	ret = coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_equal(ret, -EBADMSG, "ETag mismatch should abort");

	zassert_equal(req->block2_etag_len, 0, "ETag state should be cleared");
	zassert_false(req->request_ongoing, "Request should be released");
}

/* Test: Missing ETag after being available aborts */
ZTEST(coap, test_block2_etag_missing_after_present_aborts)
{
	struct coap_client client = {0};
	uint8_t token[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
	uint8_t etag[] = {0x01, 0x02, 0x03, 0x04};
	uint8_t payload[] = "Test payload data";
	uint8_t response_buf[256];
	struct coap_packet response;
	int ret;

	k_mutex_init(&client.lock);
	client.fd = 1;

	struct coap_client_internal_request *req = &client.requests[0];
	memset(req, 0, sizeof(*req));
	memcpy(req->request_token, token, sizeof(token));
	req->request_tkl = sizeof(token);
	req->request_ongoing = true;
	req->last_response_id = -1;

	/* Inject Block 0 with ETag */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1234,
				    0, true, etag, sizeof(etag),
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 0");

	coap_client_test_inject_response(&client, response.data, response.offset);

	/* Restore state after block 0 */
	setup_block_state(req, token, sizeof(token), etag, sizeof(etag));

	/* Inject Block 1 without ETag */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1235,
				    1, false, NULL, 0,
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 1");

	ret = coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_equal(ret, -EBADMSG, "Missing ETag should abort");

	zassert_equal(req->block2_etag_len, 0, "ETag state should be cleared");
	zassert_false(req->request_ongoing, "Request should be released");
}

/* Test: No ETag in any block allows transfer */
ZTEST(coap, test_block2_no_etag_allows_transfer)
{
	struct coap_client client = {0};
	uint8_t token[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
	uint8_t payload[] = "Test payload";
	uint8_t response_buf[256];
	struct coap_packet response;
	int ret;

	k_mutex_init(&client.lock);
	client.fd = 1;

	struct coap_client_internal_request *req = &client.requests[0];
	memset(req, 0, sizeof(*req));
	memcpy(req->request_token, token, sizeof(token));
	req->request_tkl = sizeof(token);
	req->request_ongoing = true;
	req->last_response_id = -1;

	/* Inject Block 0 without ETag */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1234,
				    0, true, NULL, 0, payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 0");

	coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_equal(req->block2_etag_len, 0, "No ETag should be stored");

	req->request_ongoing = true;

	/* Inject Block 1 without ETag (should succeed) */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1235,
				    1, false, NULL, 0, payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 1");

	ret = coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_not_equal(ret, -EBADMSG, "Transfer without ETag should not abort");
}

/* Test: Multiple ETag options in response aborts */
ZTEST(coap, test_block2_multiple_etag_aborts)
{
	struct coap_client client = {0};
	uint8_t token[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
	uint8_t etag1[] = {0x01, 0x02};
	uint8_t etag2[] = {0x03, 0x04};
	uint8_t payload[] = "Test payload";
	uint8_t response_buf[256];
	struct coap_packet response;
	int ret;

	k_mutex_init(&client.lock);
	client.fd = 1;

	struct coap_client_internal_request *req = &client.requests[0];
	memset(req, 0, sizeof(*req));
	memcpy(req->request_token, token, sizeof(token));
	req->request_tkl = sizeof(token);
	req->request_ongoing = true;
	req->last_response_id = -1;

	/* Build response with multiple ETags (RFC 7252 §5.10.6.1 violation) */
	ret = coap_packet_init(&response, response_buf, sizeof(response_buf),
			       COAP_VERSION_1, COAP_TYPE_ACK, sizeof(token), token,
			       COAP_RESPONSE_CODE_CONTENT, 0x1234);
	zassert_equal(ret, 0, "Failed to init response");

	ret = coap_packet_append_option(&response, COAP_OPTION_ETAG, etag1, sizeof(etag1));
	zassert_equal(ret, 0, "Failed to add first ETag");

	ret = coap_packet_append_option(&response, COAP_OPTION_ETAG, etag2, sizeof(etag2));
	zassert_equal(ret, 0, "Failed to add second ETag");

	ret = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				     COAP_CONTENT_FORMAT_TEXT_PLAIN);
	zassert_equal(ret, 0, "Failed to add Content-Format");

	int block_opt = (0 << 4) | 0x08 | COAP_BLOCK_64;
	ret = coap_append_option_int(&response, COAP_OPTION_BLOCK2, block_opt);
	zassert_equal(ret, 0, "Failed to add Block2");

	ret = coap_packet_append_payload_marker(&response);
	zassert_equal(ret, 0, "Failed to add payload marker");
	ret = coap_packet_append_payload(&response, payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to add payload");

	ret = coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_equal(ret, -EBADMSG, "Multiple ETags should abort");

	zassert_equal(req->block2_etag_len, 0, "ETag state should be cleared");
	zassert_false(req->request_ongoing, "Request should be released");
}

/* Test: Matching ETag across blocks allows transfer */
ZTEST(coap, test_block2_matching_etag_succeeds)
{
	struct coap_client client = {0};
	uint8_t token[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};
	uint8_t etag[] = {0x01, 0x02, 0x03, 0x04};
	uint8_t payload[] = "Test payload";
	uint8_t response_buf[256];
	struct coap_packet response;
	int ret;

	k_mutex_init(&client.lock);
	client.fd = 1;

	struct coap_client_internal_request *req = &client.requests[0];
	memset(req, 0, sizeof(*req));
	memcpy(req->request_token, token, sizeof(token));
	req->request_tkl = sizeof(token);
	req->request_ongoing = true;
	req->last_response_id = -1;

	/* Inject Block 0 with ETag */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1234,
				    0, true, etag, sizeof(etag),
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 0");

	coap_client_test_inject_response(&client, response.data, response.offset);

	/* Restore state after block 0 */
	setup_block_state(req, token, sizeof(token), etag, sizeof(etag));

	/* Inject Block 1 with same ETag */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1235,
				    1, true, etag, sizeof(etag),
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 1");

	ret = coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_not_equal(ret, -EBADMSG, "Block 1 with matching ETag should not abort");

	req->request_ongoing = true;

	/* Inject Block 2 (last) with same ETag */
	ret = build_block2_response(&response, response_buf, sizeof(response_buf),
				    token, sizeof(token), 0x1236,
				    2, false, etag, sizeof(etag),
				    payload, sizeof(payload));
	zassert_equal(ret, 0, "Failed to build block 2");

	ret = coap_client_test_inject_response(&client, response.data, response.offset);
	zassert_not_equal(ret, -EBADMSG, "Last block with matching ETag should not abort");

	zassert_equal(req->block2_etag_len, 0, "ETag state should be cleared after last block");
}
