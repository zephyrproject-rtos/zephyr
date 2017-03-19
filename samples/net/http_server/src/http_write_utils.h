/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_WRITE_UTILS_H
#define _HTTP_WRITE_UTILS_H

#include "http_types.h"
#include <net/http.h>

/* Writes the received HTTP header fields to the client */
int http_response_header_fields(struct http_server_ctx *ctx);

/* Writes an elementary It Works! HTML web page to the client */
int http_response_it_works(struct http_server_ctx *ctx);

int http_response_401(struct http_server_ctx *ctx);

/* Writes a 200 OK HTTP status code with a 404 Not Found HTML msg */
int http_response_soft_404(struct http_server_ctx *ctx);

int http_response_auth(struct http_server_ctx *ctx);

#endif
