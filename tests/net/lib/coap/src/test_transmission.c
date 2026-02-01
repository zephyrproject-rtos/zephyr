/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_test, LOG_LEVEL_DBG);

#include "test_common.h"


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
