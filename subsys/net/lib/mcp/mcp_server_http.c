/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/mcp/mcp_server_http.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/min_heap.h>
#include <string.h>
#include <inttypes.h>

#include "mcp_common.h"
#include "mcp_json.h"
#include "mcp_server_internal.h"

#if defined(CONFIG_MCP_HTTP_LOG_ADDRESS)
#include <zephyr/net/net_mgmt.h>
#endif /* CONFIG_MCP_HTTP_LOG_ADDRESS */

LOG_MODULE_REGISTER(mcp_http_transport, CONFIG_MCP_LOG_LEVEL);

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_RESPONSE_HEADERS 4 /* Content-Type, Last-Event-Id, Mcp-Session-Id, extra buffer */

/* Maximum amount of poll attempts before switching to SSE mode */
#define MCP_MAX_POLLS ((CONFIG_MCP_HTTP_TIMEOUT_MS / CONFIG_MCP_HTTP_POLL_INTERVAL_MS)\
			+ (CONFIG_MCP_HTTP_TIMEOUT_MS % CONFIG_MCP_HTTP_POLL_INTERVAL_MS != 0))

/* Request accumulation buffer for each client */
struct mcp_http_request_accumulator {
	size_t data_len;
	uint32_t last_event_id_hdr;
	uint32_t fd;
	char data[CONFIG_MCP_MAX_MESSAGE_SIZE];
	char session_id_hdr[CONFIG_HTTP_SERVER_MAX_HEADER_LEN];
	char content_type_hdr[CONFIG_HTTP_SERVER_MAX_HEADER_LEN];
	char origin_hdr[CONFIG_HTTP_SERVER_MAX_HEADER_LEN];
	char protocol_version_hdr[CONFIG_HTTP_SERVER_MAX_HEADER_LEN];
	bool in_use;
};

/* Response queue item structure */
struct mcp_http_response_item {
	char *data;
	size_t length;
	uint32_t event_id;
};

/* HTTP Client context management */
struct mcp_http_client_ctx {
	struct mcp_transport_binding binding;
	struct http_header response_headers[MAX_RESPONSE_HEADERS];
	struct min_heap responses;
	struct k_mutex responses_mutex;
	atomic_t ref_count;
	uint32_t next_event_id;
	char session_uuid_str[UUID_STR_LEN];
	char response_body[CONFIG_MCP_MAX_MESSAGE_SIZE];
	uint8_t responses_storage[CONFIG_MCP_MAX_CLIENT_REQUESTS *
				  sizeof(struct mcp_http_response_item)];
	bool in_use;
};

/* HTTP transport state */
struct mcp_http_transport_state {
	mcp_server_ctx_t *server_core;
	struct k_mutex accumulators_mutex;
	struct k_mutex clients_mutex;
	struct mcp_http_request_accumulator accumulators[CONFIG_HTTP_SERVER_MAX_CLIENTS];
	struct mcp_http_client_ctx clients[CONFIG_HTTP_SERVER_MAX_CLIENTS];
	bool initialized;
};

/*******************************************************************************
 * Forward declarations
 ******************************************************************************/
static struct mcp_http_request_accumulator *get_accumulator(int fd);
static int release_accumulator(struct mcp_http_request_accumulator *accumulator);
static int accumulate_request(struct http_client_ctx *client,
			      struct mcp_http_request_accumulator *accumulator,
			      const struct http_request_ctx *request_ctx,
			      enum http_transaction_status status);

static struct mcp_http_client_ctx *get_client_by_uuid_str(const char *uuid_str);
static void client_ref_get(struct mcp_http_client_ctx *client);
static void client_ref_put(struct mcp_http_client_ctx *client);

static struct mcp_http_client_ctx *allocate_client(void);
static int release_client(struct mcp_http_client_ctx *client);

static int format_response(char *buffer, size_t buffer_size, const char *json_data);
static int format_sse_response(char *buffer, size_t buffer_size, uint32_t event_id,
			       const char *data);
static int format_sse_retry_response(char *buffer, size_t buffer_size, uint32_t retry_ms);

static int mcp_endpoint_post_handler(struct http_client_ctx *client,
				     const struct http_request_ctx *request_ctx,
				     struct mcp_http_request_accumulator *accumulator,
				     struct http_response_ctx *response_ctx);
static int mcp_endpoint_get_handler(struct http_client_ctx *client,
				    const struct http_request_ctx *request_ctx,
				    struct mcp_http_request_accumulator *accumulator,
				    struct http_response_ctx *response_ctx);
