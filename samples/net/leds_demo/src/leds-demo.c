/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "coap-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>

#include <zephyr.h>
#include <board.h>

#include <misc/byteorder.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/net_pkt.h>
#include <net/net_context.h>
#include <net/udp.h>

#include <net_private.h>

#include <gpio.h>

#include <net/coap.h>
#include <net/coap_link_format.h>

#define MY_COAP_PORT 5683

#define ALL_NODES_LOCAL_COAP_MCAST					\
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#define MY_IP6ADDR \
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }

#ifndef LED0_GPIO_CONTROLLER
#ifdef LED0_GPIO_PORT
#define LED0_GPIO_CONTROLLER 	LED0_GPIO_PORT
#else
#define LED0_GPIO_CONTROLLER "(fail)"
#define LED0_GPIO_PIN 0
#endif
#endif

#define LED_GPIO_NAME LED0_GPIO_CONTROLLER
#define LED_PIN LED0_GPIO_PIN

static struct net_context *context;

static struct device *led0;

#define MAX_PACKET_SIZE (256 + 32)
/* FIXME: these buffers aren't thread safe: need pool mgmt */
static u8_t in_buf[MAX_PACKET_SIZE];
static u8_t out_buf[MAX_PACKET_SIZE];

static const char led_on[] = "LED ON\n";
static const char led_off[] = "LED OFF\n";
static const char led_toggle_on[] = "LED Toggle ON\n";
static const char led_toggle_off[] = "LED Toggle OFF\n";

static bool fake_led;

static void get_from_ip_addr(struct net_pkt *pkt,
			     struct sockaddr_in6 *from)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		return;
	}

	net_ipaddr_copy(&from->sin6_addr, &NET_IPV6_HDR(pkt)->src);
	from->sin6_port = udp_hdr->src_port;
	from->sin6_family = AF_INET6;
}

static int send_coap_packet(struct fbuf_ctx *fbuf,
			    const struct sockaddr *from)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	int r;

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
	net_pkt_append(pkt, fbuf->buf_len, fbuf->buf,
		       K_FOREVER);
	r = net_context_sendto(pkt, from, sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
	}

	return r;
}

static int well_known_core_get(struct coap_resource *resource,
			       struct coap_packet *request,
			       const struct sockaddr *from)
{
	struct coap_packet response;
	int r;

	NET_DBG("");

	r = coap_well_known_core_get(resource, request, &response,
				     out_buf, sizeof(out_buf));
	if (r < 0) {
		return r;
	}

	return send_coap_packet(&response.fbuf, from);
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

	return !led;
}

static void write_led(bool led)
{
	if (!led0) {
		fake_led = led;
		return;
	}

	gpio_pin_write(led0, LED_PIN, !led);
}

static int led_get(struct coap_resource *resource,
		   struct coap_packet *request,
		   const struct sockaddr *from)
{
	struct coap_packet response;
	const char *str;
	int r;
	u16_t len;

	r = coap_packet_init(&response, out_buf, sizeof(out_buf), 1,
			     COAP_TYPE_ACK, 0, NULL, COAP_RESPONSE_CODE_CONTENT,
			     coap_header_get_id(request));
	if (r < 0) {
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
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		return -EINVAL;
	}

	return send_coap_packet(&response.fbuf, from);
}

static int led_post(struct coap_resource *resource,
		    struct coap_packet *request,
		    const struct sockaddr *from)
{
	struct coap_packet response;
	const char *str;
	int r;
	u32_t led;
	u16_t offset;
	u8_t payload;
	u8_t len;

	led = 0;

	offset = request->offset;
	r = fbuf_skip(&request->fbuf, &offset,
		      request->hdr_len + request->opt_len);
	if (r < 0) {
		return -EINVAL;
	}

	r = fbuf_read_u8(&request->fbuf, &offset, &payload);
	if (r < 0) {
		printk("packet without payload, so toggle the led");
		led = read_led();
		led = !led;
	} else {
		if (payload == 0x31) {
			led = 1;
		}
	}

	write_led(led);

	r = coap_packet_init(&response, out_buf, sizeof(out_buf), 1,
			     COAP_TYPE_ACK, 0, NULL, COAP_RESPONSE_CODE_CONTENT,
			     coap_header_get_id(request));
	if (r < 0) {
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
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		return -EINVAL;
	}

	return send_coap_packet(&response.fbuf, from);
}

