/*
 * Copyright (c) 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef REST_API_H
#define REST_API_H

#include <zephyr/net/http/server.h>

/* REST API initialization */
int rest_api_init(void);

/* REST API handlers */
int rest_api_status_handler(struct http_client_ctx *client, enum http_data_status status,
                           const struct http_request_ctx *request_ctx,
                           struct http_response_ctx *response_ctx, void *user_data);

int rest_api_thread_start_handler(struct http_client_ctx *client, enum http_data_status status,
                                 const struct http_request_ctx *request_ctx,
                                 struct http_response_ctx *response_ctx, void *user_data);

int rest_api_thread_stop_handler(struct http_client_ctx *client, enum http_data_status status,
                                const struct http_request_ctx *request_ctx,
                                struct http_response_ctx *response_ctx, void *user_data);

#endif /* REST_API_H */
