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
#include <zephyr/net/http/server.h>

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

static int send_http1_error_common(struct http_client_ctx *client,
				   const char *response, size_t len)
{
	int ret;

	ret = http_server_sendall(client, response, len);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	client->http1_headers_sent = true;

	return 0;
}

static int send_http1_404(struct http_client_ctx *client)
{
	return send_http1_error_common(client, not_found_response,
				       sizeof(not_found_response) - 1);
}

static int send_http1_405(struct http_client_ctx *client)
{
	return send_http1_error_common(client, not_allowed_response,
				       sizeof(not_allowed_response) - 1);
}

static int send_http1_409(struct http_client_ctx *client)
{
	return send_http1_error_common(client, conflict_response,
				       sizeof(conflict_response) - 1);
}

static void send_http1_500(struct http_client_ctx *client, int error_code)
{
#define HTTP_500_RESPONSE_TEMPLATE			\
	"HTTP/1.1 500 Internal Server Error\r\n"	\
	"Content-Type: text/plain\r\n"			\
	"Content-Length: %d\r\n\r\n"			\
	"Internal Server Error%s%s\r\n"
#define MAX_ERROR_DESC_LEN 32

	/* Placeholder for the error description. */
	char error_str[] = "xxx";
	char http_response[sizeof(HTTP_500_RESPONSE_TEMPLATE) +
			   sizeof("xx") + /* Content-Length */
			   MAX_ERROR_DESC_LEN + 1]; /* For the error description */
	const char *error_desc;
	const char *desc_separator;
	int desc_len;

	if (IS_ENABLED(CONFIG_HTTP_SERVER_REPORT_FAILURE_REASON)) {
		/* Try to fetch error description, fallback to error number if
		 * not available
		 */
		error_desc = strerror(error_code);
		if (strlen(error_desc) == 0) {
			/* Cast error value to uint8_t to avoid truncation warnings. */
			(void)snprintk(error_str, sizeof(error_str), "%u",
				       (uint8_t)error_code);
			error_desc = error_str;
		}
		desc_separator = ": ";
		desc_len = MIN(MAX_ERROR_DESC_LEN, strlen(error_desc));
		desc_len += 2; /* For ": " */
	} else {
		error_desc = "";
		desc_separator = "";
		desc_len = 0;
	}

	(void)snprintk(http_response, sizeof(http_response),
		       HTTP_500_RESPONSE_TEMPLATE,
		       (int)sizeof("Internal Server Error\r\n") - 1 + desc_len,
		       desc_separator, error_desc);
	(void)http_server_sendall(client, http_response, strlen(http_response));
}

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

	if (client->method != HTTP_GET) {
		return send_http1_405(client);
	}

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

	ret = http_server_sendall(client, http_response, strlen(http_response));
	if (ret < 0) {
		return ret;
	}

	client->http1_headers_sent = true;

	ret = http_server_sendall(client, data, len);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

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

#define RESPONSE_TEMPLATE_DYNAMIC_PART1                                                            \
	"HTTP/1.1 %d\r\n"                                                                          \
	"Transfer-Encoding: chunked\r\n"

static int http1_send_headers(struct http_client_ctx *client, enum http_status status,
			      const struct http_header *headers, size_t header_count,
			      struct http_resource_detail_dynamic *dynamic_detail)
{
	int ret;
	bool content_type_sent = false;
	char http_response[MAX(sizeof(RESPONSE_TEMPLATE_DYNAMIC_PART1) + sizeof("xxx"),
			       CONFIG_HTTP_SERVER_MAX_HEADER_LEN + 2)];

	if (status < HTTP_100_CONTINUE || status > HTTP_511_NETWORK_AUTHENTICATION_REQUIRED) {
		LOG_DBG("Invalid HTTP status code: %d", status);
		return -EINVAL;
	}

	if (headers == NULL && header_count > 0) {
		LOG_DBG("NULL headers, but count is > 0");
		return -EINVAL;
	}

	/* Send response code and transfer encoding */
	snprintk(http_response, sizeof(http_response), RESPONSE_TEMPLATE_DYNAMIC_PART1, status);

	ret = http_server_sendall(client, http_response,
				  strnlen(http_response, sizeof(http_response) - 1));
	if (ret < 0) {
		LOG_DBG("Failed to send HTTP headers part 1");
		return ret;
	}