static int mcp_server_http_resource_handler(struct http_client_ctx *client,
					    enum http_transaction_status status,
					    const struct http_request_ctx *request_ctx,
					    struct http_response_ctx *response_ctx,
					    void *user_data);

static int mcp_server_http_send(struct mcp_transport_message *response);
static int mcp_server_http_disconnect(struct mcp_transport_binding *binding);

/*******************************************************************************
 * Static variables and resource definitions
 ******************************************************************************/
const struct mcp_transport_ops mcp_http_transport_ops = {
	.send = mcp_server_http_send,
	.disconnect = mcp_server_http_disconnect,
};

static struct mcp_http_transport_state http_transport_state;
static struct http_resource_detail_dynamic mcp_resource_detail = {
	.common = {
			.type = HTTP_RESOURCE_TYPE_DYNAMIC,
			.bitmask_of_supported_http_methods = BIT(HTTP_POST) | BIT(HTTP_GET),
			.content_type = "application/json",
		},
	.cb = mcp_server_http_resource_handler,
	.user_data = NULL,
};

uint32_t mcp_http_port = CONFIG_MCP_HTTP_PORT;
atomic_t available_responses;

/* HTTP resource definition */
HTTP_RESOURCE_DEFINE(mcp_endpoint_resource, mcp_http_service, CONFIG_MCP_HTTP_ENDPOINT,
		     &mcp_resource_detail);

/* HTTP service definition */
HTTP_SERVICE_DEFINE(mcp_http_service, NULL, &mcp_http_port, 1,
		    CONFIG_HTTP_SERVER_MAX_CLIENTS, 10, NULL, NULL);

/* HTTP headers capture */
HTTP_SERVER_REGISTER_HEADER_CAPTURE(origin_hdr, "Origin");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(content_type_hdr, "Content-Type");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(mcp_session_id_hdr, "Mcp-Session-Id");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(last_event_id_hdr, "Last-Event-Id");
HTTP_SERVER_REGISTER_HEADER_CAPTURE(mcp_protocol_version_hdr, "Mcp-Protocol-Version");

/*******************************************************************************
 * Min Heap Helpers
 ******************************************************************************/
static int mcp_http_response_compare(const void *a, const void *b)
{
	const struct mcp_http_response_item *da = a;
	const struct mcp_http_response_item *db = b;

	return da->event_id - db->event_id;
}

static bool mcp_http_response_match(const void *a, const void *b)
{
	const struct mcp_http_response_item *da = a;
	const uint32_t *event_id = b;

	return da->event_id == *event_id;
}

/*******************************************************************************
 * Accumulator helpers
 ******************************************************************************/
/**
 * Get or allocate an accumulator for the given file descriptor
 * This function searches for an existing accumulator associated with the given fd.
 * If found, it returns a pointer to it. If not found, it allocates the first.
 */
static struct mcp_http_request_accumulator *get_accumulator(int fd)
{
	int ret;
	struct mcp_http_request_accumulator *first_available = NULL;
	struct mcp_http_request_accumulator *result = NULL;

	ret = k_mutex_lock(&http_transport_state.accumulators_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock accumulator mutex: %d", ret);
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(http_transport_state.accumulators); i++) {
		if (http_transport_state.accumulators[i].in_use &&
		    (http_transport_state.accumulators[i].fd == fd)) {
			result = &http_transport_state.accumulators[i];
			goto unlock;
		}

		if ((first_available == NULL) && !http_transport_state.accumulators[i].in_use) {
			first_available = &http_transport_state.accumulators[i];
		}
	}

	if (first_available != NULL) {
		first_available->fd = fd;
		first_available->in_use = true;
		result = first_available;
	}

unlock:
	k_mutex_unlock(&http_transport_state.accumulators_mutex);
	return result;
}

static int release_accumulator(struct mcp_http_request_accumulator *accumulator)
{
	int ret;

	if (accumulator == NULL) {
		return -EINVAL;
	}

	ret = k_mutex_lock(&http_transport_state.accumulators_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock accumulator mutex: %d", ret);
		return ret;
	}

	memset(accumulator, 0, sizeof(*accumulator));

	k_mutex_unlock(&http_transport_state.accumulators_mutex);

	return 0;
}

