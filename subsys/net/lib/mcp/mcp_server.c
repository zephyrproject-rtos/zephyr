/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <zephyr/random/random.h>
#include <errno.h>
#include "mcp_common.h"
#include "mcp_server_internal.h"
#include "mcp_json.h"

LOG_MODULE_REGISTER(mcp_server, CONFIG_MCP_LOG_LEVEL);

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MCP_WORKER_PRIORITY  7
#define MCP_MAX_REQUESTS     (CONFIG_MCP_MAX_CLIENTS * CONFIG_MCP_MAX_CLIENT_REQUESTS)
#define MCP_PROTOCOL_VERSION "2025-11-25"

/**
 * @brief Lifecycle monitoring of a client's session
 */
enum mcp_lifecycle_state {
	MCP_LIFECYCLE_DEINITIALIZED = 0,	/**< No client allocated */
	MCP_LIFECYCLE_DEINITIALIZING,		/**< Deinitialize in progress */
	MCP_LIFECYCLE_NEW,			/**< Client allocated, not initialized */
	MCP_LIFECYCLE_INITIALIZING,		/**< Initialize response sent */
	MCP_LIFECYCLE_INITIALIZED		/**< Client confirmed initialization */
};

/**
 * @brief Lifecycle monitoring of a tool execution
 */
enum mcp_execution_state {
	MCP_EXEC_FREE,		/**< Not in use */
	MCP_EXEC_ACTIVE,	/**< Execution in progress */
	MCP_EXEC_CANCELED,	/**< Cancellation requested */
	MCP_EXEC_FINISHED	/**< Execution completed */
};

enum mcp_canceled_state {
	MCP_NOT_CANCELED,		/**< Execution is not canceled */
	MCP_CANCELED,			/**< Canceled, doesn't need cleanup */
	MCP_CANCELED_WITH_CLEANUP	/**< Canceled, needs cleanup */
};

/**
 * @brief Client context
 * @note Used to hold lifecycle and request tracking information
 *		 and the transport layer binding
 */
struct mcp_client_context {
	int64_t last_message_timestamp;			/**< Last activity time (uptime ms) */
	/**< Active request tracking */
	struct mcp_execution_context *active_requests[CONFIG_MCP_MAX_CLIENT_REQUESTS];
	struct mcp_transport_binding *binding;		/**< Client's transport layer binding */
	atomic_t refcount;                              /**< Reference count for cleanup */
	enum mcp_lifecycle_state lifecycle_state;       /**< Current state */
	uint8_t active_request_count;                   /**< Number of pending requests */
};

/**
 * @brief Struct holding the pointer to a client's request's data
 */
struct mcp_queue_msg {
	struct mcp_client_context *client;	/**< Pointer to client context */
	void *data;                             /**< Pointer to request data payload */
	uint32_t transport_msg_id;              /**< Message ID from transport layer */
};

/**
 * @brief Registry holding the tools added by the user application to the server
 */
struct mcp_tool_registry {
	struct mcp_tool_record tools[CONFIG_MCP_MAX_TOOLS];
	struct k_mutex mutex;
	uint8_t tool_count;
};

/**
 * @brief Tool execution context
 * @note Tracks the state and metadata of a single tool execution
 */
struct mcp_execution_context {
	int64_t start_timestamp;		/**< Execution start time in milliseconds */
	int64_t cancel_timestamp;		/**< Time when cancellation was requested */
	int64_t last_message_timestamp;		/**< Last activity/ping time for idle detection */
	struct mcp_client_context *client;      /**< Client that initiated this execution */
	struct mcp_tool_record *tool;           /**< Tool being executed */
	k_tid_t worker_id;                      /**< Thread ID of worker handling this execution */
	struct mcp_request_id request_id;       /**< Actual request ID from the client message */
	uint32_t transport_msg_id;              /**< Message ID from transport layer */
	enum mcp_execution_state execution_state; /**< Current execution state */
	char execution_token[UUID_STR_LEN];       /**< Unique token for this execution */
};

/**
 * @brief Registry holding all active tool executions
 */
struct mcp_execution_registry {
	struct mcp_execution_context executions[MCP_MAX_REQUESTS];
	struct k_mutex mutex;
};

/**
 * @brief Registry holding all active clients connected to the server
 */
struct mcp_client_registry {
	struct mcp_client_context clients[CONFIG_MCP_MAX_CLIENTS];
	struct k_mutex mutex;
};

/**
 * @brief MCP server context structure
 * @details Contains all state and resources needed to manage an MCP server instance,
 * including client registry, transport operations, request processing workers,
 * and message queues for handling MCP protocol requests and responses.
 */
struct mcp_server_ctx {
	/**< Registry of all connected clients */
	struct mcp_client_registry client_registry;
	/**< Request worker threads */
	struct k_thread request_workers[CONFIG_MCP_REQUEST_WORKERS];
	/**< Message queue for pending client requests */
	struct k_msgq request_queue;
	struct mcp_tool_registry tool_registry;	/**< Registry of available tools */
	/**< Registry of active tool executions */
	struct mcp_execution_registry execution_registry;
	struct k_thread health_monitor_thread;	/**< Timeout monitoring thread */
	/**< Request queue storage */
	char request_queue_storage[MCP_MAX_REQUESTS * sizeof(struct mcp_queue_msg)];
	/**< Server instance index in the global mcp_servers array */
	uint8_t idx;
	/**< Flag indicating if this server context is allocated */
	bool in_use;
};

/*******************************************************************************
 * Variables
 ******************************************************************************/
static struct mcp_server_ctx mcp_servers[CONFIG_MCP_SERVER_COUNT];

K_THREAD_STACK_ARRAY_DEFINE(mcp_request_worker_stacks,
			    CONFIG_MCP_REQUEST_WORKERS * CONFIG_MCP_SERVER_COUNT,
			    CONFIG_MCP_REQUEST_WORKER_STACK_SIZE);
K_THREAD_STACK_ARRAY_DEFINE(mcp_health_monitor_stack, CONFIG_MCP_SERVER_COUNT,
			    CONFIG_MCP_HEALTH_MONITOR_STACK_SIZE);

/*******************************************************************************
 * Server Context Helper Functions
 ******************************************************************************/

/**
 * @brief Allocate an MCP Server context
 *
 * @return Pointer to allocated server context, or NULL if no slots available
 */
static struct mcp_server_ctx *allocate_mcp_server_context(void)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(mcp_servers); i++) {
		if (!mcp_servers[i].in_use) {
			memset(&mcp_servers[i], 0, sizeof(struct mcp_server_ctx));
			mcp_servers[i].idx = i;
			mcp_servers[i].in_use = true;
			return &mcp_servers[i];
		}
	}

	return NULL;
}

/*******************************************************************************
 * Client Context Helper Functions
 * NOTE: Functions with _locked suffix assume the caller holds client_registry.mutex
 ******************************************************************************/
/**
 * @brief Find a client context by transport binding
 * @note Must be called with client_registry.mutex held
 * @return Pointer to client context if found, NULL otherwise
 */
static struct mcp_client_context *
get_client_by_binding_locked(struct mcp_server_ctx *server,
			     const struct mcp_transport_binding *binding)
{
	struct mcp_client_registry *client_registry = &server->client_registry;

	for (int i = 0; i < ARRAY_SIZE(client_registry->clients); i++) {
		if (client_registry->clients[i].binding == binding) {
			return &client_registry->clients[i];
		}
	}

	return NULL;
}

/**
 * @brief Atomically increase the refcount for a client
 * @note Must be called with client_registry.mutex held
 * @return Pointer to client context if found and valid, NULL otherwise
 */
static struct mcp_client_context *client_get_locked(struct mcp_client_context *client)
{
	if (client == NULL || client->lifecycle_state == MCP_LIFECYCLE_DEINITIALIZED ||
	    client->lifecycle_state == MCP_LIFECYCLE_DEINITIALIZING) {
		return NULL;
	}
	atomic_inc(&client->refcount);
	return client;
}

/**
 * @brief Atomically decrease the refcount for a client and clean up if needed
 * @note Can be called without lock
 */
