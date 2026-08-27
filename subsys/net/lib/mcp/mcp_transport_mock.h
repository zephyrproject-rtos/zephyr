/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_MCP_TRANSPORT_MOCK_H_
#define ZEPHYR_INCLUDE_NET_MCP_TRANSPORT_MOCK_H_

/**
 * @file
 * @brief Model Context Protocol (MCP) Server MOCK Transport API for unit tests
 */

#ifdef CONFIG_MCP_TRANSPORT_MOCK

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Mock transport API for MCP testing
 * @defgroup mcp_transport_mock MCP Mock Transport
 * @ingroup mcp
 * @{
 */

struct mcp_transport_binding;

/**
 * @brief Allocate a new mock client binding
 *
 * Creates a new mock client context with an initialized transport binding.
 * The binding is configured with mock transport operations.
 *
 * @return Pointer to transport binding on success, NULL if no slots available
 */
struct mcp_transport_binding *mcp_transport_mock_allocate_client(void);

/**
 * @brief Release a mock client binding
 *
 * Frees the client context and marks it as inactive.
 *
 * @param binding Transport binding to release
 */
void mcp_transport_mock_release_client(struct mcp_transport_binding *binding);

/**
 * @brief Reset mock transport state
 *
 * Clears all tracked calls, messages, and client state.
 * Should be called between tests.
 */
void mcp_transport_mock_reset(void);

/**
 * @brief Inject error for next send operation
 *
 * @param error Error code to return (0 to disable injection)
 */
void mcp_transport_mock_inject_send_error(int error);

/**
 * @brief Inject error for next disconnect operation
 *
 * @param error Error code to return (0 to disable injection)
 */
void mcp_transport_mock_inject_disconnect_error(int error);

/**
 * @brief Get number of send calls made
 *
 * @return Number of times send was called
 */
int mcp_transport_mock_get_send_count(void);

/**
 * @brief Reset number of send calls made
 */
void mcp_transport_mock_reset_send_count(void);

/**
 * @brief Get number of disconnect calls made
 *
 * @return Number of times disconnect was called
 */
int mcp_transport_mock_get_disconnect_count(void);

/**
 * @brief Get the last message sent to a client
 *
 * @param binding Transport binding to query
 * @param length Pointer to store message length (can be NULL)
 * @return Pointer to message buffer, or NULL if client not found
 */
const char *mcp_transport_mock_get_last_message(struct mcp_transport_binding *binding,
						size_t *length);

/**
 * @brief Get the last message ID sent to a client
 *
 * @param binding Transport binding to query
 * @return Last message ID, or 0 if client not found
 */
uint32_t mcp_transport_mock_get_last_msg_id(struct mcp_transport_binding *binding);

/**
 * @}
 */

#endif /* CONFIG_MCP_TRANSPORT_MOCK */

#endif /* ZEPHYR_INCLUDE_NET_MCP_TRANSPORT_MOCK_H_ */
