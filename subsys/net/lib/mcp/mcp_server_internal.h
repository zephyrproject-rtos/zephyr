/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_MCP_SERVER_INTERNAL_H_
#define ZEPHYR_SUBSYS_MCP_SERVER_INTERNAL_H_

/**
 * @file
 * @brief Model Context Protocol (MCP) Server Internal API
 */

#include <zephyr/kernel.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <zephyr/sys/uuid.h>
#include "mcp_common.h"
#include "mcp_json.h"

/* Forward declaration */
struct mcp_transport_binding;
struct mcp_transport_message;

/**
 * @brief Transport operations structure for MCP server communication.
 */
struct mcp_transport_ops {
	/**
	 * @brief Send MCP response data to a client via transport
	 *
	 * @note This function queues response data for delivery to a client.
	 *
	 *		 Data Ownership and Memory Management:
	 *		 -------------------------------------
	 *		 INPUT (data parameter):
	 *		 - Caller (MCP core) allocates the response data buffer
	 *		 - This function takes OWNERSHIP of the data pointer
	 *		 - Data is NOT copied - the pointer is stored directly in the response item
	 *		 - Caller must NOT free the data after calling this function
	 *
	 * @param msg Transport message data
	 *
	 * @return 0 on success, negative error code on failure.
	 *
	 * @note The data pointer MUST remain valid until freed by the transport
	 */
	int (*send)(struct mcp_transport_message *msg);

	/**
	 * @brief Disconnect a client
	 *
	 * @note Disconnects a client from the transport and cleans up associated resources.
	 *
	 *		 IMPORTANT: The transport implementation MUST drain and free any queued
	 *		 response data that has not yet been sent to the client. This includes:
	 *		 - Response items in any FIFO/message queues
	 *		 - The actual response data buffers (allocated by MCP core)
	 *		 - Any other dynamically allocated resources associated with the client
	 *
	 *		 Failure to properly free queued data will result in memory leaks.
	 *
	 * @param binding Client transport binding
	 * @return 0 on success, negative errno on failure
	 */
	int (*disconnect)(struct mcp_transport_binding *binding);
};

/**
 * @brief MCP endpoint structure for managing server communication
 * @details Contains transport operations and endpoint-specific context
 * for handling client connections and message delivery.
 */
struct mcp_transport_binding {
	const struct mcp_transport_ops *ops;
	void *context;
};

/**
 * @brief Request data structure for submitting requests to the MCP server
 *
 * This structure encapsulates all information needed to process an incoming
 * request.
 */
struct mcp_transport_message {
	char *json_data;
	size_t json_len;
	uint32_t msg_id;
	struct mcp_transport_binding *binding;
	char protocol_version[MCP_MAX_PROTO_VER_LEN];
};

/**
 * @brief Handle an incoming MCP request from a client
 *
 * @note This function is the main entry point for processing MCP protocol requests.
 *		 It parses the incoming JSON request, determines the method type, and routes
 *		 the request to the appropriate handler or queues it for asynchronous processing.
 *
 *		 Request handling flow:
 *		 1. Parse JSON request into MCP message structure
 *		 2. Determine the method type (initialize, ping, tools_list, etc.)
 *		 3. Route based on method:
 *		 - INITIALIZE: Handle directly and create new client context
 *		 - PING: Handle directly with immediate response
 *		 - TOOLS_LIST/TOOLS_CALL/NOTIF_*: Queue for async processing
 *		 - UNKNOWN: Send error response for unsupported methods
 *
 * @param ctx MCP server context handle
 * @param request Request data containing JSON payload and client hint
 * @param method Output parameter for the detected method type
 *
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid parameters (NULL pointers)
 *         -ENOMEM: Failed to allocate memory for message parsing
 *         -ENOENT: Client not found for given client_id_hint
 *         -ENOTSUP: Request method not recognized/supported
 *         Other negative values from parsing or handler functions
 *
 * @note The parsed message is either freed immediately (for direct handlers)
 *       or ownership is transferred to the request queue (for async handlers)
 * @note The client_binding output is only valid if a client context was found
 */
int mcp_server_handle_request(mcp_server_ctx_t ctx, struct mcp_transport_message *request,
			      enum mcp_method *method);
/**
 * @brief Update the last activity timestamp for a client
 */
int mcp_server_update_client_timestamp(mcp_server_ctx_t ctx, struct mcp_transport_binding *binding);

#endif /* ZEPHYR_SUBSYS_MCP_SERVER_INTERNAL_H_ */
