/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_CLIENT_TYPES_H_
#define _HTTP_CLIENT_TYPES_H_

#include <net/http_parser.h>
#include "tcp_client.h"
#include "config.h"

struct http_client_ctx {
	struct http_parser parser;
	struct http_parser_settings settings;

	struct tcp_client_ctx tcp_ctx;

	uint32_t content_length;
	uint32_t processed;

	/* https://tools.ietf.org/html/rfc7230#section-3.1.2
	 * The status-code element is a 3-digit integer code
	 *
	 * The reason-phrase element exists for the sole purpose of
	 * providing a textual description associated with the
	 * numeric status code. A client SHOULD ignore the
	 * reason-phrase content.
	 */
	char http_status[HTTP_STATUS_STR_SIZE];

	uint8_t cl_present:1;
	uint8_t body_found:1;
};

#endif