static void client_put(struct mcp_client_context *client)
{
	if (client == NULL) {
		return;
	}

	if (atomic_dec(&client->refcount) == 1) {
		/**
		 * Last reference. Safe to cleanup.
		 *
		 * Cleanup is safe because:
		 *     - In this case, we know that remove_client_locked() set
		 *       lifecycle_state=DEINITIALIZED under mutex.
		 *     - This prevents client_get_locked() from creating new references
		 *
		 * So even though remove_client_locked() may not have been the last to drop
		 * a reference (other threads may have held references from queued messages
		 * or active handlers), we know that:
		 *     1. No NEW references can be created (lifecycle_state is DEINITIALIZED)
		 *     2. We just dropped the LAST existing reference
		 *     3. Therefore cleanup is safe - no one else can access this client
		 */
		client->binding->ops->disconnect(client->binding);
		memset(client, 0, sizeof(struct mcp_client_context));
		client->lifecycle_state = MCP_LIFECYCLE_DEINITIALIZED;
	}
}

/**
 * @brief Add a new client to the registry
 * @note Must be called with client_registry.mutex held
 * @return Pointer to client context if successful, NULL otherwise
 */
static struct mcp_client_context *add_client_locked(struct mcp_server_ctx *server,
						    struct mcp_transport_binding *binding)
{
	struct mcp_client_registry *client_registry = &server->client_registry;

	for (int i = 0; i < ARRAY_SIZE(client_registry->clients); i++) {
		if (client_registry->clients[i].lifecycle_state == MCP_LIFECYCLE_DEINITIALIZED) {
			client_registry->clients[i].lifecycle_state = MCP_LIFECYCLE_NEW;
			client_registry->clients[i].active_request_count = 0;
			client_registry->clients[i].binding = binding;
			memset(client_registry->clients[i].active_requests, 0,
			       CONFIG_MCP_MAX_CLIENT_REQUESTS *
				       sizeof(struct mcp_execution_context *));
			atomic_set(&client_registry->clients[i].refcount, 1);
			return &client_registry->clients[i];
		}
	}

	return NULL;
}

/**
 * @brief Remove a client from the registry
 * @note Must be called with client_registry.mutex held
 */
static void remove_client_locked(struct mcp_server_ctx *server, struct mcp_client_context *client)
{
	/* Mark as deinitializing. Cleanup happens when refcount reaches 0 */
	client->lifecycle_state = MCP_LIFECYCLE_DEINITIALIZING;

	/* Drop the initial reference */
	k_mutex_unlock(&server->client_registry.mutex);
	client_put(client);
	k_mutex_lock(&server->client_registry.mutex, K_FOREVER);
}

/*******************************************************************************
 * Execution Context Helper Functions
 * NOTE: All these functions assume the caller holds execution_registry.mutex
 ******************************************************************************/
/**
 * @brief Generates an execution token
 * @note The token is used to track each running execution of a tool
 * @param token_out Buffer to store the generated UUID string
 * @return 0 on success, negative errno on failure
 */
static int generate_execution_token(char *token_out)
{
	struct uuid execution_uuid;

	if (token_out == NULL) {
		return -EINVAL;
	}

	uuid_generate_v4(&execution_uuid);
	uuid_to_string(&execution_uuid, token_out);

	return 0;
}

/**
 * @brief Find an execution context by its token
 * @note Must be called with execution_registry.mutex held
 * @return Pointer to execution context if found, NULL otherwise
 */
static struct mcp_execution_context *get_execution_context(struct mcp_server_ctx *server,
							   const char *execution_token)
{
	struct mcp_execution_registry *execution_registry = &server->execution_registry;

	if (execution_token != NULL) {
		for (int i = 0; i < ARRAY_SIZE(execution_registry->executions); i++) {
			if ((execution_registry->executions[i].execution_state != MCP_EXEC_FREE) &&
				(strcmp(execution_registry->executions[i].execution_token,
					execution_token) == 0)) {
				return &execution_registry->executions[i];
			}
		}
	}

	return NULL;
}

/**
 * @brief Add a new execution context to the registry
 * @note Must be called with execution_registry.mutex held
 * @return Pointer to execution context if successful, NULL otherwise
 */
static struct mcp_execution_context *add_execution_context(struct mcp_server_ctx *server,
							   struct mcp_client_context *client,
							   struct mcp_request_id *request_id,
							   uint32_t msg_id)
{
	struct mcp_execution_context *context = NULL;
	struct mcp_execution_registry *execution_registry = &server->execution_registry;
	int ret;

	for (int i = 0; i < ARRAY_SIZE(execution_registry->executions); i++) {
		context = &execution_registry->executions[i];
		if (context->execution_state == MCP_EXEC_FREE) {
			ret = generate_execution_token(context->execution_token);
			if (ret != 0) {
				LOG_ERR("Failed to generate execution token: %d", ret);
				return NULL;
			}

			context->request_id = *request_id;
			context->transport_msg_id = msg_id;
			context->client = client;
			context->worker_id = k_current_get();
			context->start_timestamp = k_uptime_get();
			context->cancel_timestamp = 0;
			context->last_message_timestamp = k_uptime_get();
			context->execution_state = MCP_EXEC_ACTIVE;
			return context;
		}
	}

	LOG_ERR("Execution registry full");
	return NULL;
}

/**
 * @brief Remove an execution context from the registry
 * @note Must be called with execution_registry.mutex held
 */
static void remove_execution_context(struct mcp_server_ctx *server,
				     struct mcp_execution_context *execution_context)
{
	memset(execution_context, 0, sizeof(struct mcp_execution_context));
}


/**
 * @brief Check if execution is canceled and determine action
 * @note Must be called with execution_registry.mutex held
 * @return MCP_NOT_CANCELED, MCP_CANCELED_WITH_CLEANUP, or MCP_CANCELED
 */
static int check_canceled_execution(struct mcp_execution_context *execution_ctx,
					   enum mcp_tool_msg_type msg_type)
{
	if (execution_ctx->execution_state != MCP_EXEC_CANCELED) {
		return MCP_NOT_CANCELED;
	}

	if (msg_type == MCP_USR_TOOL_CANCEL_ACK) {
		execution_ctx->execution_state = MCP_EXEC_FINISHED;
		return MCP_CANCELED_WITH_CLEANUP;
	}

	if (msg_type == MCP_USR_TOOL_RESPONSE) {
		execution_ctx->execution_state = MCP_EXEC_FINISHED;
		LOG_WRN("Execution canceled, tool message will be dropped.");
		return MCP_CANCELED_WITH_CLEANUP;
	}

	LOG_DBG("Execution canceled, ignoring tool message type: %d", msg_type);
	return MCP_CANCELED;
}

/*******************************************************************************
 * Tool response processing helper functions
 ******************************************************************************/

/**
 * @brief Validate tool message parameters
 * @return 0 on success, negative error code on failure
 */
