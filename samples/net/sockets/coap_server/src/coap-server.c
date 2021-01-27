/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_coap_server_sample, LOG_LEVEL_DBG);

#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>
#include <net/coap_link_format.h>

#include "net_private.h"
#if defined(CONFIG_NET_IPV6)
#include "ipv6.h"
#endif

#define MAX_COAP_MSG_LEN 256

#define MY_COAP_PORT 5683

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048

#if defined(CONFIG_NET_IPV6)
#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#define MY_IP6ADDR \
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }
#endif

#define NUM_OBSERVERS 3

#define NUM_PENDINGS 3

/* CoAP socket fd */
static int sock;

static struct coap_observer observers[NUM_OBSERVERS];

static struct coap_pending pendings[NUM_PENDINGS];

static struct k_delayed_work observer_work;

static int obs_counter;

static struct coap_resource *resource_to_notify;

static struct k_delayed_work retransmit_work;

#if defined(CONFIG_NET_IPV6)
static bool join_coap_multicast_group(void)
{
	static struct in6_addr my_addr = MY_IP6ADDR;
	static struct sockaddr_in6 mcast_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.sin6_port = htons(MY_COAP_PORT) };
	struct net_if_addr *ifaddr;
	struct net_if *iface;
	int ret;

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("Could not get te default interface\n");
		return false;
	}

#if defined(CONFIG_NET_CONFIG_SETTINGS)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			  &my_addr) < 0) {
		LOG_ERR("Invalid IPv6 address %s",
			CONFIG_NET_CONFIG_MY_IPV6_ADDR);
	}
#endif

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Could not add unicast address to interface");
		return false;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ret = net_ipv6_mld_join(iface, &mcast_addr.sin6_addr);
	if (ret < 0) {
		LOG_ERR("Cannot join %s IPv6 multicast group (%d)",
			log_strdup(net_sprint_ipv6_addr(&mcast_addr.sin6_addr)), ret);
		return false;
	}

	return true;
}
#endif

#if defined(CONFIG_NET_IPV6)
static int start_coap_server(void)
{
	struct sockaddr_in6 addr6;
	int r;

	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
	addr6.sin6_port = htons(MY_COAP_PORT);

	sock = socket(addr6.sin6_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	r = bind(sock, (struct sockaddr *)&addr6, sizeof(addr6));
	if (r < 0) {
		LOG_ERR("Failed to bind UDP socket %d", errno);
		return -errno;
	}

	return 0;
}
#endif

#if defined(CONFIG_NET_IPV4)
static int start_coap_server(void)
{
	struct sockaddr_in addr;
	int r;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MY_COAP_PORT);

	sock = socket(addr.sin_family, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		LOG_ERR("Failed to create UDP socket %d", errno);
		return -errno;
	}

	r = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	if (r < 0) {
		LOG_ERR("Failed to bind UDP socket %d", errno);
		return -errno;
	}

	return 0;
}
#endif

static int send_coap_reply(struct coap_packet *cpkt,
			   const struct sockaddr *addr,
			   socklen_t addr_len)
{
	int r;

	net_hexdump("Response", cpkt->data, cpkt->offset);

	r = sendto(sock, cpkt->data, cpkt->offset, 0, addr, addr_len);
	if (r < 0) {
		LOG_ERR("Failed to send %d", errno);
		r = -errno;
	}

	return r;
}

