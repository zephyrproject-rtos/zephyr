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
#include <net/net_pkt.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>

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

struct coap_pending pendings[NUM_PENDINGS];
struct coap_reply replies[NUM_REPLIES];
struct k_delayed_work retransmit_work;

#if defined(CONFIG_NET_MGMT_EVENT)
static struct net_mgmt_event_callback cb;
#endif

static const char * const test_path[] = { "test", NULL };

static void msg_dump(const char *s, u8_t *data, unsigned len)
{
	unsigned i;

	printk("%s: ", s);
	for (i = 0; i < len; i++)
		printk("%02x ", data[i]);
	printk("(%u bytes)\n", len);
}

static void strip_headers(struct net_pkt *pkt)
{
	/* Get rid of IP + UDP/TCP header if it is there. The IP header
	 * will be put back just before sending the packet.
	 */
	if (net_pkt_appdatalen(pkt) > 0) {
		int header_len;

		header_len = net_buf_frags_len(pkt->frags) -
			     net_pkt_appdatalen(pkt);
		if (header_len > 0) {
			net_buf_pull(pkt->frags, header_len);
		}
	} else {
		net_pkt_set_appdatalen(pkt, net_buf_frags_len(pkt->frags));
	}
}

static int resource_reply_cb(const struct coap_packet *response,
			     struct coap_reply *reply,
			     const struct sockaddr *from)
{
	struct net_pkt *pkt = response->pkt;

	msg_dump("reply", pkt->frags->data, pkt->frags->len);

	return 0;
}

static void get_from_ip_addr(struct coap_packet *cpkt,
			     struct sockaddr_in6 *from)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(cpkt->pkt, &hdr);
	if (!udp_hdr) {
		return;
	}

	net_ipaddr_copy(&from->sin6_addr, &NET_IPV6_HDR(cpkt->pkt)->src);
	from->sin6_port = udp_hdr->src_port;
	from->sin6_family = AF_INET6;
}

static void udp_receive(struct net_context *context,
			struct net_pkt *pkt,
			int status,
			void *user_data)
{
	struct coap_pending *pending;
	struct coap_reply *reply;
	struct coap_packet response;
	struct sockaddr_in6 from;
	int r;

	r = coap_packet_parse(&response, pkt, NULL, 0);
	if (r < 0) {
		printk("Invalid data received (%d)\n", r);
		return;
	}

	pending = coap_pending_received(&response, pendings,
					NUM_PENDINGS);
	if (pending) {
		/* If necessary cancel retransmissions */
	}

	get_from_ip_addr(&response, &from);
	reply = coap_response_received(&response,
				       (const struct sockaddr *) &from,
				       replies, NUM_REPLIES);
	if (!reply) {
		printk("No handler for response (%d)\n", r);
		return;
	}
}

static void retransmit_request(struct k_work *work)
{
	struct coap_pending *pending;
	int r;

	pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	/* ref to avoid being freed after sendto() */
	net_pkt_ref(pending->pkt);
	/* drop IP + UDP headers */
	strip_headers(pending->pkt);

	r = net_context_sendto(pending->pkt, (struct sockaddr *) &mcast_addr,
			       sizeof(mcast_addr), NULL, 0, NULL, NULL);
	if (r < 0) {
		/* no error, keep retry */
		net_pkt_unref(pending->pkt);
	}

	if (!coap_pending_cycle(pending)) {
		/* last retransmit, clear pending and unreference packet */
		coap_pending_clear(pending);
		return;
	}

	/* unref to balance ref we made for sendto() */
	net_pkt_unref(pending->pkt);
	k_delayed_work_submit(&retransmit_work, pending->timeout);
}

static void event_iface_up(struct net_mgmt_event_callback *cb,
			   u32_t mgmt_event, struct net_if *iface)
{
	static struct sockaddr_in6 any_addr = { .sin6_addr = IN6ADDR_ANY_INIT,
						.sin6_family = AF_INET6 };
	struct coap_packet request;
	struct coap_pending *pending;
	struct coap_reply *reply;
	const char * const *p;
	struct net_pkt *pkt;
	struct net_buf *frag;
	int r;
	u8_t observe = 0;

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

	pkt = net_pkt_get_tx(context, K_FOREVER);
	if (!pkt) {
		printk("Unable to get TX packet, not enough memory.\n");
		return;
	}

	frag = net_pkt_get_data(context, K_FOREVER);
	if (!frag) {
		printk("Unable to get DATA buffer, not enough memory.\n");
		return;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&request, pkt, 1, COAP_TYPE_CON,
			     8, coap_next_token(),
			     COAP_METHOD_GET, coap_next_id());
	if (r < 0) {
		return;
	}

	/* Enable observing the resource. */
	r = coap_packet_append_option(&request, COAP_OPTION_OBSERVE,
				      &observe, sizeof(observe));
	if (r < 0) {
		printk("Unable add option to request.\n");
		return;
	}

	for (p = test_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			printk("Unable add option to request.\n");
			return;
		}
	}

	/* setup appdatalen */
	strip_headers(pkt);

	pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		printk("Unable to find a free pending to track "
		       "retransmissions.\n");
		return;
	}

	r = coap_pending_init(pending, &request,
			      (struct sockaddr *) &mcast_addr);
	if (r < 0) {
		printk("Unable to initialize a pending retransmission.\n");
		return;
	}

	reply = coap_reply_next_unused(replies, NUM_REPLIES);
	if (!reply) {
		printk("No resources for waiting for replies.\n");
		return;
	}

	coap_reply_init(reply, &request);
	reply->reply = resource_reply_cb;

	/* Increase packet ref count to avoid being unref after sendto() */
	coap_pending_cycle(pending);

	r = net_context_sendto(pkt, (struct sockaddr *) &mcast_addr,
			       sizeof(mcast_addr),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		printk("Error sending the packet (%d).\n", r);
		coap_pending_clear(pending);
		return;
	}

	k_delayed_work_submit(&retransmit_work, pending->timeout);

}

void main(void)
{
	struct net_if *iface = net_if_get_default();

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