static int validate_tool_message_params(struct mcp_server_ctx *server,
					const struct mcp_tool_message *tool_msg,
					const char *execution_token)
{
	bool data_required;

	if (server == NULL) {
		LOG_ERR("Invalid server context");
		return -EINVAL;
	}

	if (tool_msg == NULL) {
		LOG_ERR("Invalid user message");
		return -EINVAL;
	}

	data_required = (tool_msg->type != MCP_USR_TOOL_CANCEL_ACK) &&
			     (tool_msg->type != MCP_USR_TOOL_PING);

	if ((tool_msg->data == NULL) && data_required) {
		LOG_ERR("Invalid user message");
		return -EINVAL;
	}

	if ((execution_token == NULL) || (execution_token[0] == '\0')) {
		LOG_ERR("Invalid execution token");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Send tool call response to client
 * @return 0 on success, negative error code on failure
 */
static int send_tool_response(struct mcp_client_context *client,
			      struct mcp_execution_context *execution_ctx,
			      const struct mcp_tool_message *tool_msg,
			      struct mcp_execution_registry *execution_registry,
			      struct mcp_result_tools_call **response_out,
			      uint8_t **json_buffer_out)
{
	int ret;
	struct mcp_result_tools_call *response_data;
	struct mcp_transport_message tx_msg;
	uint8_t *json_buffer;

	if (client == NULL) {
		LOG_ERR("Client context not found for client: %p", client);
		return -ENOENT;
	}

	response_data = (struct mcp_result_tools_call *)mcp_alloc(
		sizeof(struct mcp_result_tools_call));
	if (response_data == NULL) {
		LOG_ERR("Failed to allocate memory for response");
		return -ENOMEM;
	}

	*response_out = response_data;

	mcp_safe_strcpy((char *)response_data->content.items[0].text,
			sizeof(response_data->content.items[0].text),
			(char *)tool_msg->data);
	response_data->content.count = 1;
	response_data->content.items[0].type = MCP_CONTENT_TEXT;
	response_data->is_error = tool_msg->is_error;

	ret = k_mutex_lock(&execution_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock execution registry: %d", ret);
		return ret;
	}
	execution_ctx->execution_state = MCP_EXEC_FINISHED;
	k_mutex_unlock(&execution_registry->mutex);

	json_buffer = (uint8_t *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);
	if (json_buffer == NULL) {
		LOG_ERR("Failed to allocate buffer, dropping message");
		return -ENOMEM;
	}

	*json_buffer_out = json_buffer;

	ret = mcp_json_serialize_tools_call_result((char *)json_buffer,
						   CONFIG_MCP_MAX_MESSAGE_SIZE,
						   &execution_ctx->request_id,
						   response_data);
	if (ret <= 0) {
		LOG_ERR("Failed to serialize response: %d", ret);
		return ret;
	}

	tx_msg = (struct mcp_transport_message){
		.binding = client->binding,
		.msg_id = execution_ctx->transport_msg_id,
		.json_data = json_buffer,
		.json_len = ret
	};

	ret = client->binding->ops->send(&tx_msg);
	if (ret != 0) {
		LOG_ERR("Failed to send tool response");
		return ret;
	}

	return 0;
}

/**
 * @brief Free allocated response resources
 */
static void free_response_resources(struct mcp_result_tools_call *response_data,
				    uint8_t *json_buffer,
				    bool free_json_buffer)
{
	if (response_data != NULL) {
		mcp_free(response_data);
	}

	if (free_json_buffer && json_buffer) {
		mcp_free(json_buffer);
	}
}

/**
 * @brief Clean up client active request tracking
 */
static void cleanup_client_request(struct mcp_client_registry *client_registry,
				   struct mcp_client_context *client,
				   uint32_t msg_id,
				   bool is_execution_canceled)
{
	int ret;

	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry mutex: %d. Client registry is broken.",
			ret);
		return;
	}

	if (client == NULL) {
		if (is_execution_canceled) {
			LOG_DBG("Execution canceled, client was already cleaned up.");
		} else {
			LOG_ERR("Failed to find client in the client registry. "
				"Client registry is broken.");
		}
		k_mutex_unlock(&client_registry->mutex);
		return;
	}

	client->active_request_count--;

	for (int i = 0; i < CONFIG_MCP_MAX_CLIENT_REQUESTS; i++) {
		if ((client->active_requests[i] != NULL) &&
		    (client->active_requests[i]->transport_msg_id == msg_id)) {
			client->active_requests[i] = 0;
			break;
		}
	}

	k_mutex_unlock(&client_registry->mutex);
}

/**
 * @brief Clean up execution context from registry
 * @return 0 on success, negative error code on failure
 */
static int cleanup_execution(struct mcp_server_ctx *server,
			     struct mcp_execution_context *execution_ctx)
{
	int ret;
	struct mcp_execution_registry *execution_registry = &server->execution_registry;

	ret = k_mutex_lock(&execution_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock execution registry: %d. Execution registry is broken.",
			ret);
		return ret;
	}

	if (execution_ctx != NULL) {
		remove_execution_context(server, execution_ctx);
	}

	k_mutex_unlock(&execution_registry->mutex);
	return 0;
}

/**
 * @brief Perform final cleanup of client, execution context, and tool
 */
static int perform_final_cleanup(struct mcp_server_ctx *server,
				 struct mcp_client_context *client,
				 struct mcp_execution_context *execution_ctx,
				 struct mcp_tool_record *tool,
				 uint32_t msg_id,
				 bool is_execution_canceled)
{
	int ret;

	cleanup_client_request(&server->client_registry, client, msg_id, is_execution_canceled);

	ret = cleanup_execution(server, execution_ctx);

	if (tool != NULL) {
		atomic_dec(&tool->refcount);
	}

	return ret;
}

/*******************************************************************************
 * Tools Context Helper Functions
 * NOTE: All these functions assume the caller holds tool_registry.mutex
 ******************************************************************************/

/**
 * @brief Find a tool by name in the registry
 * @note Must be called with tool_registry.mutex held
 * @return Pointer to tool record if found, NULL otherwise
 */
static struct mcp_tool_record *get_tool(struct mcp_server_ctx *server, const char *tool_name)
{
	struct mcp_tool_registry *tool_registry = &server->tool_registry;

	for (int i = 0; i < ARRAY_SIZE(tool_registry->tools); i++) {
		if (tool_registry->tools[i].metadata.name[0] != '\0' &&
		    strncmp(tool_registry->tools[i].metadata.name, tool_name,
			    CONFIG_MCP_TOOL_NAME_MAX_LEN) == 0) {
			return &tool_registry->tools[i];
		}
	}

	return NULL;
}

/**
 * @brief Add a new tool to the registry
 * @note Must be called with tool_registry.mutex held
 * @return Pointer to tool record if successful, NULL otherwise
 */
static struct mcp_tool_record *add_tool(struct mcp_server_ctx *server,
					const struct mcp_tool_record *tool_info)
{
	struct mcp_tool_registry *tool_registry = &server->tool_registry;

	for (int i = 0; i < ARRAY_SIZE(tool_registry->tools); i++) {
		if (tool_registry->tools[i].metadata.name[0] == '\0') {
			memcpy(&tool_registry->tools[i], tool_info, sizeof(struct mcp_tool_record));
			tool_registry->tool_count++;
			atomic_set(&tool_registry->tools[i].refcount, 1);
			return &tool_registry->tools[i];
		}
	}

	return NULL;
}

/**
 * @brief Copy tool metadata to response buffer
 * @note Must be called with tool_registry.mutex held
 * @return 0 on success, negative error code on failure
 */
static int copy_tool_metadata_to_response(struct mcp_server_ctx *server,
					  struct mcp_result_tools_list *response_data)
{
	struct mcp_tool_metadata *tool_info;
	struct mcp_tool_registry *tool_registry = &server->tool_registry;
	char *buf = response_data->tools_json;
	size_t buf_size = sizeof(response_data->tools_json);
	size_t offset = 0;
	int ret;
	int tools_written = 0;

	/* Start with opening bracket */
	ret = snprintk(buf + offset, buf_size - offset, "[");
	if (ret < 0 || (size_t)ret >= buf_size - offset) {
		LOG_ERR("Buffer overflow adding opening bracket");
		return -ENOMEM;
	}
	offset += ret;

	for (int i = 0; i < CONFIG_MCP_MAX_TOOLS; i++) {
		if (tool_registry->tools[i].metadata.name[0] == '\0') {
			continue;
		}

		tool_info = &tool_registry->tools[i].metadata;

		ret = snprintk(buf + offset, buf_size - offset,
			       "%s{"
			       "\"name\":\"%s\","
#ifdef CONFIG_MCP_TOOL_TITLE
			       "\"title\":\"%s\","
#endif
#ifdef CONFIG_MCP_TOOL_DESC
			       "\"description\":\"%s\","
#endif
			       "\"inputSchema\":%s"
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			       ",\"outputSchema\":%s"
#endif
			       "}",
			       (tools_written > 0)
				       ? ","
				       : "", /* Add comma separator except for first item */
			       tool_info->name,
#ifdef CONFIG_MCP_TOOL_TITLE
			       tool_info->title,
#endif
#ifdef CONFIG_MCP_TOOL_DESC
			       tool_info->description,
#endif
			       tool_info->input_schema
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			       ,
			       tool_info->output_schema
#endif
		);

		if (ret < 0 || (size_t)ret >= buf_size - offset) {
			LOG_ERR("Buffer overflow adding tool %d", i);
			return -ENOMEM;
		}

		offset += ret;
		tools_written++;
	}

	/* Add closing bracket */
	ret = snprintk(buf + offset, buf_size - offset, "]");
	if (ret < 0 || (size_t)ret >= buf_size - offset) {
		LOG_ERR("Buffer overflow adding closing bracket");
		return -ENOMEM;
	}

	return 0;
}
/*******************************************************************************
 * Request/Response Handling Functions
 ******************************************************************************/

/**
 * @brief Serialize the error response to JSON and send it to the client
 * @return 0 on success, negative error code on failure
 */
