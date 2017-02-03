/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_WRITE_UTILS_H
#define _HTTP_WRITE_UTILS_H

#include "http_types.h"

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"\r\n"

#define HTTP_STATUS_400_BR	"HTTP/1.1 400 Bad Request\r\n" \
				"\r\n"

#define HTTP_STATUS_403_FBD	"HTTP/1.1 403 Forbidden\r\n" \
				"\r\n"

#define HTTP_STATUS_404_NF	"HTTP/1.1 404 Not Found\r\n" \
				"\r\n"

#define HTML_STR_HEADER		"<html><head>\r\n" \
				"<title>Zephyr HTTP Server</title>\r\n" \
				"</head>\r\n"

#define HTML_STR_FOOTER		"</html>\r\n"

uint16_t http_strlen(const char *str);

/* Adds the HTTP header to the tx buffer */
int http_add_header(struct net_buf *tx, const char *str);

/* Writes a generic data chunk to the tx buffer, could be HTML */
int http_add_chunk(struct net_buf *tx, const char *str);

/* Writes an HTTP response with an HTML structure to the client */
int http_write(struct http_server_ctx *ctx, const char *http_header,
	       const char *html_header, const char *html_body,
	       const char *html_footer);

/* Writes the received HTTP header fields to the client */
int http_write_header_fields(struct http_server_ctx *ctx);

/* Writes an elementary It Works! HTML web page to the client */
int http_write_it_works(struct http_server_ctx *ctx);

/* Writes a 400 Bad Request HTTP status code to the client */
int http_write_400_bad_request(struct http_server_ctx *ctx);

/* Writes a 403 forbidden HTTP status code to the client */
int http_write_403_forbidden(struct http_server_ctx *ctx);

/* Writes a 404 Not Found HTTP status code to the client */
int http_write_404_not_found(struct http_server_ctx *ctx);

/* Writes a 200 OK HTTP status code with a 404 Not Found HTML msg */
int http_write_soft_404_not_found(struct http_server_ctx *ctx);

#endif
