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
#include <stdio.h>

#include <zephyr.h>

#include <net/net_core.h>
#include <net/net_socket.h>
#include <net/ip_buf.h>
#include <net/net_ip.h>

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#endif

#include <zoap.h>

#define MY_COAP_PORT 5683

#define STACKSIZE 2000

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

char fiberStack[STACKSIZE];

static struct net_context *context;

static int test_get(struct zoap_resource *resource,
		       struct zoap_packet *request,
		       const void *from)
{
	struct net_context *ctx = (struct net_context *) from;
	struct net_buf *buf;
	struct zoap_packet response;
	uint8_t *payload, code, type;
	uint16_t len, id;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		printf("packet without payload");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);

	/* Re-using the request buffer for the response */
	buf = request->buf;

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_OK);
	zoap_header_set_id(&response, id);

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintf(payload, len, "Type: %u\nCode: %u\nMID: %u\n",
		     type, code, id);
	if (r < 0 || r > len) {
		return -EINVAL;
	}

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	return net_reply(ctx, buf);
}

static const char * const test_path[] = { "test", NULL };

static struct zoap_resource resources[] = {
	{ .get = test_get,
	  .path = test_path },
	{ },
};

static void udp_receive(void)
{
	struct net_buf *buf;
	struct zoap_packet request;
	int r;

	while (true) {
		buf = net_receive(context, TICKS_UNLIMITED);
		if (!buf) {
			continue;
		}

		r = zoap_packet_parse(&request, buf);
		if (r < 0) {
			printf("Invalid data received (%d)\n", r);
			continue;
		}

		r = zoap_handle_request(&request, resources, context);
		if (r < 0) {
			printf("No handler for such request (%d)\n", r);
			continue;
		}
	}
}

void main(void)
{
	static struct net_addr mcast_addr = {
		.in6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.family = AF_INET6 };
	static struct net_addr any_addr = { .in6_addr = IN6ADDR_ANY_INIT,
					    .family = AF_INET6 };

	net_init();

#if defined(CONFIG_NET_TESTING)
	net_testing_setup();
#endif

	context = net_context_get(IPPROTO_UDP, &any_addr, 0,
				  &mcast_addr, MY_COAP_PORT);

	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t) udp_receive, 0, 0, 7, 0);
}
