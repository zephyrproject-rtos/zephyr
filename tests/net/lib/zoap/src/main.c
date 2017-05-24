/*
 * Copyright (c) 2016 Intel Corporation
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
#include <net/net_pkt.h>
#include <net/net_ip.h>

#include <tc_util.h>

#include <net/zoap.h>

#define ZOAP_BUF_SIZE 128
#define ZOAP_LIMITED_BUF_SIZE 13

#define NUM_PENDINGS 3
#define NUM_OBSERVERS 3
#define NUM_REPLIES 3

NET_PKT_TX_SLAB_DEFINE(zoap_pkt_slab, 4);

NET_BUF_POOL_DEFINE(zoap_data_pool, 4, ZOAP_BUF_SIZE, 0, NULL);

NET_BUF_POOL_DEFINE(zoap_limited_data_pool, 4, ZOAP_LIMITED_BUF_SIZE, 0, NULL);

static struct zoap_pending pendings[NUM_PENDINGS];
static struct zoap_observer observers[NUM_OBSERVERS];
static struct zoap_reply replies[NUM_REPLIES];

/* This is exposed for this test in subsys/net/lib/zoap/zoap_link_format.c */
bool _zoap_match_path_uri(const char * const *path,
			  const char *uri, u16_t len);

/* Some forward declarations */
static void server_notify_callback(struct zoap_resource *resource,
				   struct zoap_observer *observer);

static int server_resource_1_get(struct zoap_resource *resource,
				 struct zoap_packet *request,
				 const struct sockaddr *from);

static const char * const server_resource_1_path[] = { "s", "1", NULL };
static struct zoap_resource server_resources[] =  {
	{ .path = server_resource_1_path,
	  .get = server_resource_1_get,
	  .notify = server_notify_callback },
	{ },
};

#define MY_PORT 12345
static struct sockaddr_in6 dummy_addr = {
	.sin6_family = AF_INET6,
	.sin6_addr = IN6ADDR_LOOPBACK_INIT };

static int test_build_empty_pdu(void)
{
	u8_t result_pdu[] = { 0x40, 0x01, 0x0, 0x0 };
	struct zoap_packet zpkt;
	struct net_pkt *pkt;
	struct net_buf *frag;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&zpkt, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&zpkt, 1);
	zoap_header_set_type(&zpkt, ZOAP_TYPE_CON);
	zoap_header_set_code(&zpkt, ZOAP_METHOD_GET);
	zoap_header_set_id(&zpkt, 0);

	if (frag->len != sizeof(result_pdu)) {
		TC_PRINT("Different size from the reference packet\n");
		goto done;
	}

	if (memcmp(result_pdu, frag->data, frag->len)) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);

	TC_END_RESULT(result);

	return result;
}

