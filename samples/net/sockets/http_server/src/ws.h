/*
 * Copyright (c) 2024, Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Setup websocket for echoing data back to client
 *
 * @param ws_socket Socket file descriptor associated with websocket
 * @param user_data User data pointer
 *
 * @return 0 on success
 */
int ws_echo_setup(int ws_socket, void *user_data);

/**
 * @brief Setup websocket for sending net statistics to client
 *
 * @param ws_socket Socket file descriptor associated with websocket
 * @param user_data User data pointer
 *
 * @return 0 on success
 */
int ws_netstats_setup(int ws_socket, void *user_data);