	/* Send user-defined headers */
	for (size_t i = 0; i < header_count; i++) {
		const struct http_header *hdr = &headers[i];

		if (strcasecmp(hdr->name, "Transfer-Encoding") == 0) {
			LOG_DBG("Application is not permitted to change Transfer-Encoding header");
			return -EACCES;
		}

		if (strcasecmp(hdr->name, "Content-Type") == 0) {
			content_type_sent = true;
		}

		snprintk(http_response, sizeof(http_response), "%s: ", hdr->name);

		ret = http_server_sendall(client, http_response,
					  strnlen(http_response, sizeof(http_response) - 1));
		if (ret < 0) {
			LOG_DBG("Failed to send HTTP header name");
			return ret;
		}

		ret = http_server_sendall(client, hdr->value, strlen(hdr->value));
		if (ret < 0) {
			LOG_DBG("Failed to send HTTP header value");
			return ret;
		}

		ret = http_server_sendall(client, crlf, 2);
		if (ret < 0) {
			LOG_DBG("Failed to send CRLF");
			return ret;
		}
	}

	/* Send content-type header if it was not already sent */
	if (!content_type_sent) {
		const char *content_type = NULL;

		if (dynamic_detail != NULL) {
			content_type = dynamic_detail->common.content_type;
		}

		snprintk(http_response, sizeof(http_response), "Content-Type: %s\r\n",
			 content_type == NULL ? "text/html" : content_type);

		ret = http_server_sendall(client, http_response,
					  strnlen(http_response, sizeof(http_response) - 1));
		if (ret < 0) {
			LOG_DBG("Failed to send Content-Type");
			return ret;
		}
	}

	/* Send final CRLF */
	ret = http_server_sendall(client, crlf, 2);
	if (ret < 0) {
		LOG_DBG("Failed to send CRLF");
		return ret;
	}

	return ret;
}

static int http1_dynamic_response(struct http_client_ctx *client, struct http_response_ctx *rsp,
				  struct http_resource_detail_dynamic *dynamic_detail)
{
	int ret;
	char tmp[TEMP_BUF_LEN];

	if (client->http1_headers_sent && (rsp->header_count > 0 || rsp->status != 0)) {
		LOG_WRN("Already sent headers, dropping new headers and/or response code");
	}

	/* Send headers and response code if not already sent */
	if (!client->http1_headers_sent) {
		/* Use '200 OK' status if not specified by application */
		if (rsp->status == 0) {
			rsp->status = 200;
		}

		ret = http1_send_headers(client, rsp->status, rsp->headers, rsp->header_count,
					 dynamic_detail);
		if (ret < 0) {
			return ret;
		}

		client->http1_headers_sent = true;
	}

	/* Send body data if provided */
	if (rsp->body != NULL && rsp->body_len > 0) {
		ret = snprintk(tmp, sizeof(tmp), "%zx\r\n", rsp->body_len);
		ret = http_server_sendall(client, tmp, ret);
		if (ret < 0) {
			return ret;
		}

		ret = http_server_sendall(client, rsp->body, rsp->body_len);
		if (ret < 0) {
			return ret;
		}

		(void)http_server_sendall(client, crlf, 2);
	}

	return 0;
}

static int dynamic_get_del_req(struct http_resource_detail_dynamic *dynamic_detail,
			       struct http_client_ctx *client)
{
	int ret, len;
	char *ptr;
	enum http_data_status status;
	struct http_request_ctx request_ctx;
	struct http_response_ctx response_ctx;

	/* Start of GET params */
	ptr = &client->url_buffer[dynamic_detail->common.path_len];
	len = strlen(ptr);
	status = HTTP_SERVER_DATA_FINAL;

	do {
		memset(&response_ctx, 0, sizeof(response_ctx));
		populate_request_ctx(&request_ctx, ptr, len, &client->header_capture_ctx);

		ret = dynamic_detail->cb(client, status, &request_ctx, &response_ctx,
					 dynamic_detail->user_data);
		if (ret < 0) {
			return ret;
		}

		ret = http1_dynamic_response(client, &response_ctx, dynamic_detail);
		if (ret < 0) {
			return ret;
		}

		/* URL params are passed in the first cb only */
		len = 0;
	} while (!http_response_is_final(&response_ctx, status));

	dynamic_detail->holder = NULL;

