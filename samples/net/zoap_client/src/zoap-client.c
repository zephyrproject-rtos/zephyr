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

#include <misc/nano_work.h>
#include <net/net_core.h>
#include <net/net_socket.h>
#include <net/net_ip.h>
#include <net/ip_buf.h>

#include <net_testing.h>

#include <zoap.h>

#define MY_COAP_PORT 9998

#define STACKSIZE 2000

#define NUM_PENDINGS 3
#define NUM_REPLIES 3

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

char fiberStack[STACKSIZE];

static struct net_context *send_context, *receive_context;

struct zoap_pending pendings[NUM_PENDINGS];
struct zoap_reply replies[NUM_REPLIES];
struct nano_delayed_work retransmit_work;

static const char * const a_light_path[] = { "a", "light", NULL };

static void msg_dump(const char *s, uint8_t *data, unsigned len)
{
	unsigned i;

	printf("%s: ", s);
	for (i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("(%u bytes)\n", len);
}

static int resource_reply_cb(const struct zoap_packet *response,
			     struct zoap_reply *reply, const void *from)
{
	struct net_buf *buf = response->buf;

	msg_dump("reply", buf->data, buf->len);

	return 0;
}

static void udp_receive(void)
{
	struct zoap_packet response;
	struct zoap_pending *pending;
	struct zoap_reply *reply;
	struct net_buf *buf;
	int r;

	while (true) {
		buf = net_receive(receive_context, TICKS_UNLIMITED);
		if (!buf) {
			continue;
		}

		r = zoap_packet_parse(&response, buf);
		if (r < 0) {
			printf("Invalid data received (%d)\n", r);
			continue;
		}

		pending = zoap_pending_received(&response, pendings,
						NUM_PENDINGS);
		if (pending) {
			/* If necessary cancel retransmissions */
		}

		reply = zoap_response_received(&response, buf,
					       replies, NUM_REPLIES);
		if (!reply) {
			printf("No handler for response (%d)\n", r);
			continue;
		}
	}
}

static void retransmit_request(struct nano_work *work)
{
	struct zoap_pending *pending;
	struct zoap_packet *request;
	struct net_buf *buf;
	int r, timeout;

	pending = zoap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	request = &pending->request;
	buf = request->buf;

	r = net_send(buf);
	if (r < 0) {
		return;
	}

	if (!zoap_pending_cycle(pending)) {
		net_buf_unref(buf);
		zoap_pending_clear(pending);
		return;
	}

	timeout = pending->timeout * (sys_clock_ticks_per_sec / MSEC_PER_SEC);
	nano_delayed_work_submit(&retransmit_work, timeout);
}

void main(void)
{
	static struct net_addr mcast_addr = {
		.in6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.family = AF_INET6 };
	static struct net_addr any_addr = { .in6_addr = IN6ADDR_ANY_INIT,
					   .family = AF_INET6 };
	struct zoap_packet request;
	struct zoap_pending *pending;
	struct zoap_reply *reply;
	const char * const *p;
	struct net_buf *buf;
	int r, timeout;
	uint8_t observe = 0;

	net_init();
	net_testing_setup();

	send_context = net_context_get(IPPROTO_UDP,
				  &mcast_addr, MY_COAP_PORT,
				  &any_addr, MY_COAP_PORT);

	receive_context = net_context_get(IPPROTO_UDP,
				  &any_addr, MY_COAP_PORT,
				  &mcast_addr, MY_COAP_PORT);

	task_fiber_start(&fiberStack[0], STACKSIZE,
			(nano_fiber_entry_t) udp_receive, 0, 0, 7, 0);

	nano_delayed_work_init(&retransmit_work, retransmit_request);

	buf = ip_buf_get_tx(send_context);
	if (!buf) {
		printk("Unable to get TX buffer, not enough memory.\n");
		return;
	}

	r = zoap_packet_init(&request, buf);
	if (r < 0) {
		return;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&request, 1);
	zoap_header_set_type(&request, ZOAP_TYPE_CON);
	zoap_header_set_code(&request, ZOAP_METHOD_GET);
	zoap_header_set_id(&request, zoap_next_id());

	/* Enable observing the resource. */
	r = zoap_add_option(&request, ZOAP_OPTION_OBSERVE,
			    &observe, sizeof(observe));
	if (r < 0) {
		printk("Unable add option to request.\n");
		return;
	}

	for (p = a_light_path; p && *p; p++) {
		r = zoap_add_option(&request, ZOAP_OPTION_URI_PATH,
				     *p, strlen(*p));
		if (r < 0) {
			printk("Unable add option to request.\n");
			return;
		}
	}

	pending = zoap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		printk("Unable to find a free pending to track "
		       "retransmissions.\n");
		return;
	}

	r = zoap_pending_init(pending, &request);
	if (r < 0) {
		printk("Unable to initialize a pending retransmission.\n");
		return;
	}

	reply = zoap_reply_next_unused(replies, NUM_REPLIES);
	if (!reply) {
		printk("No resources for waiting for replies.\n");
		return;
	}

	zoap_reply_init(reply, &request);
	reply->reply = resource_reply_cb;

	r = net_send(buf);
	if (r < 0) {
		printk("Error sending the packet (%d).\n", r);
	}

	zoap_pending_cycle(pending);
	timeout = pending->timeout * (sys_clock_ticks_per_sec / MSEC_PER_SEC);

	nano_delayed_work_submit(&retransmit_work, timeout);
}