static int led_put(struct coap_resource *resource,
		   struct coap_packet *request,
		   const struct sockaddr *from)
{
	struct coap_packet response;
	const char *str;
	int r;
	u32_t led;
	u16_t offset;
	u8_t payload;
	u8_t len;

	led = 0;
	offset = request->offset;
	r = fbuf_skip(&request->fbuf, &offset,
		      request->hdr_len + request->opt_len);
	if (r < 0) {
		return -EINVAL;
	}

	r = fbuf_read_u8(&request->fbuf, &offset, &payload);
	if (r < 0) {
		printk("packet without payload, so toggle the led");
		led = read_led();
		led = !led;
	} else {
		if (payload == 0x31) {
			led = 1;
		}
	}

	write_led(led);

	r = coap_packet_init(&response, out_buf, sizeof(out_buf), 1,
			     COAP_TYPE_ACK, 0, NULL, COAP_RESPONSE_CODE_CHANGED,
			     coap_header_get_id(request));
	if (r < 0) {
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
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)str, len);
	if (r < 0) {
		return -EINVAL;
	}

	return send_coap_packet(&response.fbuf, from);
}

static int dummy_get(struct coap_resource *resource,
		     struct coap_packet *request,
		     const struct sockaddr *from)
{
	static const char dummy_str[] = "Just a test\n";
	struct coap_packet response;
	int r;

	r = coap_packet_init(&response, out_buf, sizeof(out_buf), 1,
			     COAP_TYPE_ACK, 0, NULL, COAP_RESPONSE_CODE_CONTENT,
			     coap_header_get_id(request));
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)dummy_str,
				      sizeof(dummy_str));
	if (r < 0) {
		return -EINVAL;
	}

	return send_coap_packet(&response.fbuf, from);
}

static const char * const led_default_path[] = { "led", NULL };
static const char * const led_default_attributes[] = {
	"title=\"LED\"",
	"rt=Text",
	NULL };

static const char * const dummy_path[] = { "dummy", NULL };
static const char * const dummy_attributes[] = {
	"title=\"Dummy\"",
	"rt=dummy",
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
	{ .get = dummy_get,
	  .path = dummy_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = dummy_attributes,
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
	struct sockaddr_in6 from;
	struct coap_option options[16] = { 0 };
	int r;
	u16_t hdr_len = net_pkt_ip_hdr_len(pkt) + NET_UDPH_LEN +
				net_pkt_ipv6_ext_len(pkt);
	u16_t buf_len = net_pkt_get_len(pkt) - hdr_len;
	u8_t opt_num = 16;

	r = net_frag_linearize(in_buf, sizeof(in_buf), pkt, hdr_len, buf_len);
	if (r < 0) {
		NET_ERR("Unable to linearize data (%d)\n", r);
		net_pkt_unref(pkt);
		return;
	}

	r = coap_packet_parse(&request, in_buf, buf_len, 0, options, opt_num);
	if (r < 0) {
		NET_ERR("Invalid data received (%d)\n", r);
		net_pkt_unref(pkt);
		return;
	}

	/* save from address */
	get_from_ip_addr(pkt, &from);

	r = coap_handle_request(&request, resources, options, opt_num,
				(const struct sockaddr *)&from);
	if (r < 0) {
		NET_ERR("No handler for such request (%d)\n", r);
	}

	net_pkt_unref(pkt);
}

static bool join_coap_multicast_group(void)
{
	static struct in6_addr my_addr = MY_IP6ADDR;
	static struct sockaddr_in6 mcast_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.sin6_port = htons(MY_COAP_PORT) };
	struct net_if_mcast_addr *mcast;
	struct net_if_addr *ifaddr;
	struct net_if *iface;

	iface = net_if_get_default();
	if (!iface) {
		NET_ERR("Could not get default interface");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(net_if_get_default(),
				      &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Could not add IPv6 address to default interface");
		return false;
	}
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	mcast = net_if_ipv6_maddr_add(iface, &mcast_addr.sin6_addr);
	if (!mcast) {
		NET_ERR("Could not add multicast address to interface\n");
		return false;
	}

	return true;
}

void main(void)
{
	static struct sockaddr_in6 any_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(MY_COAP_PORT) };
	int r;

	led0 = device_get_binding(LED_GPIO_NAME);
	/* Want it to be NULL if not available */

	if (led0) {
		gpio_pin_configure(led0, LED_PIN, GPIO_DIR_OUT);
	}

	if (!join_coap_multicast_group()) {
		NET_ERR("Could not join CoAP multicast group\n");
		return;
	}

	r = net_context_get(PF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (r) {
		NET_ERR("Could not get an UDP context\n");
		return;
	}

	r = net_context_bind(context, (struct sockaddr *) &any_addr,
			     sizeof(any_addr));
	if (r) {
		NET_ERR("Could not bind the context\n");
		return;
	}

	r = net_context_recv(context, udp_receive, 0, NULL);
	if (r) {
		NET_ERR("Could not receive in the context\n");
		return;
	}
}
