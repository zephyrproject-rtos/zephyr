/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "coap-client"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

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

#include "net_private.h"

#define MY_COAP_PORT 5683

static struct sockaddr_in6 peer_addr = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(MY_COAP_PORT)};

static struct net_context *context;

static const char * const test_path[] = { "test", NULL };

/* define semaphores */
K_SEM_DEFINE(coap_sem, 0, 1);

static int dump_payload(const struct coap_packet *response)
{
	struct net_buf *frag;
	u16_t offset;
	u16_t len;

	frag = coap_packet_get_payload(response, &offset, &len);
	if (!frag && len == 0xffff) {
		NET_ERR("Invalid payload");
		return -EINVAL;
	}

	while (frag) {
		_hexdump(frag->data + offset, frag->len - offset, 0);
		frag = frag->frags;
		offset = 0;
	}

	printk("\n");

	k_sem_give(&coap_sem);

	return 0;
}

static void udp_receive(struct net_context *context,
			struct net_pkt *pkt,
			int status,
			void *user_data)
{
	struct coap_packet response;
	int r;

	r = coap_packet_parse(&response, pkt, NULL, 0);
	if (r < 0) {
		NET_ERR("Invalid data received (%d)\n", r);
		return;
	}

	dump_payload(&response);
}

static void send_coap_request(u8_t method)
{
	u8_t payload[] = "payload";
	struct coap_packet request;
	struct net_pkt *pkt;
	struct net_buf *frag;
	const char * const *p;
	int r;

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&request, pkt, 1, COAP_TYPE_CON,
			     8, coap_next_token(),
			     method, coap_next_id());
	if (r < 0) {
		goto end;
	}

	for (p = test_path; p && *p; p++) {
		r = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			NET_ERR("Unable add option to request");
			goto end;
		}
	}

	switch (method) {
	case COAP_METHOD_GET:
		break;

	case COAP_METHOD_PUT:
	case COAP_METHOD_POST:
		r = coap_packet_append_payload_marker(&request);
		if (r < 0) {
			NET_ERR("Unable to append payload marker");
			goto end;
		}

		r = coap_packet_append_payload(&request, (u8_t *)payload,
					       sizeof(payload) - 1);
		if (r < 0) {
			NET_ERR("Not able to append payload");
			goto end;
		}
	}

	r = net_context_sendto(pkt, (struct sockaddr *) &peer_addr,
			       sizeof(peer_addr), NULL, 0, NULL, NULL);
	if (!r) {
		return;
	}

	NET_ERR("Error sending the packet (%d)", r);

end:
	net_pkt_unref(pkt);
}

static int init_app(void)
{
	struct sockaddr_in6 my_addr;
	int r;

	if (net_addr_pton(AF_INET6, CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			  &my_addr.sin6_addr)) {
		NET_ERR("Invalid my IPv6 address: %s",
			CONFIG_NET_CONFIG_MY_IPV6_ADDR);
		return -1;
	}

	if (net_addr_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  &peer_addr.sin6_addr)) {
		NET_ERR("Invalid peer IPv6 address: %s",
			CONFIG_NET_CONFIG_PEER_IPV6_ADDR);
		return -1;
	}

	r = net_context_get(PF_INET6, SOCK_DGRAM, IPPROTO_UDP, &context);
	if (r) {
		NET_ERR("Could not get an UDP context");
		return r;
	}

	my_addr.sin6_family = AF_INET6;

	r = net_context_bind(context, (struct sockaddr *) &my_addr,
			     sizeof(my_addr));
	if (r) {
		NET_ERR("Could not bind the context");
		return r;
	}

	r = net_context_recv(context, udp_receive, 0, NULL);
	if (r) {
		NET_ERR("Could not set receive callback");
		return r;
	}

	return 0;
}

void main(void)
{
	int r;

	r = init_app();
	if (r < 0) {
		return;
	}

	/* Test CoAP GET method */
	NET_DBG("CoAP client GET test");
	send_coap_request(COAP_METHOD_GET);

	/* Take semaphore */
	k_sem_take(&coap_sem, K_FOREVER);

	/* Test CoAP PUT method */
	NET_DBG("CoAP client PUT test");
	send_coap_request(COAP_METHOD_PUT);

	/* Take semaphore */
	k_sem_take(&coap_sem, K_FOREVER);

	/* Test CoAP POST method*/
	NET_DBG("CoAP client POST test");
	send_coap_request(COAP_METHOD_POST);

	NET_DBG("Done");
}
