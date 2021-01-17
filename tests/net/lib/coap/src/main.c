/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_DBG);

#include <errno.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/printk.h>
#include <kernel.h>

#include <net/coap.h>

#include <tc_util.h>

#include "net_private.h"

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

static int test_build_empty_pdu(void)
{
	uint8_t result_pdu[] = { 0x40, 0x01, 0x0, 0x0 };
	struct coap_packet cpkt;
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE,
			     1, COAP_TYPE_CON, 0, NULL,
			     COAP_METHOD_GET, 0);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	if (cpkt.offset != sizeof(result_pdu)) {
		TC_PRINT("Different size from the reference packet\n");
		goto done;
	}

	if (memcmp(result_pdu, cpkt.data, cpkt.offset)) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_build_simple_pdu(void)
{
	uint8_t result_pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
				 'n', 0xC0, 0xFF, 'p', 'a', 'y', 'l',
				 'o', 'a', 'd', 0x00 };
	struct coap_packet cpkt;
	const char token[] = "token";
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE,
			     1, COAP_TYPE_NON_CON,
			     strlen(token), (uint8_t *)token,
			     COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED,
			     0x1234);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = coap_append_option_int(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		TC_PRINT("Could not append option\n");
		goto done;
	}

	r = coap_packet_append_payload_marker(&cpkt);
	if (r < 0) {
		TC_PRINT("Failed to set the payload marker\n");
		goto done;
	}

	if (memcmp(result_pdu, cpkt.data, cpkt.offset)) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

/* No options, No payload */
static int test_parse_empty_pdu(void)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0 };
	struct coap_packet cpkt;
	uint8_t *data;
	uint8_t ver;
	uint8_t type;
	uint8_t code;
	uint16_t id;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	if (ver != 1U) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != COAP_TYPE_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != COAP_METHOD_GET) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0U) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

