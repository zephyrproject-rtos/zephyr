/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <misc/printk.h>

#include <kernel.h>

#include <net/buf.h>
#include <net/net_ip.h>
#include <net/udp.h>

#include <tc_util.h>

#include <net/coap.h>

#define COAP_BUF_SIZE 128
#define COAP_LIMITED_BUF_SIZE 13

#define NUM_PENDINGS 3
#define NUM_OBSERVERS 3
#define NUM_REPLIES 3

#define MAX_PACKET_SIZE 256

static u8_t test_buf[MAX_PACKET_SIZE];
static u8_t test_buf2[MAX_PACKET_SIZE];

static struct coap_pending pendings[NUM_PENDINGS];
static struct coap_observer observers[NUM_OBSERVERS];
static struct coap_reply replies[NUM_REPLIES];

/* This is exposed for this test in subsys/net/lib/coap/coap_link_format.c */
bool _coap_match_path_uri(const char * const *path,
			  const char *uri, u16_t len);

/* Some forward declarations */
static void server_notify_callback(struct coap_resource *resource,
				   struct coap_observer *observer);

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 const struct sockaddr *from);

static const char * const server_resource_1_path[] = { "s", "1", NULL };
static struct coap_resource server_resources[] =  {
	{ .path = server_resource_1_path,
	  .get = server_resource_1_get,
	  .notify = server_notify_callback },
	{ },
};

#define peer_addr { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, \
			0, 0, 0, 0, 0, 0, 0, 0x2 } } }
static struct sockaddr_in6 dummy_addr = {
	.sin6_family = AF_INET6,
	.sin6_addr = peer_addr };

