/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_coap_service_sample);

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/coap_service.h>

#define BLOCK_WISE_TRANSFER_SIZE_GET 2048

#define LARGE_MAX_CLIENTS CONFIG_NET_SAMPLE_COAP_MAX_LARGE_TRANSFERS
#define LARGE_TRANSFER_TIMEOUT_MSEC                                   \
	(CONFIG_NET_SAMPLE_COAP_LARGE_TRANSFER_TIMEOUT_SEC * MSEC_PER_SEC)

struct large_client_ctx {
	struct sockaddr_storage addr;
	socklen_t addr_len;
	bool in_use;
	int64_t last_seen;
	struct coap_block_context block;
};

static struct large_client_ctx large_get_clients[LARGE_MAX_CLIENTS];
static struct large_client_ctx large_update_clients[LARGE_MAX_CLIENTS];
static struct large_client_ctx large_create_clients[LARGE_MAX_CLIENTS];

static struct large_client_ctx *large_ctx_lookup(struct large_client_ctx *pool,
						  size_t pool_len,
						  const struct sockaddr *addr,
						  socklen_t addr_len)
{
	struct large_client_ctx *free_slot = NULL;
	struct large_client_ctx *oldest = NULL;
	int64_t now = k_uptime_get();

	for (size_t i = 0; i < pool_len; i++) {
		if (pool[i].in_use && pool[i].addr_len == addr_len &&
		    memcmp(&pool[i].addr, addr, addr_len) == 0) {
			pool[i].last_seen = now;
			return &pool[i];
		}

		if (!pool[i].in_use) {
			if (free_slot == NULL) {
				free_slot = &pool[i];
			}
		} else if (oldest == NULL || pool[i].last_seen < oldest->last_seen) {
			oldest = &pool[i];
		}
	}

	if (free_slot == NULL) {
		/* Pool is full. Only reuse a slot that's been quiet for a while.
		 * Otherwise reject the request so we don't corrupt an active transfer.
		 */
		if (oldest == NULL || now - oldest->last_seen < LARGE_TRANSFER_TIMEOUT_MSEC) {
			LOG_WRN("Client pool exhausted, rejecting new block-wise transfer");
			return NULL;
		}

		LOG_WRN("Reusing slot from a stalled block-wise transfer");
		free_slot = oldest;
	}

	free_slot->in_use = true;
	free_slot->addr_len = addr_len;
	free_slot->last_seen = now;
	memcpy(&free_slot->addr, addr, addr_len);
	memset(&free_slot->block, 0, sizeof(free_slot->block));

	return free_slot;
}

static int large_reply_busy(struct coap_resource *resource, struct coap_packet *request,
			    struct sockaddr *addr, socklen_t addr_len, uint8_t *data,
			    size_t data_len)
{
	struct coap_packet response;
	int r;

	r = coap_ack_init(&response, request, data, data_len,
			  COAP_RESPONSE_CODE_SERVICE_UNAVAILABLE);
	if (r < 0) {
		return r;
	}

	return coap_resource_send(resource, &response, addr, addr_len, NULL);
}

static int large_get(struct coap_resource *resource,
		     struct coap_packet *request,
		     struct sockaddr *addr, socklen_t addr_len)

{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct large_client_ctx *client = large_ctx_lookup(
		large_get_clients, ARRAY_SIZE(large_get_clients), addr, addr_len);
	struct coap_block_context *ctx;
	struct coap_packet response;
	uint8_t payload[64];
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t size;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;

	if (client == NULL) {
		return large_reply_busy(resource, request, addr, addr_len, data, sizeof(data));
	}

	ctx = &client->block;

	if (ctx->total_size == 0) {
		coap_block_transfer_init(ctx, COAP_BLOCK_64, BLOCK_WISE_TRANSFER_SIZE_GET);
	}

	r = coap_update_from_block(request, ctx);
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

	r = coap_packet_init(&response, data, sizeof(data),
			     COAP_VERSION_1, COAP_TYPE_ACK, tkl, token,
			     COAP_RESPONSE_CODE_CONTENT, id);
	if (r < 0) {
		return -EINVAL;
	}

	r = coap_append_option_int(&response, COAP_OPTION_CONTENT_FORMAT,
				   COAP_CONTENT_FORMAT_TEXT_PLAIN);
	if (r < 0) {
		return r;
	}

