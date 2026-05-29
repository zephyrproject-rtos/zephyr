/*
 * Copyright (c) 2024, Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/http/server.h>

/**
 * @brief Setup websocket for echoing data back to client
 *
 * @param ws_socket Socket file descriptor associated with websocket
 * @param request_ctx Request context associated with websocket HTTP upgrade request
 * @param user_data User data pointer
 *
 * @return 0 on success
 */
int ws_echo_setup(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);

/**
 * @brief Setup websocket for sending net statistics to client
 *
 * @param ws_socket Socket file descriptor associated with websocket
 * @param request_ctx Request context associated with websocket HTTP upgrade request
 * @param user_data User data pointer
 *
 * @return 0 on success
 */
int ws_netstats_setup(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);