static int accumulate_header(struct mcp_http_request_accumulator *accumulator,
			     const struct http_header *header)
{
	if (!header->name || !header->value) {
		return 0;
	}

	if (strcmp(header->name, "Mcp-Session-Id") == 0) {
		strncpy(accumulator->session_id_hdr, header->value,
			sizeof(accumulator->session_id_hdr) - 1);
		accumulator->session_id_hdr[sizeof(accumulator->session_id_hdr) - 1] = '\0';
	} else if (strcmp(header->name, "Mcp-Protocol-Version") == 0) {
		strncpy(accumulator->protocol_version_hdr, header->value,
			sizeof(accumulator->protocol_version_hdr) - 1);
		accumulator->protocol_version_hdr[sizeof(accumulator->protocol_version_hdr) - 1] =
			'\0';
	} else if (strcmp(header->name, "Last-Event-Id") == 0) {
		char *endptr;

		accumulator->last_event_id_hdr = strtoul(header->value, &endptr, 10);
		if (*endptr != '\0') {
			LOG_ERR("Invalid Last-Event-Id format: %s", header->value);
			return -EINVAL;
		}
	} else if (strcmp(header->name, "Origin") == 0) {
		strncpy(accumulator->origin_hdr, header->value,
			sizeof(accumulator->origin_hdr) - 1);
		accumulator->origin_hdr[sizeof(accumulator->origin_hdr) - 1] = '\0';
	} else if (strcmp(header->name, "Content-Type") == 0) {
		strncpy(accumulator->content_type_hdr, header->value,
			sizeof(accumulator->content_type_hdr) - 1);
		accumulator->content_type_hdr[sizeof(accumulator->content_type_hdr) - 1] = '\0';
	} else {
		LOG_DBG("Unhandled header: %s", header->name);
	}

	return 0;
}

static int accumulate_request(struct http_client_ctx *client,
			      struct mcp_http_request_accumulator *accumulator,
			      const struct http_request_ctx *request_ctx,
			      enum http_transaction_status status)
{
	int ret;

	if (request_ctx->data_len > 0) {
		if ((accumulator->data_len + request_ctx->data_len) > CONFIG_MCP_MAX_MESSAGE_SIZE) {
			LOG_WRN("Accumulator full. Dropping current chunk");
			return -ENOMEM;
		}

		memcpy(accumulator->data + accumulator->data_len, request_ctx->data,
		       request_ctx->data_len);
		accumulator->data_len += request_ctx->data_len;
	}

	for (uint32_t i = 0; i < request_ctx->header_count; i++) {
		ret = accumulate_header(accumulator, &request_ctx->headers[i]);
		if (ret < 0) {
			return ret;
		}
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		if (accumulator->data_len < CONFIG_MCP_MAX_MESSAGE_SIZE) {
			accumulator->data[accumulator->data_len] = '\0';
		} else {
			LOG_WRN("Cannot null-terminate accumulator: buffer full");
		}
	}

	return 0;
}

/*******************************************************************************
 * UUID helpers
 ******************************************************************************/
/**
 * @brief Find HTTP client by UUID string (thread-safe version with reference counting)
 *
 * @param uuid_str Session UUID string
 * @return Pointer to HTTP client context if found (with incremented ref count), NULL otherwise
 */
static struct mcp_http_client_ctx *get_client_by_uuid_str(const char *uuid_str)
{
	struct mcp_http_client_ctx *client = NULL;
	int ret;

	if (uuid_str == NULL || uuid_str[0] == '\0') {
		return NULL;
	}

	ret = k_mutex_lock(&http_transport_state.clients_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock clients mutex: %d", ret);
		return NULL;
	}

	for (int i = 0; i < ARRAY_SIZE(http_transport_state.clients); i++) {
		if (http_transport_state.clients[i].in_use &&
		    strcmp(http_transport_state.clients[i].session_uuid_str, uuid_str) == 0) {
			client = &http_transport_state.clients[i];
			client_ref_get(client);
			break;
		}
	}

	k_mutex_unlock(&http_transport_state.clients_mutex);

	return client;
}

/*******************************************************************************
 * Reference Counting Helpers
 ******************************************************************************/
/**
 * @brief Increment client reference count
 * Must be called with clients_mutex held OR on a client already referenced
 */
static void client_ref_get(struct mcp_http_client_ctx *client)
{
	if (client != NULL) {
		atomic_inc(&client->ref_count);
	}
}

/**
 * @brief Decrement client reference count and cleanup if zero
 */