static int send_error_response(struct mcp_server_ctx *server, struct mcp_client_context *client,
			       struct mcp_request_id *request_id, int32_t error_code,
			       const char *error_message, uint32_t msg_id)
{
	struct mcp_error *error_response;
	struct mcp_transport_message tx_msg;
	char *json_buffer;
	int ret;

	error_response = (struct mcp_error *)mcp_alloc(sizeof(struct mcp_error));
	if (error_response == NULL) {
		LOG_ERR("Failed to allocate error response");
		return -ENOMEM;
	}

	mcp_safe_strcpy(error_response->message, sizeof(error_response->message), error_message);

	error_response->code = error_code;
	error_response->has_data = false;

	/* Allocate buffer for serialization */
	json_buffer = (char *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);

	if (json_buffer == NULL) {
		LOG_ERR("Failed to allocate buffer, dropping message");
		mcp_free(error_response);
		return -ENOMEM;
	}

	ret = mcp_json_serialize_error(json_buffer, CONFIG_MCP_MAX_MESSAGE_SIZE, request_id,
				       error_response);
	if (ret <= 0) {
		LOG_ERR("Failed to serialize response: %d", ret);
		mcp_free(error_response);
		mcp_free(json_buffer);
		return ret;
	}

	tx_msg = (struct mcp_transport_message){
		.binding = client->binding,
		.msg_id = msg_id,
		.json_data = json_buffer,
		.json_len = ret
	};

	ret = client->binding->ops->send(&tx_msg);
	if (ret != 0) {
		LOG_ERR("Failed to send error response");
		mcp_free(error_response);
		mcp_free(json_buffer);
		return ret;
	}

	mcp_free(error_response);
	return 0;
}

/**
 * @brief Handle initialize request from client and respond with server capabilities
 * @return 0 on success, negative error code on failure
 */
static int handle_initialize_request(struct mcp_server_ctx *server, struct mcp_message *request,
				     struct mcp_transport_binding *binding, uint32_t msg_id)
{
	int ret;
	struct mcp_client_registry *client_registry = &server->client_registry;
	struct mcp_client_context *new_client;
	struct mcp_result_initialize *response_data = NULL;
	struct mcp_transport_message tx_msg;
	uint8_t *json_buffer = NULL;

	if (strcmp(request->protocol_version, MCP_PROTOCOL_VERSION) != 0) {
		LOG_WRN("Protocol version mismatch: %s", request->protocol_version);
	}

	/* Lock client registry and add new client */
	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry mutex: %d", ret);
		return ret;
	}

	new_client = add_client_locked(server, binding);
	if (new_client == NULL) {
		LOG_ERR("Client registry full");
		k_mutex_unlock(&client_registry->mutex);
		return -ENOMEM;
	}

	new_client->lifecycle_state = MCP_LIFECYCLE_INITIALIZING;
	new_client->last_message_timestamp = k_uptime_get();

	client_get_locked(new_client);
	k_mutex_unlock(&client_registry->mutex);

	/* Allocate and prepare response */
	response_data =
		(struct mcp_result_initialize *)mcp_alloc(sizeof(struct mcp_result_initialize));
	if (response_data == NULL) {
		LOG_ERR("Failed to allocate response");
		ret = -ENOMEM;
		goto cleanup;
	}

	mcp_safe_strcpy(response_data->server_version, sizeof(response_data->server_version),
			CONFIG_MCP_SERVER_INFO_VERSION);
	mcp_safe_strcpy(response_data->server_name, sizeof(response_data->server_name),
			CONFIG_MCP_SERVER_INFO_NAME);
	mcp_safe_strcpy(response_data->protocol_version, sizeof(response_data->protocol_version),
			MCP_PROTOCOL_VERSION);
	mcp_safe_strcpy(response_data->capabilities_json, sizeof(response_data->capabilities_json),
			"{\"tools\":{\"listChanged\":false}}");
	response_data->has_capabilities = true;

	/* Allocate buffer for serialization */
	json_buffer = (uint8_t *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);
	if (json_buffer == NULL) {
		LOG_ERR("Failed to allocate buffer, dropping message");
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Serialize response to JSON */
	ret = mcp_json_serialize_initialize_result((char *)json_buffer, CONFIG_MCP_MAX_MESSAGE_SIZE,
						   &request->id, response_data);
	if (ret < 0) {
		LOG_ERR("Failed to serialize response: %d", ret);
		goto cleanup;
	}

	tx_msg = (struct mcp_transport_message){
		.binding = new_client->binding,
		.msg_id = msg_id,
		.json_data = json_buffer,
		.json_len = ret
	};

	ret = new_client->binding->ops->send(&tx_msg);
	if (ret != 0) {
		LOG_ERR("Failed to send initialize response %d", ret);
		goto cleanup;
	}

	mcp_free(response_data);
	client_put(new_client);
	return 0;

cleanup:
	if (json_buffer != NULL) {
		mcp_free(json_buffer);
	}
	if (response_data != NULL) {
		mcp_free(response_data);
	}

	/* Remove client on failure */
	if (k_mutex_lock(&client_registry->mutex, K_FOREVER) == 0) {
		remove_client_locked(server, new_client);
		k_mutex_unlock(&client_registry->mutex);
	}

	client_put(new_client);
	return ret;
}

/**
 * @brief Handle tools/list request from client and respond with available tools
 * @return 0 on success, negative error code on failure
 */
static int handle_tools_list_request(struct mcp_server_ctx *server,
				     struct mcp_client_context *client, struct mcp_message *request,
				     uint32_t msg_id)
{
	struct mcp_result_tools_list *response_data = NULL;
	struct mcp_transport_message tx_msg;
	uint8_t *json_buffer = NULL;
	int ret;

	struct mcp_client_registry *client_registry = &server->client_registry;
	struct mcp_tool_registry *tool_registry = &server->tool_registry;

	LOG_DBG("Processing tools list request");

	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry mutex: %d", ret);
		return ret;
	}

	if (client->lifecycle_state != MCP_LIFECYCLE_INITIALIZED) {
		LOG_DBG("Client not in initialized state: %p", client);
		k_mutex_unlock(&client_registry->mutex);
		return -EACCES;
	}

	client_get_locked(client);
	k_mutex_unlock(&client_registry->mutex);

	/* Allocate response structure */
	response_data =
		(struct mcp_result_tools_list *)mcp_alloc(sizeof(struct mcp_result_tools_list));
	if (response_data == NULL) {
		LOG_ERR("Failed to allocate response");
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Copy tool metadata while holding tool registry lock */
	ret = k_mutex_lock(&tool_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock tool registry");
		goto cleanup;
	}

	ret = copy_tool_metadata_to_response(server, response_data);
	k_mutex_unlock(&tool_registry->mutex);

	if (ret != 0) {
		goto cleanup;
	}

	/* Allocate buffer for serialization */
	json_buffer = (uint8_t *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);
	if (json_buffer == NULL) {
		LOG_ERR("Failed to allocate buffer, dropping message");
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Serialize response to JSON */
	ret = mcp_json_serialize_tools_list_result((char *)json_buffer, CONFIG_MCP_MAX_MESSAGE_SIZE,
						   &request->id, response_data);
	if (ret <= 0) {
		LOG_ERR("Failed to serialize response: %d", ret);
		goto cleanup;
	}

	tx_msg = (struct mcp_transport_message){
		.binding = client->binding,
		.msg_id = msg_id,
		.json_data = json_buffer,
		.json_len = ret
	};

	ret = client->binding->ops->send(&tx_msg);
	if (ret != 0) {
		LOG_ERR("Failed to send tools list response");
		goto cleanup;
	}

	mcp_free(response_data);
	client_put(client);
	return 0;

cleanup:
	if (json_buffer != NULL) {
		mcp_free(json_buffer);
	}

	if (response_data != NULL) {
		mcp_free(response_data);
	}

	client_put(client);
	return ret;
}

/**
 * @brief Handle tools/call request from client and execute the requested tool
 * @return 0 on success, negative error code on failure
 */
static int handle_tools_call_request(struct mcp_server_ctx *server,
				     struct mcp_client_context *client, struct mcp_message *request,
				     uint32_t msg_id)
{
	int ret;
	int request_index = -1;
	struct mcp_client_registry *client_registry = &server->client_registry;
	struct mcp_tool_registry *tool_registry = &server->tool_registry;
	struct mcp_execution_registry *execution_registry = &server->execution_registry;
	struct mcp_tool_record *tool = NULL;
	struct mcp_execution_context *exec_ctx = NULL;

	LOG_DBG("Processing tools call request");

	/* Check client state, increment active request count, and acquire reference */
	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry mutex: %d", ret);
		return ret;
	}

	if (client->lifecycle_state != MCP_LIFECYCLE_INITIALIZED) {
		LOG_DBG("Client not in initialized state: %p", client);
		k_mutex_unlock(&client_registry->mutex);
		return -EACCES;
	}

	if (client->active_request_count >= CONFIG_MCP_MAX_CLIENT_REQUESTS) {
		LOG_DBG("Client (%p) has reached maximum active requests", client);
		k_mutex_unlock(&client_registry->mutex);
		return -EBUSY;
	}

	client_get_locked(client);
	k_mutex_unlock(&client_registry->mutex);

	/* Look up tool */
	ret = k_mutex_lock(&tool_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock tool registry: %d", ret);
		client_put(client);
		return ret;
	}

	tool = get_tool(server, request->req.tools_call.name);
	if (tool == NULL) {
		k_mutex_unlock(&tool_registry->mutex);
		LOG_ERR("Tool '%s' not found", request->req.tools_call.name);
		client_put(client);
		return -ENOENT;
	}

	atomic_inc(&tool->refcount);
	k_mutex_unlock(&tool_registry->mutex);

	/* Create execution context */
	ret = k_mutex_lock(&execution_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock execution registry: %d", ret);
		goto cleanup_tool;
	}

	exec_ctx = add_execution_context(server, client, &request->id, msg_id);
	if (exec_ctx == NULL) {
		LOG_ERR("Failed to create execution context");
		k_mutex_unlock(&execution_registry->mutex);
		ret = -ENOMEM;
		goto cleanup_tool;
	}

	exec_ctx->tool = tool;
	k_mutex_unlock(&execution_registry->mutex);

	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry: %d", ret);
		goto cleanup_execution;
	}

	/* After reaching here, active_request needs to be cleaned up in case of later error */
	for (request_index = 0; request_index < CONFIG_MCP_MAX_CLIENT_REQUESTS; request_index++) {
		if (client->active_requests[request_index] == 0) {
			client->active_requests[request_index] = exec_ctx;
			break;
		}
	}

	client->active_request_count++;
	k_mutex_unlock(&client_registry->mutex);

	/* Call the tool callback */
	ret = tool->callback(MCP_TOOL_CALL_REQUEST, request->req.tools_call.arguments_json,
			     exec_ctx->execution_token);
	if (ret != 0) {
		LOG_ERR("Tool callback failed: %d", ret);
		goto cleanup_active_request;
	}

	client_put(client);
	return 0;

