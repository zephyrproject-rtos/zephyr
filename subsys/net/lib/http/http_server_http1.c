/*
 * Copyright (c) 2023, Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* Required for strnlen() */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/http/service.h>

LOG_MODULE_DECLARE(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#include "headers/server_internal.h"

#define TEMP_BUF_LEN 64

static const char not_found_response[] = "HTTP/1.1 404 Not Found\r\n"
					 "Content-Length: 9\r\n\r\n"
					 "Not Found";
static const char not_allowed_response[] = "HTTP/1.1 405 Method Not Allowed\r\n"
					   "Content-Length: 18\r\n\r\n"
					   "Method Not Allowed";
static const char conflict_response[] = "HTTP/1.1 409 Conflict\r\n\r\n";

static const char final_chunk[] = "0\r\n\r\n";
static const char *crlf = &final_chunk[3];

static int handle_http1_static_resource(
	struct http_resource_detail_static *static_detail,
	struct http_client_ctx *client)
{
#define RESPONSE_TEMPLATE			\
	"HTTP/1.1 200 OK\r\n"			\
	"%s%s\r\n"				\
	"Content-Length: %d\r\n"

	/* Add couple of bytes to total response */
	char http_response[sizeof(RESPONSE_TEMPLATE) +
			   sizeof("Content-Encoding: 01234567890123456789\r\n") +
			   sizeof("Content-Type: \r\n") + HTTP_SERVER_MAX_CONTENT_TYPE_LEN +
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
				 "Content-Type: ",
				 static_detail->common.content_type == NULL ?
				 "text/html" : static_detail->common.content_type,
				 len, static_detail->common.content_encoding);
		} else {
			snprintk(http_response, sizeof(http_response),
				 RESPONSE_TEMPLATE "\r\n",
				 "Content-Type: ",
				 static_detail->common.content_type == NULL ?
				 "text/html" : static_detail->common.content_type,
				 len);
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
	"%s%s\r\n"					\
	"Transfer-Encoding: chunked\r\n\r\n"

#define RESPONSE_TEMPLATE_DYNAMIC			\
	"HTTP/1.1 200 OK\r\n"				\
	"%s%s\r\n\r\n"

#define SEND_RESPONSE(_template, _content_type)	({			\
	char http_response[sizeof(_template) +				\
			   sizeof("Content-Type: \r\n") +		\
			   HTTP_SERVER_MAX_CONTENT_TYPE_LEN +		\
			   sizeof("xxxx") +				\
			   sizeof("\r\n")];				\
	snprintk(http_response, sizeof(http_response),			\
		 _template,						\
		 "Content-Type: ",					\
		 _content_type == NULL ?				\
		 "text/html" : _content_type);				\
	ret = http_server_sendall(client, http_response,		\
				  strnlen(http_response,		\
					  sizeof(http_response) - 1));	\
	ret; })

static int dynamic_get_req(struct http_resource_detail_dynamic *dynamic_detail,
			   struct http_client_ctx *client)
{
	/* offset tells from where the GET params start */
	int ret, remaining, offset = dynamic_detail->common.path_len;
	char *ptr;
	char tmp[TEMP_BUF_LEN];

	ret = SEND_RESPONSE(RESPONSE_TEMPLATE_CHUNKED,
			    dynamic_detail->common.content_type);
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

	if (!client->http1_headers_sent) {
		ret = SEND_RESPONSE(RESPONSE_TEMPLATE_CHUNKED,
				    dynamic_detail->common.content_type);
		if (ret < 0) {
			return ret;
		}
		client->http1_headers_sent = true;
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
		}

		offset += copy_len;
		remaining -= copy_len;
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

#if defined(CONFIG_FILE_SYSTEM)

int handle_http1_static_fs_resource(struct http_resource_detail_static_fs *static_fs_detail,
				    struct http_client_ctx *client)
{
#define RESPONSE_TEMPLATE_STATIC_FS                                                                \
	"HTTP/1.1 200 OK\r\n"                                                                      \
	"Content-Type: %s%s\r\n\r\n"
#define CONTENT_ENCODING_GZIP "\r\nContent-Encoding: gzip"

