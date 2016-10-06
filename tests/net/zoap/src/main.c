/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <misc/printk.h>

#include <nanokernel.h>

#include <net/buf.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>
#include <net/net_core.h>

#include <tc_util.h>

#include <zoap.h>

#define ZOAP_BUF_SIZE 128
#define ZOAP_LIMITED_BUF_SIZE 9

#define NUM_PENDINGS 3
#define NUM_OBSERVERS 3
#define NUM_REPLIES 3

static struct nano_fifo zoap_fifo;
static NET_BUF_POOL(zoap_pool, 2, ZOAP_BUF_SIZE,
		    &zoap_fifo, NULL, sizeof(struct ip_buf));

static struct nano_fifo zoap_incoming_fifo;
static NET_BUF_POOL(zoap_incoming_pool, 1, ZOAP_BUF_SIZE,
		    &zoap_incoming_fifo, NULL, sizeof(struct ip_buf));

static struct nano_fifo zoap_limited_fifo;
static NET_BUF_POOL(zoap_limited_pool, 1, ZOAP_LIMITED_BUF_SIZE,
		    &zoap_limited_fifo, NULL, sizeof(struct ip_buf));

static struct zoap_pending pendings[NUM_PENDINGS];
static struct zoap_observer observers[NUM_OBSERVERS];
static struct zoap_reply replies[NUM_REPLIES];

/* Some forward declarations */
static void server_notify_callback(struct zoap_resource *resource,
				   struct zoap_observer *observer);
static int server_resource_1_get(struct zoap_resource *resource,
				struct zoap_packet *request,
				const uip_ipaddr_t *addr,
				uint16_t port);

static const char * const server_resource_1_path[] = { "s", "1", NULL };
static struct zoap_resource server_resources[] =  {
	{ .path = server_resource_1_path,
	  .get = server_resource_1_get,
	  .notify = server_notify_callback },
	{ },
};

#define MY_PORT 12345
static uip_ipaddr_t dummy_addr;