static void client_ref_put(struct mcp_http_client_ctx *client)
{
	struct mcp_http_response_item *response_item;
	int ret;

	if (!client) {
		return;
	}

	if (atomic_dec(&client->ref_count) > 1) {
		/* Still has references */
		return;
	}

	/* Last reference cleanup */
	ret = k_mutex_lock(&client->responses_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock responses mutex during cleanup: %d", ret);
	} else {
		/* Clean up any pending responses in the queue */
		MIN_HEAP_FOREACH(&client->responses, response_item) {
			if (response_item->data) {
				mcp_free(response_item->data);
				atomic_dec(&available_responses);
			}
		}
		k_mutex_unlock(&client->responses_mutex);
	}

	ret = k_mutex_lock(&http_transport_state.clients_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock clients mutex during cleanup: %d", ret);
		return;
	}

	/* Make sure client is still marked for deletion */
	if (atomic_get(&client->ref_count) == 0 && client->in_use) {
		/* Mark as not in use first to prevent new references */
		client->in_use = false;
		k_mutex_unlock(&http_transport_state.clients_mutex);

		/* Now safe to destroy the mutex as no one can acquire new references */
		memset(client, 0, sizeof(*client));
	} else {
		k_mutex_unlock(&http_transport_state.clients_mutex);
	}
}

/*******************************************************************************
 * Client context helpers
 ******************************************************************************/
static struct mcp_http_client_ctx *allocate_client(void)
{
	struct mcp_http_client_ctx *client = NULL;
	struct uuid session_uuid;
	int ret;

	ret = k_mutex_lock(&http_transport_state.clients_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock clients mutex: %d", ret);
		return client;
	}

	for (int i = 0; i < CONFIG_HTTP_SERVER_MAX_CLIENTS; i++) {
		if (!http_transport_state.clients[i].in_use) {
			client = &http_transport_state.clients[i];

			ret = k_mutex_init(&client->responses_mutex);
			if (ret != 0) {
				LOG_ERR("Failed to initialize client mutex: %d", ret);
				client = NULL;
				goto unlock;
			}

			client->in_use = true;
			client->next_event_id = 0;
			client->binding.ops = &mcp_http_transport_ops;
			client->binding.context = (void *)client;

			atomic_set(&client->ref_count, 1);

			uuid_generate_v4(&session_uuid);
			uuid_to_string(&session_uuid, client->session_uuid_str);

			min_heap_init(&client->responses, client->responses_storage,
				      CONFIG_MCP_MAX_CLIENT_REQUESTS,
				      sizeof(struct mcp_http_response_item),
				      mcp_http_response_compare);

			break;
		}
	}

unlock:
	k_mutex_unlock(&http_transport_state.clients_mutex);

	if (client == NULL) {
		LOG_ERR("No available client slots");
	}

	return client;
}

static int release_client(struct mcp_http_client_ctx *client)
{
	if (!client) {
		return -EINVAL;
	}

	client_ref_put(client);
	return 0;
}

/*******************************************************************************
 * Helper functions to format response body
 ******************************************************************************/

/**
 * Format a JSON response body
 */
static int format_response(char *buffer, size_t buffer_size, const char *json_data)
{
	int len;

	if (!buffer || buffer_size == 0 || !json_data) {
		return -EINVAL;
	}

	len = snprintk(buffer, buffer_size, "%s", json_data);

	if (len < 0 || (size_t)len >= buffer_size) {
		LOG_ERR("Failed to format JSON response (length: %d)", len);
		return -ENOSPC;
	}

	return len;
}

/**
 * Format a Server-Sent Events (SSE) response with event ID and data
 */
static int format_sse_response(char *buffer, size_t buffer_size, uint32_t event_id,
			       const char *data)
{
	int len;

	if (!buffer || (buffer_size == 0)) {
		return -EINVAL;
	}

	if (data && (strlen(data) > 0)) {
		len = snprintk(buffer, buffer_size, "id: %u\ndata: %s\n\n", event_id, data);
	} else {
		len = snprintk(buffer, buffer_size, "id: %u\ndata:\n\n", event_id);
	}

	if ((len < 0) || ((size_t)len >= buffer_size)) {
		LOG_ERR("Failed to format SSE response (event_id: %u)", event_id);
		return -ENOSPC;
	}

	return len;
}

/**
 * Format SSE retry response
 * Sets the retry interval for SSE reconnection attempts
 */