static int test_build_simple_pdu(void)
{
	u8_t result_pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
				 'n', 0xC1, 0x00, 0xFF, 'p', 'a', 'y', 'l',
				 'o', 'a', 'd', 0x00 };
	struct zoap_packet zpkt;
	struct net_pkt *pkt;
	struct net_buf *frag;
	const char token[] = "token";
	u8_t *appdata, payload[] = "payload";
	u16_t buflen;
	u8_t format = 0;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&zpkt, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&zpkt, 1);
	zoap_header_set_type(&zpkt, ZOAP_TYPE_NON_CON);
	zoap_header_set_code(&zpkt,
			      ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED);
	zoap_header_set_id(&zpkt, 0x1234);

	r = zoap_header_set_token(&zpkt, (u8_t *)token, strlen(token));
	if (r) {
		TC_PRINT("Could not set token\n");
		goto done;
	}

	r = zoap_add_option(&zpkt, ZOAP_OPTION_CONTENT_FORMAT,
			     &format, sizeof(format));
	if (r) {
		TC_PRINT("Could not add option\n");
		goto done;
	}

	appdata = zoap_packet_get_payload(&zpkt, &buflen);
	if (!appdata || buflen <= sizeof(payload)) {
		TC_PRINT("Not enough space to insert payload\n");
		goto done;
	}

	if (buflen != (ZOAP_BUF_SIZE - 4 - strlen(token) - 2 - 1)) {
		/*
		 * The remaining length will be the buffer size less
		 * 4: basic CoAP header
		 * strlen(token): token length
		 * 2: options (content-format)
		 * 1: payload marker (added by zoap_packet_get_payload())
		 */
		TC_PRINT("Invalid packet length\n");
		goto done;
	}

	memcpy(appdata, payload, sizeof(payload));

	r = zoap_packet_set_used(&zpkt, sizeof(payload));
	if (r) {
		TC_PRINT("Failed to set the amount of bytes used\n");
		goto done;
	}

	if (frag->len != sizeof(result_pdu)) {
		TC_PRINT("Different size from the reference packet\n");
		goto done;
	}

	if (memcmp(result_pdu, frag->data, frag->len)) {
		TC_PRINT("Built packet doesn't match reference packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);

	TC_END_RESULT(result);

	return result;
}

static int test_build_no_size_for_options(void)
{
	struct zoap_packet zpkt;
	struct net_pkt *pkt;
	struct net_buf *frag;
	const char token[] = "token";
	u8_t format = 0;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_limited_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&zpkt, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	zoap_header_set_version(&zpkt, 1);
	zoap_header_set_type(&zpkt, ZOAP_TYPE_NON_CON);
	zoap_header_set_code(&zpkt,
			      ZOAP_RESPONSE_CODE_PROXYING_NOT_SUPPORTED);
	zoap_header_set_id(&zpkt, 0x1234);

	r = zoap_header_set_token(&zpkt, (u8_t *)token, strlen(token));
	if (r) {
		TC_PRINT("Could not set token\n");
		goto done;
	}

	/* There won't be enough space for the option value */
	r = zoap_add_option(&zpkt, ZOAP_OPTION_CONTENT_FORMAT,
			     &format, sizeof(format));
	if (!r) {
		TC_PRINT("Shouldn't have added the option, not enough space\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_empty_pdu(void)
{
	u8_t pdu[] = { 0x40, 0x01, 0, 0 };
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct zoap_packet zpkt;
	u8_t ver, type, code;
	u16_t id;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	memcpy(frag->data, pdu, sizeof(pdu));
	frag->len = sizeof(pdu);

	r = zoap_packet_parse(&zpkt, pkt);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = zoap_header_get_version(&zpkt);
	type = zoap_header_get_type(&zpkt);
	code = zoap_header_get_code(&zpkt);
	id = zoap_header_get_id(&zpkt);

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
	net_pkt_unref(pkt);

	TC_END_RESULT(result);

	return result;
}

static int test_parse_simple_pdu(void)
{
	u8_t pdu[] = { 0x55, 0xA5, 0x12, 0x34, 't', 'o', 'k', 'e',
			  'n',  0x00, 0xc1, 0x00, 0xff, 'p', 'a', 'y',
			  'l', 'o', 'a', 'd', 0x00 };
	struct zoap_packet zpkt;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct zoap_option options[16];
	u8_t ver, type, code, tkl;
	const u8_t *token, *payload;
	u16_t id, len;
	int result = TC_FAIL;
	int r, count = 16;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	memcpy(frag->data, pdu, sizeof(pdu));
	frag->len = sizeof(pdu);

	r = zoap_packet_parse(&zpkt, pkt);
	if (r) {
		TC_PRINT("Could not parse packet\n");
		goto done;
	}

	ver = zoap_header_get_version(&zpkt);
	type = zoap_header_get_type(&zpkt);
	code = zoap_header_get_code(&zpkt);
	id = zoap_header_get_id(&zpkt);

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

	token = zoap_header_get_token(&zpkt, &tkl);

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

	count = zoap_find_options(&zpkt, ZOAP_OPTION_CONTENT_FORMAT,
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
	count = zoap_find_options(&zpkt, ZOAP_OPTION_ETAG, options, count);
	if (count) {
		TC_PRINT("There shouldn't be any ETAG option in the packet\n");
		goto done;
	}

	payload = zoap_packet_get_payload(&zpkt, &len);
	if (!payload || !len) {
		TC_PRINT("There should be a payload in the packet\n");
		goto done;
	}

	if (len != (strlen("payload") + 1)) {
		TC_PRINT("Invalid payload in the packet\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);

	TC_END_RESULT(result);

	return result;
}

static int test_retransmit_second_round(void)
{
	struct zoap_packet zpkt, resp;
	struct zoap_pending *pending, *resp_pending;
	struct net_pkt *pkt, *resp_pkt = NULL;
	struct net_buf *frag;
	int result = TC_FAIL;
	int r;
	u16_t id;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&zpkt, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	id = zoap_next_id();

	zoap_header_set_version(&zpkt, 1);
	zoap_header_set_type(&zpkt, ZOAP_TYPE_CON);
	zoap_header_set_code(&zpkt, ZOAP_METHOD_GET);
	zoap_header_set_id(&zpkt, id);

	pending = zoap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		TC_PRINT("No free pending\n");
		goto done;
	}

	r = zoap_pending_init(pending, &zpkt, (struct sockaddr *) &dummy_addr);
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

	resp_pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!resp_pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(resp_pkt, frag);

	r = zoap_packet_init(&resp, resp_pkt);
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
	net_pkt_unref(pkt);
	if (resp_pkt) {
		net_pkt_unref(resp_pkt);
	}

	TC_END_RESULT(result);

	return result;
}

static bool ipaddr_cmp(const struct sockaddr *a, const struct sockaddr *b)
{
	if (a->family != b->family) {
		return false;
	}

	if (a->family == AF_INET6) {
		return net_ipv6_addr_cmp(&net_sin6(a)->sin6_addr,
					 &net_sin6(b)->sin6_addr);
	} else if (a->family == AF_INET) {
		return net_ipv4_addr_cmp(&net_sin(a)->sin_addr,
					 &net_sin(b)->sin_addr);
	}

	return false;
}

static void server_notify_callback(struct zoap_resource *resource,
				   struct zoap_observer *observer)
{
	if (!ipaddr_cmp(&observer->addr,
			(const struct sockaddr *) &dummy_addr)) {
		TC_ERROR("The address of the observer doesn't match.\n");
		return;
	}

	zoap_remove_observer(resource, observer);

	TC_PRINT("You should see this\n");
}

static int server_resource_1_get(struct zoap_resource *resource,
				 struct zoap_packet *request,
				 const struct sockaddr *from)
{
	struct zoap_packet response;
	struct zoap_observer *observer;
	struct net_pkt *pkt;
	struct net_buf *frag;
	char payload[] = "This is the payload";
	const u8_t *token;
	u8_t tkl, *p;
	u16_t id, len;
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

	zoap_observer_init(observer, request, from);

	zoap_register_observer(resource, observer);

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		return -ENOMEM;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&response, pkt);
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

	resource->user_data = pkt;

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
	struct zoap_packet req;
	struct net_pkt *pkt;
	struct net_buf *frag;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	memcpy(frag->data, valid_request_pdu, sizeof(valid_request_pdu));
	frag->len = sizeof(valid_request_pdu);

	r = zoap_packet_parse(&req, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = zoap_handle_request(&req, server_resources,
				(const struct sockaddr *) &dummy_addr);
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

	net_pkt_unref(pkt);

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	memcpy(frag->data, not_found_request_pdu,
	       sizeof(not_found_request_pdu));
	frag->len = sizeof(not_found_request_pdu);

	r = zoap_packet_parse(&req, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = zoap_handle_request(&req, server_resources,
				(const struct sockaddr *) &dummy_addr);
	if (r != -ENOENT) {
		TC_PRINT("There should be no handler for this resource\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);

	TC_END_RESULT(result);

	return result;
}

static int resource_reply_cb(const struct zoap_packet *response,
			     struct zoap_reply *reply,
			     const struct sockaddr *from)
{
	TC_PRINT("You should see this\n");

	return 0;
}

static int test_observer_client(void)
{
	struct zoap_packet req, rsp;
	struct zoap_reply *reply;
	struct net_pkt *pkt, *rsp_pkt = NULL;
	struct net_buf *frag;
	const char token[] = "rndtoken";
	const char * const *p;
	int observe = 0;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&req, pkt);
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
			      (const u8_t *) token, strlen(token));

	/* Enable observing the resource. */
	r = zoap_add_option_int(&req, ZOAP_OPTION_OBSERVE,
				observe);
	if (r < 0) {
		TC_PRINT("Unable to add option to request.\n");
		goto done;
	}

	for (p = server_resource_1_path; p && *p; p++) {
		r = zoap_add_option(&req, ZOAP_OPTION_URI_PATH,
				    *p, strlen(*p));
		if (r < 0) {
			TC_PRINT("Unable to add option to request.\n");
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
	r = zoap_packet_parse(&req, pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	r = zoap_handle_request(&req, server_resources,
				(const struct sockaddr *) &dummy_addr);
	if (r) {
		TC_PRINT("Could not handle packet\n");
		goto done;
	}

	/* We cheat, and communicate using the resource's user_data */
	rsp_pkt = server_resources[0].user_data;

	/* The uninteresting part ends here */

	/* 'rsp_pkt' contains the response now */

	r = zoap_packet_parse(&rsp, rsp_pkt);
	if (r) {
		TC_PRINT("Could not initialize packet\n");
		goto done;
	}

	reply = zoap_response_received(&rsp,
				       (const struct sockaddr *) &dummy_addr,
				       replies, NUM_REPLIES);
	if (!reply) {
		TC_PRINT("Couldn't find a matching waiting reply\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);
	if (rsp_pkt) {
		net_pkt_unref(rsp_pkt);
	}

	TC_END_RESULT(result);

	return result;
}

static int test_block_size(void)
{
	struct zoap_block_context req_ctx, rsp_ctx;
	struct zoap_packet req;
	struct net_pkt *pkt = NULL;
	struct net_buf *frag;
	const char token[] = "rndtoken";
	u8_t *payload;
	u16_t len;
	int result = TC_FAIL;
	int r;

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&req, pkt);
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
			      (const u8_t *) token, strlen(token));

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

	/* Suppose that pkt was sent */
	net_pkt_unref(pkt);

	/* Let's try the second packet */
	zoap_next_block(&req_ctx);

	pkt = net_pkt_get_reserve(&zoap_pkt_slab, 0, K_NO_WAIT);
	if (!pkt) {
		TC_PRINT("Could not get packet from pool\n");
		goto done;
	}

	frag = net_buf_alloc(&zoap_data_pool, K_NO_WAIT);
	if (!frag) {
		TC_PRINT("Could not get buffer from pool\n");
		goto done;
	}

	net_pkt_frag_add(pkt, frag);

	r = zoap_packet_init(&req, pkt);
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
			      (const u8_t *) token, strlen(token));

	zoap_add_block1_option(&req, &req_ctx);

	memset(payload, 0xFE, ZOAP_BLOCK_32);

	zoap_packet_set_used(&req, zoap_block_size_to_bytes(ZOAP_BLOCK_32));

	r = zoap_update_from_block(&req, &rsp_ctx);
	if (r < 0) {
		TC_PRINT("Couldn't parse Block options\n");
		goto done;
	}

	if (rsp_ctx.block_size != ZOAP_BLOCK_32) {
		TC_PRINT("Couldn't get block size from request\n");
		goto done;
	}

	if (rsp_ctx.current != zoap_block_size_to_bytes(ZOAP_BLOCK_32)) {
		TC_PRINT("Couldn't get the current block size position\n");
		goto done;
	}

	if (rsp_ctx.total_size != 127) {
		TC_PRINT("Couldn't packet total size from request\n");
		goto done;
	}

	result = TC_PASS;

done:
	net_pkt_unref(pkt);

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
	if (_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/s";
	if (!_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar";
	if (!_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar2";
	if (_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar*";
	if (!_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/foobar3*";
	if (!_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	uri = "/devnull*";
	if (_zoap_match_path_uri(resource_path, uri, strlen(uri))) {
		TC_PRINT("Matching %s failed\n", uri);
		goto out;
	}

	result = TC_PASS;

out:
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
	{ "Test match path uri", test_match_path_uri, },
};

int main(int argc, char *argv[])
{
	int count, pass, result;

	TC_START("Test Zoap CoAP PDU parsing and building");

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
