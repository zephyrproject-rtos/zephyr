/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "rpl-node"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>
#include <board.h>

#include <misc/byteorder.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/udp.h>

#include <net_private.h>
#include <rpl.h>
#include <icmpv6.h>
#include <6lo.h>
#include <ipv6.h>
#include <rpl.h>

#include <gpio.h>

#include <net/coap.h>
#include <net/coap_link_format.h>

#define MY_COAP_PORT 5683

#define ALL_NODES_LOCAL_COAP_MCAST					\
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#ifndef LED0_GPIO_CONTROLLER
#ifdef LED0_GPIO_PORT
#define LED0_GPIO_CONTROLLER LED0_GPIO_PORT
#else
#define LED0_GPIO_CONTROLLER "(fail)"
#define LED0_GPIO_PIN 0
#endif
#endif

#define LED_GPIO_NAME LED0_GPIO_CONTROLLER
#define LED_PIN LED0_GPIO_PIN

#define RPL_MAX_REPLY 75

#define PKT_WAIT_TIME K_SECONDS(1)

static struct net_context *context;
static struct device *led0;

#define NUM_OBSERVERS 3
#define NUM_PENDINGS 3

static struct coap_observer observers[NUM_OBSERVERS];
static struct coap_pending pendings[NUM_PENDINGS];

static struct k_delayed_work retransmit_work;

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

static void send_error_response(struct coap_resource *resource,
				struct coap_packet *request,
				struct sockaddr_in6 *from)
{
	struct net_context *context;
	struct coap_packet response;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u16_t id;
	int r;

	id = coap_header_get_id(request);
	context = net_pkt_context(request->pkt);

	pkt = net_pkt_get_tx(context, PKT_WAIT_TIME);
	if (!pkt) {
		return;
	}

	frag = net_pkt_get_data(context, PKT_WAIT_TIME);
	if (!frag) {
		net_pkt_unref(pkt);
		return;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, COAP_RESPONSE_CODE_BAD_REQUEST, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return;
	}

	r = net_context_sendto(pkt, (const struct sockaddr *)from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}
}

static int well_known_core_get(struct coap_resource *resource,
			      struct coap_packet *request)
{
	struct coap_packet response;
	struct sockaddr_in6 from;
	struct net_pkt *pkt;
	struct net_buf *frag;
	int r;

	NET_DBG("");

	pkt = net_pkt_get_tx(context, K_FOREVER);
	if (!pkt) {
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_well_known_core_get(resource, request, &response, pkt);
	if (r < 0) {
		net_pkt_unref(response.pkt);
		send_error_response(resource, request, &from);

		return r;
	}

	get_from_ip_addr(request, &from);

	r = net_context_sendto(response.pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(response.pkt);
	}

	return r;
}

static void toggle_led(void)
{
	u32_t led = 0;
	int r;

	if (!led0) {
		return;
	}

	r = gpio_pin_read(led0, LED_PIN, &led);
	if (r < 0) {
		return;
	}

	gpio_pin_write(led0, LED_PIN, !led);
}

static int led_post(struct coap_resource *resource,
		    struct coap_packet *request)
{
	NET_DBG("");

	toggle_led();

