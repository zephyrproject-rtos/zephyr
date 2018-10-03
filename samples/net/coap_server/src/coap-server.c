/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "coap-server"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <misc/printk.h>

#include <zephyr.h>

#include <misc/byteorder.h>
#include <misc/util.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/udp.h>

#include <net/coap.h>
#include <net/coap_link_format.h>

#include "net_private.h"

#define MY_COAP_PORT 5683

#define STACKSIZE 2000

/* FIXME */
#define BLOCK_WISE_TRANSFER_SIZE_GET 2048

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#define MY_IP6ADDR \
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }

#define NUM_OBSERVERS 3

#define NUM_PENDINGS 3

/* block option helper */
#define GET_BLOCK_NUM(v)	((v) >> 4)
#define GET_BLOCK_SIZE(v)	(((v) & 0x7))
#define GET_MORE(v)		(!!((v) & 0x08))

static struct net_context *context;

static const u8_t plain_text_format;

static struct coap_observer observers[NUM_OBSERVERS];

static struct coap_pending pendings[NUM_PENDINGS];

static struct net_context *context;

static struct k_delayed_work observer_work;

static int obs_counter;

static struct coap_resource *resource_to_notify;

struct k_delayed_work retransmit_work;

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
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	r = coap_well_known_core_get(resource, request, &response, pkt);
	if (r < 0) {
		net_pkt_unref(response.pkt);
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

static void payload_dump(const char *s, struct net_buf *frag,
			 u16_t offset, u16_t len)
{
	printk("payload message = %s [%u]\n", s, len);

	while (frag) {
		_hexdump(frag->data + offset, frag->len - offset, 0);
		frag = frag->frags;
		offset = 0;
	}
}

static int test_del(struct coap_resource *resource,
		    struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t tkl, code, type;
	u8_t token[8];
	u16_t id;
	int r;

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_DELETED, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int test_put(struct coap_resource *resource,
		    struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t code, type, tkl;
	u8_t token[8];
	u16_t id;
	int r;

	struct net_buf *payloadfrag;

	u16_t offset;
	u16_t len;

	/* TODO: Check for payload, empty payload is an error case. */

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("\n ****** test put method  *******\n");

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	payloadfrag = coap_packet_get_payload(request, &offset, &len);
	if (!payloadfrag && len == 0xffff) {
		NET_ERR("Invalid payload");
		return -EINVAL;
	} else if (!payloadfrag && !len) {
		NET_INFO("Packet without payload\n");
		goto next;
	}

	payload_dump("put_payload", payloadfrag, offset, len);

next:
	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CHANGED, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int test_post(struct coap_resource *resource,
		     struct coap_packet *request)
{
	static const char * const location_path[] = { "location1",
						      "location2",
						      "location3",
						      NULL };
	const char * const *p;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t code, type, tkl;
	u8_t token[8];
	u16_t id;
	int r;

	struct net_buf *payloadfrag;

	u16_t offset;
	u16_t len;

	/* TODO: Check for payload, empty payload is an error case. */

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("\n ****** test post method  *******\n");
	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	payloadfrag = coap_packet_get_payload(request, &offset, &len);
	if (!payloadfrag && len == 0xffff) {
		NET_ERR("Invalid payload");
		return -EINVAL;
	} else if (!payloadfrag && !len) {
		NET_INFO("Packet without payload\n");
		goto next;
	}

	payload_dump("post_payload", payloadfrag, offset, len);

next:
	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CREATED, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	for (p = location_path; *p; p++) {
		r = coap_packet_append_option(&response,
					      COAP_OPTION_LOCATION_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			return -EINVAL;
		}
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int location_query_post(struct coap_resource *resource,
			       struct coap_packet *request)
{
	static const char *const location_query[] = { "first=1",
						      "second=2",
						      NULL };
	const char * const *p;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t code, type, tkl;
	u8_t token[8];
	u16_t id;
	int r;

	/* TODO: Check for payload, empty payload is an error case. */

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CREATED, id);
	if (r < 0) {
		return -EINVAL;
	}

	for (p = location_query; *p; p++) {
		r = coap_packet_append_option(&response,
					      COAP_OPTION_LOCATION_QUERY,
					      *p, strlen(*p));
		if (r < 0) {
			return -EINVAL;
		}
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int piggyback_get(struct coap_resource *resource,
			 struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t token[8];
	u8_t payload[40], code, type;
	u16_t id;
	u8_t tkl;
	int r;

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CONTENT, id);
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

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int query_get(struct coap_resource *resource,
		     struct coap_packet *request)
{
	struct coap_option options[4];
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t payload[40], code, type, tkl;
	u8_t token[8];
	u16_t id;
	int i, r;

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	r = coap_find_options(request, COAP_OPTION_URI_QUERY, options, 4);
	if (r < 0) {
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

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, (u8_t *) token,
			     COAP_RESPONSE_CODE_CONTENT, id);
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

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int separate_get(struct coap_resource *resource,
			struct coap_packet *request)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	struct coap_pending *pending;
	u8_t payload[40], code, type, tkl;
	u8_t token[8];
	u16_t id;
	int r;

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	if (type == COAP_TYPE_NON_CON) {
		goto done;
	}

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, (u8_t *)token, 0, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = net_context_sendto(pkt, (const struct sockaddr *)&from,
			       sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

done:
	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_CON;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	r = coap_packet_init(&response, pkt, 1, type,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CONTENT, id);
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

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	if (type == COAP_TYPE_CON) {
		pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
		if (!pending) {
			net_pkt_unref(pkt);
			return -EINVAL;
		}

		r = coap_pending_init(pending, &response,
				      (const struct sockaddr *)&from);
		if (r) {
			net_pkt_unref(pkt);
			return -EINVAL;
		}

		coap_pending_cycle(pending);
		pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);

		k_delayed_work_submit(&retransmit_work, pending->timeout);
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int large_get(struct coap_resource *resource,
		     struct coap_packet *request)
{
	static struct coap_block_context ctx;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t token[8], code, type;
	u8_t payload[64];
	u16_t size;
	u16_t id;
	u8_t tkl;
	int r;

	if (ctx.total_size == 0) {
		coap_block_transfer_init(&ctx, COAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	r = coap_update_from_block(request, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, (u8_t *) token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_append_block2_option(&response, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	size = min(coap_block_size_to_bytes(ctx.block_size),
		   ctx.total_size - ctx.current);

	(void)memset(payload, 'A', min(size, sizeof(payload)));

	r = coap_packet_append_payload(&response, (u8_t *)payload, size);
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_next_block(&response, &ctx);
	if (!r) {
		/* Will return 0 when it's the last block. */
		(void)memset(&ctx, 0, sizeof(ctx));
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int get_option_int(const struct coap_packet *pkt, u8_t opt)
{
	struct coap_option option = {};
	int r;

	r = coap_find_options(pkt, opt, &option, 1);
	if (r <= 0) {
		return -ENOENT;
	}

	return coap_option_value_to_int(&option);
}

static int large_update_put(struct coap_resource *resource,
			    struct coap_packet *request)
{
	static struct coap_block_context ctx;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t token[8];
	u16_t len, id, offset;
	u8_t code;
	u8_t type;
	u8_t tkl;
	int r;
	bool last_block;

	r = get_option_int(request, COAP_OPTION_BLOCK1);
	if (r < 0) {
		return -EINVAL;
	}

	last_block = !GET_MORE(r);

	/* initialize block context upon the arrival of first block */
	if (!GET_BLOCK_NUM(r)) {
		coap_block_transfer_init(&ctx, COAP_BLOCK_64, 0);
	}

	r = coap_update_from_block(request, &ctx);
	if (r < 0) {
		NET_ERR("Invalid block size option from request");
		return -EINVAL;
	}

	frag = coap_packet_get_payload(request, &offset, &len);
	if (!last_block && frag == NULL && len == 0) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	NET_INFO("**************\n");
	NET_INFO("[ctx] current %u block_size %u total_size %u\n",
		 ctx.current, coap_block_size_to_bytes(ctx.block_size),
		 ctx.total_size);
	NET_INFO("**************\n");

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	/* Do something with the payload */

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	if (!last_block) {
		code = COAP_RESPONSE_CODE_CONTINUE;
	} else {
		code = COAP_RESPONSE_CODE_CHANGED;
	}

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, (u8_t *) token, code, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_append_block1_option(&response, &ctx);
	if (r < 0) {
		NET_ERR("Could not add Block1 option to response");
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int large_create_post(struct coap_resource *resource,
			     struct coap_packet *request)
{
	static struct coap_block_context ctx;
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t token[8];
	u8_t code, type;
	u16_t len, id, offset;
	u8_t tkl;
	int r;
	bool last_block;

	r = get_option_int(request, COAP_OPTION_BLOCK1);
	if (r < 0) {
		return -EINVAL;
	}

	last_block = !GET_MORE(r);

	/* initialize block context upon the arrival of first block */
	if (!GET_BLOCK_NUM(r)) {
		coap_block_transfer_init(&ctx, COAP_BLOCK_32, 0);
	}

	r = coap_update_from_block(request, &ctx);
	if (r < 0) {
		NET_ERR("Invalid block size option from request");
		return -EINVAL;
	}

	frag = coap_packet_get_payload(request, &offset, &len);
	if (!last_block && frag == NULL && len == 0) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	get_from_ip_addr(request, &from);
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);
	net_pkt_frag_add(pkt, frag);

	if (!last_block) {
		code = COAP_RESPONSE_CODE_CONTINUE;
	} else {
		code = COAP_RESPONSE_CODE_CREATED;
	}

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, (u8_t *)token, code, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_append_block1_option(&response, &ctx);
	if (r < 0) {
		NET_ERR("Could not add Block1 option to response");
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static void update_counter(struct k_work *work)
{
	obs_counter++;

	if (resource_to_notify) {
		coap_resource_notify(resource_to_notify);
	}

	k_delayed_work_submit(&observer_work, 5 * MSEC_PER_SEC);
}

static int send_notification_packet(const struct sockaddr *addr, u16_t age,
				    socklen_t addrlen, u16_t id,
				    const u8_t *token, u8_t tkl,
				    bool is_response)
{
	struct coap_packet response;
	struct coap_pending *pending;
	struct net_pkt *pkt;
	struct net_buf *frag;
	char payload[14];
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

	r = coap_packet_append_option(&response, COAP_OPTION_CONTENT_FORMAT,
				      &plain_text_format,
				      sizeof(plain_text_format));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Counter: %d\n", obs_counter);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)payload,
				       strlen(payload));
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

static int obs_get(struct coap_resource *resource,
		   struct coap_packet *request)
{
	struct coap_observer *observer;
	struct sockaddr_in6 from;
	u8_t token[8];
	u8_t code, type;
	u16_t id;
	u8_t tkl;
	bool observe = true;

	get_from_ip_addr(request, &from);

	if (!coap_request_is_observe(request)) {
		observe = false;
		goto done;
	}

	observer = coap_observer_next_unused(observers, NUM_OBSERVERS);
	if (!observer) {
		return -ENOMEM;
	}

	coap_observer_init(observer, request,
			   (const struct sockaddr *)&from);

	coap_register_observer(resource, observer);

	resource_to_notify = resource;

done:
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	return send_notification_packet((const struct sockaddr *)&from,
					observe ? resource->age : 0,
					sizeof(struct sockaddr_in6), id,
					token, tkl, true);
}

static void obs_notify(struct coap_resource *resource,
		       struct coap_observer *observer)
{
	send_notification_packet(&observer->addr, resource->age,
				 sizeof(observer->addr), 0,
				 observer->token, observer->tkl, false);
}

static int core_get(struct coap_resource *resource,
		     struct coap_packet *request)
{
	static const char dummy_str[] = "Just a test\n";
	struct net_pkt *pkt;
	struct net_buf *frag;
	struct sockaddr_in6 from;
	struct coap_packet response;
	u8_t tkl;
	u8_t token[8];
	u16_t id;
	int r;

	get_from_ip_addr(request, &from);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	pkt = net_pkt_get_tx(context, K_FOREVER);
	frag = net_pkt_get_data(context, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	r = coap_packet_init(&response, pkt, 1, COAP_TYPE_ACK,
			     tkl, (u8_t *)token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	r = coap_packet_append_payload(&response, (u8_t *)dummy_str,
				       sizeof(dummy_str));
	if (r < 0) {
		net_pkt_unref(pkt);
		return -EINVAL;
	}

	return net_context_sendto(pkt, (const struct sockaddr *)&from,
				  sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static const char * const test_path[] = { "test", NULL };

static const char * const segments_path[] = { "seg1", "seg2", "seg3", NULL };

static const char * const query_path[] = { "query", NULL };

static const char * const separate_path[] = { "separate", NULL };

static const char * const large_path[] = { "large", NULL };

static const char * const location_query_path[] = { "location-query", NULL };

static const char * const large_update_path[] = { "large-update", NULL };

static const char * const large_create_path[] = { "large-create", NULL };

static const char * const obs_path[] = { "obs", NULL };

static const char * const core_1_path[] = { "core1", NULL };
static const char * const core_1_attributes[] = {
	"title=\"Core 1\"",
	"rt=core1",
	NULL };

static const char * const core_2_path[] = { "core2", NULL };
static const char * const core_2_attributes[] = {
	"title=\"Core 1\"",
	"rt=core1",
	NULL };

static struct coap_resource resources[] = {
	{ .get = piggyback_get,
	  .post = test_post,
	  .del = test_del,
	  .put = test_put,
	  .path = test_path
	},
	{ .get = piggyback_get,
	  .path = segments_path,
	},
	{ .get = query_get,
	  .path = query_path,
	},
	{ .get = separate_get,
	  .path = separate_path,
	},
	{ .path = large_path,
	  .get = large_get,
	},
	{ .path = location_query_path,
	  .post = location_query_post,
	},
	{ .path = large_update_path,
	  .put = large_update_put,
	},
	{ .path = large_create_path,
	  .post = large_create_post,
	},
	{ .path = obs_path,
	  .get = obs_get,
	  .notify = obs_notify,
	},
	{ .get = well_known_core_get,
	  .path = COAP_WELL_KNOWN_CORE_PATH,
	},
	{ .get = well_known_core_get,
	  .path = COAP_WELL_KNOWN_CORE_PATH,
	},
	{ .get = core_get,
	  .path = core_1_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = core_1_attributes,
			}),
	},
	{ .get = core_get,
	  .path = core_2_path,
	  .user_data = &((struct coap_core_metadata) {
			  .attributes = core_2_attributes,
			}),
	},
	{ },
};

static struct coap_resource *find_resouce_by_observer(
	struct coap_resource *resources, struct coap_observer *o)
{
	struct coap_resource *r;

	for (r = resources; r && r->path; r++) {
		sys_snode_t *node;

		SYS_SLIST_FOR_EACH_NODE(&r->observers, node) {
			if (&o->list == node) {
				return r;
			}
		}
	}

	return NULL;
}

static void udp_receive(struct net_context *context,
			struct net_pkt *pkt,
			int status,
			void *user_data)
{
	struct coap_packet request;
	struct coap_pending *pending;
	struct sockaddr_in6 from;
	struct coap_option options[16] = { 0 };
	u8_t opt_num = 16;
	int r;

	r = coap_packet_parse(&request, pkt, options, opt_num);
	if (r < 0) {
		NET_ERR("Invalid data received (%d)\n", r);
		net_pkt_unref(pkt);
		return;
	}

	get_from_ip_addr(&request, &from);
	pending = coap_pending_received(&request, pendings,
					NUM_PENDINGS);
	if (!pending) {
		goto not_found;
	}

	if (coap_header_get_type(&request) == COAP_TYPE_RESET) {
		struct coap_resource *r;
		struct coap_observer *o;

		o = coap_find_observer_by_addr(observers, NUM_OBSERVERS,
					       (struct sockaddr *)&from);
		if (!o) {
			NET_ERR("Observer not found\n");
			goto not_found;
		}

		r = find_resouce_by_observer(resources, o);
		if (!r) {
			NET_ERR("Observer found but Resource not found\n");
			goto not_found;
		}

		coap_remove_observer(r, o);
	}

	net_pkt_unref(pkt);
	return;

not_found:
	r = coap_handle_request(&request, resources, options, opt_num);
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
		NET_ERR("Could not get te default interface\n");
		return false;
	}

#if defined(CONFIG_NET_CONFIG_SETTINGS)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			  &my_addr) < 0) {
		NET_ERR("Invalid IPv6 address %s",
			CONFIG_NET_CONFIG_MY_IPV6_ADDR);
	}
#endif

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Could not add unicast address to interface");
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

static void retransmit_request(struct k_work *work)
{
	struct coap_pending *pending;
	int r;

	pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	/* ref to avoid being freed by sendto() */
	net_pkt_ref(pending->pkt);

	r = net_context_sendto(pending->pkt, &pending->addr,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		/* no error, keeps retry */
		net_pkt_unref(pending->pkt);
	}

	if (!coap_pending_cycle(pending)) {
		/* last retransmit, clear pending and unreference packet */
		coap_pending_clear(pending);
		return;
	}

	/* unref to balance ref made previously */
	net_pkt_unref(pending->pkt);
	k_delayed_work_submit(&retransmit_work, pending->timeout);
}

void main(void)
{
	static struct sockaddr_in6 any_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = IN6ADDR_ANY_INIT,
		.sin6_port = htons(MY_COAP_PORT) };
	int r;

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

	k_delayed_work_init(&retransmit_work, retransmit_request);

	k_delayed_work_init(&observer_work, update_counter);
	k_delayed_work_submit(&observer_work, 5 * MSEC_PER_SEC);

	r = net_context_recv(context, udp_receive, 0, NULL);
	if (r) {
		NET_ERR("Could not receive in the context\n");
		return;
	}
}
