/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SUBSYS_MCP_JSON_H_
#define ZEPHYR_SUBSYS_MCP_JSON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define JSON_RPC_VERSION       "2.0"
#define MCP_MAX_NAME_LEN       64  /* tool names, client/server names */
#define MCP_MAX_DESC_LEN       128 /* log messages, reasons, etc. */
#define MCP_MAX_TEXT_LEN       256 /* text content block */
#define MCP_MAX_PROTO_VER_LEN  32  /* "2025-11-25"  */
#define MCP_MAX_VERSION_LEN    32
#define MCP_MAX_JSON_CHUNK_LEN 1024 /* for small opaque JSON blobs */
#define MCP_MAX_CONTENT_ITEMS  2
#define MCP_MAX_ID_LEN         64 /* for string IDs */

/* JSON-RPC message kind (incoming) */
enum mcp_msg_kind {
	MCP_MSG_INVALID = 0,
	MCP_MSG_REQUEST,
	MCP_MSG_NOTIFICATION,
};

/* Method identifiers */
enum mcp_method {
	MCP_METHOD_UNKNOWN = 0,
	MCP_METHOD_INITIALIZE,
	MCP_METHOD_PING,
	MCP_METHOD_TOOLS_LIST,
	MCP_METHOD_TOOLS_CALL,
	MCP_METHOD_NOTIF_INITIALIZED,
	MCP_METHOD_NOTIF_CANCELLED,
};

/* JSON-RPC error codes */
enum mcp_error_code {
	MCP_ERR_PARSE_ERROR = -32700,
	MCP_ERR_INVALID_REQUEST = -32600,
	MCP_ERR_METHOD_NOT_FOUND = -32601,
	MCP_ERR_INVALID_PARAMS = -32602,
	MCP_ERR_INTERNAL_ERROR = -32603,
	MCP_ERR_SERVER_GENERIC = -32000,
	MCP_ERR_CANCELLED = -32001,
	MCP_ERR_BUSY = -32002,
	MCP_ERR_NOT_INITIALIZED = -32003,
};

/* Request ID stored as string
 * - For integer IDs: stored without quotes
 * - For string IDs: stored with quotes
 * - This preserves the original type for serialization
 */
struct mcp_request_id {
	char string[MCP_MAX_ID_LEN];
};

/* Generic JSON-RPC error object (outgoing) */
struct mcp_error {
	int32_t code;
	char message[MCP_MAX_DESC_LEN];
	char data_json[MCP_MAX_JSON_CHUNK_LEN]; /* optional; empty if !has_data */
	bool has_data;
};

/* Content type (for tool results) */
enum mcp_content_type {
	MCP_CONTENT_TEXT = 0, /* extend later with more types if needed */
};

struct mcp_content {
	enum mcp_content_type type;
	char text[MCP_MAX_TEXT_LEN]; /* if type == TEXT */
};

struct mcp_content_list {
	uint8_t count;
	struct mcp_content items[MCP_MAX_CONTENT_ITEMS];
};

struct mcp_params_initialize {
	/* Capabilities/clientInfo not used for now */
	bool dummy;
};

struct mcp_result_initialize {
	char protocol_version[MCP_MAX_PROTO_VER_LEN];
	char server_name[MCP_MAX_NAME_LEN];
	char server_version[MCP_MAX_VERSION_LEN];
	/* Optional server capabilities */
	char capabilities_json[MCP_MAX_JSON_CHUNK_LEN];
	bool has_capabilities;
};

/* --- ping --- */
struct mcp_params_ping {
	/* Optional payload JSON */
	char payload_json[MCP_MAX_JSON_CHUNK_LEN];
	bool has_payload;
};

struct mcp_result_ping {
	char payload_json[MCP_MAX_JSON_CHUNK_LEN];
	bool has_payload;
};

struct mcp_params_tools_list {
	/* Reserved for future filters; usually empty. */
	char filter_json[MCP_MAX_JSON_CHUNK_LEN];
	bool has_filter;
};

struct mcp_result_tools_list {
	/* Raw JSON that the user generates for now */
	char tools_json[MCP_MAX_JSON_CHUNK_LEN];
};

struct mcp_params_tools_call {
	char name[MCP_MAX_NAME_LEN];
	char arguments_json[MCP_MAX_JSON_CHUNK_LEN]; /* full JSON of "arguments" object */
	bool has_arguments;
};

struct mcp_result_tools_call {
	struct mcp_content_list content; /* one or more content blocks */
	bool is_error;
};

struct mcp_params_notif_cancelled {
	struct mcp_request_id request_id;
	char reason[MCP_MAX_DESC_LEN];
	bool has_reason;
};

struct mcp_message {
	enum mcp_msg_kind kind;
	struct mcp_request_id id;
	enum mcp_method method;
	char protocol_version[MCP_MAX_PROTO_VER_LEN];
	union {
		/* For requests */
		struct {
			union {
				struct mcp_params_initialize initialize;
				struct mcp_params_ping ping;
				struct mcp_params_tools_list tools_list;
				struct mcp_params_tools_call tools_call;
			};
		} req;
		/* For notifications */
		struct {
			union {
				struct mcp_params_notif_cancelled cancelled;
			};
		} notif;
	};
};

/*******************************************************************************
 * Public API – parser
 ******************************************************************************/