cleanup_active_request:
	if (k_mutex_lock(&client_registry->mutex, K_FOREVER) == 0) {
		client->active_request_count--;
		client->active_requests[request_index] = 0;
		k_mutex_unlock(&client_registry->mutex);
	}

cleanup_execution:
	if ((exec_ctx != NULL) && (k_mutex_lock(&execution_registry->mutex, K_FOREVER) == 0)) {
		remove_execution_context(server, exec_ctx);
		k_mutex_unlock(&execution_registry->mutex);
	}

cleanup_tool:
	atomic_dec(&tool->refcount);

	client_put(client);
	return ret;
}

/**
 * @brief Handle notification from client
 * @return 0 on success, negative error code on failure
 */
static int handle_notification(struct mcp_server_ctx *server, struct mcp_client_context *client,
			       struct mcp_message *notification, uint32_t msg_id)
{
	int ret;
	struct mcp_client_registry *client_registry = &server->client_registry;

	LOG_DBG("Processing notification");

	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry");
		return ret;
	}

	if (client_get_locked(client) == NULL) {
		k_mutex_unlock(&client_registry->mutex);
		return -ENOENT;
	}

	switch (notification->method) {
	case MCP_METHOD_NOTIF_INITIALIZED:
		/* State transition: INITIALIZING -> INITIALIZED */
		if (client->lifecycle_state == MCP_LIFECYCLE_INITIALIZING) {
			client->lifecycle_state = MCP_LIFECYCLE_INITIALIZED;
		} else {
			LOG_ERR("Invalid state transition for client %p", client);
			k_mutex_unlock(&client_registry->mutex);
			client_put(client);
			return -EPERM;
		}
		break;

	case MCP_METHOD_NOTIF_CANCELLED:
		if (client->lifecycle_state != MCP_LIFECYCLE_INITIALIZED) {
			LOG_DBG("Client not in initialized state: %p", client);
			k_mutex_unlock(&client_registry->mutex);
			client_put(client);
			return -EACCES;
		}

		if (client->active_request_count <= 0) {
			LOG_DBG("Client (%p) has no active requests", client);
			k_mutex_unlock(&client_registry->mutex);
			client_put(client);
			return -EINVAL;
		}

		for (int i = 0; i < CONFIG_MCP_MAX_CLIENT_REQUESTS; i++) {
			if ((client->active_requests[i] != NULL) &&
			    (client->active_requests[i]->transport_msg_id == msg_id)) {
				struct mcp_execution_context *execution_ctx =
					client->active_requests[i];

				ret = k_mutex_lock(&server->execution_registry.mutex, K_FOREVER);
				if (ret != 0) {
					LOG_ERR("Failed to lock execution registry: %d. Can't "
						"cancel execution.",
						ret);
					k_mutex_unlock(&client_registry->mutex);
					client_put(client);
					return -ENOSPC;
				}

				execution_ctx->execution_state = MCP_EXEC_CANCELED;
				execution_ctx->cancel_timestamp = k_uptime_get();
				execution_ctx->tool->callback(MCP_TOOL_CANCEL_REQUEST, NULL,
							      execution_ctx->execution_token);
				k_mutex_unlock(&server->execution_registry.mutex);
				break;
			}
		}
		break;

	default:
		LOG_ERR("Unknown notification method %u", notification->method);
		k_mutex_unlock(&client_registry->mutex);
		client_put(client);
		return -EINVAL;
	}

	k_mutex_unlock(&client_registry->mutex);
	client_put(client);
	return 0;
}

/**
 * @brief Handle ping request from client and respond with ping response
 * @return 0 on success, negative error code on failure
 */
static int handle_ping_request(struct mcp_server_ctx *server, struct mcp_client_context *client,
			       struct mcp_message *request, uint32_t msg_id)
{
	int ret;
	struct mcp_client_registry *client_registry = &server->client_registry;
	struct mcp_transport_message tx_msg;
	uint8_t *json_buffer = NULL;

	LOG_DBG("Processing ping request");

	/* Check client state and acquire reference */
	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry mutex: %d", ret);
		return ret;
	}

	if (client->lifecycle_state != MCP_LIFECYCLE_INITIALIZED) {
		LOG_DBG("Client not in initialized state: %p", client);
		k_mutex_unlock(&client_registry->mutex);
		return -EACCES;
	}

	client_get_locked(client);
	k_mutex_unlock(&client_registry->mutex);

	/* Allocate buffer for serialization */
	json_buffer = (uint8_t *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);
	if (json_buffer == NULL) {
		LOG_ERR("Failed to allocate buffer, dropping message");
		ret = -ENOMEM;
		goto cleanup;
	}

	/* Serialize response to JSON */
	ret = mcp_json_serialize_ping_result((char *)json_buffer, CONFIG_MCP_MAX_MESSAGE_SIZE,
					     &request->id, NULL);
	if (ret <= 0) {
		LOG_ERR("Failed to serialize response: %d", ret);
		goto cleanup;
	}

	tx_msg = (struct mcp_transport_message){
		.binding = client->binding,
		.msg_id = msg_id,
		.json_data = json_buffer,
		.json_len = ret
	};

	ret = client->binding->ops->send(&tx_msg);
	if (ret != 0) {
		LOG_ERR("Failed to send ping response");
		goto cleanup;
	}

	client_put(client);
	return 0;

cleanup:
	if (json_buffer != NULL) {
		mcp_free(json_buffer);
	}
	client_put(client);
	return ret;
}

/*******************************************************************************
 * Worker threads
 ******************************************************************************/
/**
 * @brief MCP Request worker thread
 * @param ctx pointer to server context
 * @param wid worker id
 * @param arg3 NULL
 */
