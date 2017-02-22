/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "zoap-server"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <misc/printk.h>

#include <zephyr.h>

#include <misc/byteorder.h>
#include <misc/util.h>
#include <net/buf.h>
#include <net/nbuf.h>
#include <net/net_ip.h>

#include <net/zoap.h>
#include <net/zoap_link_format.h>

#if defined(CONFIG_NET_L2_BLUETOOTH)
#include <bluetooth/bluetooth.h>
#include <gatt/ipss.h>
#endif

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

static struct net_context *context;

static const uint8_t plain_text_format;

static struct zoap_observer observers[NUM_OBSERVERS];

static struct zoap_pending pendings[NUM_PENDINGS];

static struct net_context *context;

static struct k_delayed_work observer_work;

static int obs_counter;

static struct zoap_resource *resource_to_notify;

struct k_delayed_work retransmit_work;

static int test_del(struct zoap_resource *resource,
		    struct zoap_packet *request,
		    const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t tkl, code, type;
	const uint8_t *token;
	uint16_t id;
	int r;

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		type = ZOAP_TYPE_ACK;
	} else {
		type = ZOAP_TYPE_NON_CON;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_DELETED);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int test_put(struct zoap_resource *resource,
		    struct zoap_packet *request,
		    const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type, tkl;
	const uint8_t *token;
	uint16_t len, id;
	int r;

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		type = ZOAP_TYPE_ACK;
	} else {
		type = ZOAP_TYPE_NON_CON;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CHANGED);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int test_post(struct zoap_resource *resource,
		     struct zoap_packet *request,
		     const struct sockaddr *from)
{
	static const char * const location_path[] = { "location1",
						      "location2",
						      "location3",
						      NULL };
	const char * const *p;
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type, tkl;
	const uint8_t *token;
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
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		type = ZOAP_TYPE_ACK;
	} else {
		type = ZOAP_TYPE_NON_CON;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CREATED);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	for (p = location_path; *p; p++) {
		zoap_add_option(&response, ZOAP_OPTION_LOCATION_PATH,
				*p, strlen(*p));
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int location_query_post(struct zoap_resource *resource,
			       struct zoap_packet *request,
			       const struct sockaddr *from)
{
	static const char *const location_query[] = { "first=1",
						      "second=2",
						      NULL };
	const char * const *p;
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, code, type, tkl;
	const uint8_t *token;
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
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		type = ZOAP_TYPE_ACK;
	} else {
		type = ZOAP_TYPE_NON_CON;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CREATED);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	for (p = location_query; *p; p++) {
		zoap_add_option(&response, ZOAP_OPTION_LOCATION_QUERY,
				*p, strlen(*p));
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int piggyback_get(struct zoap_resource *resource,
			 struct zoap_packet *request,
			 const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const uint8_t *token;
	uint8_t *payload, code, type;
	uint16_t len, id;
	uint8_t tkl;
	int r;

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);

	if (type == ZOAP_TYPE_CON) {
		type = ZOAP_TYPE_ACK;
	} else {
		type = ZOAP_TYPE_NON_CON;
	}

	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &plain_text_format, sizeof(plain_text_format));
	if (r < 0) {
		return -EINVAL;
	}

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, len,
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
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
	uint8_t *payload, code, type, tkl;
	const uint8_t *token;
	uint16_t len, id;
	int i, r;

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	r = zoap_find_options(request, ZOAP_OPTION_URI_QUERY, options, 4);
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

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

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
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &plain_text_format, sizeof(plain_text_format));
	if (r < 0) {
		return -EINVAL;
	}

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, len,
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
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

static int separate_get(struct zoap_resource *resource,
			struct zoap_packet *request,
			const struct sockaddr *from)
{
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	struct zoap_pending *pending;
	uint8_t *payload, code, type, tkl;
	const uint8_t *token;
	uint16_t len, id;
	int r;

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	if (type == ZOAP_TYPE_NON_CON) {
		goto done;
	}

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, 0);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	r = net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
	if (r < 0) {
		return -EINVAL;
	}