	bool gzipped = false;
	int len;
	int remaining;
	int ret;
	size_t file_size;
	struct fs_file_t file;
	char fname[HTTP_SERVER_MAX_URL_LENGTH];
	char content_type[HTTP_SERVER_MAX_CONTENT_TYPE_LEN] = "text/html";
	/* Add couple of bytes to response template size to have space
	 * for the content type and encoding
	 */
	char http_response[sizeof(RESPONSE_TEMPLATE_STATIC_FS) + HTTP_SERVER_MAX_CONTENT_TYPE_LEN +
			   sizeof(CONTENT_ENCODING_GZIP)];

	if (!(static_fs_detail->common.bitmask_of_supported_http_methods & BIT(HTTP_GET))) {
		ret = http_server_sendall(client, not_allowed_response,
					  sizeof(not_allowed_response) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
		}
		return ret;
	}

	/* get filename and content-type from url */
	len = strlen(client->url_buffer);
	if (len == 1) {
		/* url is just the leading slash, use index.html as filename */
		snprintk(fname, sizeof(fname), "%s/index.html", static_fs_detail->fs_path);
	} else {
		http_server_get_content_type_from_extension(client->url_buffer, content_type,
							    sizeof(content_type));
		snprintk(fname, sizeof(fname), "%s%s", static_fs_detail->fs_path,
			 client->url_buffer);
	}

	/* open file, if it exists */
	ret = http_server_find_file(fname, sizeof(fname), &file_size, &gzipped);
	if (ret < 0) {
		LOG_ERR("fs_stat %s: %d", fname, ret);
		ret = http_server_sendall(client, not_found_response,
					  sizeof(not_found_response) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
		}
		return ret;
	}
	fs_file_t_init(&file);
	ret = fs_open(&file, fname, FS_O_READ);
	if (ret < 0) {
		LOG_ERR("fs_open %s: %d", fname, ret);
		if (ret < 0) {
			return ret;
		}
	}

	LOG_DBG("found %s, file size: %zu", fname, file_size);

	/* send HTTP header */
	len = snprintk(http_response, sizeof(http_response), RESPONSE_TEMPLATE_STATIC_FS,
		       content_type, gzipped ? CONTENT_ENCODING_GZIP : "");
	ret = http_server_sendall(client, http_response, len);
	if (ret < 0) {
		goto close;
	}

	/* read and send file */
	remaining = file_size;
	while (remaining > 0) {
		len = fs_read(&file, http_response, sizeof(http_response));
		ret = http_server_sendall(client, http_response, len);
		if (ret < 0) {
			goto close;
		}
		remaining -= len;
	}
	ret = http_server_sendall(client, "\r\n\r\n", 4);

close:
	/* close file */
	fs_close(&file);

	return ret;
}
#endif

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
			ret = SEND_RESPONSE(RESPONSE_TEMPLATE_DYNAMIC,
					    dynamic_detail->common.content_type);
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
	size_t offset = strnlen(ctx->header_buffer, sizeof(ctx->header_buffer));

	if (offset + length > sizeof(ctx->header_buffer) - 1U) {
		LOG_DBG("Header %s too long (by %zu bytes)", "field",
			offset + length - sizeof(ctx->header_buffer) - 1U);
		ctx->header_buffer[0] = '\0';
	} else {
		memcpy(ctx->header_buffer + offset, at, length);
		offset += length;
		ctx->header_buffer[offset] = '\0';

		if (parser->state == s_header_value_discard_ws) {
			/* This means that the header field is fully parsed,
			 * and we can use it directly.
			 */
			if (strncasecmp(ctx->header_buffer, "Upgrade",
					sizeof("Upgrade") - 1) == 0) {
				ctx->has_upgrade_header = true;
			} else if (strncasecmp(ctx->header_buffer,
					       "Sec-WebSocket-Key",
					       sizeof("Sec-WebSocket-Key") - 1) == 0) {
				ctx->websocket_sec_key_next = true;
			}

			ctx->header_buffer[0] = '\0';
		}
	}

	ctx->parser_state = HTTP1_RECEIVING_HEADER_STATE;

	return 0;
}

