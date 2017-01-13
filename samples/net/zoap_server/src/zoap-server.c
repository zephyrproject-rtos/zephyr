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

#if 1
#define SYS_LOG_DOMAIN "zoap-server"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <stdio.h>

#include <zephyr.h>

#include <misc/byteorder.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_ip.h>

#include <net/zoap.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

#define MY_COAP_PORT 5683

#define STACKSIZE 2000

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

static struct net_context *context;

static int test_del(struct zoap_resource *resource,
		    struct zoap_packet *request,
		    const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t code, type, *payload;
	uint16_t id, len;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

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
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_DELETED);
	zoap_header_set_id(&response, id);

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int test_put(struct zoap_resource *resource,
		    struct zoap_packet *request,
		    const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type;
	uint16_t len, id;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

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

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int test_post(struct zoap_resource *resource,
		     struct zoap_packet *request,
		     const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type;
	uint16_t len, id;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

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
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CREATED);
	zoap_header_set_id(&response, id);

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int piggyback_get(struct zoap_resource *resource,
			 struct zoap_packet *request,
			 const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type;
	uint16_t len, id;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

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

	/* The response that coap-client expects */
	r = snprintf((char *) payload, len, "Type: %u\nCode: %u\nMID: %u\n",
		     type, code, id);
	if (r < 0 || r > len) {
		return -EINVAL;
	}

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int query_get(struct zoap_resource *resource,
		     struct zoap_packet *request,
		     const struct sockaddr *from)
{
	struct zoap_option options[4];
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type;
	uint16_t len, id;
	int i, r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		NET_ERR("packet without payload\n");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);

	r = zoap_find_options(request, ZOAP_OPTION_URI_QUERY, options, 4);
	if (r <= 0) {
		return -EINVAL;
	}

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("num queries: %d\n", r);

	for (i = 0; i < r; i++) {
		char str[16];

		if (options[i].len + 1 > sizeof(str)) {
			NET_INFO("Unexpected length of query: "
				 "%d (expected %zu)\n",
				 options[i].len, sizeof(str));
			break;
		}

		memcpy(str, options[i].value, options[i].len);
		str[options[i].len] = '\0';

		NET_INFO("query[%d]: %s\n", i + 1, str);
	}

	NET_INFO("*******\n");

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

	/* The response that coap-client expects */
	r = snprintf((char *) payload, len, "Type: %u\nCode: %u\nMID: %u\n",
		     type, code, id);
	if (r < 0 || r > len) {
		return -EINVAL;
	}

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static const char * const test_path[] = { "test", NULL };

static const char * const segments_path[] = { "seg1", "seg2", "seg3", NULL };

static const char * const query_path[] = { "query", NULL };

static struct zoap_resource resources[] = {
	{ .get = piggyback_get,
	  .post = test_post,
	  .del = test_del,
	  .put = test_put,
	  .path = test_path },
	{ .get = piggyback_get,
	  .path = segments_path,
	},
	{ .get = query_get,
	  .path = query_path,
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

void main(void)
{
	static struct sockaddr_in6 any_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
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
