/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <misc/printk.h>

#include <zephyr.h>

#include <misc/byteorder.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/zoap.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#define MY_COAP_PORT 5683

#define NUM_PENDINGS 3
#define NUM_REPLIES 3

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

static const struct sockaddr_in6 mcast_addr = {
	.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
	.sin6_family = AF_INET6,
	.sin6_port = htons(MY_COAP_PORT)};

static struct net_context *context;

struct zoap_pending pendings[NUM_PENDINGS];
struct zoap_reply replies[NUM_REPLIES];
struct k_delayed_work retransmit_work;

#if defined(CONFIG_NET_MGMT_EVENT)
static struct net_mgmt_event_callback cb;
#endif

static const char * const test_path[] = { "test", NULL };

static void msg_dump(const char *s, uint8_t *data, unsigned len)
{
	unsigned i;

	printk("%s: ", s);
	for (i = 0; i < len; i++)
		printk("%02x ", data[i]);
	printk("(%u bytes)\n", len);
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
		printk("Invalid data received (%d)\n", r);
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
		printk("No handler for response (%d)\n", r);
		return;
	}
}

static void retransmit_request(struct k_work *work)
{
	struct zoap_pending *pending;
	struct zoap_packet *request;
	struct net_buf *buf;
	int r;

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

	k_delayed_work_submit(&retransmit_work, pending->timeout);
}

static void event_iface_up(struct net_mgmt_event_callback *cb,
			   uint32_t mgmt_event, struct net_if *iface)
{
	static struct sockaddr_in6 any_addr = { .sin6_addr = IN6ADDR_ANY_INIT,
						.sin6_family = AF_INET6 };
	struct zoap_packet request;
	struct zoap_pending *pending;
	struct zoap_reply *reply;
	const char * const *p;
	struct net_buf *buf, *frag;
	int r, timeout;
	uint8_t observe = 0;

	r = net_context_get(PF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (r) {
		printk("Could not get an UDP context\n");
		return;
	}

	r = net_context_bind(context, (struct sockaddr *) &any_addr,
			     sizeof(any_addr));
	if (r) {
		printk("Could not bind the context\n");
		return;
	}

	r = net_context_recv(context, udp_receive, 0, NULL);
	if (r) {
		printk("Could not receive in the context\n");
		return;
	}

	k_delayed_work_init(&retransmit_work, retransmit_request);

	buf = net_nbuf_get_tx(context);
	if (!buf) {
		printk("Unable to get TX buffer, not enough memory.\n");
		return;
	}

	frag = net_nbuf_get_data(context);
	if (!frag) {
		printk("Unable to get DATA buffer, not enough memory.\n");
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
		printk("Unable add option to request.\n");
		return;
	}

	for (p = test_path; p && *p; p++) {
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

	r = net_context_sendto(buf, (struct sockaddr *) &mcast_addr,
			       sizeof(mcast_addr),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		printk("Error sending the packet (%d).\n", r);
		return;
	}

	zoap_pending_cycle(pending);
	timeout = pending->timeout * (sys_clock_ticks_per_sec / MSEC_PER_SEC);

	k_delayed_work_submit(&retransmit_work, timeout);

}

void main(void)
{
	struct net_if *iface = net_if_get_default();

#if defined(CONFIG_NET_L2_BLUETOOTH)
	if (bt_enable(NULL)) {
		NET_ERR("Bluetooth init failed\n");
		return;
	}
#endif

#if defined(CONFIG_NET_MGMT_EVENT)
	/* Subscribe to NET_IF_UP if interface is not ready */
	if (!atomic_test_bit(iface->flags, NET_IF_UP)) {
		net_mgmt_init_event_callback(&cb, event_iface_up,
					     NET_EVENT_IF_UP);
		net_mgmt_add_event_callback(&cb);
		return;
	}
#endif

	event_iface_up(NULL, NET_EVENT_IF_UP, iface);
}