static int on_header_value(struct http_parser *parser,
			   const char *at, size_t length)
{
	struct http_client_ctx *ctx = CONTAINER_OF(parser,
						   struct http_client_ctx,
						   parser);
	size_t offset = strnlen(ctx->header_buffer, sizeof(ctx->header_buffer));

	if (offset + length > sizeof(ctx->header_buffer) - 1U) {
		LOG_DBG("Header %s too long (by %zu bytes)", "value",
			offset + length - sizeof(ctx->header_buffer) - 1U);
		ctx->header_buffer[0] = '\0';
	} else {
		memcpy(ctx->header_buffer + offset, at, length);
		offset += length;
		ctx->header_buffer[offset] = '\0';

		if (parser->state == s_header_almost_done) {
			if (ctx->has_upgrade_header) {
				if (strncasecmp(ctx->header_buffer, "h2c",
						sizeof("h2c") - 1) == 0) {
					ctx->http2_upgrade = true;
				} else if (strncasecmp(ctx->header_buffer,
						       "websocket",
						       sizeof("websocket") - 1) == 0) {
					ctx->websocket_upgrade = true;
				}

				ctx->has_upgrade_header = false;
			}

			if (ctx->websocket_sec_key_next) {
#if defined(CONFIG_WEBSOCKET)
				strncpy(ctx->ws_sec_key, ctx->header_buffer,
					MIN(sizeof(ctx->ws_sec_key), offset));
#endif
				ctx->websocket_sec_key_next = false;
			}

			ctx->header_buffer[0] = '\0';
		}
	}

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
	client->parser_settings.on_header_value = on_header_value;
	client->parser_settings.on_headers_complete = on_headers_complete;
	client->parser_settings.on_url = on_url;
	client->parser_settings.on_body = on_body;
	client->parser_settings.on_message_complete = on_message_complete;
	client->parser_state = HTTP1_INIT_HEADER_STATE;

	memset(client->header_buffer, 0, sizeof(client->header_buffer));
	memset(client->url_buffer, 0, sizeof(client->url_buffer));

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
		static const char upgrade_required[] =
			"HTTP/1.1 426 Upgrade required\r\n"
			"Upgrade: ";
		static const char upgrade_msg[] =
			"Content-Length: 13\r\n\r\n"
			"Wrong upgrade";
		const char *needed_upgrade = "h2c\r\n";

		if (client->websocket_upgrade) {
			if (IS_ENABLED(CONFIG_HTTP_SERVER_WEBSOCKET)) {
				detail = get_resource_detail(client->url_buffer,
							     &path_len, true);
				if (detail == NULL) {
					goto not_found;
				}

				client->current_detail = detail;
				return handle_http1_to_websocket_upgrade(client);
			}

			goto upgrade_not_found;
		}

		if (client->http2_upgrade) {
			return handle_http1_to_http2_upgrade(client);
		}

upgrade_not_found:
		ret = http_server_sendall(client, upgrade_required,
					  sizeof(upgrade_required) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}

		ret = http_server_sendall(client, needed_upgrade,
					  strlen(needed_upgrade));
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}

		ret = http_server_sendall(client, upgrade_msg,
					  sizeof(upgrade_msg) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}
	}

	detail = get_resource_detail(client->url_buffer, &path_len, false);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http1_static_resource(
				(struct http_resource_detail_static *)detail,
				client);
			if (ret < 0) {
				return ret;
			}
#if defined(CONFIG_FILE_SYSTEM)
		} else if (detail->type == HTTP_RESOURCE_TYPE_STATIC_FS) {
			ret = handle_http1_static_fs_resource(
				(struct http_resource_detail_static_fs *)detail, client);
			if (ret < 0) {
				return ret;
			}
#endif
		} else if (detail->type == HTTP_RESOURCE_TYPE_DYNAMIC) {
			ret = handle_http1_dynamic_resource(
				(struct http_resource_detail_dynamic *)detail,
				client);
			if (ret < 0) {
				return ret;
			}
		}
	} else {
not_found: ; /* Add extra semicolon to make clang to compile when using label */
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
		if ((client->parser.flags & F_CONNECTION_CLOSE) == 0) {
			LOG_DBG("Waiting for another request, client %p", client);
			client->server_state = HTTP_SERVER_PREFACE_STATE;
		} else {
			LOG_DBG("Connection closed, client %p", client);
			enter_http_done_state(client);
		}
	}

	return 0;
}