	return 0;
}

static void retransmit_request(struct k_work *work)
{
	struct coap_pending *pending;
	int r;

	pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	r = net_context_sendto(pending->pkt, &pending->addr,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		return;
	}

	if (!coap_pending_cycle(pending)) {
		coap_pending_clear(pending);
		return;
	}

	k_delayed_work_submit(&retransmit_work, pending->timeout);
}

static int append_rpl_parent(struct coap_packet *response)
{
	struct net_rpl_instance *rpl;
	struct in6_addr *parent;
	char out[RPL_MAX_REPLY];
	u16_t out_len;
	u8_t pos;

	rpl = net_rpl_get_default_instance();

	pos = snprintk(out, sizeof(out), "parent-");
	out_len = pos;

	if (!rpl || !rpl->current_dag || !rpl->current_dag->preferred_parent) {
		pos = snprintk(&out[out_len], sizeof(out), "None");
		out_len += pos;

	} else {
		parent = net_rpl_get_parent_addr(net_pkt_iface(response->pkt),
					rpl->current_dag->preferred_parent);
		pos = snprintk(&out[out_len], sizeof(out), "%s",
			       net_sprint_ipv6_addr(parent));
		out_len += pos;
	}

	pos = snprintk(&out[out_len], sizeof(out), "\nrank-");
	out_len += pos;

	if (!rpl || !rpl->current_dag) {
		pos = snprintk(&out[out_len], sizeof(out), "None");
		out_len += pos;
	} else {
		pos = snprintk(&out[out_len], sizeof(out), "%u",
			       rpl->current_dag->rank);
		out_len += pos;
	}

	return  coap_packet_append_payload(response, (u8_t *)out, out_len);
}

static int send_notification_packet(const struct sockaddr *addr,
				    u16_t age,
				    socklen_t addrlen,
				    u16_t id,
				    const u8_t *token,
				    u8_t tkl,
				    bool is_response)
{
	struct coap_packet response;
	struct coap_pending *pending;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u8_t type = COAP_TYPE_CON;
	int r;

	if (is_response) {
		type = COAP_TYPE_ACK;
	}

	if (!is_response) {
		id = coap_next_id();
	}

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (age >= 2) {
		coap_append_option_int(&response, COAP_OPTION_OBSERVE, age);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = append_rpl_parent(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (type == COAP_TYPE_CON) {
		pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
		if (!pending) {
			return -EINVAL;
		}

		r = coap_pending_init(pending, &response, addr);
		if (r) {
			return -EINVAL;
		}

		coap_pending_cycle(pending);
		pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);

		k_delayed_work_submit(&retransmit_work, pending->timeout);
	}

	return net_context_sendto(pkt, addr, addrlen, NULL, 0, NULL, NULL);
}

/* RPL Observe Server */
static int rpl_obs_get(struct coap_resource *resource,
		       struct coap_packet *request)
{
	struct coap_observer *observer;
	struct sockaddr_in6 from;
	u8_t token[8];
	u16_t id;
	u8_t tkl;
	bool observe = true;

	get_from_ip_addr(request, &from);

	if (!coap_request_is_observe(request)) {
		observe = false;
		goto done;
	}

	observer = coap_find_observer_by_addr(observers, NUM_OBSERVERS,
					      (const struct sockaddr *)&from);
	if (observer) {
		goto done;
	}

	observer = coap_observer_next_unused(observers, NUM_OBSERVERS);
	if (!observer) {
		return -ENOMEM;
	}

	coap_observer_init(observer, request, (const struct sockaddr *)&from);
	coap_register_observer(resource, observer);

done:
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	return send_notification_packet((const struct sockaddr *)&from,
					observe ? resource->age : 0,
					sizeof(struct sockaddr_in6), id,
					token, tkl, true);
}

static const char * const led_default_path[] = { "led", NULL };
static const char * const led_default_attributes[] = {
	"title=\"LED toggle\"",
	"rt=\"Text\"",
	NULL };

static const char * const rpl_obs_default_path[] = { "rpl-obs",
						      NULL };
static const char * const rpl_obs_default_attributes[] = {
	"title=\"RPL Observe\"",
	"rt=Text",
	NULL };

static struct coap_resource resources[] = {
	{ .get = well_known_core_get,
	  .post = NULL,
	  .put = NULL,
	  .path = COAP_WELL_KNOWN_CORE_PATH,
	  .user_data = NULL,
	},
	{ .get = NULL,
	  .post = led_post,
	  .put = NULL,
	  .path = led_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = led_default_attributes,
			}),
	},
	{ .get = rpl_obs_get,
	  .post = NULL,
	  .put = NULL,
	  .path = rpl_obs_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = rpl_obs_default_attributes,
			}),
	},
	{ },
};

static void udp_receive(struct net_context *context,
			struct net_pkt *pkt,
			int status,
			void *user_data)
{
	struct coap_packet request;
	struct coap_option options[16] = { 0 };
	u8_t opt_num = 16;
	int r;

	r = coap_packet_parse(&request, pkt, options, opt_num);
	if (r < 0) {
		NET_ERR("Invalid data received (%d)\n", r);
		goto end;
	}

	r = coap_handle_request(&request, resources, options, opt_num);
	if (r < 0) {
		NET_ERR("No handler for such request (%d)\n", r);
	}

end:
	net_pkt_unref(pkt);
}

static bool join_coap_multicast_group(void)
{
	static struct sockaddr_in6 mcast_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.sin6_port = htons(MY_COAP_PORT) };
	struct net_if_mcast_addr *mcast;
	struct net_if *iface;

	iface = net_if_get_default();
	if (!iface) {
		NET_ERR("Could not get te default interface\n");
		return false;
	}

	mcast = net_if_ipv6_maddr_add(iface, &mcast_addr.sin6_addr);
	if (!mcast) {
		NET_ERR("Could not add multicast address to interface\n");
		return false;
	}

	return true;
}

static void init_app(void)
{
	static struct sockaddr_in6 any_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(MY_COAP_PORT) };
	int r;

	led0 = device_get_binding(LED_GPIO_NAME);
	if (led0) {
		gpio_pin_configure(led0, LED_PIN, GPIO_DIR_OUT);
	} else {
		NET_WARN("Failed to bind '%s'"
			 "fake_led will provide dummpy replies",
			 LED_GPIO_NAME);
	}

	if (!join_coap_multicast_group()) {
		NET_ERR("Could not join CoAP multicast group");
		return;
	}

	r = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (r) {
		NET_ERR("Could not get an UDP context");
		return;
	}

	r = net_context_bind(context, (struct sockaddr *) &any_addr,
			     sizeof(any_addr));
	if (r) {
		NET_ERR("Could not bind the context");
		return;
	}

	r = net_context_recv(context, udp_receive, 0, NULL);
	if (r) {
		NET_ERR("Could not receive in the context");
	}

	k_delayed_work_init(&retransmit_work, retransmit_request);
}

void main(void)
{
	NET_DBG("Start Demo");

	init_app();
}