static void mcp_request_worker(void *ctx, void *wid, void *arg3)
{
	struct mcp_queue_msg request;
	struct mcp_message *message;
	int32_t error_code;
	char *error_message;

	int worker_id = POINTER_TO_INT(wid);
	int ret;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;

	LOG_INF("Request worker %d started", worker_id);

	while (1) {
		ret = k_msgq_get(&server->request_queue, &request, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to get request: %d", ret);
			continue;
		}

		if (request.data == NULL) {
			LOG_ERR("NULL data in request");
			if (request.client) {
				client_put(request.client);
			}
			continue;
		}

		message = (struct mcp_message *)request.data;

		switch (message->method) {
		case MCP_METHOD_INITIALIZE:
			/* Handled immediately in mcp_server_handle_request */
			LOG_DBG("Should never reach here");
			ret = 0;
			break;
		case MCP_METHOD_PING:
			ret = handle_ping_request(server, request.client, message,
						  request.transport_msg_id);
			break;
		case MCP_METHOD_NOTIF_INITIALIZED:
			ret = handle_notification(server, request.client, message, 0);
			break;
		case MCP_METHOD_TOOLS_LIST:
			ret = handle_tools_list_request(server, request.client, message,
							request.transport_msg_id);
			break;
		case MCP_METHOD_TOOLS_CALL:
			ret = handle_tools_call_request(server, request.client, message,
							request.transport_msg_id);
			break;
		case MCP_METHOD_NOTIF_CANCELLED:
			ret = handle_notification(server, request.client, message,
						  request.transport_msg_id);
			break;
		default:
			/* Should never get here. Requests are validated in
			 * mcp_server_handle_request
			 */
			LOG_ERR("Unknown request");
			mcp_free(request.data);
			client_put(request.client);
			continue;
		}

		if ((message->kind != MCP_MSG_NOTIFICATION) && (ret != 0)) {
			switch (ret) {
			case -ENOENT:
				error_code = MCP_ERR_METHOD_NOT_FOUND;
				error_message = "Resource not found";
				break;
			case -EPERM:
				error_code = MCP_ERR_INVALID_PARAMS;
				error_message = "Permission denied";
				break;
			case -ENOSPC:
				error_code = MCP_ERR_INTERNAL_ERROR;
				error_message = "Resource exhausted";
				break;
			case -ENOMEM:
				error_code = MCP_ERR_INTERNAL_ERROR;
				error_message = "Memory allocation failed";
				break;
			case -EACCES:
				error_code = MCP_ERR_INVALID_PARAMS;
				error_message = "Client not initialized";
				break;
			case -EBUSY:
				error_code = MCP_ERR_BUSY;
				error_message = "Client is busy";
				break;
			default:
				error_code = MCP_ERR_INTERNAL_ERROR;
				error_message = "Internal server error";
				break;
			}
			send_error_response(server, request.client, &message->id, error_code,
					    error_message, request.transport_msg_id);
		}

		mcp_free(request.data);
		/* Release reference acquired when queuing */
		client_put(request.client);
	}
}

/**
 * @brief MCP Core health monitor worker thread
 * @note Monitors execution timeouts and client health status, cancels stale requests
 * @param ctx pointer to server context
 * @param wid worker id
 * @param arg3 NULL
 */
static void mcp_health_monitor_worker(void *ctx, void *arg2, void *arg3)
{
	int ret;
	int64_t current_time;
	int64_t execution_duration;
	int64_t idle_duration;
	int64_t cancel_duration;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;
	struct mcp_execution_registry *execution_registry = &server->execution_registry;
	struct mcp_client_registry *client_registry = &server->client_registry;

	while (1) {
		k_sleep(K_MSEC(CONFIG_MCP_HEALTH_CHECK_INTERVAL_MS));

		current_time = k_uptime_get();

		/* Check execution contexts */
		ret = k_mutex_lock(&execution_registry->mutex, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to lock execution registry: %d", ret);
			continue;
		}

		for (int i = 0; i < MCP_MAX_REQUESTS; i++) {
			uint8_t *json_buffer = NULL;
			struct mcp_execution_context *context = &execution_registry->executions[i];
			struct mcp_params_notif_cancelled *params = NULL;

			if (context->execution_state == MCP_EXEC_FREE) {
				continue;
			}

			if (context->execution_state == MCP_EXEC_CANCELED) {
				cancel_duration = current_time - context->cancel_timestamp;

				if (cancel_duration > CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS) {
					LOG_ERR("Execution token %s exceeded cancellation "
						"timeout (%lld ms). Client: %p, Worker ID %u",
						context->execution_token, cancel_duration,
						context->client, (uint32_t)context->worker_id);
				}
				continue;
			}

			if (context->execution_state == MCP_EXEC_FINISHED) {
				continue;
			}

			execution_duration = current_time - context->start_timestamp;

			if (execution_duration > CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS) {
				struct mcp_transport_message tx_msg;

				LOG_WRN("Execution token %s exceeded execution timeout "
					"(%lld ms). Client: %p, Worker ID %u",
					context->execution_token, execution_duration,
					context->client, (uint32_t)context->worker_id);
				/* Allocate notification params structure */
				params = (struct mcp_params_notif_cancelled *)mcp_alloc(
					sizeof(struct mcp_params_notif_cancelled));

				if (params == NULL) {
					LOG_ERR("Failed to allocate notification params");
					ret = -ENOMEM;
					continue;
				}

				/* Allocate buffer for serialization */
				json_buffer = (uint8_t *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);
				if (json_buffer == NULL) {
					LOG_ERR("Failed to allocate buffer, dropping notification");
					mcp_free(params);
					continue;
				}

				/* Fill in the notification parameters */
				params->request_id = context->request_id;
				mcp_safe_strcpy(params->reason, sizeof(params->reason),
						"Execution timeout");
				params->has_reason = true;

				/* Serialize the notification */
				ret = mcp_json_serialize_cancel_notification(
					(char *)json_buffer, CONFIG_MCP_MAX_MESSAGE_SIZE, params);
				if (ret <= 0) {
					LOG_ERR("Failed to serialize cancel notification: %d", ret);
					mcp_free(params);
					mcp_free(json_buffer);
					continue;
				}

				/* Send the notification to the client */
				tx_msg = (struct mcp_transport_message){
					.binding = context->client->binding,
					/* Notifications don't have a response msg_id */
					.msg_id = 0,
					.json_data = (char *)json_buffer,
					.json_len = ret
				};

				ret = context->client->binding->ops->send(&tx_msg);
				if (ret != 0) {
					LOG_ERR("Failed to send cancel notification: %d", ret);
				}

				/* Clean up */
				mcp_free(params);
				mcp_free(json_buffer);

				/* Update execution state */
				context->execution_state = MCP_EXEC_CANCELED;
				context->cancel_timestamp = current_time;
				context->tool->callback(MCP_TOOL_CANCEL_REQUEST, NULL,
							context->execution_token);
				continue;
			}

			if (context->last_message_timestamp > 0) {
				idle_duration = current_time - context->last_message_timestamp;

				if (idle_duration > CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS) {
					struct mcp_transport_message tx_msg;

					LOG_WRN("Execution token %s exceeded idle timeout "
						"(%lld ms). Client: %p, Worker ID %u",
						context->execution_token, idle_duration,
						context->client, (uint32_t)context->worker_id);

					/* Allocate notification params structure */
					params = (struct mcp_params_notif_cancelled *)mcp_alloc(
						sizeof(struct mcp_params_notif_cancelled));

					if (params == NULL) {
						LOG_ERR("Failed to allocate notification params");
						ret = -ENOMEM;
						continue;
					}

					/* Allocate buffer for serialization */
					json_buffer =
						(uint8_t *)mcp_alloc(CONFIG_MCP_MAX_MESSAGE_SIZE);
					if (json_buffer == NULL) {
						LOG_ERR("Failed to allocate buffer, dropping "
							"notification");
						mcp_free(params);
						continue;
					}

					/* Fill in the notification parameters */
					params->request_id = context->request_id;
					mcp_safe_strcpy(params->reason, sizeof(params->reason),
							"Tool idle timeout");
					params->has_reason = true;

					/* Serialize the notification */
					ret = mcp_json_serialize_cancel_notification(
						(char *)json_buffer, CONFIG_MCP_MAX_MESSAGE_SIZE,
						params);
					if (ret <= 0) {
						LOG_ERR("Failed to serialize cancel notification: "
							"%d",
							ret);
						mcp_free(params);
						mcp_free(json_buffer);
						continue;
					}

					/* Send the notification to the client */
					tx_msg = (struct mcp_transport_message){
						.binding = context->client->binding,
						/* Notifications don't have a response msg_id */
						.msg_id = 0,
						.json_data = (char *)json_buffer,
						.json_len = ret
					};

					ret = context->client->binding->ops->send(&tx_msg);
					if (ret != 0) {
						LOG_ERR("Failed to send cancel notification: %d",
							ret);
					}

					/* Clean up */
					mcp_free(params);
					mcp_free(json_buffer);

					context->execution_state = MCP_EXEC_CANCELED;
					context->cancel_timestamp = current_time;
					context->tool->callback(MCP_TOOL_CANCEL_REQUEST, NULL,
								context->execution_token);
				}
			}
		}

		k_mutex_unlock(&execution_registry->mutex);

		/* Check client contexts */
		ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to lock client registry: %d", ret);
			continue;
		}

		for (int i = 0; i < ARRAY_SIZE(client_registry->clients); i++) {
			struct mcp_client_context *client_context = &client_registry->clients[i];

			if (client_context->lifecycle_state == MCP_LIFECYCLE_DEINITIALIZED ||
			    client_context->lifecycle_state == MCP_LIFECYCLE_DEINITIALIZING) {
				continue;
			}

			if (client_context->last_message_timestamp > 0) {
				idle_duration =
					current_time - client_context->last_message_timestamp;

				if (idle_duration > CONFIG_MCP_CLIENT_TIMEOUT_MS) {
					LOG_WRN("Client %p exceeded idle timeout (%lld ms). "
						"Marking as disconnected.",
						client_context, idle_duration);

					remove_client_locked(server, client_context);
				}
			}
		}
		k_mutex_unlock(&client_registry->mutex);
	}
}