/* 1 option, No payload (No payload marker) */
static int test_parse_empty_pdu_1(void)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0, 0x40};
	struct coap_packet cpkt;
	uint8_t *data;
	uint8_t ver;
	uint8_t type;
	uint8_t code;
	uint16_t id;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	if (ver != 1U) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != COAP_TYPE_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != COAP_METHOD_GET) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0U) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_simple_pdu(void)
{
	uint8_t pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y', 'l', 'o',
		       'a', 'd', 0x00 };
	struct coap_packet cpkt;
	struct coap_option options[16] = {};
	const uint8_t token[8];
	uint8_t *data;
	uint8_t ver;
	uint8_t type;
	uint8_t code;
	uint8_t tkl;
	uint16_t id;
	int result = TC_FAIL;
	int r, count = ARRAY_SIZE(options) - 1;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	if (ver != 1U) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != COAP_TYPE_NON_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0x1234) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	tkl = coap_header_get_token(&cpkt, (uint8_t *)token);

	if (tkl != 5U) {
		TC_PRINT("Token length doesn't match reference\n");
		goto done;
	}

	if (memcmp(token, "token", tkl)) {
		TC_PRINT("Token value doesn't match the reference\n");
		goto done;
	}

	count = coap_find_options(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				   options, count);
	if (count != 1) {
		TC_PRINT("Unexpected number of options in the packet\n");
		goto done;
	}

	if (options[0].len != 1U) {
		TC_PRINT("Option length doesn't match the reference\n");
		goto done;
	}

	if (((uint8_t *)options[0].value)[0] !=
			COAP_CONTENT_FORMAT_TEXT_PLAIN) {
		TC_PRINT("Option value doesn't match the reference\n");
		goto done;
	}

	/* Not existent */
	count = coap_find_options(&cpkt, COAP_OPTION_ETAG, options, count);
	if (count) {
		TC_PRINT("There shouldn't be any ETAG option in the packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt(void)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xD0 };
	struct coap_packet cpkt;
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt_len(void)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xC1 };
	struct coap_packet cpkt;
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt_ext(void)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xE0, 0x01 };
	struct coap_packet cpkt;
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt_len_ext(void)
{
	uint8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xEE, 0x01, 0x02, 0x01};
	struct coap_packet cpkt;
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, opt, sizeof(opt));

	r = coap_packet_parse(&cpkt, data, sizeof(opt), NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

/* 1 option, No payload (with payload marker) */
static int test_parse_malformed_marker(void)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0, 0x40, 0xFF};
	struct coap_packet cpkt;
	uint8_t *data;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		goto done;
	}

	memcpy(data, pdu, sizeof(pdu));

	r = coap_packet_parse(&cpkt, data, sizeof(pdu), NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int test_match_path_uri(void)
{
	int result = TC_FAIL;
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

	uri = "/k";
	if (_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/s";
	if (!_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar";
	if (!_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar2";
	if (_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar*";
	if (!_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar3*";
	if (!_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/devnull*";
	if (_coap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	result = TC_PASS;

out:
	TC_END_RESULT(result);

	return result;

}

#define BLOCK_WISE_TRANSFER_SIZE_GET 128

static int prepare_block1_request(struct coap_packet *req,
				  struct coap_block_context *req_ctx,
				  int *more)
{
	const char token[] = "token";
	uint8_t payload[32] = { 0 };
	uint8_t *data;
	bool first;
	int r;

	/* Request Context */
	if (req_ctx->total_size == 0) {
		first = true;
		coap_block_transfer_init(req_ctx, COAP_BLOCK_32,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	} else {
		first = false;
	}

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	r = coap_packet_init(req, data, COAP_BUF_SIZE, 1,
			     COAP_TYPE_CON, strlen(token),
			     (uint8_t *) token, COAP_METHOD_POST,
			     coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	r = coap_append_block1_option(req, req_ctx);
	if (r < 0) {
		TC_PRINT("Unable to append block1 option\n");
		goto done;
	}

	if (first) {
		r = coap_append_size1_option(req, req_ctx);
		if (r < 0) {
			TC_PRINT("Unable to append size1 option\n");
			goto done;
		}
	}

	r = coap_packet_append_payload_marker(req);
	if (r < 0) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(req, payload,
				       coap_block_size_to_bytes(COAP_BLOCK_32));
	if (r < 0) {
		TC_PRINT("Unable to append payload\n");
		goto done;
	}

	*more = coap_next_block(req, req_ctx);

	return 0;
done:
	return -EINVAL;
}

static int prepare_block1_response(struct coap_packet *rsp,
				   struct coap_block_context *rsp_ctx,
				   struct coap_packet *req)
{
	uint8_t token[8];
	uint8_t *data;
	uint16_t id;
	uint8_t tkl;
	int r;

	if (rsp_ctx->total_size == 0) {
		coap_block_transfer_init(rsp_ctx, COAP_BLOCK_32,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	r = coap_update_from_block(req, rsp_ctx);
	if (r < 0) {
		return -EINVAL;
	}

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	id = coap_header_get_id(req);
	tkl = coap_header_get_token(req, token);

	r = coap_packet_init(rsp, data, COAP_BUF_SIZE, 1,
			     COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CREATED, id);
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	r = coap_append_block1_option(rsp, rsp_ctx);
	if (r < 0) {
		TC_PRINT("Unable to append block1 option\n");
		goto done;
	}

	return 0;

done:
	return -EINVAL;
}

static int test_block1_request(struct coap_block_context *req_ctx, uint8_t iter)
{
	int result = TC_FAIL;

	if (req_ctx->block_size != COAP_BLOCK_32) {
		TC_PRINT("req:%d,Couldn't get block size\n", iter);
		goto done;
	}

	/* In last iteration "current" field not updated */
	if (iter < 4) {
		if (req_ctx->current !=
		    coap_block_size_to_bytes(COAP_BLOCK_32) * iter) {
			TC_PRINT("req:%d,Couldn't get the current block "
				 "position\n", iter);
			goto done;
		}
	} else {
		if (req_ctx->current !=
		    coap_block_size_to_bytes(COAP_BLOCK_32) * (iter - 1U)) {
			TC_PRINT("req:%d,Couldn't get the current block "
				 "position\n", iter);
			goto done;
		}
	}

	if (req_ctx->total_size != BLOCK_WISE_TRANSFER_SIZE_GET) {
		TC_PRINT("req:%d,Couldn't packet total size\n", iter);
		goto done;
	}

	result = TC_PASS;

done:
	return result;
}

static int test_block1_response(struct coap_block_context *rsp_ctx, uint8_t iter)
{
	int result = TC_FAIL;

	if (rsp_ctx->block_size != COAP_BLOCK_32) {
		TC_PRINT("rsp:%d,Couldn't get block size\n", iter);
		goto done;
	}

	if (rsp_ctx->current !=
	    coap_block_size_to_bytes(COAP_BLOCK_32) * (iter - 1U)) {
		TC_PRINT("rsp:%d, Couldn't get the current block position\n",
			 iter);
		goto done;
	}

	if (rsp_ctx->total_size != BLOCK_WISE_TRANSFER_SIZE_GET) {
		TC_PRINT("rsp:%d, Couldn't packet total size\n", iter);
		goto done;
	}

	result = TC_PASS;

done:
	return result;
}

static int test_block1_size(void)
{
	struct coap_block_context req_ctx;
	struct coap_block_context rsp_ctx;
	struct coap_packet req;
	struct coap_packet rsp;
	int result;
	int more;
	int r;
	uint8_t i;

	i = 0U;
	result = TC_FAIL;
	more = 1;
	memset(&req_ctx, 0, sizeof(req_ctx));
	memset(&rsp_ctx, 0, sizeof(rsp_ctx));

	while (more) {
		r = prepare_block1_request(&req, &req_ctx, &more);
		if (r < 0) {
			result = TC_FAIL;
			goto done;
		}

		r = prepare_block1_response(&rsp, &rsp_ctx, &req);
		if (r < 0) {
			result = TC_FAIL;
			goto done;
		}

		k_free(req.data);
		k_free(rsp.data);

		i++;

		result = test_block1_request(&req_ctx, i);
		if (result == TC_FAIL) {
			goto done;
		}

		result = test_block1_response(&rsp_ctx, i);
		if (result == TC_FAIL) {
			goto done;
		}

	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

#define BLOCK2_WISE_TRANSFER_SIZE_GET 256

static int prepare_block2_request(struct coap_packet *req,
				  struct coap_block_context *req_ctx,
				  struct coap_packet *rsp)
{
	const char token[] = "token";
	uint8_t *data;
	int r;

	/* Request Context */
	if (req_ctx->total_size == 0) {
		coap_block_transfer_init(req_ctx, COAP_BLOCK_64,
					 BLOCK2_WISE_TRANSFER_SIZE_GET);
	} else {
		coap_next_block(rsp, req_ctx);
	}

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	r = coap_packet_init(req, data, COAP_BUF_SIZE, 1,
			     COAP_TYPE_CON, strlen(token),
			     (uint8_t *) token, COAP_METHOD_GET,
			     coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	r = coap_append_block2_option(req, req_ctx);
	if (r < 0) {
		TC_PRINT("Unable to append block1 option\n");
		goto done;
	}

	return 0;
done:
	return -EINVAL;
}

static int prepare_block2_response(struct coap_packet *rsp,
				   struct coap_block_context *rsp_ctx,
				   struct coap_packet *req,
				   int *more)
{
	uint8_t payload[64];
	uint8_t token[8];
	uint8_t *data;
	uint16_t id;
	uint8_t tkl;
	bool first;
	int r;

	if (rsp_ctx->total_size == 0) {
		first = true;
		coap_block_transfer_init(rsp_ctx, COAP_BLOCK_64,
					 BLOCK2_WISE_TRANSFER_SIZE_GET);
	} else {
		first = false;
	}

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	id = coap_header_get_id(req);
	tkl = coap_header_get_token(req, token);

	r = coap_packet_init(rsp, data, COAP_BUF_SIZE, 1,
			     COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	r = coap_append_block2_option(rsp, rsp_ctx);
	if (r < 0) {
		TC_PRINT("Unable to append block2 option\n");
		goto done;
	}

	if (first) {
		r = coap_append_size2_option(rsp, rsp_ctx);
		if (r < 0) {
			TC_PRINT("Unable to append size2 option\n");
			goto done;
		}
	}

	r = coap_packet_append_payload_marker(rsp);
	if (r < 0) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(rsp, payload,
				       coap_block_size_to_bytes(COAP_BLOCK_64));
	if (r < 0) {
		TC_PRINT("Unable to append payload\n");
		goto done;
	}

	*more = coap_next_block(rsp, rsp_ctx);

	return 0;

done:
	return -EINVAL;
}

static int test_block2_request(struct coap_block_context *req_ctx, uint8_t iter)
{
	int result = TC_FAIL;

	if (req_ctx->block_size != COAP_BLOCK_64) {
		TC_PRINT("req:%d,Couldn't get block size\n", iter);
		goto done;
	}

	if (req_ctx->current !=
	    coap_block_size_to_bytes(COAP_BLOCK_64) * (iter - 1U)) {
		TC_PRINT("req:%d, Couldn't get the current block position\n",
			 iter);
		goto done;
	}

	if (req_ctx->total_size != BLOCK2_WISE_TRANSFER_SIZE_GET) {
		TC_PRINT("req:%d,Couldn't packet total size\n", iter);
		goto done;
	}

	result = TC_PASS;

done:
	return result;
}

static int test_block2_response(struct coap_block_context *rsp_ctx, uint8_t iter)
{
	int result = TC_FAIL;

	if (rsp_ctx->block_size != COAP_BLOCK_64) {
		TC_PRINT("rsp:%d,Couldn't get block size\n", iter);
		goto done;
	}

	/* In last iteration "current" field not updated */
	if (iter < 4) {
		if (rsp_ctx->current !=
		    coap_block_size_to_bytes(COAP_BLOCK_64) * iter) {
			TC_PRINT("req:%d,Couldn't get the current block "
				 "position\n", iter);
			goto done;
		}
	} else {
		if (rsp_ctx->current !=
		    coap_block_size_to_bytes(COAP_BLOCK_64) * (iter - 1U)) {
			TC_PRINT("req:%d,Couldn't get the current block "
				 "position\n", iter);
			goto done;
		}
	}

	if (rsp_ctx->total_size != BLOCK2_WISE_TRANSFER_SIZE_GET) {
		TC_PRINT("rsp:%d, Couldn't packet total size\n", iter);
		goto done;
	}

	result = TC_PASS;

done:
	return result;
}

static int test_block2_size(void)
{
	struct coap_block_context req_ctx;
	struct coap_block_context rsp_ctx;
	struct coap_packet req;
	struct coap_packet rsp;
	int result;
	int more;
	int r;
	uint8_t i;

	i = 0U;
	result = TC_FAIL;
	more = 1;
	memset(&req_ctx, 0, sizeof(req_ctx));
	memset(&rsp_ctx, 0, sizeof(rsp_ctx));

	while (more) {
		r = prepare_block2_request(&req, &req_ctx, &rsp);
		if (r < 0) {
			result = TC_FAIL;
			goto done;
		}

		if (i != 0U) {
			k_free(rsp.data);
		}

		r = prepare_block2_response(&rsp, &rsp_ctx, &req, &more);
		if (r < 0) {
			result = TC_FAIL;
			goto done;
		}

		k_free(req.data);

		i++;

		result = test_block2_request(&req_ctx, i);
		if (result == TC_FAIL) {
			goto done;
		}

		result = test_block2_response(&rsp_ctx, i);
		if (result == TC_FAIL) {
			goto done;
		}

	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

static int test_retransmit_second_round(void)
{
	struct coap_packet cpkt;
	struct coap_packet rsp;
	struct coap_pending *pending;
	struct coap_pending *rsp_pending;
	uint8_t *data = NULL;
	uint8_t *rsp_data = NULL;
	int result = TC_FAIL;
	int r;
	uint16_t id;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	id = coap_next_id();

	r = coap_packet_init(&cpkt, data, COAP_BUF_SIZE,
			     1, COAP_TYPE_CON, 0, coap_next_token(),
			     COAP_METHOD_GET, id);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		TC_PRINT("No free pending\n");
		goto done;
	}

	r = coap_pending_init(pending, &cpkt, (struct sockaddr *) &dummy_addr,
			      COAP_DEFAULT_MAX_RETRANSMIT);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	/* We "send" the packet the first time here */
	if (!coap_pending_cycle(pending)) {
		TC_PRINT("Pending expired too early\n");
		goto done;
	}

	/* We simulate that the first transmission got lost */
	if (!coap_pending_cycle(pending)) {
		TC_PRINT("Pending expired too early\n");
		goto done;
	}

	rsp_data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!rsp_data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	r = coap_packet_init(&rsp, rsp_data, COAP_BUF_SIZE,
			     1, COAP_TYPE_ACK, 0, NULL,
			     COAP_METHOD_GET, id);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	/* Now we get the ack from the remote side */
	rsp_pending = coap_pending_received(&rsp, pendings, NUM_PENDINGS);
	if (pending != rsp_pending) {
		TC_PRINT("Invalid pending %p should be %p\n",
			 rsp_pending, pending);
		goto done;
	}

	k_free(rsp_pending->data);
	coap_pending_clear(rsp_pending);

	rsp_pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (rsp_pending) {
		TC_PRINT("There should be no active pendings\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(rsp_data);

	TC_END_RESULT(result);

	return result;
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
	if (!ipaddr_cmp(&observer->addr,
			(const struct sockaddr *) &dummy_addr)) {
		TC_ERROR("The address of the observer doesn't match.\n");
		return;
	}

	coap_remove_observer(resource, observer);

	TC_PRINT("You should see this\n");
}

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	struct coap_observer *observer;
	uint8_t *data;
	char payload[] = "This is the payload";
	uint8_t token[8];
	uint8_t tkl;
	uint16_t id;
	int r;

	if (!coap_request_is_observe(request)) {
		TC_PRINT("The request should enable observing\n");
		return -EINVAL;
	}

	observer = coap_observer_next_unused(observers, NUM_OBSERVERS);
	if (!observer) {
		TC_PRINT("There should be an available observer.\n");
		return -EINVAL;
	}

	tkl = coap_header_get_token(request, (uint8_t *) token);
	id = coap_header_get_id(request);

	coap_observer_init(observer, request, addr);

	coap_register_observer(resource, observer);

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		return -EINVAL;
	}

	r = coap_packet_init(&response, data, COAP_BUF_SIZE,
			     1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_OK, id);
	if (r < 0) {
		TC_PRINT("Unable to initialize packet.\n");
		goto done;
	}

	coap_append_option_int(&response, COAP_OPTION_OBSERVE, resource->age);

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		TC_PRINT("Failed to set the payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		TC_PRINT("Unable to append payload\n");
		goto done;
	}

	resource->user_data = data;

	return 0;

done:
	k_free(data);

	return -EINVAL;
}

static int test_observer_server(void)
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
	uint8_t *data;
	uint8_t opt_num = ARRAY_SIZE(options) - 1;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	memcpy(data, valid_request_pdu, sizeof(valid_request_pdu));

	r = coap_packet_parse(&req, data, sizeof(valid_request_pdu),
			      options, opt_num);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	if (r < 0) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}

	/* Suppose some time passes */
	r = coap_resource_notify(&server_resources[0]);
	if (r) {
		TC_PRINT("Could not notify resource\n");
		goto done;
	}

	k_free(data);

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	memcpy(data, not_found_request_pdu, sizeof(not_found_request_pdu));

	r = coap_packet_parse(&req, data, sizeof(not_found_request_pdu),
			      options, opt_num);
	if (r < 0) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	if (r != -ENOENT) {
		TC_PRINT("There should be no handler for this resource\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);

	TC_END_RESULT(result);

	return result;
}

static int resource_reply_cb(const struct coap_packet *response,
			     struct coap_reply *reply,
			     const struct sockaddr *from)
{
	TC_PRINT("You should see this\n");

	return 0;
}

static int test_observer_client(void)
{
	struct coap_packet req;
	struct coap_packet rsp;
	struct coap_reply *reply;
	struct coap_option options[4] = {};
	const char token[] = "token";
	const char * const *p;
	uint8_t *data = NULL;
	uint8_t *rsp_data = NULL;
	uint8_t opt_num = ARRAY_SIZE(options) - 1;
	int observe = 0;
	int result = TC_FAIL;
	int r;

	data = (uint8_t *)k_malloc(COAP_BUF_SIZE);
	if (!data) {
		TC_PRINT("Unable to allocate memory for req");
		goto done;
	}

	r = coap_packet_init(&req, data, COAP_BUF_SIZE,
			     1, COAP_TYPE_CON,
			     strlen(token), (uint8_t *)token,
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	/* Enable observing the resource. */
	r = coap_append_option_int(&req, COAP_OPTION_OBSERVE, observe);
	if (r < 0) {
		TC_PRINT("Unable to add option to request int.\n");
		goto done;
	}

	for (p = server_resource_1_path; p && *p; p++) {
		r = coap_packet_append_option(&req, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			TC_PRINT("Unable to add option to request.\n");
			goto done;
		}
	}

	reply = coap_reply_next_unused(replies, NUM_REPLIES);
	if (!reply) {
		TC_PRINT("No resources for waiting for replies.\n");
		goto done;
	}

	coap_reply_init(reply, &req);
	reply->reply = resource_reply_cb;

	/* Server side, not interesting for this test */
	r = coap_packet_parse(&req, data, req.offset, options, opt_num);
	if (r < 0) {
		TC_PRINT("Could not parse req packet\n");
		goto done;
	}

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr,
				sizeof(dummy_addr));
	if (r < 0) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}

	/* We cheat, and communicate using the resource's user_data */
	rsp_data = server_resources[0].user_data;

	/* 'rsp_pkt' contains the response now */

	r = coap_packet_parse(&rsp, rsp_data, req.offset, options, opt_num);
	if (r) {
		TC_PRINT("Could not parse rsp packet\n");
		goto done;
	}

	reply = coap_response_received(&rsp,
				       (const struct sockaddr *) &dummy_addr,
				       replies, NUM_REPLIES);
	if (!reply) {
		TC_PRINT("Couldn't find a matching waiting reply\n");
		goto done;
	}

	result = TC_PASS;

done:
	k_free(data);
	k_free(rsp_data);

	TC_END_RESULT(result);

	return result;
}

static const struct {
	const char *name;
	int (*func)(void);
} tests[] = {
	{ "Build empty PDU test", test_build_empty_pdu, },
	{ "Build simple PDU test", test_build_simple_pdu, },
	{ "Parse empty PDU test", test_parse_empty_pdu, },
	{ "Parse empty PDU test no marker", test_parse_empty_pdu_1, },
	{ "Parse simple PDU test", test_parse_simple_pdu, },
	{ "Parse malformed option", test_parse_malformed_opt },
	{ "Parse malformed option length", test_parse_malformed_opt_len },
	{ "Parse malformed option ext", test_parse_malformed_opt_ext },
	{ "Parse malformed option length ext",
		test_parse_malformed_opt_len_ext },
	{ "Parse malformed empty payload with marker",
		test_parse_malformed_marker, },
	{ "Test match path uri", test_match_path_uri, },
	{ "Test block sized 1 transfer", test_block1_size, },
	{ "Test block sized 2 transfer", test_block2_size, },
	{ "Test retransmission", test_retransmit_second_round, },
	{ "Test observer server", test_observer_server, },
	{ "Test observer client", test_observer_client, },
};

void main(void)
{
	int count, pass, result;

	TC_START("Test CoAP PDU parsing and building");

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		if (tests[count].func() == TC_PASS) {
			pass++;
		}
	}

	TC_PRINT("%d / %d tests passed\n", pass, count);

	result = pass == count ? TC_PASS : TC_FAIL;

	TC_END_REPORT(result);
}