	r = coap_append_block2_option(&response, ctx);
	if (r < 0) {
		return r;
	}

	r = coap_packet_append_payload_marker(&response);
	if (r < 0) {
		return r;
	}

	size = MIN(coap_block_size_to_bytes(ctx->block_size),
		   ctx->total_size - ctx->current);

	memset(payload, 'A', MIN(size, sizeof(payload)));

	r = coap_packet_append_payload(&response, (uint8_t *)payload, size);
	if (r < 0) {
		return r;
	}

	r = coap_next_block(&response, ctx);
	if (!r) {
		/* Will return 0 when it's the last block. */
		client->in_use = false;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static int large_update_put(struct coap_resource *resource,
			    struct coap_packet *request,
			    struct sockaddr *addr, socklen_t addr_len)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct large_client_ctx *client = large_ctx_lookup(
		large_update_clients, ARRAY_SIZE(large_update_clients), addr, addr_len);
	struct coap_block_context *ctx;
	struct coap_packet response;
	const uint8_t *payload;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t id;
	uint16_t len;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;
	bool last_block;

	if (client == NULL) {
		return large_reply_busy(resource, request, addr, addr_len, data, sizeof(data));
	}

	ctx = &client->block;

	r = coap_get_option_int(request, COAP_OPTION_BLOCK1);
	if (r < 0) {
		return -EINVAL;
	}

	last_block = !GET_MORE(r);

	/* initialize block context upon the arrival of first block */
	if (!GET_BLOCK_NUM(r)) {
		coap_block_transfer_init(ctx, COAP_BLOCK_64, 0);
	}

	r = coap_update_from_block(request, ctx);
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
		ctx->current, coap_block_size_to_bytes(ctx->block_size),
		ctx->total_size);
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
		client->in_use = false;
	}

	r = coap_ack_init(&response, request, data, sizeof(data), code);
	if (r < 0) {
		return r;
	}

	r = coap_append_block1_option(&response, ctx);
	if (r < 0) {
		LOG_ERR("Could not add Block1 option to response");
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static int large_create_post(struct coap_resource *resource,
			     struct coap_packet *request,
			     struct sockaddr *addr, socklen_t addr_len)
{
	uint8_t data[CONFIG_COAP_SERVER_MESSAGE_SIZE];
	struct large_client_ctx *client = large_ctx_lookup(
		large_create_clients, ARRAY_SIZE(large_create_clients), addr, addr_len);
	struct coap_block_context *ctx;
	struct coap_packet response;
	const uint8_t *payload;
	uint8_t token[COAP_TOKEN_MAX_LEN];
	uint16_t len;
	uint16_t id;
	uint8_t code;
	uint8_t type;
	uint8_t tkl;
	int r;
	bool last_block;

	if (client == NULL) {
		return large_reply_busy(resource, request, addr, addr_len, data, sizeof(data));
	}

	ctx = &client->block;

	r = coap_get_option_int(request, COAP_OPTION_BLOCK1);
	if (r < 0) {
		return -EINVAL;
	}

	last_block = !GET_MORE(r);

	/* initialize block context upon the arrival of first block */
	if (!GET_BLOCK_NUM(r)) {
		coap_block_transfer_init(ctx, COAP_BLOCK_32, 0);
	}

	r = coap_update_from_block(request, ctx);
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
		client->in_use = false;
	}

	r = coap_ack_init(&response, request, data, sizeof(data), code);
	if (r < 0) {
		return r;
	}

	r = coap_append_block1_option(&response, ctx);
	if (r < 0) {
		LOG_ERR("Could not add Block1 option to response");
		return r;
	}

	r = coap_resource_send(resource, &response, addr, addr_len, NULL);

	return r;
}

static const char * const large_path[] = { "large", NULL };
COAP_RESOURCE_DEFINE(large, coap_server,
{
	.get = large_get,
	.path = large_path,
});

static const char * const large_update_path[] = { "large-update", NULL };
COAP_RESOURCE_DEFINE(large_update, coap_server,
{
	.put = large_update_put,
	.path = large_update_path,
});

static const char * const large_create_path[] = { "large-create", NULL };
COAP_RESOURCE_DEFINE(large_create, coap_server,
{
	.post = large_create_post,
	.path = large_create_path,
});
