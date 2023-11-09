/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>

LOG_MODULE_DECLARE(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#include "headers/server_internal.h"

#define TEMP_BUF_LEN 64

static const char final_chunk[] = "0\r\n\r\n";
static const char *crlf = &final_chunk[3];

static int handle_http1_static_resource(
	struct http_resource_detail_static *static_detail,
	struct http_client_ctx *client)
{
#define RESPONSE_TEMPLATE			\
	"HTTP/1.1 200 OK\r\n"			\
	"Content-Type: text/html\r\n"		\
	"Content-Length: %d\r\n"

	/* Add couple of bytes to total response */
	char http_response[sizeof(RESPONSE_TEMPLATE) +
			   sizeof("Content-Encoding: 01234567890123456789\r\n") +
			   sizeof("xxxx") +
			   sizeof("\r\n")];
	const char *data;
	int len;
	int ret;

	if (static_detail->common.bitmask_of_supported_http_methods & BIT(HTTP_GET)) {
		data = static_detail->static_data;
		len = static_detail->static_data_len;

		if (static_detail->common.content_encoding != NULL &&
		    static_detail->common.content_encoding[0] != '\0') {
			snprintk(http_response, sizeof(http_response),
				 RESPONSE_TEMPLATE "Content-Encoding: %s\r\n\r\n",
				 len, static_detail->common.content_encoding);
		} else {
			snprintk(http_response, sizeof(http_response),
				 RESPONSE_TEMPLATE "\r\n", len);
		}

		ret = http_server_sendall(client, http_response,
					  strlen(http_response));
		if (ret < 0) {
			return ret;
		}

		ret = http_server_sendall(client, data, len);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

#define RESPONSE_TEMPLATE_CHUNKED			\
	"HTTP/1.1 200 OK\r\n"				\
	"Content-Type: text/html\r\n"			\
	"Transfer-Encoding: chunked\r\n\r\n"

#define RESPONSE_TEMPLATE_DYNAMIC			\
	"HTTP/1.1 200 OK\r\n"				\
	"Content-Type: text/html\r\n\r\n"		\

static int dynamic_get_req(struct http_resource_detail_dynamic *dynamic_detail,
			   struct http_client_ctx *client)
{
	/* offset tells from where the GET params start */
	int ret, remaining, offset = dynamic_detail->common.path_len;
	char *ptr;
	char tmp[TEMP_BUF_LEN];

	ret = http_server_sendall(client, RESPONSE_TEMPLATE_CHUNKED,
				  sizeof(RESPONSE_TEMPLATE_CHUNKED) - 1);
	if (ret < 0) {
		return ret;
	}

	remaining = strlen(&client->url_buffer[dynamic_detail->common.path_len]);

	/* Pass URL to the client */
	while (1) {
		int copy_len, send_len;
		enum http_data_status status;

		ptr = &client->url_buffer[offset];
		copy_len = MIN(remaining, dynamic_detail->data_buffer_len);

		memcpy(dynamic_detail->data_buffer, ptr, copy_len);

		if (copy_len == remaining) {
			status = HTTP_SERVER_DATA_FINAL;
		} else {
			status = HTTP_SERVER_DATA_MORE;
		}

		send_len = dynamic_detail->cb(client, status,
					      dynamic_detail->data_buffer,
					      copy_len, dynamic_detail->user_data);
		if (send_len > 0) {
			ret = snprintk(tmp, sizeof(tmp), "%x\r\n", send_len);
			ret = http_server_sendall(client, tmp, ret);
			if (ret < 0) {
				return ret;
			}

			ret = http_server_sendall(client,
						  dynamic_detail->data_buffer,
						  send_len);
			if (ret < 0) {
				return ret;
			}

			(void)http_server_sendall(client, crlf, 2);

			offset += copy_len;
			remaining -= copy_len;

			continue;
		}

		break;
	}

	dynamic_detail->holder = NULL;

	ret = http_server_sendall(client, final_chunk,
				  sizeof(final_chunk) - 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int dynamic_post_req(struct http_resource_detail_dynamic *dynamic_detail,
			    struct http_client_ctx *client)
{
	/* offset tells from where the POST params start */
	char *start = client->cursor;
	int ret, remaining = client->data_len, offset = 0;
	int copy_len;
	char *ptr;
	char tmp[TEMP_BUF_LEN];

	if (start == NULL) {
		return -ENOENT;
	}

	if (!client->headers_sent) {
		ret = http_server_sendall(client, RESPONSE_TEMPLATE_CHUNKED,
			sizeof(RESPONSE_TEMPLATE_CHUNKED) - 1);
		if (ret < 0) {
			return ret;
		}
		client->headers_sent = true;
	}

	copy_len = MIN(remaining, dynamic_detail->data_buffer_len);
	while (copy_len > 0) {
		enum http_data_status status;
		int send_len;

		ptr = &start[offset];

		memcpy(dynamic_detail->data_buffer, ptr, copy_len);

		if (copy_len == remaining &&
		    client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
			status = HTTP_SERVER_DATA_FINAL;
		} else {
			status = HTTP_SERVER_DATA_MORE;
		}

		send_len = dynamic_detail->cb(client, status,
					      dynamic_detail->data_buffer,
					      copy_len, dynamic_detail->user_data);
		if (send_len > 0) {
			ret = snprintk(tmp, sizeof(tmp), "%x\r\n", send_len);
			ret = http_server_sendall(client, tmp, ret);
			if (ret < 0) {
				return ret;
			}

			ret = http_server_sendall(client,
						  dynamic_detail->data_buffer,
						  send_len);
			if (ret < 0) {
				return ret;
			}

			(void)http_server_sendall(client, crlf, 2);

			offset += copy_len;
			remaining -= copy_len;
		}

		copy_len = MIN(remaining, dynamic_detail->data_buffer_len);
	}

	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		ret = http_server_sendall(client, final_chunk,
					sizeof(final_chunk) - 1);
		if (ret < 0) {
			return ret;
		}

		dynamic_detail->holder = NULL;
	}

	return 0;
}

static int handle_http1_dynamic_resource(
	struct http_resource_detail_dynamic *dynamic_detail,
	struct http_client_ctx *client)
{
	uint32_t user_method;
	int ret;

	if (dynamic_detail->cb == NULL) {
		return -ESRCH;
	}

	user_method = dynamic_detail->common.bitmask_of_supported_http_methods;

	if (!(BIT(client->method) & user_method)) {
		return -ENOPROTOOPT;
	}

	if (dynamic_detail->holder != NULL && dynamic_detail->holder != client) {
		static const char conflict_response[] =
				"HTTP/1.1 409 Conflict\r\n\r\n";

		ret = http_server_sendall(client, conflict_response,
					  sizeof(conflict_response) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}

		return enter_http_done_state(client);
	}

	dynamic_detail->holder = client;

	switch (client->method) {
	case HTTP_HEAD:
		if (user_method & BIT(HTTP_HEAD)) {
			ret = http_server_sendall(
					client, RESPONSE_TEMPLATE_DYNAMIC,
					sizeof(RESPONSE_TEMPLATE_DYNAMIC) - 1);
			if (ret < 0) {
				return ret;
			}

			dynamic_detail->holder = NULL;

			return 0;
		}

	case HTTP_GET:
		/* For GET request, we do not pass any data to the app but let the app
		 * send data to the peer.
		 */
		if (user_method & BIT(HTTP_GET)) {
			return dynamic_get_req(dynamic_detail, client);
		}

		goto not_supported;

	case HTTP_POST:
		if (user_method & BIT(HTTP_POST)) {
			return dynamic_post_req(dynamic_detail, client);
		}

		goto not_supported;

not_supported:
	default:
		LOG_DBG("HTTP method %s (%d) not supported.",
			http_method_str(client->method),
			client->method);

		return -ENOTSUP;
	}

	return 0;
}

static int on_header_field(struct http_parser *parser, const char *at,
			   size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	ctx->parser_state = HTTP1_RECEIVING_HEADER_STATE;

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	ctx->parser_state = HTTP1_RECEIVED_HEADER_STATE;

	return 0;
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);
	size_t offset = strlen(ctx->url_buffer);

	ctx->parser_state = HTTP1_WAITING_HEADER_STATE;

	if (offset + length > sizeof(ctx->url_buffer) - 1) {
		LOG_DBG("URL too long to handle");
		return -EMSGSIZE;
	}

	memcpy(ctx->url_buffer + offset, at, length);
	offset += length;
	ctx->url_buffer[offset] = '\0';

	return 0;
}

static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	ctx->parser_state = HTTP1_RECEIVING_DATA_STATE;

	ctx->http1_frag_data_len += length;

	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);

	ctx->parser_state = HTTP1_MESSAGE_COMPLETE_STATE;

	return 0;
}

int enter_http1_request(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_REQUEST_STATE;

	http_parser_init(&client->parser, HTTP_REQUEST);
	http_parser_settings_init(&client->parser_settings);

	client->parser_settings.on_header_field = on_header_field;
	client->parser_settings.on_headers_complete = on_headers_complete;
	client->parser_settings.on_url = on_url;
	client->parser_settings.on_body = on_body;
	client->parser_settings.on_message_complete = on_message_complete;
	client->parser_state = HTTP1_INIT_HEADER_STATE;

	return 0;
}

int handle_http1_request(struct http_client_ctx *client)
{
	int ret, path_len = 0;
	struct http_resource_detail *detail;
	bool skip_headers = (client->parser_state < HTTP1_RECEIVING_DATA_STATE);
	size_t parsed;

	LOG_DBG("HTTP_SERVER_REQUEST");

	client->http1_frag_data_len = 0;

	parsed = http_parser_execute(&client->parser, &client->parser_settings,
				     client->cursor, client->data_len);

	if (parsed > client->data_len) {
		LOG_ERR("HTTP/1 parser error, too much data consumed");
		return -EBADMSG;
	}

	if (client->parser.http_errno != HPE_OK) {
		LOG_ERR("HTTP/1 parsing error, %d", client->parser.http_errno);
		return -EBADMSG;
	}

	if (client->parser_state < HTTP1_RECEIVED_HEADER_STATE) {
		client->cursor += parsed;
		client->data_len -= parsed;

		return 0;
	}

	client->method = client->parser.method;
	client->has_upgrade_header = client->parser.upgrade;

	if (skip_headers) {
		LOG_DBG("Requested URL: %s", client->url_buffer);

		size_t frag_headers_len;

		if (parsed < client->http1_frag_data_len) {
			return -EBADMSG;
		}

		frag_headers_len = parsed - client->http1_frag_data_len;
		parsed -= frag_headers_len;

		client->cursor += frag_headers_len;
		client->data_len -= frag_headers_len;
	}

	if (client->has_upgrade_header) {
		return handle_http1_to_http2_upgrade(client);
	}

	detail = get_resource_detail(client->url_buffer, &path_len);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http1_static_resource(
				(struct http_resource_detail_static *)detail,
				client);
			if (ret < 0) {
				return ret;
			}
		} else if (detail->type == HTTP_RESOURCE_TYPE_DYNAMIC) {
			ret = handle_http1_dynamic_resource(
				(struct http_resource_detail_dynamic *)detail,
				client);
			if (ret < 0) {
				return ret;
			}
		}
	} else {
		static const char not_found_response[] =
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Length: 9\r\n\r\n"
			"Not Found";

		ret = http_server_sendall(client, not_found_response,
					  sizeof(not_found_response) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}
	}

	client->cursor += parsed;
	client->data_len -= parsed;

	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		LOG_DBG("Connection closed client %p", client);
		enter_http_done_state(client);
	}

	return 0;
}
