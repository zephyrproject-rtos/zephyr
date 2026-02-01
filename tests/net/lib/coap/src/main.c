/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, LOG_LEVEL_DBG);

#include "test_common.h"

/* Shared data definitions */
struct coap_pending pendings[NUM_PENDINGS];
struct coap_observer observers[NUM_OBSERVERS];
struct coap_reply replies[NUM_REPLIES];

struct net_sockaddr_in6 dummy_addr = {
	.sin6_family = NET_AF_INET6,
	.sin6_addr = peer_addr
};

uint8_t data_buf[2][COAP_BUF_SIZE];

/* Forward declarations for callbacks */
static void server_resource_1_callback(struct coap_resource *resource,
				       struct coap_observer *observer);

static void server_resource_2_callback(struct coap_resource *resource,
				       struct coap_observer *observer);

static int server_resource_1_get(struct coap_resource *resource,
				 struct coap_packet *request,
				 struct net_sockaddr *addr, net_socklen_t addr_len);

/* Server resources */
const char * const server_resource_1_path[] = { "s", "1", NULL };
const char *const server_resource_2_path[] = { "s", "2", NULL };
struct coap_resource server_resources[] = {
	{ .path = server_resource_1_path,
	  .get = server_resource_1_get,
	  .notify = server_resource_1_callback },
	{ .path = server_resource_2_path,
	  .get = server_resource_1_get, /* Get can be shared with the first resource */
	  .notify = server_resource_2_callback },
	{ },
};

/* Shared helper functions */
bool ipaddr_cmp(const struct net_sockaddr *a, const struct net_sockaddr *b)
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

/* Main test suite */
ZTEST_SUITE(coap, NULL, NULL, NULL, NULL, NULL);
