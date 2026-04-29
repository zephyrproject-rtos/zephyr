/**
 * @defgroup mcp_server MCP Server API
 * @since 4.5
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_MCP_SERVER_H_
#define ZEPHYR_INCLUDE_NET_MCP_SERVER_H_

/**
 * @file
 * @brief Model Context Protocol (MCP) Server API
 */

#include <zephyr/kernel.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCP Tool message type
 */
enum mcp_tool_msg_type {
	MCP_USR_TOOL_RESPONSE,		/**< JSON-RPC response to a client request */
	MCP_USR_TOOL_NOTIFICATION,	/**< Tool to client notification */
	MCP_USR_TOOL_CANCEL_ACK,	/**< Cancellation acknowledgment to the MCP Core */
	MCP_USR_TOOL_PING		/**< Ping to signify the tool is alive to the MCP Core */
};

/**
 * @brief MCP Tool event type
 */
enum mcp_tool_event_type {
	MCP_TOOL_CALL_REQUEST,	/**< JSON-RPC request calling the tool */
	MCP_TOOL_CANCEL_REQUEST	/**< Tool cancellation request */
};

/**
 * @brief Tool metadata structure
 */
struct mcp_tool_metadata {
	char name[CONFIG_MCP_TOOL_NAME_MAX_LEN];		/**< Tool name */
	char input_schema[CONFIG_MCP_TOOL_SCHEMA_MAX_LEN];	/**< Tool input schema (JSON) */
#ifdef CONFIG_MCP_TOOL_DESC
	char description[CONFIG_MCP_TOOL_DESC_MAX_LEN];		/**< Tool description */
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
	char title[CONFIG_MCP_TOOL_NAME_MAX_LEN];		/**< Tool title */
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
	char output_schema[CONFIG_MCP_TOOL_SCHEMA_MAX_LEN];	/**< Tool output schema (JSON) */
#endif
};

/**
 * @brief Server context handle
 */
typedef void *mcp_server_ctx_t;

/**
 * @brief Tool callback function
 *
 * @param event Type of tool event (call or cancel request)
 * @param arguments JSON string with tool parameters
 * @param execution_token Unique execution identifier (UUID string)
 *
 * @note This callback is executed in the context of an MCP request worker thread.
 *       The default worker thread stack size is CONFIG_MCP_REQUEST_WORKER_STACK_SIZE.
 *       If your tool callback requires more stack space increase this value.
 *
 *       Consider using heap allocation for large buffers to minimize stack usage.
 *
 *       This callback blocks an MCP server worker thread. For long-running operations,
 *       it is recommended to use your own thread pool to execute the work asynchronously
 *       and return quickly from this callback to avoid blocking the MCP server worker
 *       thread and potentially degrading server responsiveness.
 *
 *		 It is up to the user application whether it allows concurrent executions
 *		 of the same tool or not.
 *
 * @return 0 on success, negative errno on failure
 */
typedef int (*mcp_tool_callback_t)(enum mcp_tool_event_type event, const char *arguments,
				   const char *execution_token);

/**
 * @brief Tool definition structure
 *
 * @note refcount is used internally by the MCP server to track tool execution state
 * and protect against the removal of a tool while it is actively executing.
 *
 */
struct mcp_tool_record {
	struct mcp_tool_metadata metadata;	/**< Tool metadata */
	atomic_t refcount;					/**< Internal refcount */
	mcp_tool_callback_t callback;		/**< Tool callback (tools/call) */
};

/**
 * @brief Message sent from a tool (response/notification)
 *
 * This structure is used to submit tool execution results back to the MCP server.
 *
 * @note For tool responses (MCP_USR_TOOL_RESPONSE):
 *       - data should point to a JSON string containing the tool result
 *       - The result should be a list of content items, each with "type" and "text" fields
 *         without the square brackets wrapper (the server adds those).
 *       - Example: "{\"type\":\"text\",\"text\":\"Tool execution result\"},
 *                   {\"type\":\"text\",\"text":"More results\"}"
 *       - is_error should be set to true if the tool execution failed
 *
 */
struct mcp_tool_message {
	enum mcp_tool_msg_type type;	/**< Type of message */
	int length;						/**< Message length */
	void *data;						/**< Message data pointer */
	bool is_error;					/**< Signifies if message is error */
};

/**
 * @brief Initialize the MCP Server
 *
 * @return 0 on success, negative errno on failure
 */
mcp_server_ctx_t mcp_server_init(void);

/**
 * @brief Start the MCP Server
 *
 * @param server_ctx Server context handle
 * @return 0 on success, negative errno on failure
 */
int mcp_server_start(mcp_server_ctx_t server_ctx);

/**
 * @brief Submit a message from a tool (response/notification)
 *
 * @param server_ctx Server context handle
 * @param user_msg Application message to submit
 * @param execution_token Execution token for tracking (UUID string)
 * @return 0 on success, negative errno on failure
 */
int mcp_server_submit_tool_message(mcp_server_ctx_t server_ctx,
				   const struct mcp_tool_message *user_msg,
				   const char *execution_token);

/**
 * @brief Add a tool to the server
 *
 * @param server_ctx Server context handle
 * @param tool_record Tool definition with metadata and callback
 * @return 0 on success, negative errno on failure
 * @retval -EINVAL Invalid tool_record
 * @retval -EEXIST Tool name already exists
 * @retval -ENOSPC Registry full
 */
int mcp_server_add_tool(mcp_server_ctx_t server_ctx, const struct mcp_tool_record *tool_record);

/**
 * @brief Remove a tool from the server
 *
 * @note Should be called again if it returns -EBUSY, which signifies that the
 * tool is currently being executed.
 *
 * @param server_ctx Server context handle
 * @param tool_name Name of tool to remove
 * @return 0 on success, negative errno on failure
 * @retval -EINVAL Invalid tool name
 * @retval -ENOENT Tool not found
 * @retval -EBUSY Tool is being executed and can't be removed (try again later)
 */
int mcp_server_remove_tool(mcp_server_ctx_t server_ctx, const char *tool_name);

/**
 * @brief Helper for checking the execution state of a tool
 *
 * @note Alternative way of checking the cancellation. Primary check should be through the
 * tool call event (struct mcp_tool_event_type). This checker needs to use mutexes
 * and linear search to locate the required execution context.
 *
 * @param server_ctx Server context handle
 * @param execution_token Token representing the execution (UUID string)
 * @param is_canceled Pointer to store cancellation state
 * @return 0 on success, negative errno on failure
 * @retval -EINVAL Invalid tool name
 * @retval -ENOENT Tool not found
 */
int mcp_server_is_execution_canceled(mcp_server_ctx_t server_ctx, const char *execution_token,
				     bool *is_canceled);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MCP_SERVER_H_ */

/**
 * @}
 */