static int format_sse_retry_response(char *buffer, size_t buffer_size, uint32_t retry_ms)
{
	int len;

	if (!buffer || (buffer_size == 0)) {
		return -EINVAL;
	}

	len = snprintk(buffer, buffer_size, "retry: %u\n\n", retry_ms);

	if ((len < 0) || ((size_t)len >= buffer_size)) {
		LOG_ERR("Failed to format SSE retry response (retry: %u ms)", retry_ms);
		return -ENOSPC;
	}

	return len;
}

/**
 * Poll for a response matching the given message ID
 *
 * @return Length of formatted response on success, negative error code on failure
 */
static int poll_for_response(struct mcp_http_client_ctx *mcp_client, uint32_t msg_id)
{
	int ret;
	int times_polled = 0;
	size_t index;
	struct mcp_http_response_item response_data;

	while (times_polled < MCP_MAX_POLLS) {
		atomic_val_t any_responses = atomic_get(&available_responses);

		if (any_responses > 0) {
			LOG_DBG("Responses available, checking if any matches request.");
			k_mutex_lock(&mcp_client->responses_mutex, K_FOREVER);

			if (min_heap_find(&mcp_client->responses, mcp_http_response_match, &msg_id,
					  &index)) {
				LOG_DBG("Matching response found.");
				min_heap_remove(&mcp_client->responses, index, &response_data);
				ret = format_response(mcp_client->response_body,
						      sizeof(mcp_client->response_body),
						      response_data.data);
				mcp_free(response_data.data);
				atomic_dec(&available_responses);
				k_mutex_unlock(&mcp_client->responses_mutex);
				return ret;
			}

			k_mutex_unlock(&mcp_client->responses_mutex);
		}

		LOG_DBG("Response not yet available, will poll again");
		k_sleep(K_MSEC(CONFIG_MCP_HTTP_POLL_INTERVAL_MS));
		times_polled++;
	}

	LOG_DBG("Request is taking too long to process, switching to SSE");
	k_mutex_lock(&mcp_client->responses_mutex, K_FOREVER);
	mcp_client->response_headers[0].value = "text/event-stream";
	ret = format_sse_response(mcp_client->response_body, sizeof(mcp_client->response_body),
				  msg_id, NULL);
	k_mutex_unlock(&mcp_client->responses_mutex);

	return ret;
}

/*******************************************************************************
 * POST Handler
 ******************************************************************************/
static int mcp_endpoint_post_handler(struct http_client_ctx *client,
				     const struct http_request_ctx *request_ctx,
				     struct mcp_http_request_accumulator *accumulator,
				     struct http_response_ctx *response_ctx)
{
	int ret;
	bool is_initialize_request = accumulator->session_id_hdr[0] == '\0';
	enum mcp_method request_type;
	struct mcp_transport_message request_data;
	struct mcp_http_client_ctx *mcp_client = NULL;

	if (is_initialize_request) {
		mcp_client = allocate_client();
	} else {
		mcp_client = get_client_by_uuid_str(accumulator->session_id_hdr);
	}

	if (mcp_client == NULL) {
		response_ctx->status = is_initialize_request ? HTTP_500_INTERNAL_SERVER_ERROR
							     : HTTP_400_BAD_REQUEST;
		response_ctx->final_chunk = true;
		return 0;
	}

	request_data = (struct mcp_transport_message){
		.json_data = accumulator->data,
		.json_len = accumulator->data_len,
		.msg_id = mcp_client->next_event_id++,
		.binding = &mcp_client->binding
	};

	if (accumulator->protocol_version_hdr[0] != '\0') {
		mcp_safe_strcpy(request_data.protocol_version,
				sizeof(request_data.protocol_version),
				accumulator->protocol_version_hdr);
	} else {
		mcp_safe_strcpy(request_data.protocol_version,
				sizeof(request_data.protocol_version), "2025-03-26");
	}

	ret = mcp_server_handle_request(http_transport_state.server_core, &request_data,
					&request_type);
	if (ret < 0) {
		if (is_initialize_request) {
			release_client(mcp_client);
		} else {
			client_ref_put(mcp_client);
		}

		response_ctx->status =
			(ret == -EPROTO) ? HTTP_400_BAD_REQUEST : HTTP_500_INTERNAL_SERVER_ERROR;

		LOG_ERR("Error processing request: %d", ret);
		response_ctx->final_chunk = true;
		return 0;
	}

	mcp_client->response_headers[0].name = "Content-Type";
	mcp_client->response_headers[0].value = "application/json";
	mcp_client->response_headers[1].name = "Mcp-Session-Id";
	mcp_client->response_headers[1].value = (const char *)mcp_client->session_uuid_str;
	mcp_client->response_headers[2].name = "Cache-Control";
	mcp_client->response_headers[2].value = "no-cache";