static int well_known_core_get(struct coap_resource *resource,
			       struct coap_packet *request,
			       struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	uint8_t *data;
	int r;

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_well_known_core_get(resource, request, &response,
				     data, MAX_COAP_MSG_LEN);
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int piggyback_get(struct coap_resource *resource,
			 struct coap_packet *request,
			 struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	uint8_t payload[40];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		goto end;
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		goto end;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int test_del(struct coap_resource *resource,
		    struct coap_packet *request,
		    struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint8_t tkl;
	uint8_t code;
	uint8_t type;
	uint16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_DELETED, id);
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int test_put(struct coap_resource *resource,
		    struct coap_packet *request,
		    struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	const uint8_t *payload;
	uint8_t *data;
	uint16_t payload_len;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	uint16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	payload = coap_packet_get_payload(request, &payload_len);
	if (payload) {
		net_hexdump("PUT Payload", payload, payload_len);
	}

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CHANGED, id);
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int test_post(struct coap_resource *resource,
		     struct coap_packet *request,
		     struct sockaddr *addr, socklen_t addr_len)
{
	static const char * const location_path[] = { "location1",
						      "location2",
						      "location3",
						      NULL };
	const char * const *p;
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	const uint8_t *payload;
	uint8_t *data;
	uint16_t payload_len;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	uint16_t id;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	payload = coap_packet_get_payload(request, &payload_len);
	if (payload) {
		net_hexdump("POST Payload", payload, payload_len);
	}

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CREATED, id);
	if (r < 0) {
		goto end;
	}

	for (p = location_path; *p; p++) {
		r = coap_packet_append_option(&response,
					      COAP_OPTION_LOCATION_PATH,
					      *p, strlen(*p));
		if (r < 0) {
			goto end;
		}
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int query_get(struct coap_resource *resource,
		     struct coap_packet *request,
		     struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_option options[4];
	struct coap_packet response;
	uint8_t payload[40];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int i, r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	r = coap_find_options(request, COAP_OPTION_URI_QUERY, options, 4);
	if (r < 0) {
		return -EINVAL;
	}

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("num queries: %d", r);

	for (i = 0; i < r; i++) {
		char str[16];

		if (options[i].len + 1 > sizeof(str)) {
			LOG_INF("Unexpected length of query: "
				"%d (expected %zu)",
				options[i].len, sizeof(str));
			break;
		}

		memcpy(str, options[i].value, options[i].len);
		str[options[i].len] = '\0';

		LOG_INF("query[%d]: %s", i + 1, str);
	}

	LOG_INF("*******");

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		goto end;
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		goto end;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int location_query_post(struct coap_resource *resource,
			       struct coap_packet *request,
			       struct sockaddr *addr, socklen_t addr_len)
{
	static const char *const location_query[] = { "first=1",
						      "second=2",
						      NULL };
	const char * const *p;
	struct coap_packet response;
	uint8_t *data;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CREATED, id);
	if (r < 0) {
		goto end;
	}

	for (p = location_query; *p; p++) {
		r = coap_packet_append_option(&response,
					      COAP_OPTION_LOCATION_QUERY,
					      *p, strlen(*p));
		if (r < 0) {
			goto end;
		}
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int separate_get(struct coap_resource *resource,
			struct coap_packet *request,
			struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_packet response;
	uint8_t payload[40];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (type == COAP_TYPE_ACK) {
		return 0;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token, 0, id);
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);
	if (r < 0) {
		goto end;
	}

	if (type == COAP_TYPE_CON) {
		type = COAP_TYPE_CON;
	} else {
		type = COAP_TYPE_NON_CON;
	}

	/* Do not free and allocate "data" again, re-use the buffer */
	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		goto end;
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		goto end;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Type: %u\nCode: %u\nMID: %u\n", type, code, id);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int large_get(struct coap_resource *resource,
		     struct coap_packet *request,
		     struct sockaddr *addr, socklen_t addr_len)

{
	static struct coap_block_context ctx;
	struct coap_packet response;
	uint8_t payload[64];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t size;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	if (ctx.total_size == 0) {
		coap_block_transfer_init(&ctx, COAP_BLOCK_64,
					 BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	r = coap_update_from_block(request, &ctx);
	if (r < 0) {
		return -EINVAL;
	}

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		goto end;
	}

	r = coap_append_block2_option(&response, &ctx);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		goto end;
	}

	size = MIN(coap_block_size_to_bytes(ctx.block_size),
		   ctx.total_size - ctx.current);

	memset(payload, 'A', MIN(size, sizeof(payload)));

	r = coap_packet_append_payload(&response, (uint8_t *)payload, size);
	if (r < 0) {
		goto end;
	}

	r = coap_next_block(&response, &ctx);
	if (!r) {
		/* Will return 0 when it's the last block. */
		memset(&ctx, 0, sizeof(ctx));
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int large_update_put(struct coap_resource *resource,
			    struct coap_packet *request,
			    struct sockaddr *addr, socklen_t addr_len)
{
	static struct coap_block_context ctx;
	struct coap_packet response;
	const uint8_t *payload;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t id;
	uint16_t len;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;
	bool last_block;

	r = coap_get_option_int(request, COAP_OPTION_BLOCK1);
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
		LOG_ERR("Invalid block size option from request");
		return -EINVAL;
	}

	payload = coap_packet_get_payload(request, &len);
	if (!last_block && payload == NULL) {
		LOG_ERR("Packet without payload");
		return -EINVAL;
	}

	LOG_INF("**************");
	LOG_INF("[ctx] current %zu block_size %u total_size %zu",
		ctx.current, coap_block_size_to_bytes(ctx.block_size),
		ctx.total_size);
	LOG_INF("**************");

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	/* Do something with the payload */

	if (!last_block) {
		code = COAP_RESPONSE_CODE_CONTINUE;
	} else {
		code = COAP_RESPONSE_CODE_CHANGED;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     code, id);
	if (r < 0) {
		goto end;
	}

	r = coap_append_block1_option(&response, &ctx);
	if (r < 0) {
		LOG_ERR("Could not add Block1 option to response");
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int large_create_post(struct coap_resource *resource,
			     struct coap_packet *request,
			     struct sockaddr *addr, socklen_t addr_len)
{
	static struct coap_block_context ctx;
	struct coap_packet response;
	const uint8_t *payload;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t len;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;
	bool last_block;

	r = coap_get_option_int(request, COAP_OPTION_BLOCK1);
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
		LOG_ERR("Invalid block size option from request");
		return -EINVAL;
	}

	payload = coap_packet_get_payload(request, &len);
	if (!last_block && payload) {
		LOG_ERR("Packet without payload");
		return -EINVAL;
	}

	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	if (!last_block) {
		code = COAP_RESPONSE_CODE_CONTINUE;
	} else {
		code = COAP_RESPONSE_CODE_CREATED;
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     code, id);
	if (r < 0) {
		goto end;
	}

	r = coap_append_block1_option(&response, &ctx);
	if (r < 0) {
		LOG_ERR("Could not add Block1 option to response");
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static int send_notification_packet(const struct sockaddr *addr,
				    socklen_t addr_len,
				    uint16_t age, uint16_t id,
				    const uint8_t *token, uint8_t tkl,
				    bool is_response);

static void retransmit_request(struct k_work *work)
{
	struct coap_pending *pending;

	pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return;
	}

	if (!coap_pending_cycle(pending)) {
		k_free(pending->data);
		coap_pending_clear(pending);
		return;
	}

	k_delayed_work_submit(&retransmit_work, K_MSEC(pending->timeout));
}

static void update_counter(struct k_work *work)
{
	obs_counter++;

	if (resource_to_notify) {
		coap_resource_notify(resource_to_notify);
	}

	k_delayed_work_submit(&observer_work, K_SECONDS(5));
}

static int create_pending_request(struct coap_packet *response,
				  const struct sockaddr *addr)
{
	struct coap_pending *pending;
	int r;

	pending = coap_pending_next_unused(pendings, NUM_PENDINGS);
	if (!pending) {
		return -ENOMEM;
	}

	r = coap_pending_init(pending, response, addr,
			      COAP_DEFAULT_MAX_RETRANSMIT);
	if (r < 0) {
		return -EINVAL;
	}

	coap_pending_cycle(pending);

	pending = coap_pending_next_to_expire(pendings, NUM_PENDINGS);
	if (!pending) {
		return 0;
	}

	k_delayed_work_submit(&retransmit_work, K_MSEC(pending->timeout));

	return 0;
}

static int send_notification_packet(const struct sockaddr *addr,
				    socklen_t addr_len,
				    uint16_t age, uint16_t id,
				    const uint8_t *token, uint8_t tkl,
				    bool is_response)
{
	struct coap_packet response;
	char payload[14];
	uint8_t *data;
	uint8_t type;
	int r;

	if (is_response) {
		type = COAP_TYPE_ACK;
	} else {
		type = COAP_TYPE_CON;
	}

	if (!is_response) {
		id = coap_next_id();
	}

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, type, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		goto end;
	}

	if (age >= 2U) {
		r = coap_append_option_int(&response, COAP_OPTION_OBSERVE, age);
		if (r < 0) {
			goto end;
		}
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		goto end;
	}

	/* The response that coap-client expects */
	r = snprintk((char *) payload, sizeof(payload),
		     "Counter: %d\n", obs_counter);
	if (r < 0) {
		goto end;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)payload,
				       strlen(payload));
	if (r < 0) {
		goto end;
	}

	if (type == COAP_TYPE_CON) {
		r = create_pending_request(&response, addr);
		if (r < 0) {
			goto end;
		}
	}

	k_delayed_work_submit(&observer_work, K_SECONDS(5));

	r = send_coap_reply(&response, addr, addr_len);

	/* On succesfull creation of pending request, do not free memory */
	if (type == COAP_TYPE_CON) {
		return r;
	}

end:
	k_free(data);

	return r;
}

static int obs_get(struct coap_resource *resource,
		   struct coap_packet *request,
		   struct sockaddr *addr, socklen_t addr_len)
{
	struct coap_observer *observer;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	bool observe = true;

	if (!coap_request_is_observe(request)) {
		observe = false;
		goto done;
	}

	observer = coap_observer_next_unused(observers, NUM_OBSERVERS);
	if (!observer) {
		return -ENOMEM;
	}

	coap_observer_init(observer, request, addr);

	coap_register_observer(resource, observer);

	resource_to_notify = resource;

done:
	code = coap_header_get_code(request);
	type = coap_header_get_type(request);
	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	LOG_INF("*******");
	LOG_INF("type: %u code %u id %u", type, code, id);
	LOG_INF("*******");

	return send_notification_packet(addr, addr_len,
					observe ? resource->age : 0,
					id, token, tkl, true);
}

static void obs_notify(struct coap_resource *resource,
		       struct coap_observer *observer)
{
	send_notification_packet(&observer->addr,
				 sizeof(observer->addr),
				 resource->age, 0,
				 observer->token, observer->tkl, false);
}

static int core_get(struct coap_resource *resource,
		    struct coap_packet *request,
		    struct sockaddr *addr, socklen_t addr_len)
{
	static const char dummy_str[] = "Just a test\n";
	struct coap_packet response;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint8_t *data;
	uint16_t id;
	uint8_t tkl;
	int r;

	id = coap_header_get_id(request);
	tkl = coap_header_get_token(request, token);

	data = (uint8_t *)k_malloc(MAX_COAP_MSG_LEN);
	if (!data) {
		return -ENOMEM;
	}

	r = coap_packet_init(&response, data, MAX_COAP_MSG_LEN,
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		r = -EINVAL;
		goto end;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		r = -EINVAL;
		goto end;
	}

	r = coap_packet_append_payload(&response, (uint8_t *)dummy_str,
				       sizeof(dummy_str));
	if (r < 0) {
		r = -EINVAL;
		goto end;
	}

	r = send_coap_reply(&response, addr, addr_len);

end:
	k_free(data);

	return r;
}

static const char * const test_path[] = { "test", NULL };

static const char * const segments_path[] = { "seg1", "seg2", "seg3", NULL };

#if defined(CONFIG_COAP_URI_WILDCARD)
static const char * const wildcard1_path[] = { "wild1", "+", "wild3", NULL };

static const char * const wildcard2_path[] = { "wild2", "#", NULL };
#endif /* CONFIG_COAP_URI_WILDCARD */

static const char * const query_path[] = { "query", NULL };

static const char * const separate_path[] = { "separate", NULL };

static const char * const location_query_path[] = { "location-query", NULL };

static const char * const large_path[] = { "large", NULL };

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
	{ .get = well_known_core_get,
	  .path = COAP_WELL_KNOWN_CORE_PATH,
	},
	{ .get = piggyback_get,
	  .post = test_post,
	  .del = test_del,
	  .put = test_put,
	  .path = test_path
	},
	{ .get = piggyback_get,
	  .path = segments_path,
	},
#if defined(CONFIG_COAP_URI_WILDCARD)
	{ .get = piggyback_get,
	  .path = wildcard1_path,
	},
	{ .get = piggyback_get,
	  .path = wildcard2_path,
	},
#endif /* CONFIG_COAP_URI_WILDCARD */
	{ .get = query_get,
	  .path = query_path,
	},
	{ .get = separate_get,
	  .path = separate_path,
	},
	{ .path = location_query_path,
	  .post = location_query_post,
	},
	{ .path = large_path,
	  .get = large_get,
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

static void process_coap_request(uint8_t *data, uint16_t data_len,
				 struct sockaddr *client_addr,
				 socklen_t client_addr_len)
{
	struct coap_packet request;
	struct coap_pending *pending;
	struct coap_option options[16] = { 0 };
	uint8_t opt_num = 16U;
	uint8_t type;
	int r;

	r = coap_packet_parse(&request, data, data_len, options, opt_num);
	if (r < 0) {
		LOG_ERR("Invalid data received (%d)\n", r);
		return;
	}

	type = coap_header_get_type(&request);

	pending = coap_pending_received(&request, pendings, NUM_PENDINGS);
	if (!pending) {
		goto not_found;
	}

	/* Clear CoAP pending request */
	if (type == COAP_TYPE_ACK) {
		k_free(pending->data);
		coap_pending_clear(pending);
	}

	return;

not_found:

	if (type == COAP_TYPE_RESET) {
		struct coap_resource *r;
		struct coap_observer *o;

		o = coap_find_observer_by_addr(observers, NUM_OBSERVERS,
					       client_addr);
		if (!o) {
			LOG_ERR("Observer not found\n");
			goto end;
		}

		r = find_resouce_by_observer(resources, o);
		if (!r) {
			LOG_ERR("Observer found but Resource not found\n");
			goto end;
		}

		coap_remove_observer(r, o);

		return;
	}

end:
	r = coap_handle_request(&request, resources, options, opt_num,
				client_addr, client_addr_len);
	if (r < 0) {
		LOG_WRN("No handler for such request (%d)\n", r);
	}
}

static int process_client_request(void)
{
	int received;
	struct sockaddr client_addr;
	socklen_t client_addr_len;
	uint8_t request[MAX_COAP_MSG_LEN];

	do {
		client_addr_len = sizeof(client_addr);
		received = recvfrom(sock, request, sizeof(request), 0,
				    &client_addr, &client_addr_len);
		if (received < 0) {
			LOG_ERR("Connection error %d", errno);
			return -errno;
		}

		process_coap_request(request, received, &client_addr,
				     client_addr_len);
	} while (true);

	return 0;
}

void main(void)
{
	int r;

	LOG_DBG("Start CoAP-server sample");

#if defined(CONFIG_NET_IPV6)
	bool res;

	res = join_coap_multicast_group();
	if (!res) {
		goto quit;
	}
#endif

	r = start_coap_server();
	if (r < 0) {
		goto quit;
	}

	k_delayed_work_init(&retransmit_work, retransmit_request);
	k_delayed_work_init(&observer_work, update_counter);

	while (1) {
		r = process_client_request();
		if (r < 0) {
			goto quit;
		}
	}

	LOG_DBG("Done");
	return;

quit:
	LOG_ERR("Quit");
}
