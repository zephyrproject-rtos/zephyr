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

#include <gpio.h>

#include <net/coap.h>
#include <net/coap_link_format.h>

#define MY_COAP_PORT 5683

#define ALL_NODES_LOCAL_COAP_MCAST					\
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#if defined(LED0_GPIO_PORT)
#define LED_GPIO_NAME LED0_GPIO_PORT
#define LED_PIN LED0_GPIO_PIN
#else
#define LED_GPIO_NAME "(fail)"
#define LED_PIN 0
#endif

#define RPL_MAX_REPLY 75

#define PKT_WAIT_TIME K_SECONDS(1)

static struct net_context *context;
static struct device *led0;
static const u8_t plain_text_format;

/* LED */
static const char led_on[] = "LED0 ON";
static const char led_off[] = "LED0 OFF";
static const char led_toggle_on[] = "LED Toggle ON";
static const char led_toggle_off[] = "LED Toggle OFF";

static bool fake_led;

/* RPL */
static const char rpl_parent[] = "RPL Parent:";
static const char rpl_no_parent[] = "No parent yet";

static const char rpl_rank[] = "RPL Rank:";
static const char rpl_no_rank[] = "No rank yet";

static const char rpl_link[] = "Link Metric:";
static const char rpl_no_link[] = "No link metric yet";

static const char ipv6_no_nbr[] = "No IPv6 Neighbors";

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

static bool read_led(void)
{
	u32_t led = 0;
	int r;

	if (!led0) {
		return fake_led;
	}

	r = gpio_pin_read(led0, LED_PIN, &led);
	if (r < 0) {
		return false;
	}

	return led;
}

static void write_led(bool led)
{
	if (!led0) {
		fake_led = led;
		return;
	}

	gpio_pin_write(led0, LED_PIN, led);
}