	response_ctx->headers = mcp_client->response_headers;
	response_ctx->status = HTTP_200_OK;
	response_ctx->final_chunk = true;
	response_ctx->header_count = 3;

	switch (request_type) {
	case MCP_METHOD_UNKNOWN:
	case MCP_METHOD_PING:
	case MCP_METHOD_TOOLS_LIST:
	case MCP_METHOD_INITIALIZE:
	case MCP_METHOD_TOOLS_CALL:
		ret = poll_for_response(mcp_client, request_data.msg_id);

		if (ret < 0) {
			response_ctx->status = HTTP_500_INTERNAL_SERVER_ERROR;

			if (!is_initialize_request) {
				client_ref_put(mcp_client);
			}

			return 0;
		}

		response_ctx->body = (const char *)mcp_client->response_body;
		response_ctx->body_len = ret;

		LOG_DBG("Serialized response: %s", response_ctx->body);
		break;
	case MCP_METHOD_NOTIF_INITIALIZED:
		response_ctx->status = HTTP_202_ACCEPTED;
		break;
	default:
		response_ctx->status = HTTP_405_METHOD_NOT_ALLOWED;
		LOG_WRN("Unhandled method type: %d", request_type);
		break;
	}

	if (!is_initialize_request) {
		client_ref_put(mcp_client);
	}

	return 0;
}

/*******************************************************************************
 * GET Handler
 ******************************************************************************/
static int mcp_endpoint_get_handler(struct http_client_ctx *client,
				    const struct http_request_ctx *request_ctx,
				    struct mcp_http_request_accumulator *accumulator,
				    struct http_response_ctx *response_ctx)
{
	int ret;
	struct mcp_http_client_ctx *mcp_client;
	struct mcp_http_response_item response_data;
	struct mcp_http_response_item *temp;

	/* Find client by UUID string and increment ref count) */
	mcp_client = get_client_by_uuid_str(accumulator->session_id_hdr);
	if (mcp_client == NULL) {
		LOG_ERR("Client session not found for UUID: %s", accumulator->session_id_hdr);
		response_ctx->status = HTTP_400_BAD_REQUEST;
		response_ctx->final_chunk = true;
		return 0;
	}

	mcp_server_update_client_timestamp(http_transport_state.server_core, &mcp_client->binding);

	mcp_client->response_headers[0].name = "Content-Type";
	mcp_client->response_headers[0].value = "text/event-stream";
	mcp_client->response_headers[1].name = "Mcp-Session-Id";
	mcp_client->response_headers[1].value = (const char *)mcp_client->session_uuid_str;
	mcp_client->response_headers[2].name = "Cache-Control";
	mcp_client->response_headers[2].value = "no-cache";

	response_ctx->headers = mcp_client->response_headers;
	response_ctx->header_count = 3;
	response_ctx->status = HTTP_200_OK;
	response_ctx->final_chunk = true;

	k_mutex_lock(&mcp_client->responses_mutex, K_FOREVER);
	if ((mcp_client->next_event_id <= accumulator->last_event_id_hdr) ||
	    min_heap_is_empty(&mcp_client->responses)) {
		/* Send retry interval when there are no events to stream */
		ret = format_sse_retry_response(mcp_client->response_body,
						sizeof(mcp_client->response_body),
						CONFIG_MCP_HTTP_SSE_RETRY_MS);
		if (ret < 0) {
			LOG_ERR("Failed to format SSE retry response: %d", ret);
			response_ctx->status = HTTP_500_INTERNAL_SERVER_ERROR;
			goto get_handler_done;
		}

		response_ctx->body = mcp_client->response_body;
		response_ctx->body_len = ret;

		LOG_DBG("Sending retry event: %s", response_ctx->body);
	} else {
		/* Format SSE response with data */
		temp = min_heap_peek(&mcp_client->responses);
		if (temp && temp->event_id >= accumulator->last_event_id_hdr) {
			min_heap_pop(&mcp_client->responses, &response_data);
			atomic_dec(&available_responses);
			ret = format_sse_response(mcp_client->response_body,
						  sizeof(mcp_client->response_body),
						  mcp_client->next_event_id, response_data.data);
			mcp_client->next_event_id++;

			if (ret < 0) {
				LOG_ERR("Failed to format SSE response: %d", ret);
				response_ctx->status = HTTP_500_INTERNAL_SERVER_ERROR;
				mcp_free(response_data.data);
				goto get_handler_done;
			}

			temp = min_heap_peek(&mcp_client->responses);
			response_ctx->body = mcp_client->response_body;
			response_ctx->body_len = ret;
			response_ctx->final_chunk =
				!(temp && temp->event_id >= accumulator->last_event_id_hdr);

			mcp_free(response_data.data);
			LOG_DBG("Sending SSE response: %s", response_ctx->body);
		}
	}
get_handler_done:
	k_mutex_unlock(&mcp_client->responses_mutex);
	client_ref_put(mcp_client);

