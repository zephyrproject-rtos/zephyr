/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


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

	r = coap_packet_parse(&rsp, rsp_data, COAP_BUF_SIZE, options, opt_num);
	zassert_equal(r, 0, "Could not parse rsp packet");

	reply = coap_response_received(&rsp,
				       (const struct net_sockaddr *) &dummy_addr,
				       replies, NUM_REPLIES);
	zassert_not_null(reply, "Couldn't find a matching waiting reply");
}