static int test_build_empty_pdu(void)
{
	uint8_t result_pdu[] = { 0x40, 0x01, 0x0, 0x0 };
	struct zoap_packet pkt;
	struct net_buf *buf;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_CON);
	zoap_header_set_code(&pkt, ZOAP_METHOD_GET);
	zoap_header_set_id(&pkt, 0);

	if (ip_buf_appdatalen(buf) != sizeof(result_pdu)) {
		TC_PRINT("Failed to build packet\n");
		goto done;
	}

	if (memcmp(result_pdu, ip_buf_appdata(buf), ip_buf_appdatalen(buf))) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_build_simple_pdu(void)
{
	uint8_t result_pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
				 'n', 0xC1, 0x00, 0xFF, 'p', 'a', 'y', 'l',
				 'o', 'a', 'd', 0x00 };
	struct zoap_packet pkt;
	struct net_buf *buf;
	const char token[] = "token";
	uint8_t *appdata, payload[] = "payload";
	uint16_t buflen;
	uint8_t format = 0;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_NON_CON);
	zoap_header_set_code(&pkt,
			      ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED);
	zoap_header_set_id(&pkt, 0x1234);

	r = zoap_header_set_token(&pkt, (uint8_t *)token, strlen(token));
	if (r) {
		TC_PRINT("Could not set token\n");
		goto done;
	}

	r = zoap_add_option(&pkt, ZOAP_OPTION_CONTENT_FORMAT,
			     &format, sizeof(format));
	if (r) {
		TC_PRINT("Could not add option\n");
		goto done;
	}

	appdata = zoap_packet_get_payload(&pkt, &buflen);
	if (!buf || buflen <= sizeof(payload)) {
		TC_PRINT("Not enough space to insert payload\n");
		goto done;
	}

	memcpy(appdata, payload, sizeof(payload));

	r = zoap_packet_set_used(&pkt, sizeof(payload));
	if (r) {
		TC_PRINT("Failed to set the amount of bytes used\n");
		goto done;
	}

	if (ip_buf_appdatalen(buf) != sizeof(result_pdu)) {
		TC_PRINT("Different size from the reference packet\n");
		goto done;
	}

	if (memcmp(result_pdu, ip_buf_appdata(buf), ip_buf_appdatalen(buf))) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_build_no_size_for_options(void)
{
	struct zoap_packet pkt;
	struct net_buf *buf;
	const char token[] = "token";
	uint8_t format = 0;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_limited_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_NON_CON);
	zoap_header_set_code(&pkt,
			      ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED);
	zoap_header_set_id(&pkt, 0x1234);

	r = zoap_header_set_token(&pkt, (uint8_t *)token, strlen(token));
	if (r) {
		TC_PRINT("Could not set token\n");
		goto done;
	}

	/* There won't be enough space for the option value */
	r = zoap_add_option(&pkt, ZOAP_OPTION_CONTENT_FORMAT,
			     &format, sizeof(format));
	if (!r) {
		TC_PRINT("Shouldn't have added the option, not enough space\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_empty_pdu(void)
{
	uint8_t pdu[] = { 0x40, 0x01, 0, 0 };
	struct net_buf *buf;
	struct zoap_packet pkt;
	uint8_t ver, type, code;
	uint16_t id;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	memcpy(ip_buf_appdata(buf), pdu, sizeof(pdu));
	ip_buf_appdatalen(buf) = sizeof(pdu);

	r = zoap_packet_parse(&pkt, buf);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = zoap_header_get_version(&pkt);
	type = zoap_header_get_type(&pkt);
	code = zoap_header_get_code(&pkt);
	id = zoap_header_get_id(&pkt);

	if (ver != 1) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != ZOAP_TYPE_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != ZOAP_METHOD_GET) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_simple_pdu(void)
{
	uint8_t pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
			  'n',  0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y',
			  'l', 'o', 'a', 'd', 0x00 };
	struct zoap_packet pkt;
	struct net_buf *buf;
	struct zoap_option options[16];
	uint8_t ver, type, code, tkl;
	const uint8_t *token;
	uint16_t id;
	int result = TC_FAIL;
	int r, count = 16;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	memcpy(ip_buf_appdata(buf), pdu, sizeof(pdu));
	ip_buf_appdatalen(buf) = sizeof(pdu);

	r = zoap_packet_parse(&pkt, buf);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = zoap_header_get_version(&pkt);
	type = zoap_header_get_type(&pkt);
	code = zoap_header_get_code(&pkt);
	id = zoap_header_get_id(&pkt);

	if (ver != 1) {
		TC_PRINT("Invalid version for parsed packet\n");
		goto done;
	}

	if (type != ZOAP_TYPE_NON_CON) {
		TC_PRINT("Packet type doesn't match reference\n");
		goto done;
	}

	if (code != ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED) {
		TC_PRINT("Packet code doesn't match reference\n");
		goto done;
	}

	if (id != 0x1234) {
		TC_PRINT("Packet id doesn't match reference\n");
		goto done;
	}

	token = zoap_header_get_token(&pkt, &tkl);

	if (!token) {
		TC_PRINT("Couldn't extract token from packet\n");
		goto done;
	}

	if (tkl != 5) {
		TC_PRINT("Token length doesn't match reference\n");
		goto done;
	}

	if (memcmp(token, "token", tkl)) {
		TC_PRINT("Token value doesn't match the reference\n");
		goto done;
	}

	count = zoap_find_options(&pkt, ZOAP_OPTION_CONTENT_FORMAT,
				   options, count);
	if (count != 1) {
		TC_PRINT("Unexpected number of options in the packet\n");
		goto done;
	}

	if (options[0].len != 1) {
		TC_PRINT("Option length doesn't match the reference\n");
		goto done;
	}

	if (((uint8_t *)options[0].value)[0] != 0) {
		TC_PRINT("Option value doesn't match the reference\n");
		goto done;
	}

	/* Not existent */
	count = zoap_find_options(&pkt, ZOAP_OPTION_ETAG, options, count);
	if (count) {
		TC_PRINT("There shouldn't be any ETAG option in the packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_retransmit_second_round(void)
{
	struct zoap_packet pkt, resp;
	struct zoap_pending *pending, *resp_pending;
	struct net_buf *buf, *resp_buf = NULL;
	uint16_t id;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&pkt, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	id = zoap_next_id();

	zoap_header_set_version(&pkt, 1);
	zoap_header_set_type(&pkt, ZOAP_TYPE_CON);
	zoap_header_set_code(&pkt, ZOAP_METHOD_GET);
	zoap_header_set_id(&pkt, id);

	pending = zoap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		TC_PRINT("No free pending\n");
		goto done;
	}

	r = zoap_pending_init(pending, &pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	/* We "send" the packet the first time here */
	if (!zoap_pending_cycle(pending)) {
		TC_PRINT("Pending expired too early\n");
		goto done;
	}

	/* We simulate that the first transmission got lost */

	if (!zoap_pending_cycle(pending)) {
		TC_PRINT("Pending expired too early\n");
		goto done;
	}

	resp_buf = net_buf_get(&zoap_fifo, 0);
	if (!resp_buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(resp_buf) = net_buf_tail(resp_buf);
	ip_buf_appdatalen(resp_buf) = net_buf_tailroom(resp_buf);

	r = zoap_packet_init(&resp, resp_buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&resp, 1);
	zoap_header_set_type(&resp, ZOAP_TYPE_ACK);
	zoap_header_set_id(&resp, id); /* So it matches the request */

	/* Now we get the ack from the remote side */
	resp_pending = zoap_pending_received(&resp, pendings, NUM_PENDINGS);
	if (pending != resp_pending) {
		TC_PRINT("Invalid pending %p should be %p\n",
			 resp_pending, pending);
		goto done;
	}

	resp_pending = zoap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (resp_pending) {
		TC_PRINT("There should be no active pendings\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);
	if (resp_buf) {
		net_buf_unref(resp_buf);
	}

	TC_END_RESULT(result);

	return result;
}


static void server_notify_callback(struct zoap_resource *resource,
				   struct zoap_observer *observer)
{
	if (!uip_ipaddr_cmp(&observer->addr, &dummy_addr)) {
		TC_ERROR("The address of the observer doesn't match.\n");
		return;
	}

	if (observer->port != MY_PORT) {
		TC_ERROR("The port of the observer doesn't match.\n");
		return;
	}

	zoap_remove_observer(resource, observer);

	TC_PRINT("You should see this\n");
}

static int server_resource_1_get(struct zoap_resource *resource,
				 struct zoap_packet *request,
				 const uip_ipaddr_t *addr,
				 uint16_t port)
{
	struct zoap_packet response;
	struct zoap_observer *observer;
	struct net_buf *buf = resource->user_data;
	char payload[] = "This is the payload";
	const uint8_t *token;
	uint8_t tkl, *p;
	uint16_t id, len;
	int r;

	if (!zoap_request_is_observe(request)) {
		TC_PRINT("The request should enable observing\n");
		return -EINVAL;
	}

	observer = zoap_observer_next_unused(observers, NUM_OBSERVERS);
	if (!observer) {
		TC_PRINT("There should be an available observer.\n");
		return -EINVAL;
	}

	token = zoap_header_get_token(request, &tkl);
	id = zoap_header_get_id(request);

	zoap_observer_init(observer, request, addr, port);

	zoap_register_observer(resource, observer);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		TC_PRINT("Unable to initialize packet.\n");
		return -EINVAL;
	}

	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_OK);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	zoap_add_option_int(&response, ZOAP_OPTION_OBSERVE,
			    resource->age);

	p = zoap_packet_get_payload(&response, &len);
	memcpy(p, payload, sizeof(payload));

	r = zoap_packet_set_used(&response, sizeof(payload));
	if (r < 0) {
		TC_PRINT("Not enough room for payload.\n");
		return -EINVAL;
	}

	return 0;
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
	struct zoap_packet req;
	struct net_buf *buf, *rsp_buf = NULL;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	memcpy(ip_buf_appdata(buf), valid_request_pdu,
	       sizeof(valid_request_pdu));
	ip_buf_appdatalen(buf) = sizeof(valid_request_pdu);

	r = zoap_packet_parse(&req, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	rsp_buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(rsp_buf) = net_buf_tail(rsp_buf);
	ip_buf_appdatalen(rsp_buf) = net_buf_tailroom(rsp_buf);

	server_resources[0].user_data = rsp_buf;

	r = zoap_handle_request(&req, server_resources, &dummy_addr, MY_PORT);
	if (r) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}

	/* Suppose some time passes */

	r = zoap_resource_notify(&server_resources[0]);
	if (r) {
		TC_PRINT("Could not notify resource\n");
		goto done;
	}

	/* Everything worked fine, let's try something else */
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	memcpy(ip_buf_appdata(buf), not_found_request_pdu,
	       sizeof(not_found_request_pdu));
	ip_buf_appdatalen(buf) = sizeof(not_found_request_pdu);

	r = zoap_packet_parse(&req, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = zoap_handle_request(&req, server_resources, &dummy_addr, MY_PORT);
	if (r != -ENOENT) {
		TC_PRINT("There should be no handler for this resource\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);
	if (rsp_buf) {
		net_buf_unref(rsp_buf);
	}

	TC_END_RESULT(result);

	return result;
}

static int resource_reply_cb(const struct zoap_packet *response,
			     struct zoap_reply *reply,
			     const uip_ipaddr_t *addr,
			     uint16_t port)
{
	return 0;
}

static int test_observer_client(void)
{
	struct zoap_packet req, rsp;
	struct zoap_reply *reply;
	struct net_buf *buf, *rsp_buf = NULL;
	const char token[] = "rndtoken";
	const char * const *p;
	int observe = 0;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&req, buf);
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&req, 1);
	zoap_header_set_type(&req, ZOAP_TYPE_CON);
	zoap_header_set_code(&req, ZOAP_METHOD_GET);
	zoap_header_set_id(&req, zoap_next_id());
	zoap_header_set_token(&req,
			      (const uint8_t *) token, strlen(token));

	/* Enable observing the resource. */
	r = zoap_add_option(&req, ZOAP_OPTION_OBSERVE,
			    &observe, sizeof(observe));
	if (r < 0) {
		TC_PRINT("Unable add option to request.\n");
		goto done;
	}

	for (p = server_resource_1_path; p && *p; p++) {
		r = zoap_add_option(&req, ZOAP_OPTION_URI_PATH,
				     *p, strlen(*p));
		if (r < 0) {
			TC_PRINT("Unable add option to request.\n");
			goto done;
		}
	}

	reply = zoap_reply_next_unused(replies, NUM_REPLIES);
	if (!reply) {
		printk("No resources for waiting for replies.\n");
		goto done;
	}

	zoap_reply_init(reply, &req);
	reply->reply = resource_reply_cb;

	/* Server side, not interesting for this test */
	r = zoap_packet_parse(&req, buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	rsp_buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(rsp_buf) = net_buf_tail(rsp_buf);
	ip_buf_appdatalen(rsp_buf) = net_buf_tailroom(rsp_buf);

	server_resources[0].user_data = rsp_buf;

	r = zoap_handle_request(&req, server_resources, &dummy_addr, MY_PORT);
	if (r) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}
	/* The uninteresting part ends here */

	/* 'rsp_buf' contains the response now */

	r = zoap_packet_parse(&rsp, rsp_buf);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	reply = zoap_response_received(&rsp, &dummy_addr, MY_PORT,
				       replies, NUM_REPLIES);
	if (!reply) {
		TC_PRINT("Couldn't find a matching waiting reply\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static int test_block_size(void)
{
	struct zoap_block_context req_ctx, rsp_ctx;
	struct zoap_packet req;
	struct net_buf *buf = NULL;
	const char token[] = "rndtoken";
	uint8_t *payload;
	uint16_t len;
	int result = TC_FAIL;
	int r;

	buf = net_buf_get(&zoap_fifo, 0);
	if (!buf) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}
	ip_buf_appdata(buf) = net_buf_tail(buf);
	ip_buf_appdatalen(buf) = net_buf_tailroom(buf);

	r = zoap_packet_init(&req, buf);
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	zoap_block_transfer_init(&req_ctx, ZOAP_BLOCK_32, 127);

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&req, 1);
	zoap_header_set_type(&req, ZOAP_TYPE_CON);
	zoap_header_set_code(&req, ZOAP_METHOD_POST);
	zoap_header_set_id(&req, zoap_next_id());
	zoap_header_set_token(&req,
			      (const uint8_t *) token, strlen(token));

	zoap_add_block1_option(&req, &req_ctx);
	zoap_add_size1_option(&req, &req_ctx);

	payload = zoap_packet_get_payload(&req, &len);
	if (!payload) {
		TC_PRINT("There's no space for payload in the packet\n");
		goto done;
	}

	memset(payload, 0xFE, zoap_block_size_to_bytes(ZOAP_BLOCK_32));

	zoap_packet_set_used(&req, zoap_block_size_to_bytes(ZOAP_BLOCK_32));

	zoap_block_transfer_init(&rsp_ctx, ZOAP_BLOCK_1024, 0);

	r = zoap_update_from_block(&req, &rsp_ctx);
	if (r < 0) {
		TC_PRINT("Couldn't parse Block options\n");
		goto done;
	}

	if (rsp_ctx.block_size != ZOAP_BLOCK_32) {
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
	zoap_next_block(&req_ctx);

	r = zoap_packet_init(&req, buf);
	if (r < 0) {
		TC_PRINT("Unable to initialize request\n");
		goto done;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&req, 1);
	zoap_header_set_type(&req, ZOAP_TYPE_CON);
	zoap_header_set_code(&req, ZOAP_METHOD_POST);
	zoap_header_set_id(&req, zoap_next_id());
	zoap_header_set_token(&req,
			      (const uint8_t *) token, strlen(token));

	zoap_add_block1_option(&req, &req_ctx);

	memset(payload, 0xFE, ZOAP_BLOCK_32);

	zoap_packet_set_used(&req, ZOAP_BLOCK_32);

	zoap_update_from_block(&req, &rsp_ctx);

	if (rsp_ctx.block_size != ZOAP_BLOCK_32) {
		TC_PRINT("Couldn't get block size from request\n");
		goto done;
	}

	if (rsp_ctx.current != zoap_block_size_to_bytes(ZOAP_BLOCK_32)) {
		TC_PRINT("Couldn't get the current block size position\n");
		goto done;
	}

	if (rsp_ctx.total_size != 127) {
		TC_PRINT("[2] Couldn't packet total size from request\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_buf_unref(buf);

	TC_END_RESULT(result);

	return result;
}

static const struct {
	const char *name;
	int (*func)(void);
} tests[] = {
	{ "Build empty PDU test", test_build_empty_pdu, },
	{ "Build simple PDU test", test_build_simple_pdu, },
	{ "No size for options test", test_build_no_size_for_options, },
	{ "Parse emtpy PDU test", test_parse_empty_pdu, },
	{ "Parse simple PDU test", test_parse_simple_pdu, },
	{ "Test retransmission", test_retransmit_second_round, },
	{ "Test observer server", test_observer_server, },
	{ "Test observer client", test_observer_client, },
	{ "Test block sized transfer", test_block_size, },
};

int main(int argc, char *argv[])
{
	int count, pass, result;

	TC_START("Test Zoap CoAP PDU parsing and building");

	net_buf_pool_init(zoap_pool);
	net_buf_pool_init(zoap_limited_pool);
	net_buf_pool_init(zoap_incoming_pool);

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