	return 0;
}

/*******************************************************************************
 * HTTP resource handler
 ******************************************************************************/
static int mcp_server_http_resource_handler(struct http_client_ctx *client,
					    enum http_transaction_status status,
					    const struct http_request_ctx *request_ctx,
					    struct http_response_ctx *response_ctx, void *user_data)
{
	int stat = 0;

	/* TODO: Validate origin */

	struct mcp_http_request_accumulator *accumulator = get_accumulator(client->fd);

	if (accumulator == NULL) {
		LOG_ERR("Failed to allocate mcp accumualator fd=%d", client->fd);
		return -ENOMEM;
	}

	/* Handle transaction abort/complete - cleanup and exit */
	if (status == HTTP_SERVER_TRANSACTION_ABORTED
		|| status == HTTP_SERVER_TRANSACTION_COMPLETE) {
		stat = release_accumulator(accumulator);
		return stat;
	}

	stat = accumulate_request(client, accumulator, request_ctx, status);
	if (stat < 0) {
		LOG_ERR("Failed to accumulate request fd=%d", client->fd);
		return stat;
	}

	if (status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		LOG_DBG("HTTP %s request: %s", client->method == HTTP_POST ? "POST" : "GET",
			accumulator->data);
		/* Process complete request */
		if (client->method == HTTP_POST) {
			stat = mcp_endpoint_post_handler(client, request_ctx, accumulator,
							 response_ctx);
		} else if (client->method == HTTP_GET) {
			stat = mcp_endpoint_get_handler(client, request_ctx, accumulator,
							response_ctx);
		} else {
			LOG_WRN("Unsupported HTTP method");
			return -ENOTSUP;
		}
	}

	return stat;
}

/*******************************************************************************
 * Optional IP address logging helper
 ******************************************************************************/
#if defined(CONFIG_MCP_HTTP_LOG_ADDRESS)

#if defined(CONFIG_NET_IPV4)
static struct net_mgmt_event_callback net_addr4_cb;

static void net_addr4_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
{
	char addr_str[INET_ADDRSTRLEN];
	struct net_in_addr *addr4;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	addr4 = net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED);
	if (addr4 != NULL) {
		net_addr_ntop(AF_INET, addr4, addr_str, sizeof(addr_str));
		LOG_INF("MCP server accessible at: http://%s:%d%s (iface %d)",
			addr_str, CONFIG_MCP_HTTP_PORT, CONFIG_MCP_HTTP_ENDPOINT,
			net_if_get_by_iface(iface));
	}
}
#endif

#if defined(CONFIG_NET_IPV6)
static struct net_mgmt_event_callback net_addr6_cb;

static void net_addr6_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
{
	char addr_str[INET6_ADDRSTRLEN];
	const struct net_in6_addr *addr6;

	if (mgmt_event != NET_EVENT_IPV6_ADDR_ADD) {
		return;
	}

	addr6 = net_if_ipv6_get_global_addr(NET_ADDR_PREFERRED, &iface);
	if (addr6 != NULL) {
		net_addr_ntop(AF_INET6, addr6, addr_str, sizeof(addr_str));
		LOG_INF("MCP server accessible at: http://[%s]:%d%s (iface %d)",
			addr_str, CONFIG_MCP_HTTP_PORT, CONFIG_MCP_HTTP_ENDPOINT,
			net_if_get_by_iface(iface));
	}
}
#endif

static int mcp_http_log_address_init(void)
{
#if defined(CONFIG_NET_IPV4)
	net_mgmt_init_event_callback(&net_addr4_cb, net_addr4_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&net_addr4_cb);
#endif
#if defined(CONFIG_NET_IPV6)
	net_mgmt_init_event_callback(&net_addr6_cb, net_addr6_event_handler,
				     NET_EVENT_IPV6_ADDR_ADD);
	net_mgmt_add_event_callback(&net_addr6_cb);
#endif
	return 0;
}

