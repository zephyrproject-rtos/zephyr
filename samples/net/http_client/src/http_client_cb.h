/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_CLIENT_CB_H_
#define _HTTP_CLIENT_CB_H_

#include <net/http_parser.h>

/*
 * This are the callbacks executed by the parser. Some of them
 * are only useful when parsing requests (or responses).
 * Unused callbacks may be removed.
 */

int on_url(struct http_parser *parser, const char *at, size_t length);

int on_status(struct http_parser *parser, const char *at, size_t length);

int on_header_field(struct http_parser *parser, const char *at, size_t length);

int on_header_value(struct http_parser *parser, const char *at, size_t length);

int on_body(struct http_parser *parser, const char *at, size_t length);

int on_headers_complete(struct http_parser *parser);

int on_message_begin(struct http_parser *parser);

int on_message_complete(struct http_parser *parser);

int on_chunk_header(struct http_parser *parser);

int on_chunk_complete(struct http_parser *parser);

#endif