	ret = http_server_sendall(client, final_chunk,
				  sizeof(final_chunk) - 1);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int dynamic_post_put_req(struct http_resource_detail_dynamic *dynamic_detail,
				struct http_client_ctx *client)
{
	int ret;
	char *ptr = client->cursor;
	enum http_data_status status;
	struct http_request_ctx request_ctx;
	struct http_response_ctx response_ctx;

	if (ptr == NULL) {
		return -ENOENT;
	}

	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		status = HTTP_SERVER_DATA_FINAL;
	} else {
		status = HTTP_SERVER_DATA_MORE;
	}

	memset(&response_ctx, 0, sizeof(response_ctx));
	populate_request_ctx(&request_ctx, ptr, client->data_len, &client->header_capture_ctx);

	ret = dynamic_detail->cb(client, status, &request_ctx, &response_ctx,
				 dynamic_detail->user_data);
	if (ret < 0) {
		return ret;
	}

	/* Only send request headers in first callback to application. This is not strictly
	 * necessary for http1, but is done for consistency with the http2 behaviour.
	 */
	client->header_capture_ctx.status = HTTP_HEADER_STATUS_NONE;

	/* For POST the application might not send a response until all data has been received.
	 * Don't send a default response until the application has had a chance to respond.
	 */
	if (http_response_is_provided(&response_ctx)) {
		ret = http1_dynamic_response(client, &response_ctx, dynamic_detail);
		if (ret < 0) {
			return ret;
		}
	}

	/* Once all data is transferred to application, repeat cb until response is complete */
	while (!http_response_is_final(&response_ctx, status) && status == HTTP_SERVER_DATA_FINAL) {
		memset(&response_ctx, 0, sizeof(response_ctx));
		populate_request_ctx(&request_ctx, ptr, 0, &client->header_capture_ctx);

		ret = dynamic_detail->cb(client, status, &request_ctx, &response_ctx,
					 dynamic_detail->user_data);
		if (ret < 0) {
			return ret;
		}

		ret = http1_dynamic_response(client, &response_ctx, dynamic_detail);
		if (ret < 0) {
			return ret;
		}
	}

	/* At end of message, ensure response is sent and terminated */
	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		if (!client->http1_headers_sent) {
			memset(&response_ctx, 0, sizeof(response_ctx));
			response_ctx.final_chunk = true;
			ret = http1_dynamic_response(client, &response_ctx, dynamic_detail);
			if (ret < 0) {
				return ret;
			}
		}

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
	"Content-Length: %zd\r\n"                                                                  \
	"Content-Type: %s%s%s\r\n\r\n"
#define CONTENT_ENCODING_HEADER "\r\nContent-Encoding: "
/* Add couple of bytes to response template size to have space
 * for the content type and encoding
 */
#define STATIC_FS_RESPONSE_BASE_SIZE                                                               \
	sizeof(RESPONSE_TEMPLATE_STATIC_FS) + HTTP_SERVER_MAX_CONTENT_TYPE_LEN +                   \
		sizeof("Content-Length: 01234567890123456789\r\n")
#define CONTENT_ENCODING_HEADER_SIZE                                                               \
	sizeof(CONTENT_ENCODING_HEADER) + HTTP_COMPRESSION_MAX_STRING_LEN + sizeof("\r\n")
#define STATIC_FS_RESPONSE_SIZE                                                                    \
	COND_CODE_1(                                                                               \
		IS_ENABLED(CONFIG_HTTP_SERVER_COMPRESSION),                                        \
		(STATIC_FS_RESPONSE_BASE_SIZE + CONTENT_ENCODING_HEADER_SIZE),                     \
		(STATIC_FS_RESPONSE_BASE_SIZE))

	enum http_compression chosen_compression = 0;
	int len;
	int remaining;
	int ret;
	size_t file_size;
	struct fs_file_t file;
	char fname[HTTP_SERVER_MAX_URL_LENGTH];
	char content_type[HTTP_SERVER_MAX_CONTENT_TYPE_LEN] = "text/html";
	char http_response[STATIC_FS_RESPONSE_SIZE];