/*******************************************************************************
 * Internal Interface Implementation
 ******************************************************************************/
int mcp_server_handle_request(mcp_server_ctx_t ctx, struct mcp_transport_message *request,
			      enum mcp_method *method)
{
	int ret;
	struct mcp_queue_msg msg;
	struct mcp_client_context *client = NULL;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;
	struct mcp_client_registry *client_registry = &server->client_registry;
	struct mcp_message *parsed_msg;

	if ((server == NULL) || (request == NULL) || (request->json_data == NULL) ||
	    (method == NULL)) {
		LOG_ERR("Invalid parameters passed to %s", __func__);
		return -EINVAL;
	}

	parsed_msg = (struct mcp_message *)mcp_alloc(sizeof(struct mcp_message));
	if (parsed_msg == NULL) {
		LOG_ERR("Failed to allocate memory for response");
		return -ENOMEM;
	}

	ret = mcp_json_parse_message(request->json_data, request->json_len, parsed_msg);
	if (ret != 0) {
		LOG_ERR("Failed to parse JSON request: %d", ret);
		mcp_free(parsed_msg);
		return ret;
	}

	*method = parsed_msg->method;
	LOG_DBG("Request method: %d", parsed_msg->method);

	switch (parsed_msg->method) {
	case MCP_METHOD_INITIALIZE:
		/* We want to handle the initialize request directly */
		ret = handle_initialize_request(server, parsed_msg, request->binding,
						request->msg_id);
		mcp_free(parsed_msg);
		break;
	case MCP_METHOD_TOOLS_LIST:
	case MCP_METHOD_TOOLS_CALL:
	case MCP_METHOD_PING:
		if (strcmp(request->protocol_version, MCP_PROTOCOL_VERSION) != 0) {
			LOG_ERR("Protocol version mismatch: %s", request->protocol_version);
			mcp_free(parsed_msg);
			return -EPROTO;
		}
	case MCP_METHOD_NOTIF_INITIALIZED:
	case MCP_METHOD_NOTIF_CANCELLED:
		ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to lock client registry: %d", ret);
			mcp_free(parsed_msg);
			return ret;
		}

		client = get_client_by_binding_locked(server, request->binding);
		if (client == NULL) {
			LOG_ERR("Client does not exist");
			k_mutex_unlock(&client_registry->mutex);
			mcp_free(parsed_msg);
			ret = -ENOENT;
			break;
		}

		client->last_message_timestamp = k_uptime_get();

		/* Acquire reference for queued message */
		client_get_locked(client);
		k_mutex_unlock(&client_registry->mutex);

		msg.data = (void *)parsed_msg;
		msg.client = client;
		msg.transport_msg_id = request->msg_id;

		/* Parsed message is now owned by the queue */
		ret = k_msgq_put(&server->request_queue, &msg, K_NO_WAIT);
		if (ret != 0) {
			LOG_ERR("Failed to queue request message: %d", ret);
			mcp_free(parsed_msg);
			client_put(client);
		}

		break;
	case MCP_METHOD_UNKNOWN:
		ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
		if (ret != 0) {
			LOG_ERR("Failed to lock client registry: %d", ret);
			mcp_free(parsed_msg);
			return ret;
		}

		client = get_client_by_binding_locked(server, request->binding);
		if (client == NULL) {
			LOG_ERR("Client does not exist.");
			k_mutex_unlock(&client_registry->mutex);
			mcp_free(parsed_msg);
			ret = -ENOENT;
			break;
		}

		client->last_message_timestamp = k_uptime_get();

		client_get_locked(client);
		k_mutex_unlock(&client_registry->mutex);

		ret = send_error_response(server, client, &parsed_msg->id, MCP_ERR_METHOD_NOT_FOUND,
					  "Method not found", request->msg_id);
		mcp_free(parsed_msg);
		client_put(client);
		break;
	default:
		LOG_WRN("Request not recognized. Dropping.");
		ret = -ENOTSUP;
		mcp_free(parsed_msg);
		break;
	}

	return ret;
}

int mcp_server_update_client_timestamp(mcp_server_ctx_t ctx, struct mcp_transport_binding *binding)
{
	int ret;
	struct mcp_client_context *client = NULL;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;
	struct mcp_client_registry *client_registry = &server->client_registry;

	if (server == NULL) {
		LOG_ERR("Invalid parameters passed to %s", __func__);
		return -EINVAL;
	}

	ret = k_mutex_lock(&client_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock client registry: %d", ret);
		return ret;
	}

	client = get_client_by_binding_locked(server, binding);
	if (client == NULL) {
		LOG_ERR("Client does not exist");
		k_mutex_unlock(&client_registry->mutex);
		return -ENOENT;
	}

	client->last_message_timestamp = k_uptime_get();
	k_mutex_unlock(&client_registry->mutex);

	return 0;
}

/*******************************************************************************
 * API Implementation
 ******************************************************************************/
mcp_server_ctx_t mcp_server_init(void)
{
	int ret;
	struct mcp_server_ctx *server_ctx;

	LOG_INF("Initializing MCP Server");

	server_ctx = allocate_mcp_server_context();

	if (server_ctx == NULL) {
		LOG_ERR("No available server contexts");
		return NULL;
	}

	k_msgq_init(&server_ctx->request_queue, server_ctx->request_queue_storage,
		    sizeof(struct mcp_queue_msg), MCP_MAX_REQUESTS);

	ret = k_mutex_init(&server_ctx->client_registry.mutex);
	if (ret != 0) {
		LOG_ERR("Failed to init client mutex: %d", ret);
		return NULL;
	}

	ret = k_mutex_init(&server_ctx->tool_registry.mutex);
	if (ret != 0) {
		LOG_ERR("Failed to init tool mutex: %d", ret);
		return NULL;
	}

	ret = k_mutex_init(&server_ctx->execution_registry.mutex);
	if (ret != 0) {
		LOG_ERR("Failed to init execution mutex: %d", ret);
		return NULL;
	}

	server_ctx->in_use = true;

	LOG_INF("MCP Server initialized");
	return (mcp_server_ctx_t)server_ctx;
}

