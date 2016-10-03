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

#include <misc/byteorder.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_ip.h>

#if defined(CONFIG_NET_TESTING)
#include <net_testing.h>
#endif

#include <zoap.h>

#define MY_COAP_PORT 5683

#define NUM_PENDINGS 3
#define NUM_REPLIES 3

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

static const struct net_addr mcast_addr = {
	.in6_addr = ALL_NODES_LOCAL_COAP_MCAST,
	.family = AF_INET6 };

static struct net_context *context;

struct zoap_pending pendings[NUM_PENDINGS];
struct zoap_reply replies[NUM_REPLIES];
struct nano_delayed_work retransmit_work;

static const char * const test_path[] = { "test", NULL };

static void msg_dump(const char *s, uint8_t *data, unsigned len)
{
	unsigned i;

	printf("%s: ", s);
	for (i = 0; i < len; i++)
		printf("%02x ", data[i]);
	printf("(%u bytes)\n", len);
}

static int resource_reply_cb(const struct zoap_packet *response,
			     struct zoap_reply *reply,
			     const struct sockaddr *from)
{
	struct net_buf *buf = response->buf;

	msg_dump("reply", buf->data, buf->len);

	return 0;
}

static void udp_receive(struct net_context *context,
			struct net_buf *buf,
			int status,
			void *user_data)
{
	struct zoap_pending *pending;
	struct zoap_reply *reply;
	struct zoap_packet response;
	struct sockaddr_in6 from;
	int header_len, r;

	/*
	 * zoap expects that buffer->data starts at the
	 * beginning of the CoAP header
	 */
	header_len = net_nbuf_appdata(buf) - buf->frags->data;
	net_buf_pull(buf->frags, header_len);

	r = zoap_packet_parse(&response, buf);
	if (r < 0) {
		printf("Invalid data received (%d)\n", r);
		return;
	}

	pending = zoap_pending_received(&response, pendings,
					NUM_PENDINGS);
	if (pending) {
		/* If necessary cancel retransmissions */
	}

	net_ipaddr_copy(&from.sin6_addr, &NET_IPV6_BUF(buf)->src);
	from.sin6_port = NET_UDP_BUF(buf)->src_port;

	reply = zoap_response_received(&response,
				       (const struct sockaddr *) &from,
				       replies, NUM_REPLIES);
	if (!reply) {
		printf("No handler for response (%d)\n", r);
		return;
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

	r = net_context_sendto(buf, (struct sockaddr *) &mcast_addr,
			       sizeof(mcast_addr), NULL, 0, NULL, NULL);
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
	static struct net_addr any_addr = { .in6_addr = IN6ADDR_ANY_INIT,
					   .family = AF_INET6 };
	struct zoap_packet request;
	struct zoap_pending *pending;
	struct zoap_reply *reply;
	const char * const *p;
	struct net_buf *buf, *frag;
	int r, timeout;
	uint8_t observe = 0;

	r = net_context_get(PF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (r) {
		printf("Could not get an UDP context\n");
		return;
	}

	r = net_context_bind(context, (struct sockaddr *) &any_addr,
			     sizeof(any_addr));
	if (r) {
		printf("Could not bind the context\n");
		return;
	}

	r = net_context_recv(context, udp_receive, 0, NULL);
	if (r) {
		printf("Could not receive in the context\n");
		return;
	}

	nano_delayed_work_init(&retransmit_work, retransmit_request);

	buf = net_nbuf_get_tx(context);
	if (!buf) {
		printf("Unable to get TX buffer, not enough memory.\n");
		return;
	}

	frag = net_nbuf_get_data(context);
	if (!frag) {
		printf("Unable to get DATA buffer, not enough memory.\n");
		return;
	}

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&request, frag);
	if (r < 0) {
		return;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&request, 1);
	zoap_header_set_type(&request, ZOAP_TYPE_CON);
	zoap_header_set_code(&request, ZOAP_METHOD_GET);
	zoap_header_set_id(&request, zoap_next_id());
	zoap_header_set_token(&request, zoap_next_token(), 8);

	/* Enable observing the resource. */
	r = zoap_add_option(&request, ZOAP_OPTION_OBSERVE,
			    &observe, sizeof(observe));
	if (r < 0) {
		printf("Unable add option to request.\n");
		return;
	}

	for (p = test_path; p && *p; p++) {
		r = zoap_add_option(&request, ZOAP_OPTION_URI_PATH,
				     *p, strlen(*p));
		if (r < 0) {
			printf("Unable add option to request.\n");
			return;
		}
	}

	pending = zoap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		printf("Unable to find a free pending to track "
		       "retransmissions.\n");
		return;
	}

	r = zoap_pending_init(pending, &request);
	if (r < 0) {
		printf("Unable to initialize a pending retransmission.\n");
		return;
	}

	reply = zoap_reply_next_unused(replies, NUM_REPLIES);
	if (!reply) {
		printf("No resources for waiting for replies.\n");
		return;
	}

	zoap_reply_init(reply, &request);
	reply->reply = resource_reply_cb;

	r = net_context_sendto(buf, (struct sockaddr *) &mcast_addr,
			       sizeof(mcast_addr),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		printf("Error sending the packet (%d).\n", r);
		return;
	}

	zoap_pending_cycle(pending);
	timeout = pending->timeout * (sys_clock_ticks_per_sec / MSEC_PER_SEC);

	nano_delayed_work_submit(&retransmit_work, timeout);
}