	if (client->method != HTTP_GET) {
		return send_http1_405(client);
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
#ifdef CONFIG_HTTP_SERVER_COMPRESSION
	ret = http_server_find_file(fname, sizeof(fname), &file_size, client->supported_compression,
			    &chosen_compression);
#else
	ret = http_server_find_file(fname, sizeof(fname), &file_size, 0, NULL);
#endif /* CONFIG_HTTP_SERVER_COMPRESSION */
	if (ret < 0) {
		LOG_ERR("fs_stat %s: %d", fname, ret);
		return send_http1_404(client);
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
	if (IS_ENABLED(CONFIG_HTTP_SERVER_COMPRESSION) &&
	    http_compression_text(chosen_compression)[0] != 0) {
		len = snprintk(http_response, sizeof(http_response), RESPONSE_TEMPLATE_STATIC_FS,
			       file_size, content_type, CONTENT_ENCODING_HEADER,
			       http_compression_text(chosen_compression));
	} else {
		len = snprintk(http_response, sizeof(http_response), RESPONSE_TEMPLATE_STATIC_FS,
			       file_size, content_type, "", "");
	}
	ret = http_server_sendall(client, http_response, len);
	if (ret < 0) {
		goto close;
	}

	client->http1_headers_sent = true;

	/* read and send file */
	remaining = file_size;
	while (remaining > 0) {
		len = fs_read(&file, http_response, sizeof(http_response));
		if (len < 0) {
			LOG_ERR("Filesystem read error (%d)", len);
			goto close;
		}

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
		return send_http1_405(client);
	}

	if (dynamic_detail->holder != NULL && dynamic_detail->holder != client) {
		ret = send_http1_409(client);
		if (ret < 0) {
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

			client->http1_headers_sent = true;
			dynamic_detail->holder = NULL;

			return 0;
		}

	case HTTP_GET:
	case HTTP_DELETE:
		/* For GET/DELETE request, we do not pass any data to the app
		 * but let the app send data to the peer.
		 */
		if (user_method & BIT(client->method)) {
			return dynamic_get_del_req(dynamic_detail, client);
		}

		goto not_supported;

	case HTTP_POST:
	case HTTP_PUT:
	case HTTP_PATCH:
		if (user_method & BIT(client->method)) {
			return dynamic_post_put_req(dynamic_detail, client);
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

static void check_user_request_headers(struct http_header_capture_ctx *ctx, const char *buf)
{
	size_t header_len;
	char *dest = &ctx->buffer[ctx->cursor];
	size_t remaining = sizeof(ctx->buffer) - ctx->cursor;

	ctx->store_next_value = false;

	STRUCT_SECTION_FOREACH(http_header_name, header) {
		header_len = strlen(header->name);

		if (strcasecmp(buf, header->name) == 0) {
			if (ctx->count == ARRAY_SIZE(ctx->headers)) {
				LOG_DBG("Header '%s' dropped: not enough slots", header->name);
				ctx->status = HTTP_HEADER_STATUS_DROPPED;
				break;
			}

			if (remaining < header_len + 1) {
				LOG_DBG("Header '%s' dropped: buffer too small for name",
					header->name);
				ctx->status = HTTP_HEADER_STATUS_DROPPED;
				break;
			}

			memcpy(dest, header->name, header_len + 1);

			ctx->headers[ctx->count].name = dest;
			ctx->cursor += (header_len + 1);
			ctx->store_next_value = true;
			break;
		}
	}
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
			if (IS_ENABLED(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)) {
				check_user_request_headers(&ctx->header_capture_ctx,
							   ctx->header_buffer);
			}

			if (strcasecmp(ctx->header_buffer, "Upgrade") == 0) {
				ctx->has_upgrade_header = true;
			} else if (strcasecmp(ctx->header_buffer, "Sec-WebSocket-Key") == 0) {
				ctx->websocket_sec_key_next = true;
			}
#ifdef CONFIG_HTTP_SERVER_COMPRESSION
			else if (strcasecmp(ctx->header_buffer, "Accept-Encoding") == 0) {
				ctx->accept_encoding_next = true;
			}
#endif /* CONFIG_HTTP_SERVER_COMPRESSION */

			ctx->header_buffer[0] = '\0';
		}
	}

	ctx->parser_state = HTTP1_RECEIVING_HEADER_STATE;

	return 0;
}

static void populate_user_request_header(struct http_header_capture_ctx *ctx, const char *buf)
{
	char *dest;
	size_t value_len;
	size_t remaining;

	if (ctx->store_next_value == false) {
		return;
	}

	ctx->store_next_value = false;
	value_len = strlen(buf);
	remaining = sizeof(ctx->buffer) - ctx->cursor;

	if (value_len + 1 >= remaining) {
		LOG_DBG("Header '%s' dropped: buffer too small for value",
			ctx->headers[ctx->count].name);
		ctx->status = HTTP_HEADER_STATUS_DROPPED;
		return;
	}

	dest = &ctx->buffer[ctx->cursor];
	memcpy(dest, buf, value_len + 1);
	ctx->cursor += (value_len + 1);

	ctx->headers[ctx->count].value = dest;
	ctx->count++;
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

		if (IS_ENABLED(CONFIG_HTTP_SERVER_CAPTURE_HEADERS) &&
		    ctx->header_capture_ctx.store_next_value) {
			ctx->header_capture_ctx.store_next_value = false;
			ctx->header_capture_ctx.status = HTTP_HEADER_STATUS_DROPPED;
		}
	} else {
		memcpy(ctx->header_buffer + offset, at, length);
		offset += length;
		ctx->header_buffer[offset] = '\0';

		if (parser->state == s_header_almost_done) {
			if (IS_ENABLED(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)) {
				populate_user_request_header(&ctx->header_capture_ctx,
							     ctx->header_buffer);
			}

			if (ctx->has_upgrade_header) {
				if (strcasecmp(ctx->header_buffer, "h2c") == 0) {
					ctx->http2_upgrade = true;
				} else if (strcasecmp(ctx->header_buffer, "websocket") == 0) {
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
#ifdef CONFIG_HTTP_SERVER_COMPRESSION
			if (ctx->accept_encoding_next) {
				http_compression_parse_accept_encoding(ctx->header_buffer, offset,
								       &ctx->supported_compression);
				ctx->accept_encoding_next = false;
			}
#endif /* CONFIG_HTTP_SERVER_COMPRESSION */

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
	client->http1_headers_sent = false;

	if (IS_ENABLED(CONFIG_HTTP_SERVER_CAPTURE_HEADERS)) {
		client->header_capture_ctx.store_next_value = false;
	}

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
		ret = -EBADMSG;
		goto error;
	}

	if (client->parser.http_errno != HPE_OK) {
		LOG_ERR("HTTP/1 parsing error, %d", client->parser.http_errno);
		ret = -EBADMSG;
		goto error;
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
			ret = -EBADMSG;
			goto error;
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
				detail = get_resource_detail(client->service, client->url_buffer,
							     &path_len, true);
				if (detail == NULL) {
					goto not_found;
				}

				detail->path_len = path_len;
				client->current_detail = detail;

				ret = handle_http1_to_websocket_upgrade(client);
				if (ret < 0) {
					goto error;
				}

				return 0;
			}

			goto upgrade_not_found;
		}

		if (client->http2_upgrade) {
			ret = handle_http1_to_http2_upgrade(client);
			if (ret < 0) {
				goto error;
			}

			return 0;
		}

upgrade_not_found:
		ret = http_server_sendall(client, upgrade_required,
					  sizeof(upgrade_required) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			goto error;
		}

		client->http1_headers_sent = true;

		ret = http_server_sendall(client, needed_upgrade,
					  strlen(needed_upgrade));
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			goto error;
		}

		ret = http_server_sendall(client, upgrade_msg,
					  sizeof(upgrade_msg) - 1);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			goto error;
		}
	}

	detail = get_resource_detail(client->service, client->url_buffer, &path_len, false);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http1_static_resource(
				(struct http_resource_detail_static *)detail,
				client);
			if (ret < 0) {
				goto error;
			}
#if defined(CONFIG_FILE_SYSTEM)
		} else if (detail->type == HTTP_RESOURCE_TYPE_STATIC_FS) {
			ret = handle_http1_static_fs_resource(
				(struct http_resource_detail_static_fs *)detail, client);
			if (ret < 0) {
				goto error;
			}
#endif
		} else if (detail->type == HTTP_RESOURCE_TYPE_DYNAMIC) {
			ret = handle_http1_dynamic_resource(
				(struct http_resource_detail_dynamic *)detail,
				client);
			if (ret < 0) {
				goto error;
			}
		}
	} else {
not_found: ; /* Add extra semicolon to make clang to compile when using label */
		ret = send_http1_404(client);
		if (ret < 0) {
			goto error;
		}

		client->http1_headers_sent = true;
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

error:
	/* Best effort try to send HTTP 500 Internal Server Error response in
	 * case of errors. This can only be done however if we haven't sent
	 * response header elsewhere (i. e. can't send another reply if the
	 * error ocurred amid resource processing).
	 */
	if (ret != -EAGAIN && !client->http1_headers_sent) {
		send_http1_500(client, -ret);
	}

	return ret;
}
