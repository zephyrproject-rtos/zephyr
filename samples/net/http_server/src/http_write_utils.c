/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_write_utils.h"
#include "http_types.h"
#include "http_utils.h"
#include "config.h"

#include <stdio.h>

int http_response_401(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_401_STATUS_US, NULL);
}

int http_response_auth(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK,
			     HTML_HEADER"<h2><center>user: "HTTP_AUTH_USERNAME
			     ", password: "HTTP_AUTH_PASSWORD"</center></h2>"
			     HTML_FOOTER);
}
