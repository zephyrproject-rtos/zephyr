/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "zoap-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>

#include <zephyr.h>
#include <board.h>

#include <misc/byteorder.h>
#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/nbuf.h>
#include <net/net_context.h>

#include <net_private.h>

#include <gpio.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#include <net/zoap.h>
#include <net/zoap_link_format.h>

#define MY_COAP_PORT 5683

#define ALL_NODES_LOCAL_COAP_MCAST					\
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#define MY_IP6ADDR \
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }

#if defined(LED0_GPIO_PORT)
#define LED_GPIO_NAME LED0_GPIO_PORT
#define LED_PIN LED0_GPIO_PIN
#else
#define LED_GPIO_NAME "(fail)"
#define LED_PIN 0
#endif

static struct net_context *context;

static struct device *led0;

static const char led_on[] = "LED ON\n";
static const char led_off[] = "LED OFF\n";
static const char led_toggle_on[] = "LED Toggle ON\n";
static const char led_toggle_off[] = "LED Toggle OFF\n";

static bool fake_led;

static bool read_led(void)
{
	uint32_t led = 0;
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

static int led_get(struct zoap_resource *resource,
		   struct zoap_packet *request,
		   const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const char *str;
	uint8_t *payload;
	uint16_t len, id;
	int r;

	id = zoap_header_get_id(request);

	buf = net_nbuf_get_tx(context);
	if (!buf) {
		return -ENOMEM;
	}

	frag = net_nbuf_get_data(context);
	if (!frag) {
		return -ENOMEM;
	}

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	if (read_led()) {
		str = led_on;
		r = sizeof(led_on);
	} else {
		str = led_off;
		r = sizeof(led_off);
	}

	memcpy(payload, str, r);

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int led_post(struct zoap_resource *resource,
		    struct zoap_packet *request,
		    const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const char *str;
	uint8_t *payload;
	uint16_t len, id;
	uint32_t led;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		printk("packet without payload");
		return -EINVAL;
	}

	id = zoap_header_get_id(request);

	buf = net_nbuf_get_tx(context);
	if (!buf) {
		return -ENOMEM;
	}

	frag = net_nbuf_get_data(context);
	if (!frag) {
		return -ENOMEM;
	}

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	led = read_led();

	led = !led;

	write_led(led);

	if (led) {
		str = led_toggle_on;
		r = sizeof(led_toggle_on);
	} else {
		str = led_toggle_off;
		r = sizeof(led_toggle_off);
	}

	memcpy(payload, str, r);

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int led_put(struct zoap_resource *resource,
		   struct zoap_packet *request,
		   const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const char *str;
	uint8_t *payload;
	uint16_t len, id;
	uint32_t led;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		printk("packet without payload");
		return -EINVAL;
	}

	led = 0;
	if (len > 0 && payload[0] == '1') {
		led = 1;
	}

	id = zoap_header_get_id(request);

	buf = net_nbuf_get_tx(context);
	if (!buf) {
		return -ENOMEM;
	}

	frag = net_nbuf_get_data(context);
	if (!frag) {
		return -ENOMEM;
	}

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CHANGED);
	zoap_header_set_id(&response, id);

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	write_led(led);

	if (led) {
		str = led_on;
		r = sizeof(led_on);
	} else {
		str = led_off;
		r = sizeof(led_off);
	}

	memcpy(payload, str, r);

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int dummy_get(struct zoap_resource *resource,
		     struct zoap_packet *request,
		     const struct sockaddr *from)
{
	static const char dummy_str[] = "Just a test\n";
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload;
	uint16_t len, id;
	int r;

	id = zoap_header_get_id(request);

	buf = net_nbuf_get_tx(context);
	if (!buf) {
		return -ENOMEM;
	}

	frag = net_nbuf_get_data(context);
	if (!frag) {
		return -ENOMEM;
	}

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	memcpy(payload, dummy_str, sizeof(dummy_str));

	r = zoap_packet_set_used(&response, sizeof(dummy_str));
	if (r) {
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
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

static struct zoap_resource resources[] = {
	ZOAP_WELL_KNOWN_CORE_RESOURCE,
	{ .get = led_get,
	  .post = led_post,
	  .put = led_put,
	  .path = led_default_path,
	  .user_data = &((struct zoap_core_metadata) {
			  .attributes = led_default_attributes,
			}),
	},
	{ .get = dummy_get,
	  .path = dummy_path,
	  .user_data = &((struct zoap_core_metadata) {
			  .attributes = dummy_attributes,
			}),
	},
	{ },
};

static void udp_receive(struct net_context *context,
			struct net_buf *buf,
			int status,
			void *user_data)
{
	struct zoap_packet request;
	struct sockaddr_in6 from;
	int r, header_len;

	net_ipaddr_copy(&from.sin6_addr, &NET_IPV6_BUF(buf)->src);
	from.sin6_port = NET_UDP_BUF(buf)->src_port;
	from.sin6_family = AF_INET6;

	/*
	 * zoap expects that buffer->data starts at the
	 * beginning of the CoAP header
	 */
	header_len = net_nbuf_appdata(buf) - buf->frags->data;
	net_buf_pull(buf->frags, header_len);

	r = zoap_packet_parse(&request, buf);
	if (r < 0) {
		NET_ERR("Invalid data received (%d)\n", r);
		net_buf_unref(buf);
		return;
	}

	r = zoap_handle_request(&request, resources,
				(const struct sockaddr *) &from);

	net_buf_unref(buf);

	if (r < 0) {
		NET_ERR("No handler for such request (%d)\n", r);
		return;
	}
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
		NET_ERR("Could not get te default interface\n");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(net_if_get_default(),
				      &my_addr, NET_ADDR_MANUAL, 0);
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

#if defined(CONFIG_NET_L2_BLUETOOTH)
	if (bt_enable(NULL)) {
		NET_ERR("Bluetooth init failed");
		return;
	}
	ipss_init();
	ipss_advertise();
#endif

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