static int led_get(struct coap_resource *resource,
		   struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	const char *str;
	u16_t len, id;
	int r;

	NET_DBG("");

	id = coap_header_get_id(request);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (read_led()) {
		str = led_on;
		len = sizeof(led_on);
	} else {
		str = led_off;
		len = sizeof(led_off);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

static int led_post(struct coap_resource *resource,
		    struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	const char *str;
	u16_t offset;
	u16_t len;
	u16_t id;
	u32_t led;
	u8_t payload;
	int r;

	NET_DBG("");

	led = 0;

	frag = net_frag_skip(request->frag, request->offset, &offset,
			     request->hdr_len + request->opt_len);
	if (!frag && offset == 0xffff) {
		return -EINVAL;
	}

	frag = net_frag_read_u8(frag, offset, &offset, &payload);
	if (!frag && offset == 0xffff) {
		printk("packet without payload, so toggle the led");

		led = read_led();
		led = !led;
	} else {
		if (payload == 0x31) {
			led = 1;
		}
	}

	write_led(led);

	id = coap_header_get_id(request);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (led) {
		str = led_toggle_on;
		len = sizeof(led_toggle_on);
	} else {
		str = led_toggle_off;
		len = sizeof(led_toggle_off);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

static int led_put(struct coap_resource *resource,
		   struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	const char *str;
	u16_t offset;
	u16_t len;
	u16_t id;
	u32_t led;
	u8_t payload;
	int r;

	NET_DBG("");

	led = 0;

	frag = net_frag_skip(request->frag, request->offset, &offset,
			     request->hdr_len + request->opt_len);
	if (!frag && offset == 0xffff) {
		return -EINVAL;
	}

	frag = net_frag_read_u8(frag, offset, &offset, &payload);
	if (!frag && offset == 0xffff) {
		printk("packet without payload, so toggle the led");

		led = read_led();
		led = !led;
	} else {
		if (payload == 0x31) {
			led = 1;
		}
	}

	write_led(led);

	id = coap_header_get_id(request);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     0, NULL, COAP_RESPONSE_CODE_CHANGED, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (led) {
		str = led_on;
		len = sizeof(led_on);
	} else {
		str = led_off;
		len = sizeof(led_off);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

struct ipv6_nbr_str {
	char str[(sizeof("xx:xx:xx:xx:xx:xx:xx:xx") *
		  CONFIG_NET_IPV6_MAX_NEIGHBORS) +
		  CONFIG_NET_IPV6_MAX_NEIGHBORS];
	u8_t len;
};

static void ipv6_nbr_cb(struct net_nbr *nbr, void *user_data)
{
	struct ipv6_nbr_str *nbr_str = user_data;
	char temp[sizeof("xx:xx:xx:xx:xx:xx:xx:xx") + sizeof("\n")];

	snprintk(temp, sizeof(temp), "%s\n",
		 net_sprint_ipv6_addr(&net_ipv6_nbr_data(nbr)->addr));

	memcpy(nbr_str->str + nbr_str->len, temp, strlen(temp));
	nbr_str->len += strlen(temp);
}

/* IPv6 Neighbors */
static int ipv6_neighbors_get(struct coap_resource *resource,
			      struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	struct ipv6_nbr_str nbr_str;
	u8_t token[8];
	const char *str;
	u16_t len, id;
	u8_t tkl;
	int r;

	NET_DBG("");

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, (u8_t *)token);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	nbr_str.len = 0;
	net_ipv6_nbr_foreach(ipv6_nbr_cb, &nbr_str);

	if (nbr_str.len) {
		str = nbr_str.str;
		len = nbr_str.len;
	} else {
		str = ipv6_no_nbr;
		len = strlen(ipv6_no_nbr);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

/* RPL Information */
static int rpl_info_get(struct coap_resource *resource,
			struct coap_packet *request)
{
	struct net_rpl_instance *rpl;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	struct in6_addr *parent;
	struct net_nbr *nbr;
	const char *str;
	u8_t token[8];
	u16_t len, id;
	u16_t out_len;
	u8_t tkl;
	int r;
	char out[RPL_MAX_REPLY];

	NET_DBG("");

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, (u8_t *)token);

	pkt = net_pkt_get_tx(context, K_FOREVER);
	if (!pkt) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	frag = net_pkt_get_data(context, K_FOREVER);
	if (!frag) {
		net_pkt_unref(pkt);
		return -ENOMEM;
	}

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	rpl = net_rpl_get_default_instance();
	if (rpl && rpl->current_dag && rpl->current_dag->preferred_parent) {
		nbr = net_rpl_get_nbr(rpl->current_dag->preferred_parent);
	} else {
		nbr = NULL;
	}

	/* Write all RPL info in JSON format */
	snprintk(out, sizeof(out) - 1, "parent-");
	out_len = strlen(out);

	if (!rpl || !rpl->current_dag || !rpl->current_dag->preferred_parent) {
		snprintk(&out[out_len], sizeof(out) - out_len - 1, "None");
	} else {
		parent = net_rpl_get_parent_addr(net_pkt_iface(pkt),
					rpl->current_dag->preferred_parent);
		snprintk(&out[out_len], sizeof(out) - out_len - 1, "%s",
			 net_sprint_ipv6_addr(parent));
	}

	out_len = strlen(out);

	snprintk(&out[out_len], sizeof(out) - out_len - 1, "\nrank-");
	out_len = strlen(out);

	if (!rpl || !rpl->current_dag) {
		snprintk(&out[out_len], sizeof(out) - out_len - 1, "inf");
	} else {
		snprintk(&out[out_len], sizeof(out) - out_len - 1, "%u.%02u",
			 rpl->current_dag->rank / NET_RPL_MC_ETX_DIVISOR,
			 (100 * (rpl->current_dag->rank %
				 NET_RPL_MC_ETX_DIVISOR)) /
			 NET_RPL_MC_ETX_DIVISOR);
	}

	out_len = strlen(out);

	snprintk(&out[out_len], sizeof(out) - out_len - 1, "\nlinkmetric-");
	out_len = strlen(out);

	if (!nbr) {
		snprintk(&out[out_len], sizeof(out) - out_len - 1, "inf");
	} else {
		snprintk(&out[out_len], sizeof(out) - out_len - 1, "%u.%02u",
			 (net_ipv6_nbr_data(nbr)->link_metric) /
			 NET_RPL_MC_ETX_DIVISOR,
			 (100 * ((net_ipv6_nbr_data(nbr)->link_metric) %
				 NET_RPL_MC_ETX_DIVISOR)) /
			 NET_RPL_MC_ETX_DIVISOR);
	}

	out_len = strlen(out);

	str = out;
	len = out_len;

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

/* RPL Parent */
static int rpl_parent_get(struct coap_resource *resource,
			  struct coap_packet *request)
{
	struct net_rpl_instance *rpl;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	struct in6_addr *parent;
	const char *str;
	u8_t token[8];
	u16_t len, id;
	u8_t tkl;
	int r;
	char out[RPL_MAX_REPLY];

	NET_DBG("");

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, (u8_t *)token);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	rpl = net_rpl_get_default_instance();
	if (!rpl || !rpl->current_dag || !rpl->current_dag->preferred_parent) {
		str = rpl_no_parent;
		len = sizeof(rpl_no_parent);

	} else {
		parent = net_rpl_get_parent_addr(net_pkt_iface(pkt),
					rpl->current_dag->preferred_parent);
		snprintk(out, sizeof(out), "%s %s", rpl_parent,
			 net_sprint_ipv6_addr(parent));
		str = out;
		len = strlen(out);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

/* RPL Rank */
static int rpl_rank_get(struct coap_resource *resource,
			struct coap_packet *request)
{
	struct net_rpl_instance *rpl;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	const char *str;
	u8_t token[8];
	u16_t len, id;
	u8_t tkl;
	int r;
	char out[RPL_MAX_REPLY];

	NET_DBG("");

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, (u8_t *)token);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	rpl = net_rpl_get_default_instance();
	if (!rpl || !rpl->current_dag) {
		str = rpl_no_rank;
		len = sizeof(rpl_no_rank);

	} else {
		snprintk(out, sizeof(out), "%s %u.%02u", rpl_rank,
			 rpl->current_dag->rank / NET_RPL_MC_ETX_DIVISOR,
			 (100 * (rpl->current_dag->rank %
				 NET_RPL_MC_ETX_DIVISOR)) /
			 NET_RPL_MC_ETX_DIVISOR);
		str = out;
		len = strlen(out);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

/* RPL Link Metric */
static int rpl_link_metric_get(struct coap_resource *resource,
			       struct coap_packet *request)
{
	struct net_rpl_instance *rpl;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	struct net_nbr *nbr;
	const char *str;
	u8_t token[8];
	u16_t len, id;
	u8_t tkl;
	int r;
	char out[RPL_MAX_REPLY];

	NET_DBG("");

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, (u8_t *)token);

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

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, token, COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	rpl = net_rpl_get_default_instance();
	if (rpl && rpl->current_dag &&
	    rpl->current_dag->preferred_parent) {
		nbr = net_rpl_get_nbr(rpl->current_dag->preferred_parent);
		if (nbr) {
			snprintk(out, sizeof(out), "%s %u.%02u", rpl_link,
				 (net_ipv6_nbr_data(nbr)->link_metric) /
				 NET_RPL_MC_ETX_DIVISOR,
				 (100 * ((net_ipv6_nbr_data(nbr)->link_metric) %
					 NET_RPL_MC_ETX_DIVISOR)) /
				 NET_RPL_MC_ETX_DIVISOR);

			str = out;
			len = strlen(out);
		} else {
			str = rpl_no_link;
			len = sizeof(rpl_no_link);
		}
	} else {
		str = rpl_no_link;
		len = sizeof(rpl_no_link);
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

static const char * const led_default_path[] = { "led", "0", NULL };
static const char * const led_default_attributes[] = {
	"title=\"LED0: led=toggle ?len=0..\"",
	"rt=\"Text\"",
	NULL };

static const char * const ipv6_neighbors_default_path[] = { "ipv6",
							    "neighbors",
							    NULL };
static const char * const ipv6_neighbors_default_attributes[] = {
	"title=\"IPv6 Neighbors\"",
	"rt=Text",
	NULL };

static const char * const rpl_info_default_path[] = { "rpl-info", NULL };
static const char * const rpl_info_default_attributes[] = {
	"title=\"RPL Information\"",
	"rt=Text",
	NULL };

static const char * const rpl_parent_default_path[] = { "rpl-info",
							"parent",
							NULL };
static const char * const rpl_parent_default_attributes[] = {
	"title=\"RPL Parent\"",
	"rt=Text",
	NULL };

static const char * const rpl_rank_default_path[] = { "rpl-info",
						      "rank",
						      NULL };
static const char * const rpl_rank_default_attributes[] = {
	"title=\"RPL Rank\"",
	"rt=Text",
	NULL };

static const char * const rpl_link_default_path[] = { "rpl-info",
						      "link-metric",
						      NULL };
static const char * const rpl_link_default_attributes[] = {
	"title=\"RPL Link Metric\"",
	"rt=Text",
	NULL };

static struct coap_resource resources[] = {
	{ .get = well_known_core_get,
	  .post = NULL,
	  .put = NULL,
	  .path = COAP_WELL_KNOWN_CORE_PATH,
	  .user_data = NULL,
	},
	{ .get = led_get,
	  .post = led_post,
	  .put = led_put,
	  .path = led_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = led_default_attributes,
			}),
	},
	{ .get = ipv6_neighbors_get,
	  .post = NULL,
	  .put = NULL,
	  .path = ipv6_neighbors_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = ipv6_neighbors_default_attributes,
			}),
	},
	{ .get = rpl_info_get,
	  .post = NULL,
	  .put = NULL,
	  .path = rpl_info_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = rpl_info_default_attributes,
			}),
	},
	{ .get = rpl_parent_get,
	  .post = NULL,
	  .put = NULL,
	  .path = rpl_parent_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = rpl_parent_default_attributes,
			}),
	},
	{ .get = rpl_rank_get,
	  .post = NULL,
	  .put = NULL,
	  .path = rpl_rank_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = rpl_rank_default_attributes,
			}),
	},
	{ .get = rpl_link_metric_get,
	  .post = NULL,
	  .put = NULL,
	  .path = rpl_link_default_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = rpl_link_default_attributes,
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
}

void main(void)
{
	NET_DBG("Start Demo");

	init_app();
}
