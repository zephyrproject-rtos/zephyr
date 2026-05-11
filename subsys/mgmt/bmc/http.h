/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __HTTP_H__
#define __HTTP_H__

#ifdef CONFIG_HTTP_SERVER
int app_http_server_init(void);
struct http_request_ctx;
int ws_validate_auth(int ws_socket, struct http_request_ctx *request_ctx, void *user_data);
#else
struct http_request_ctx;
static inline int app_http_server_init(void) { return 0; }
static inline int ws_validate_auth(int ws_socket, struct http_request_ctx *request_ctx, void *user_data) { return -EACCES; }
#endif

#endif