done:
	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		type = ZOAP_TYPE_CON;
	} else {
		type = ZOAP_TYPE_NON_CON;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &plain_text_format, sizeof(plain_text_format));
	if (r < 0) {
		return -EINVAL;
	}

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, len,
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0 || r > len) {
		return -EINVAL;
	}

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		pending = zoap_pending_next_unused(pendings, NUM_PENDINGS);
		if (!pending) {
			return -EINVAL;
		}

		r = zoap_pending_init(pending, &response, from);
		if (r) {
			return -EINVAL;
		}

		zoap_pending_cycle(pending);
		pending = zoap_pending_next_to_expire(pendings, NUM_PENDINGS);

		k_delayed_work_submit(&retransmit_work, pending->timeout);
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int large_get(struct zoap_resource *resource,
		     struct zoap_packet *request,
		     const struct sockaddr *from)
{
	static struct zoap_block_context ctx;
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const uint8_t *token;
	uint8_t *payload, code, type;
	uint16_t len, id, size;
	uint8_t tkl;
	int r;

	if (ctx.total_size == 0) {
		zoap_block_transfer_init(&ctx, ZOAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	r = zoap_update_from_block(request, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

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
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &plain_text_format, sizeof(plain_text_format));
	if (r < 0) {
		return -EINVAL;
	}

	r = zoap_add_block2_option(&response, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	size = min(zoap_block_size_to_bytes(ctx.block_size),
		   ctx.total_size - ctx.current);

	if (len < size) {
		return -ENOMEM;
	}

	memset(payload, 'A', size);

	r = zoap_packet_set_used(&response, size);
	if (r) {
		return -EINVAL;
	}

	r = zoap_next_block(&ctx);
	if (!r) {
		/* Will return 0 when it's the last block. */
		memset(&ctx, 0, sizeof(ctx));
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int large_update_put(struct zoap_resource *resource,
			    struct zoap_packet *request,
			    const struct sockaddr *from)
{
	static struct zoap_block_context ctx;
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const uint8_t *token;
	uint8_t *payload, code, type;
	uint16_t len, id;
	uint8_t tkl;
	int r;

	if (ctx.total_size == 0) {
		zoap_block_transfer_init(&ctx, ZOAP_BLOCK_64, 0);
	}

	r = zoap_update_from_block(request, &ctx);
	if (r < 0) {
		NET_ERR("Invalid block size option from request");
		return -EINVAL;
	}

	NET_INFO("**************\n");
	NET_INFO("[ctx] current %u block_size %u total_size %u\n",
		 ctx.current, zoap_block_size_to_bytes(ctx.block_size),
		 ctx.total_size);
	NET_INFO("**************\n");

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	/* Do something with the payload */

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

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
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_block2_option(&response, &ctx);
	if (r < 0) {
		NET_ERR("Could not add Block2 option to response");
		return -EINVAL;
	}

	r = zoap_add_block1_option(&response, &ctx);
	if (r < 0) {
		NET_ERR("Could not add Block1 option to response");
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static int large_create_post(struct zoap_resource *resource,
			     struct zoap_packet *request,
			     const struct sockaddr *from)
{
	static struct zoap_block_context ctx;
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	const uint8_t *token;
	uint8_t *payload, code, type;
	uint16_t len, id;
	uint8_t tkl;
	int r;

	if (ctx.total_size == 0) {
		zoap_block_transfer_init(&ctx, ZOAP_BLOCK_32, 0);
	}

	r = zoap_update_from_block(request, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	payload = zoap_packet_get_payload(request, &len);
	if (!payload) {
		NET_ERR("Packet without payload\n");
		return -EINVAL;
	}

	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);
	zoap_header_set_type(&response, ZOAP_TYPE_ACK);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTINUE);
	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	r = zoap_add_block2_option(&response, &ctx);
	if (r < 0) {
		NET_ERR("Could not add Block2 option to response");
		return -EINVAL;
	}

	r = zoap_add_block1_option(&response, &ctx);
	if (r < 0) {
		NET_ERR("Could not add Block1 option to response");
		return -EINVAL;
	}

	return net_context_sendto(buf, from, sizeof(struct sockaddr_in6),
				  NULL, 0, NULL, NULL);
}

static void update_counter(struct k_work *work)
{
	obs_counter++;

	if (resource_to_notify) {
		zoap_resource_notify(resource_to_notify);
	}

	k_delayed_work_submit(&observer_work, 5 * MSEC_PER_SEC);
}

static int send_notification_packet(const struct sockaddr *addr, uint16_t age,
				    socklen_t addrlen, uint16_t id,
				    const uint8_t *token, uint8_t tkl,
				    bool is_response)
{
	struct zoap_packet response;
	struct zoap_pending *pending;
	struct net_buf *buf, *frag;
	uint8_t *payload, type = ZOAP_TYPE_CON;
	uint16_t len;
	int r;

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

	net_buf_frag_add(buf, frag);

	r = zoap_packet_init(&response, buf);
	if (r < 0) {
		return -EINVAL;
	}

	/* FIXME: Could be that zoap_packet_init() sets some defaults */
	zoap_header_set_version(&response, 1);

	if (is_response) {
		type = ZOAP_TYPE_ACK;
	}

	zoap_header_set_type(&response, type);
	zoap_header_set_code(&response, ZOAP_RESPONSE_CODE_CONTENT);

	if (!is_response) {
		id = zoap_next_id();
	}

	zoap_header_set_id(&response, id);
	zoap_header_set_token(&response, token, tkl);

	if (age >= 2) {
		zoap_add_option_int(&response, ZOAP_OPTION_OBSERVE, age);
	}

	r = zoap_add_option(&response, ZOAP_OPTION_CONTENT_FORMAT,
			    &plain_text_format, sizeof(plain_text_format));
	if (r < 0) {
		return -EINVAL;
	}

	payload = zoap_packet_get_payload(&response, &len);
	if (!payload) {
		return -EINVAL;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, len, "Counter: %d\n", obs_counter);
	if (r < 0 || r > len) {
		return -EINVAL;
	}

	r = zoap_packet_set_used(&response, r);
	if (r) {
		return -EINVAL;
	}

	if (type == ZOAP_TYPE_CON) {
		pending = zoap_pending_next_unused(pendings, NUM_PENDINGS);
		if (!pending) {
			return -EINVAL;
		}

		r = zoap_pending_init(pending, &response, addr);
		if (r) {
			return -EINVAL;
		}

		zoap_pending_cycle(pending);
		pending = zoap_pending_next_to_expire(pendings, NUM_PENDINGS);

		k_delayed_work_submit(&retransmit_work, pending->timeout);
	}

	return net_context_sendto(buf, addr, addrlen, NULL, 0, NULL, NULL);
}

static int obs_get(struct zoap_resource *resource,
		   struct zoap_packet *request,
		   const struct sockaddr *from)
{
	struct zoap_observer *observer;
	const uint8_t *token;
	uint8_t code, type;
	uint16_t id;
	uint8_t tkl;
	bool observe = true;

	if (!zoap_request_is_observe(request)) {
		observe = false;
		goto done;
	}

	observer = zoap_observer_next_unused(observers, NUM_OBSERVERS);
	if (!observer) {
		return -ENOMEM;
	}

	zoap_observer_init(observer, request, from);

	zoap_register_observer(resource, observer);

	resource_to_notify = resource;

done:
	code = zoap_header_get_code(request);
	type = zoap_header_get_type(request);
	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	NET_INFO("*******\n");
	NET_INFO("type: %u code %u id %u\n", type, code, id);
	NET_INFO("*******\n");

	return send_notification_packet(from, observe ? resource->age : 0,
					sizeof(struct sockaddr_in6), id,
					token, tkl, true);
}

static void obs_notify(struct zoap_resource *resource,
		       struct zoap_observer *observer)
{
	send_notification_packet(&observer->addr, resource->age,
				 sizeof(observer->addr), 0,
				 observer->token, observer->tkl, false);
}

static int core_get(struct zoap_resource *resource,
		     struct zoap_packet *request,
		     const struct sockaddr *from)
{
	static const char dummy_str[] = "Just a test\n";
	struct net_buf *buf, *frag;
	struct zoap_packet response;
	uint8_t *payload, tkl;
	const uint8_t *token;
	uint16_t len, id;
	int r;

	id = zoap_header_get_id(request);
	token = zoap_header_get_token(request, &tkl);

	buf = net_nbuf_get_tx(context, K_FOREVER);
	frag = net_nbuf_get_data(context, K_FOREVER);

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
	zoap_header_set_token(&response, token, tkl);

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
	ZOAP_WELL_KNOWN_CORE_RESOURCE,
	{ .get = core_get,
	  .path = core_1_path,
	  .user_data = &((struct zoap_core_metadata) {
			  .attributes = core_1_attributes,
			}),
	},
	{ .get = core_get,
	  .path = core_2_path,
	  .user_data = &((struct zoap_core_metadata) {
			  .attributes = core_2_attributes,
			}),
	},
	{ },
};

static struct zoap_resource *find_resouce_by_observer(
	struct zoap_resource *resources, struct zoap_observer *o)
{
	struct zoap_resource *r;

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
			struct net_buf *buf,
			int status,
			void *user_data)
{
	struct zoap_packet request;
	struct zoap_pending *pending;
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
		net_nbuf_unref(buf);
		return;
	}

	pending = zoap_pending_received(&request, pendings,
					NUM_PENDINGS);
	if (pending) {
		net_nbuf_unref(buf);
		return;
	}

	if (zoap_header_get_type(&request) == ZOAP_TYPE_RESET) {
		struct zoap_resource *r;
		struct zoap_observer *o;

		o = zoap_find_observer_by_addr(observers, NUM_OBSERVERS,
					       (struct sockaddr *)&from);
		if (!o) {
			goto not_found;
		}

		r = find_resouce_by_observer(resources, o);
		if (!r) {
			goto not_found;
		}

		zoap_remove_observer(r, o);
	}

not_found:
	r = zoap_handle_request(&request, resources,
				(const struct sockaddr *) &from);

	net_nbuf_unref(buf);

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

#if defined(CONFIG_NET_SAMPLES_IP_ADDRESSES)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_SAMPLES_MY_IPV6_ADDR,
			  &my_addr) < 0) {
		NET_ERR("Invalid IPv6 address %s",
			CONFIG_NET_SAMPLES_MY_IPV6_ADDR);
	}
#endif

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
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
	struct zoap_pending *pending;
	int r;

	pending = zoap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	r = net_context_sendto(pending->buf, &pending->addr,
			       sizeof(struct sockaddr_in6),
			       NULL, 0, NULL, NULL);
	if (r < 0) {
		return;
	}

	if (!zoap_pending_cycle(pending)) {
		zoap_pending_clear(pending);
		return;
	}

	k_delayed_work_submit(&retransmit_work, pending->timeout);
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