SYS_INIT(mcp_http_log_address_init, APPLICATION,
	CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_MCP_HTTP_LOG_ADDRESS */

/*******************************************************************************
 * Interface Implementation
 ******************************************************************************/
int mcp_server_http_init(mcp_server_ctx_t server_ctx)
{
	int ret;

	LOG_INF("Initializing HTTP/SSE transport");

	if (http_transport_state.initialized) {
		LOG_WRN("HTTP transport already initialized");
		return 0;
	}

	memset(http_transport_state.clients, 0, sizeof(http_transport_state.clients));
	memset(http_transport_state.accumulators, 0, sizeof(http_transport_state.accumulators));

	ret = k_mutex_init(&http_transport_state.clients_mutex);
	if (ret != 0) {
		LOG_ERR("Failed to initialize clients mutex: %d", ret);
		return ret;
	}

	ret = k_mutex_init(&http_transport_state.accumulators_mutex);
	if (ret != 0) {
		LOG_ERR("Failed to initialize accumulators mutex: %d", ret);
		return ret;
	}

	atomic_set(&available_responses, 0);

	http_transport_state.server_core = server_ctx;
	http_transport_state.initialized = true;

	return 0;
}


int mcp_server_http_start(mcp_server_ctx_t server_ctx)
{
	int ret;

	if ((server_ctx == NULL) || (server_ctx != http_transport_state.server_core) ||
	    !http_transport_state.initialized) {
		LOG_ERR("HTTP server context invalid or transport not initialized");
		return -EINVAL;
	}

	/* Start HTTP server */
	ret = http_server_start();

	if (ret != 0) {
		LOG_ERR("Failed to start HTTP server: %d", ret);
		return ret;
	}

	LOG_INF("HTTP transport running on port %d, endpoint: %s", CONFIG_MCP_HTTP_PORT,
		CONFIG_MCP_HTTP_ENDPOINT);

	return 0;
}

static int mcp_server_http_send(struct mcp_transport_message *response)
{
	struct mcp_http_response_item item;
	struct mcp_http_client_ctx *client;
	int ret;

	if (!http_transport_state.initialized) {
		LOG_WRN("HTTP transport not initialized");
		return -ENODEV;
	}

	if (response == NULL) {
		LOG_ERR("Invalid send parameters");
		return -EINVAL;
	}

	ret = k_mutex_lock(&http_transport_state.clients_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock clients mutex: %d", ret);
		return ret;
	}

	client = (struct mcp_http_client_ctx *)response->binding->context;
	if (!client || !client->in_use) {
		k_mutex_unlock(&http_transport_state.clients_mutex);
		LOG_ERR("Client not found or not in use");
		return -ENOENT;
	}

	client_ref_get(client);
	k_mutex_unlock(&http_transport_state.clients_mutex);

	/* Take ownership of the data pointer */
	item.data = response->json_data;
	item.length = response->json_len;
	item.event_id = response->msg_id;

	ret = k_mutex_lock(&client->responses_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock responses mutex: %d", ret);
		client_ref_put(client);
		return ret;
	}

	/* POST/GET handlers are responsible for freeing response data from core */
	ret = min_heap_push(&client->responses, &item);
	if (ret != 0) {
		LOG_ERR("Failed to push response to heap for client %s: %d",
			client->session_uuid_str, ret);
	}

	atomic_inc(&available_responses);

	k_mutex_unlock(&client->responses_mutex);
	client_ref_put(client);

	return ret;
}

static int mcp_server_http_disconnect(struct mcp_transport_binding *binding)
{
	struct mcp_http_client_ctx *client;
	int ret;

	if (!http_transport_state.initialized) {
		LOG_WRN("HTTP transport not initialized");
		return -ENODEV;
	}

	if (binding == NULL) {
		LOG_ERR("Invalid send parameters");
		return -EINVAL;
	}

	ret = k_mutex_lock(&http_transport_state.clients_mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock clients mutex: %d", ret);
		return ret;
	}

	client = (struct mcp_http_client_ctx *)binding->context;
	if (!client || !client->in_use) {
		k_mutex_unlock(&http_transport_state.clients_mutex);
		LOG_ERR("Client not found or not in use");
		return -ENOENT;
	}

	k_mutex_unlock(&http_transport_state.clients_mutex);

	release_client(client);
	return 0;
}