/**
 * @brief Parse a single MCP JSON message into an mcp_message structure.
 *
 * Designed for *server-side* messages:
 *   - Requests: initialize, ping, tools/list, tools/call
 *   - Notifications: notifications/initialized, notifications/cancelled
 *
 * @param buf JSON buffer
 *            NOTE: The json library modifies the buffer in place.
 * @param len Length of JSON in buf.
 * @param out Output message structure (must be non-NULL).
 *
 * @return 0 on success, -EINVAL on parse/validation error.
 */
int mcp_json_parse_message(char *buf, size_t len, struct mcp_message *out);

/*******************************************************************************
 * Public API – serializers
 ******************************************************************************/
/**
 * @brief Serialize a successful initialize response.
 *
 * Generates a JSON-RPC response message:
 * {
 *   "jsonrpc":"2.0",
 *   "id":<id>,
 *   "result":{
 *     "protocolVersion":"...",
 *     "serverInfo":{"name":"...","version":"..."},
 *     "capabilities":{...}
 *   }
 * }
 *
 * @param out Output buffer for the serialized JSON string.
 * @param out_len Size of the output buffer.
 * @param id JSON-RPC request ID
 * @param res Pointer to the initialize result structure containing response data.
 *
 * @return Number of bytes written (excluding NUL terminator) on success,
 *         negative error code on failure.
 */
int mcp_json_serialize_initialize_result(char *out, size_t out_len, const struct mcp_request_id *id,
					 const struct mcp_result_initialize *res);

/**
 * @brief Serialize a successful ping response.
 *
 * Generates a JSON-RPC response message:
 * {
 *   "jsonrpc":"2.0",
 *   "id":<id>,
 *   "result":{}
 * }
 *
 * @param out Output buffer for the serialized JSON string.
 * @param out_len Size of the output buffer.
 * @param id JSON-RPC request ID
 * @param res Pointer to the ping result structure (may contain optional payload).
 *
 * @return Number of bytes written (excluding NUL terminator) on success,
 *         negative error code on failure.
 */
int mcp_json_serialize_ping_result(char *out, size_t out_len, const struct mcp_request_id *id,
				   const struct mcp_result_ping *res);

/**
 * @brief Serialize a tools/list response.
 *
 * Generates a JSON-RPC response message:
 * {
 *   "jsonrpc":"2.0",
 *   "id":<id>,
 *   "result":{"tools":[ ... ]}
 * }
 *
 * @param out Output buffer for the serialized JSON string.
 * @param out_len Size of the output buffer.
 * @param id JSON-RPC request ID
 * @param res Pointer to the tools list result structure. The tools_json field
 *            must contain a valid JSON array, e.g.:
 *            [{"name":"foo",...}, {"name":"bar",...}]
 *
 * @return Number of bytes written (excluding NUL terminator) on success,
 *         negative error code on failure.
 */
int mcp_json_serialize_tools_list_result(char *out, size_t out_len, const struct mcp_request_id *id,
					 const struct mcp_result_tools_list *res);

/**
 * @brief Serialize a tools/call response.
 *
 * Generates a JSON-RPC response message:
 * {
 *   "jsonrpc":"2.0",
 *   "id":<id>,
 *   "result":{
 *     "content":[{"type":"text","text":"..."}]
 *   }
 * }
 *
 * @param out Output buffer for the serialized JSON string.
 * @param out_len Size of the output buffer.
 * @param id JSON-RPC request ID
 * @param res Pointer to the tools call result structure containing content blocks.
 *            Only content.type == text is supported for now.
 *
 * @return Number of bytes written (excluding NUL terminator) on success,
 *         negative error code on failure.
 */
int mcp_json_serialize_tools_call_result(char *out, size_t out_len, const struct mcp_request_id *id,
					 const struct mcp_result_tools_call *res);

/**
 * @brief Serialize a JSON-RPC error response.
 *
 * Generates a JSON-RPC error message:
 * {
 *   "jsonrpc":"2.0",
 *   "id":<id> or null,
 *   "error":{"code":X,"message":"...","data":...}
 * }
 *
 * @param out Output buffer for the serialized JSON string.
 * @param out_len Size of the output buffer.
 * @param id Pointer to request ID (NULL or empty string for id:null).
 * @param err Pointer to the error structure containing code, message, and optional data.
 *
 * @return Number of bytes written (excluding NUL terminator) on success,
 *         negative error code on failure.
 */
int mcp_json_serialize_error(char *out, size_t out_len, const struct mcp_request_id *id,
			     const struct mcp_error *err);

/**
 * @brief Serialize a notifications/cancelled notification.
 *
 * Generates a JSON-RPC notification message:
 * {
 *   "jsonrpc":"2.0",
 *   "method":"notifications/cancelled",
 *   "params":{
 *     "requestId":<id>,
 *     "reason":"..."
 *   }
 * }
 *
 * @param out Output buffer for the serialized JSON string.
 * @param out_len Size of the output buffer.
 * @param params Pointer to the cancelled notification parameters containing
 *               requestId and optional reason.
 *
 * @return Number of bytes written (excluding NUL terminator) on success,
 *         negative error code on failure.
 */
int mcp_json_serialize_cancel_notification(char *out, size_t out_len,
					   const struct mcp_params_notif_cancelled *params);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_MCP_JSON_H_ */