static int test_build_empty_pdu(void)
{
	u8_t result_pdu[] = { 0x40, 0x01, 0x0, 0x0 };
	struct coap_packet cpkt;
	int result = TC_FAIL;
	int r;

	r = coap_packet_init(&cpkt, test_buf, sizeof(test_buf), 1,
			     COAP_TYPE_CON, 0, NULL, COAP_METHOD_GET, 0);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	if (cpkt.fbuf.buf_len != sizeof(result_pdu)) {
		TC_PRINT("Different size from the reference packet\n");
		goto done;
	}

	if (memcmp(result_pdu, cpkt.fbuf.buf, cpkt.fbuf.buf_len)) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

static int test_build_simple_pdu(void)
{
	u8_t result_pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
				 'n', 0xC1, 0x00, 0xFF, 'p', 'a', 'y', 'l',
				 'o', 'a', 'd', 0x00 };
	struct coap_packet cpkt;
	const char token[] = "token";
	u8_t format = 0;
	int result = TC_FAIL;
	int r;

	r = coap_packet_init(&cpkt, test_buf, sizeof(test_buf), 1,
			     COAP_TYPE_NON_CON, strlen(token), (u8_t *)token,
			     COAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED,
			     0x1234);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = coap_packet_append_option(&cpkt, COAP_OPTION_CONTENT_FORMAT,
				      &format, sizeof(format));
	if (r) {
		TC_PRINT("Could not append option\n");
		goto done;
	}

	r = coap_packet_append_payload_marker(&cpkt);
	if (r) {
		TC_PRINT("Failed to set the payload marker\n");
		goto done;
	}

	if (memcmp(result_pdu, cpkt.fbuf.buf, cpkt.fbuf.buf_len)) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

/* No options, No payload */
static int test_parse_empty_pdu(void)
{
	u8_t pdu[] = { 0x40, 0x01, 0, 0 };
	struct coap_packet cpkt;
	u8_t ver, type, code;
	u16_t id;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, pdu, sizeof(pdu), 0, NULL, 0);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	if (ver != 1) {
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

	if (id != 0) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

/* 1 option, No payload (No payload marker) */
static int test_parse_empty_pdu_1(void)
{
	u8_t pdu[] = { 0x40, 0x01, 0, 0, 0x40};
	struct coap_packet cpkt;
	u8_t ver, type, code;
	u16_t id;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, pdu, sizeof(pdu), 0, NULL, 0);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	if (ver != 1) {
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

	if (id != 0) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

static int test_parse_simple_pdu(void)
{
	u8_t pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y', 'l', 'o',
		       'a', 'd', 0x00 };
	struct coap_packet cpkt;
	struct coap_option options[16] = {};
	u8_t ver, type, code, tkl;
	const u8_t token[8];
	u16_t id;
	int result = TC_FAIL;
	int r, count = ARRAY_SIZE(options) - 1;

	r = coap_packet_parse(&cpkt, pdu, sizeof(pdu), 0, NULL, 0);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = coap_header_get_version(&cpkt);
	type = coap_header_get_type(&cpkt);
	code = coap_header_get_code(&cpkt);
	id = coap_header_get_id(&cpkt);

	if (ver != 1) {
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

	tkl = coap_header_get_token(&cpkt, (u8_t *)token);

	if (tkl != 5) {
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

	if (options[0].len != 1) {
		TC_PRINT("Option length doesn't match the reference\n");
		goto done;
	}

	if (((u8_t *)options[0].value)[0] != 0) {
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
	TC_END_RESULT(result);

	return result;
}

static int test_retransmit_second_round(void)
{
	struct coap_packet cpkt, resp;
	struct coap_pending *pending, *resp_pending;
	int result = TC_FAIL;
	int r;
	u16_t id;

	id = coap_next_id();

	r = coap_packet_init(&cpkt, test_buf, sizeof(test_buf), 1,
			     COAP_TYPE_CON, 0, coap_next_token(),
			     COAP_METHOD_GET, id);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		TC_PRINT("No free pending\n");
		goto done;
	}

	r = coap_pending_init(pending, &cpkt, (struct sockaddr *) &dummy_addr);
	if (r) {
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

	r = coap_packet_init(&resp, test_buf2, sizeof(test_buf2), 1,
			     COAP_TYPE_ACK, 0, NULL, COAP_METHOD_GET, id);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	/* Now we get the ack from the remote side */
	resp_pending = coap_pending_received(&resp, pendings, NUM_PENDINGS);
	if (pending != resp_pending) {
		TC_PRINT("Invalid pending %p should be %p\n",
			 resp_pending, pending);
		goto done;
	}

	resp_pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (resp_pending) {
		TC_PRINT("There should be no active pendings\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

static void server_notify_callback(struct coap_resource *resource,
				   struct coap_observer *observer)
{
	coap_remove_observer(resource, observer);

	TC_PRINT("You should see this\n");
}

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 const struct sockaddr *from)
{
	struct coap_packet response;
	struct coap_observer *observer;
	char payload[] = "This is the payload";
	u8_t token[8];
	u8_t tkl;
	u16_t id;
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

	tkl = coap_header_get_token(request, (u8_t *) token);
	id = coap_header_get_id(request);

	coap_observer_init(observer, request, (struct sockaddr *) &dummy_addr);

	coap_register_observer(resource, observer);

	r = coap_packet_init(&response, test_buf2, sizeof(test_buf2), 1,
			     COAP_TYPE_ACK, tkl, token, COAP_RESPONSE_CODE_OK,
			     id);
	if (r < 0) {
		TC_PRINT("Unable to initialize packet.\n");
		return -EINVAL;
	}

	coap_append_option_int(&response, COAP_OPTION_OBSERVE, resource->age);

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		TC_PRINT("Failed to set the payload marker\n");
		return -EINVAL;
	}

	if (coap_packet_append_payload(&response, payload,
				       strlen(payload)) < 0) {
		TC_PRINT("Failed to append the payload\n");
		return -EINVAL;
	}

	resource->user_data = &response.fbuf;

	return 0;
}

static int test_observer_server(void)
{
	u8_t valid_request_pdu[] = {
		0x45, 0x01, 0x12, 0x34,
		't', 'o', 'k', 'e', 'n',
		0x60, /* enable observe option */
		0x51, 's', 0x01, '1', /* path */
	};
	u8_t not_found_request_pdu[] = {
		0x45, 0x01, 0x12, 0x34,
		't', 'o', 'k', 'e', 'n',
		0x60, /* enable observe option */
		0x51, 's', 0x01, '2', /* path */
	};
	struct coap_packet req;
	struct coap_option options[4] = {};
	u8_t opt_num = ARRAY_SIZE(options) - 1;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&req, valid_request_pdu,
			      sizeof(valid_request_pdu), 0, options, opt_num);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr);
	if (r) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}

	/* Suppose some time passes */

	r = coap_resource_notify(&server_resources[0]);
	if (r) {
		TC_PRINT("Could not notify resource\n");
		goto done;
	}

	r = coap_packet_parse(&req, not_found_request_pdu,
			      sizeof(not_found_request_pdu), 0,
			      options, opt_num);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr);
	if (r != -ENOENT) {
		TC_PRINT("There should be no handler for this resource\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

static int resource_reply_cb(struct coap_packet *response,
			     struct coap_reply *reply,
			     const struct sockaddr *from)
{
	TC_PRINT("You should see this\n");

	return 0;
}

static int test_observer_client(void)
{
	struct fbuf_ctx *rsp_fbuf;
	struct coap_packet dummy, req, rsp;
	struct coap_reply *reply;
	struct coap_option options[4] = {};
	const char token[] = "token";
	const char * const *p;
	u8_t opt_num = ARRAY_SIZE(options) - 1;
	int observe = 0;
	int result = TC_FAIL;
	int r;

	r = coap_packet_init(&dummy, test_buf, sizeof(test_buf), 1,
			     COAP_TYPE_CON, strlen(token), (u8_t *)token,
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	/* Enable observing the resource. */
	r = coap_append_option_int(&dummy, COAP_OPTION_OBSERVE, observe);
	if (r < 0) {
		TC_PRINT("Unable to add option to request int.\n");
		goto done;
	}

	for (p = server_resource_1_path; p && *p; p++) {
		r = coap_packet_append_option(&dummy, COAP_OPTION_URI_PATH,
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

	coap_reply_init(reply, &dummy);
	reply->reply = resource_reply_cb;

	/* Server side, not interesting for this test */
	r = coap_packet_parse(&req, dummy.fbuf.buf, dummy.fbuf.buf_len, 0,
			      options, opt_num);
	if (r) {
		TC_PRINT("Could not parse req packet\n");
		goto done;
	}

	r = coap_handle_request(&req, server_resources, options, opt_num,
				(struct sockaddr *) &dummy_addr);
	if (r) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}

	/* We cheat, and communicate using the resource's user_data */
	rsp_fbuf = server_resources[0].user_data;

	/* 'rsp_fbuf' contains the response now */

	r = coap_packet_parse(&rsp, rsp_fbuf->buf, rsp_fbuf->buf_len, 0,
			      options, opt_num);
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
	TC_END_RESULT(result);

	return result;
}

static int test_block_size(void)
{
	struct coap_block_context req_ctx, rsp_ctx;
	struct coap_packet req;
	const char token[] = "token";
	u8_t payload[32] = { 0 };
	int result = TC_FAIL;
	int r;

	r = coap_packet_init(&req, test_buf, sizeof(test_buf), 1, COAP_TYPE_CON,
			     strlen(token), (u8_t *) token,
			     COAP_METHOD_POST, coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	coap_block_transfer_init(&req_ctx, COAP_BLOCK_32, 127);

	r = coap_append_block1_option(&req, &req_ctx);
	if (r) {
		TC_PRINT("Unable to append block1 option\n");
		goto done;
	}

	r = coap_append_size1_option(&req, &req_ctx);
	if (r) {
		TC_PRINT("Unable to append size1 option\n");
		goto done;
	}

	r = coap_packet_append_payload_marker(&req);
	if (r) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(&req, payload,
				       coap_block_size_to_bytes(COAP_BLOCK_32));
	if (r) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	coap_block_transfer_init(&rsp_ctx, COAP_BLOCK_1024, 0);

	r = coap_update_from_block(&req, &rsp_ctx);
	if (r < 0) {
		TC_PRINT("Couldn't parse Block options\n");
		goto done;
	}

	if (rsp_ctx.block_size != COAP_BLOCK_32) {
		TC_PRINT("Couldn't get block size from request\n");
		goto done;
	}

	if (rsp_ctx.current != 0) {
		TC_PRINT("Couldn't get the current block size position\n");
		goto done;
	}

	if (rsp_ctx.total_size != 127) {
		TC_PRINT("Couldn't packet total size from request\n");
		goto done;
	}

	/* Let's try the second packet */
	coap_next_block(&req, &req_ctx);

	r = coap_packet_init(&req, test_buf, sizeof(test_buf), 1, COAP_TYPE_CON,
			     strlen(token), (u8_t *) token,
			     COAP_METHOD_POST, coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	r = coap_append_block1_option(&req, &req_ctx);
	if (r) {
		TC_PRINT("Unable to append block1 option\n");
		goto done;
	}

	r = coap_packet_append_payload_marker(&req);
	if (r) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(&req, payload,
				       coap_block_size_to_bytes(COAP_BLOCK_32));
	if (r) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_update_from_block(&req, &rsp_ctx);
	if (r < 0) {
		TC_PRINT("Couldn't parse Block options\n");
		goto done;
	}

	if (rsp_ctx.block_size != COAP_BLOCK_32) {
		TC_PRINT("Couldn't get block size from request\n");
		goto done;
	}

	if (rsp_ctx.current != coap_block_size_to_bytes(COAP_BLOCK_32)) {
		TC_PRINT("Couldn't get the current block size position\n");
		goto done;
	}

	if (rsp_ctx.total_size != 127) {
		TC_PRINT("Couldn't packet total size from request\n");
		goto done;
	}

	result = TC_PASS;

done:
	TC_END_RESULT(result);

	return result;
}

static int test_block_2_size(void)
{
	struct coap_block_context req_ctx, rsp_ctx;
	struct coap_packet req;
	const char token[] = "token";
	u8_t payload[32] = { 0 };
	int result = TC_FAIL;
	int r;

	r = coap_packet_init(&req, test_buf, sizeof(test_buf), 1, COAP_TYPE_CON,
			     strlen(token), (u8_t *) token,
			     COAP_METHOD_POST, coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	coap_block_transfer_init(&req_ctx, COAP_BLOCK_64, 255);

	r = coap_append_block2_option(&req, &req_ctx);
	if (r) {
		TC_PRINT("Unable to append block2 option\n");
		goto done;
	}

	r = coap_append_size2_option(&req, &req_ctx);
	if (r) {
		TC_PRINT("Unable to append size2 option\n");
		goto done;
	}

	r = coap_packet_append_payload_marker(&req);
	if (r) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(&req, payload,
				       coap_block_size_to_bytes(COAP_BLOCK_64));
	if (r) {
		TC_PRINT("Unable to append payload\n");
		goto done;
	}

	coap_block_transfer_init(&rsp_ctx, COAP_BLOCK_1024, 255);

	r = coap_update_from_block(&req, &rsp_ctx);
	if (r < 0) {
		TC_PRINT("Couldn't parse Block options\n");
		goto done;
	}

	if (rsp_ctx.block_size != COAP_BLOCK_64) {
		TC_PRINT("Couldn't get block size from request\n");
		goto done;
	}

	if (rsp_ctx.current != 0) {
		TC_PRINT("Couldn't get the current block size position\n");
		goto done;
	}

	if (rsp_ctx.total_size != 255) {
		TC_PRINT("Couldn't packet total size from request\n");
		goto done;
	}

	/* Let's try the second packet */
	coap_next_block(&req, &req_ctx);

	r = coap_packet_init(&req, test_buf, sizeof(test_buf), 1, COAP_TYPE_CON,
			     strlen(token), (u8_t *) token,
			     COAP_METHOD_POST, coap_next_id());
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	r = coap_append_block2_option(&req, &req_ctx);
	if (r) {
		TC_PRINT("Unable to append block2 option\n");
		goto done;
	}

	r = coap_packet_append_payload_marker(&req);
	if (r) {
		TC_PRINT("Unable to append payload marker\n");
		goto done;
	}

	r = coap_packet_append_payload(&req, payload,
				       coap_block_size_to_bytes(COAP_BLOCK_32));
	if (r) {
		TC_PRINT("Unable to append payload\n");
		goto done;
	}

	r = coap_update_from_block(&req, &rsp_ctx);
	if (r < 0) {
		TC_PRINT("Couldn't parse Block options\n");
		goto done;
	}

	if (rsp_ctx.block_size != COAP_BLOCK_64) {
		TC_PRINT("Couldn't get block size from request\n");
		goto done;
	}

	if (rsp_ctx.current != coap_block_size_to_bytes(COAP_BLOCK_64)) {
		TC_PRINT("Couldn't get the current block size position\n");
		goto done;
	}

	if (rsp_ctx.total_size != 255) {
		TC_PRINT("Couldn't packet total size from request\n");
		goto done;
	}

	result = TC_PASS;

done:
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

static int test_parse_malformed_opt(void)
{
	u8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xD0 };
	struct coap_packet cpkt;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, opt, sizeof(opt), 0, NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt_len(void)
{
	u8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xC1 };
	struct coap_packet cpkt;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, opt, sizeof(opt), 0, NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt_ext(void)
{
	u8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xE0, 0x01 };
	struct coap_packet cpkt;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, opt, sizeof(opt), 0, NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

	TC_END_RESULT(result);

	return result;
}

static int test_parse_malformed_opt_len_ext(void)
{
	u8_t opt[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e', 'n',
		       0xEE, 0x01, 0x02, 0x01};
	struct coap_packet cpkt;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, opt, sizeof(opt), 0, NULL, 0);
	if (r < 0) {
		result = TC_PASS;
	}

	TC_END_RESULT(result);

	return result;
}

/* 1 option, No payload (with payload marker) */
static int test_parse_malformed_marker(void)
{
	u8_t pdu[] = { 0x40, 0x01, 0, 0, 0x40, 0xFF};
	struct coap_packet cpkt;
	int result = TC_FAIL;
	int r;

	r = coap_packet_parse(&cpkt, pdu, sizeof(pdu), 0, NULL, 0);
	if (r) {
		result = TC_PASS;
	}

	TC_END_RESULT(result);

	return result;
}


static const struct {
	const char *name;
	int (*func)(void);
} tests[] = {
	{ "Build empty PDU test", test_build_empty_pdu, },
	{ "Build simple PDU test", test_build_simple_pdu, },
	{ "Parse emtpy PDU test", test_parse_empty_pdu, },
	{ "Parse empty PDU test no marker", test_parse_empty_pdu_1, },
	{ "Parse simple PDU test", test_parse_simple_pdu, },
	{ "Test retransmission", test_retransmit_second_round, },
	{ "Test observer server", test_observer_server, },
	{ "Test observer client", test_observer_client, },
	{ "Test block sized transfer", test_block_size, },
	{ "Test block sized 2 transfer", test_block_2_size, },
	{ "Test match path uri", test_match_path_uri, },
	{ "Parse malformed option", test_parse_malformed_opt },
	{ "Parse malformed option length", test_parse_malformed_opt_len },
	{ "Parse malformed option ext", test_parse_malformed_opt_ext },
	{ "Parse malformed option length ext",
		test_parse_malformed_opt_len_ext },
	{ "Parse malformed empty payload with marker",
		test_parse_malformed_marker, },
};

int main(int argc, char *argv[])
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

	return 0;
}
