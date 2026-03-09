/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <string.h>

#include "mcp_common.h"
#include "mcp_server_internal.h"

LOG_MODULE_REGISTER(mcp_transport_mock, CONFIG_MCP_LOG_LEVEL);

struct mock_client_context {
	struct mcp_transport_binding binding;
	bool active;
	char last_message[CONFIG_MCP_MAX_MESSAGE_SIZE];
	size_t last_message_len;
	uint32_t last_msg_id;
};

struct mock_transport_context {
	struct mock_client_context clients[CONFIG_MCP_MOCK_MAX_CLIENTS];
	int send_call_count;
	int disconnect_call_count;
	int inject_send_error;
	int inject_disconnect_error;
};

static struct mock_transport_context mock_ctx;

static int mock_transport_send(struct mcp_transport_message *response)
{
	struct mock_client_context *client;
	size_t copy_len;

	if (response == NULL || response->binding == NULL) {
		LOG_ERR("Mock: Invalid response or binding");
		return -EINVAL;
	}

	if (mock_ctx.inject_send_error != 0) {
		LOG_DBG("Mock: Injecting send error %d", mock_ctx.inject_send_error);
		return mock_ctx.inject_send_error;
	}

	mock_ctx.send_call_count++;

	client = (struct mock_client_context *)response->binding->context;

	if (client == NULL || !client->active) {
		LOG_ERR("Mock: Client not found or inactive");
		return -ENOENT;
	}

	copy_len = MIN(response->json_len, sizeof(client->last_message) - 1);

	memcpy(client->last_message, response->json_data, copy_len);
	client->last_message[copy_len] = '\0';
	client->last_message_len = copy_len;
	client->last_msg_id = response->msg_id;

	LOG_DBG("Mock: Sent %zu bytes (msg_id: %u)", response->json_len, response->msg_id);
	return 0;
}

static int mock_transport_disconnect(struct mcp_transport_binding *binding)
{
	struct mock_client_context *client;

	if (binding == NULL) {
		LOG_ERR("Mock: Invalid binding");
		return -EINVAL;
	}

	if (mock_ctx.inject_disconnect_error != 0) {
		LOG_DBG("Mock: Injecting disconnect error %d", mock_ctx.inject_disconnect_error);
		return mock_ctx.inject_disconnect_error;
	}

	mock_ctx.disconnect_call_count++;

	client = (struct mock_client_context *)binding->context;

	if (client == NULL) {
		LOG_WRN("Mock: Client not found for disconnect");
		return -ENOENT;
	}

	client->active = false;
	LOG_DBG("Mock: Disconnected client");
	return 0;
}

static const struct mcp_transport_ops mock_ops = {
	.send = mock_transport_send,
	.disconnect = mock_transport_disconnect,
};

struct mcp_transport_binding *mcp_transport_mock_allocate_client(void)
{
	for (int i = 0; i < ARRAY_SIZE(mock_ctx.clients); i++) {
		if (!mock_ctx.clients[i].active) {
			mock_ctx.clients[i].active = true;
			mock_ctx.clients[i].last_message_len = 0;
			mock_ctx.clients[i].last_msg_id = 0;
			memset(mock_ctx.clients[i].last_message, 0,
			       sizeof(mock_ctx.clients[i].last_message));

			mock_ctx.clients[i].binding.ops = &mock_ops;
			mock_ctx.clients[i].binding.context = &mock_ctx.clients[i];

			LOG_DBG("Mock: Allocated client in slot %d", i);
			return &mock_ctx.clients[i].binding;
		}
	}

	LOG_ERR("Mock: No available client slots");
	return NULL;
}

void mcp_transport_mock_release_client(struct mcp_transport_binding *binding)
{
	struct mock_client_context *client;

	if (binding == NULL) {
		return;
	}

	client = (struct mock_client_context *)binding->context;

	if (client != NULL) {
		client->active = false;
		memset(client->last_message, 0, sizeof(client->last_message));
		client->last_message_len = 0;
		client->last_msg_id = 0;
		LOG_DBG("Mock: Released client");
	}
}

void mcp_transport_mock_inject_send_error(int error)
{
	mock_ctx.inject_send_error = error;
	LOG_DBG("Mock: Will inject send error %d", error);
}

void mcp_transport_mock_inject_disconnect_error(int error)
{
	mock_ctx.inject_disconnect_error = error;
	LOG_DBG("Mock: Will inject disconnect error %d", error);
}

int mcp_transport_mock_get_send_count(void)
{
	return mock_ctx.send_call_count;
}

void mcp_transport_mock_reset_send_count(void)
{
	mock_ctx.send_call_count = 0;
}

int mcp_transport_mock_get_disconnect_count(void)
{
	return mock_ctx.disconnect_call_count;
}

const char *mcp_transport_mock_get_last_message(struct mcp_transport_binding *binding,
						size_t *length)
{
	struct mock_client_context *client;

	if (binding == NULL) {
		if (length != NULL) {
			*length = 0;
		}
		return NULL;
	}

	client = (struct mock_client_context *)binding->context;

	if (client != NULL && client->active) {
		if (length != NULL) {
			*length = client->last_message_len;
		}
		return client->last_message;
	}

	if (length != NULL) {
		*length = 0;
	}
	return NULL;
}

uint32_t mcp_transport_mock_get_last_msg_id(struct mcp_transport_binding *binding)
{
	struct mock_client_context *client;

	if (binding == NULL) {
		return 0;
	}

	client = (struct mock_client_context *)binding->context;

	if (client != NULL && client->active) {
		return client->last_msg_id;
	}

	return 0;
}

void mcp_transport_mock_reset(void)
{
	memset(&mock_ctx, 0, sizeof(mock_ctx));
	LOG_DBG("Mock: Reset all state");
}