int mcp_server_start(mcp_server_ctx_t ctx)
{
	k_tid_t tid;
	uint32_t thread_stack_idx;
#if CONFIG_THREAD_NAME
	int ret;
#endif
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;

	if (server == NULL) {
		LOG_ERR("Invalid server context");
		return -EINVAL;
	}

	LOG_INF("Starting MCP Server");

	for (int i = 0; i < ARRAY_SIZE(server->request_workers); i++) {
		thread_stack_idx = (server->idx * CONFIG_MCP_REQUEST_WORKERS) + i;
		tid = k_thread_create(
			&server->request_workers[i], mcp_request_worker_stacks[thread_stack_idx],
			K_THREAD_STACK_SIZEOF(mcp_request_worker_stacks[thread_stack_idx]),
			mcp_request_worker, server, INT_TO_POINTER(i), NULL,
			K_PRIO_COOP(MCP_WORKER_PRIORITY), 0, K_NO_WAIT);
		if (tid == NULL) {
			LOG_ERR("Failed to create request worker %d", i);
			return -ENOMEM;
		}

#if CONFIG_THREAD_NAME
		ret = k_thread_name_set(&server->request_workers[i], "mcp_req_worker");
		if (ret != 0) {
			LOG_WRN("Failed to set thread name: %d", ret);
		}
#endif
	}

	tid = k_thread_create(&server->health_monitor_thread, mcp_health_monitor_stack[server->idx],
			      K_THREAD_STACK_SIZEOF(mcp_health_monitor_stack[server->idx]),
			      mcp_health_monitor_worker, server, NULL, NULL,
			      K_PRIO_PREEMPT(MCP_WORKER_PRIORITY - 1), 0, K_NO_WAIT);
	if (tid == NULL) {
		LOG_ERR("Failed to create health monitor thread");
		return -ENOMEM;
	}

#if CONFIG_THREAD_NAME
	ret = k_thread_name_set(&server->health_monitor_thread, "mcp_health_mon");
	if (ret != 0) {
		LOG_WRN("Failed to set health monitor thread name: %d", ret);
	}
#endif

	LOG_INF("MCP server health monitor enabled");
	LOG_INF("MCP Server started: %d request workers", CONFIG_MCP_REQUEST_WORKERS);
	return 0;
}

int mcp_server_submit_tool_message(mcp_server_ctx_t ctx, const struct mcp_tool_message *tool_msg,
				   const char *execution_token)
{
	int ret;
	int canceled_state;
	struct mcp_result_tools_call *response_data = NULL;
	uint8_t *json_buffer = NULL;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;
	struct mcp_execution_context *execution_ctx = NULL;
	struct mcp_client_context *client = NULL;
	struct mcp_tool_record *temp_tool_ptr = NULL;
	struct mcp_execution_registry *execution_registry = NULL;
	uint32_t msg_id = 0;
	bool is_execution_canceled = false;

	ret = validate_tool_message_params(server, tool_msg, execution_token);
	if (ret != 0) {
		return ret;
	}

	execution_registry = &server->execution_registry;

	ret = k_mutex_lock(&execution_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock execution registry: %d", ret);
		return ret;
	}

	execution_ctx = get_execution_context(server, execution_token);
	if (execution_ctx == NULL) {
		LOG_ERR("Execution token not found");
		k_mutex_unlock(&execution_registry->mutex);
		return -ENOENT;
	}

	msg_id = execution_ctx->transport_msg_id;
	client = execution_ctx->client;
	temp_tool_ptr = execution_ctx->tool;
	is_execution_canceled = (execution_ctx->execution_state == MCP_EXEC_CANCELED);

	canceled_state = check_canceled_execution(execution_ctx, tool_msg->type);

	if (canceled_state == MCP_NOT_CANCELED) {
		execution_ctx->last_message_timestamp = k_uptime_get();
	}
	k_mutex_unlock(&execution_registry->mutex);

	if (canceled_state == MCP_CANCELED) {
		return 0;
	}

	if (canceled_state == MCP_CANCELED_WITH_CLEANUP) {
		perform_final_cleanup(server, client, execution_ctx, temp_tool_ptr,
				      msg_id, is_execution_canceled);
		return 0;
	}

	if (tool_msg->type == MCP_USR_TOOL_PING) {
		return 0;
	}

	if (tool_msg->type == MCP_USR_TOOL_RESPONSE) {
		ret = send_tool_response(client, execution_ctx, tool_msg,
					 execution_registry, &response_data, &json_buffer);
		free_response_resources(response_data, json_buffer, ret != 0);

		int cleanup_ret = perform_final_cleanup(server, client, execution_ctx,
						temp_tool_ptr, msg_id, is_execution_canceled);
		if (ret == 0) {
			ret = cleanup_ret;
		}
		return ret;
	}

	if (tool_msg->type == MCP_USR_TOOL_CANCEL_ACK) {
		LOG_ERR("Unsupported application message type: %u", tool_msg->type);
		perform_final_cleanup(server, client, execution_ctx,
				      temp_tool_ptr, msg_id, is_execution_canceled);
		return -EINVAL;
	}

	LOG_ERR("Unsupported application message type: %u", tool_msg->type);
	return -EINVAL;
}

int mcp_server_add_tool(mcp_server_ctx_t ctx, const struct mcp_tool_record *tool_record)
{
	int ret;
	struct mcp_tool_registry *tool_registry = NULL;
	struct mcp_tool_record *tool = NULL;
	struct mcp_tool_record *new_tool = NULL;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;

	if (server == NULL) {
		LOG_ERR("Invalid server context");
		return -EINVAL;
	}

	if ((tool_record == NULL) || (tool_record->metadata.name[0] == '\0') ||
	    (tool_record->callback == NULL)) {
		LOG_ERR("Invalid tool record");
		return -EINVAL;
	}

	tool_registry = &server->tool_registry;

	ret = k_mutex_lock(&tool_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock tool registry: %d", ret);
		return ret;
	}

	tool = get_tool(server, tool_record->metadata.name);

	if (tool != NULL) {
		LOG_ERR("Tool '%s' already exists", tool_record->metadata.name);
		k_mutex_unlock(&tool_registry->mutex);
		return -EEXIST;
	}

	new_tool = add_tool(server, tool_record);

	if (new_tool == NULL) {
		LOG_ERR("Tool registry full");
		k_mutex_unlock(&tool_registry->mutex);
		return -ENOSPC;
	}

	LOG_INF("Tool '%s' registered", tool_record->metadata.name);

	k_mutex_unlock(&tool_registry->mutex);
	return 0;
}

int mcp_server_remove_tool(mcp_server_ctx_t ctx, const char *tool_name)
{
	int ret;
	struct mcp_tool_registry *tool_registry = NULL;
	struct mcp_tool_record *tool = NULL;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;

	if (server == NULL) {
		LOG_ERR("Invalid server context");
		return -EINVAL;
	}

	if ((tool_name == NULL) || (tool_name[0] == '\0')) {
		LOG_ERR("Invalid tool name");
		return -EINVAL;
	}

	tool_registry = &server->tool_registry;

	ret = k_mutex_lock(&tool_registry->mutex, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to lock tool registry: %d", ret);
		return ret;
	}

	tool = get_tool(server, tool_name);

	if (tool == NULL) {
		k_mutex_unlock(&tool_registry->mutex);
		LOG_ERR("Tool '%s' not found", tool_name);
		return -ENOENT;
	}

	if (atomic_get(&tool->refcount) > 1) {
		k_mutex_unlock(&tool_registry->mutex);
		LOG_INF("Requested removal of a currently active tool '%s'", tool_name);
		return -EBUSY;
	}

	memset(tool, 0, sizeof(struct mcp_tool_record));
	tool_registry->tool_count--;
	LOG_INF("Tool '%s' removed", tool_name);

	k_mutex_unlock(&tool_registry->mutex);
	return 0;
}

int mcp_server_is_execution_canceled(mcp_server_ctx_t ctx, const char *execution_token,
				     bool *is_canceled)
{
	struct mcp_execution_registry *execution_registry = NULL;
	struct mcp_execution_context *execution_ctx = NULL;
	struct mcp_server_ctx *server = (struct mcp_server_ctx *)ctx;
	int ret;

	if (server == NULL) {
		LOG_ERR("Invalid server context");
		return -EINVAL;
	}

	if (is_canceled == NULL) {
		LOG_ERR("Invalid is_canceled pointer");
		return -EINVAL;
	}

	execution_registry = &server->execution_registry;

	ret = k_mutex_lock(&execution_registry->mutex, K_FOREVER);

	if (ret != 0) {
		LOG_ERR("Failed to lock execution registry: %d", ret);
		*is_canceled = false;
		return ret;
	}

	execution_ctx = get_execution_context(server, execution_token);

	if (execution_ctx == NULL) {
		LOG_ERR("Execution token not found");
		k_mutex_unlock(&execution_registry->mutex);
		*is_canceled = false;
		return -ENOENT;
	}

	*is_canceled = (execution_ctx->execution_state == MCP_EXEC_CANCELED);

	k_mutex_unlock(&execution_registry->mutex);

	return 0;
}
