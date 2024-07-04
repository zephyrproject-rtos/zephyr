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

static const char content_404[] = {
#ifdef INCLUDE_HTML_CONTENT
#include "not_found_page.html.gz.inc"
#endif
};

static bool is_header_flag_set(uint8_t flags, uint8_t mask)
{
	return (flags & mask) != 0;
}

static void clear_header_flag(uint8_t *flags, uint8_t mask)
{
	*flags &= ~mask;
}

static void print_http_frames(struct http_client_ctx *client)
{
#if defined(PRINT_COLOR)
	const char *bold = "\033[1m";
	const char *reset = "\033[0m";
	const char *green = "\033[32m";
	const char *blue = "\033[34m";
#else
	const char *bold = "";
	const char *reset = "";
	const char *green = "";
	const char *blue = "";
#endif

	struct http2_frame *frame = &client->current_frame;

	LOG_DBG("%s=====================================%s", green, reset);
	LOG_DBG("%sReceived %s Frame :%s", bold, get_frame_type_name(frame->type), reset);
	LOG_DBG("  %sLength:%s %u", blue, reset, frame->length);
	LOG_DBG("  %sType:%s %u (%s)", blue, reset, frame->type, get_frame_type_name(frame->type));
	LOG_DBG("  %sFlags:%s %u", blue, reset, frame->flags);
	LOG_DBG("  %sStream Identifier:%s %u", blue, reset, frame->stream_identifier);
	LOG_DBG("%s=====================================%s", green, reset);
}

static struct http2_stream_ctx *find_http_stream_context(
			struct http_client_ctx *client, uint32_t stream_id)
{
	ARRAY_FOR_EACH(client->streams, i) {
		if (client->streams[i].stream_id == stream_id) {
			return &client->streams[i];
		}
	}

	return NULL;
}

static struct http2_stream_ctx *allocate_http_stream_context(
			struct http_client_ctx *client, uint32_t stream_id)
{
	ARRAY_FOR_EACH(client->streams, i) {
		if (client->streams[i].stream_state == HTTP2_STREAM_IDLE) {
			client->streams[i].stream_id = stream_id;
			client->streams[i].stream_state = HTTP2_STREAM_OPEN;
			client->streams[i].window_size =
				HTTP_SERVER_INITIAL_WINDOW_SIZE;
			return &client->streams[i];
		}
	}

	return NULL;
}

static void release_http_stream_context(struct http_client_ctx *client,
					uint32_t stream_id)
{
	ARRAY_FOR_EACH(client->streams, i) {
		if (client->streams[i].stream_id == stream_id) {
			client->streams[i].stream_id = 0;
			client->streams[i].stream_state = HTTP2_STREAM_IDLE;
			break;
		}
	}
}

static int add_header_field(struct http_client_ctx *client, uint8_t **buf,
			    size_t *buflen, const char *name, const char *value)
{
	int ret;

	client->header_field.name = name;
	client->header_field.name_len = strlen(name);
	client->header_field.value = value;
	client->header_field.value_len = strlen(value);

	ret = http_hpack_encode_header(*buf, *buflen, &client->header_field);
	if (ret < 0) {
		return ret;
	}

	*buf += ret;
	*buflen -= ret;

	return 0;
}

static void encode_frame_header(uint8_t *buf, uint32_t payload_len,
				enum http2_frame_type frame_type,
				uint8_t flags, uint32_t stream_id)
{
	sys_put_be24(payload_len, &buf[HTTP2_FRAME_LENGTH_OFFSET]);
	buf[HTTP2_FRAME_TYPE_OFFSET] = frame_type;
	buf[HTTP2_FRAME_FLAGS_OFFSET] = flags;
	sys_put_be32(stream_id, &buf[HTTP2_FRAME_STREAM_ID_OFFSET]);
}

static int send_headers_frame(struct http_client_ctx *client,
			      enum http_status status, uint32_t stream_id,
			      struct http_resource_detail *detail_common,
			      uint8_t flags)
{
	uint8_t headers_frame[64];
	uint8_t status_str[4];
	uint8_t *buf = headers_frame + HTTP2_FRAME_HEADER_SIZE;
	size_t buflen = sizeof(headers_frame) - HTTP2_FRAME_HEADER_SIZE;
	size_t payload_len;
	int ret;

	ret = snprintf(status_str, sizeof(status_str), "%d", status);
	if (ret > sizeof(status_str) - 1) {
		return -EINVAL;
	}

	ret = add_header_field(client, &buf, &buflen, ":status", status_str);
	if (ret < 0) {
		return ret;
	}

	if (detail_common && detail_common->content_encoding != NULL) {
		ret = add_header_field(client, &buf, &buflen, "content-encoding",
				       "gzip");
		if (ret < 0) {
			return ret;
		}
	}

	if (detail_common && detail_common->content_type != NULL) {
		ret = add_header_field(client, &buf, &buflen, "content-type",
				       detail_common->content_type);
		if (ret < 0) {
			return ret;
		}
	}

	payload_len = sizeof(headers_frame) - buflen - HTTP2_FRAME_HEADER_SIZE;
	flags |= HTTP2_FLAG_END_HEADERS;

	encode_frame_header(headers_frame, payload_len, HTTP2_HEADERS_FRAME,
			    flags, stream_id);

	ret = http_server_sendall(client, headers_frame,
				  payload_len + HTTP2_FRAME_HEADER_SIZE);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	return 0;
}

static int send_data_frame(struct http_client_ctx *client, const char *payload,
			   size_t length, uint32_t stream_id, uint8_t flags)
{
	uint8_t frame_header[HTTP2_FRAME_HEADER_SIZE];
	int ret;

	encode_frame_header(frame_header, length, HTTP2_DATA_FRAME,
			    is_header_flag_set(flags, HTTP2_FLAG_END_STREAM) ?
			    HTTP2_FLAG_END_STREAM : 0,
			    stream_id);

	ret = http_server_sendall(client, frame_header, sizeof(frame_header));
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
	} else {
		if (payload != NULL && length > 0) {
			ret = http_server_sendall(client, payload, length);
			if (ret < 0) {
				LOG_DBG("Cannot write to socket (%d)", ret);
			}
		}
	}

	return ret;
}

int send_settings_frame(struct http_client_ctx *client, bool ack)
{
	uint8_t settings_frame[HTTP2_FRAME_HEADER_SIZE +
			       2 * sizeof(struct http2_settings_field)];
	struct http2_settings_field *setting;
	size_t len;
	int ret;

	if (ack) {
		encode_frame_header(settings_frame, 0,
				    HTTP2_SETTINGS_FRAME,
				    HTTP2_FLAG_SETTINGS_ACK, 0);
		len = HTTP2_FRAME_HEADER_SIZE;
	} else {
		encode_frame_header(settings_frame,
				    2 * sizeof(struct http2_settings_field),
				    HTTP2_SETTINGS_FRAME, 0, 0);

		setting = (struct http2_settings_field *)
			(settings_frame + HTTP2_FRAME_HEADER_SIZE);
		UNALIGNED_PUT(htons(HTTP2_SETTINGS_HEADER_TABLE_SIZE),
			      &setting->id);
		UNALIGNED_PUT(0, &setting->value);

		setting++;
		UNALIGNED_PUT(htons(HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS),
			      &setting->id);
		UNALIGNED_PUT(htonl(CONFIG_HTTP_SERVER_MAX_STREAMS),
			      &setting->value);

		len = HTTP2_FRAME_HEADER_SIZE +
		      2 * sizeof(struct http2_settings_field);
	}

	ret = http_server_sendall(client, settings_frame, len);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	return 0;
}

int send_window_update_frame(struct http_client_ctx *client,
			     struct http2_stream_ctx *stream)
{
	uint8_t window_update_frame[HTTP2_FRAME_HEADER_SIZE +
				    sizeof(uint32_t)];
	uint32_t window_update;
	uint32_t stream_id;
	int ret;

	if (stream != NULL) {
		window_update = HTTP_SERVER_INITIAL_WINDOW_SIZE - stream->window_size;
		stream->window_size = HTTP_SERVER_INITIAL_WINDOW_SIZE;
		stream_id = stream->stream_id;
	} else {
		window_update = HTTP_SERVER_INITIAL_WINDOW_SIZE - client->window_size;
		client->window_size = HTTP_SERVER_INITIAL_WINDOW_SIZE;
		stream_id = 0;
	}

	encode_frame_header(window_update_frame, sizeof(uint32_t),
			    HTTP2_WINDOW_UPDATE_FRAME,
			    0, stream_id);
	sys_put_be32(window_update,
		     window_update_frame + HTTP2_FRAME_HEADER_SIZE);

	ret = http_server_sendall(client, window_update_frame,
				  sizeof(window_update_frame));
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	return 0;
}

static int send_http2_404(struct http_client_ctx *client,
			  struct http2_frame *frame)
{
	int ret;

	ret = send_headers_frame(client, HTTP_404_NOT_FOUND,
				 frame->stream_identifier, NULL, 0);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	ret = send_data_frame(client, content_404, sizeof(content_404),
			      frame->stream_identifier,
			      HTTP2_FLAG_END_STREAM);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
	}

	return ret;
}

static int send_http2_409(struct http_client_ctx *client,
			  struct http2_frame *frame)
{
	int ret;

	ret = send_headers_frame(client, HTTP_409_CONFLICT,
				 frame->stream_identifier, NULL,
				 HTTP2_FLAG_END_STREAM);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
	}

	return ret;
}

static int handle_http2_static_resource(
	struct http_resource_detail_static *static_detail,
	struct http2_frame *frame, struct http_client_ctx *client)
{
	const char *content_200;
	size_t content_len;
	int ret;

	if (!(static_detail->common.bitmask_of_supported_http_methods & BIT(HTTP_GET))) {
		return -ENOTSUP;
	}

	content_200 = static_detail->static_data;
	content_len = static_detail->static_data_len;

	ret = send_headers_frame(client, HTTP_200_OK, frame->stream_identifier,
				 &static_detail->common, 0);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		goto out;
	}

	ret = send_data_frame(client, content_200, content_len,
			      frame->stream_identifier,
			      HTTP2_FLAG_END_STREAM);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		goto out;
	}

out:
	return ret;
}

static int dynamic_get_req_v2(struct http_resource_detail_dynamic *dynamic_detail,
			      struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	int ret, remaining, offset = dynamic_detail->common.path_len;
	char *ptr;

	ret = send_headers_frame(client, HTTP_200_OK, frame->stream_identifier,
				 &dynamic_detail->common, 0);
	if (ret < 0) {
		LOG_DBG("Cannot write to socket (%d)", ret);
		return ret;
	}

	remaining = strlen(&client->url_buffer[dynamic_detail->common.path_len]);

	/* Pass URL to the client */
	while (1) {
		int copy_len, send_len;
		enum http_data_status status;

		ptr = &client->url_buffer[offset];
		copy_len = MIN(remaining, dynamic_detail->data_buffer_len);

		if (copy_len > 0) {
			memcpy(dynamic_detail->data_buffer, ptr, copy_len);
		}

		if (copy_len == remaining) {
			status = HTTP_SERVER_DATA_FINAL;
		} else {
			status = HTTP_SERVER_DATA_MORE;
		}

		send_len = dynamic_detail->cb(client, status,
					      dynamic_detail->data_buffer,
					      copy_len,
					      dynamic_detail->user_data);
		if (send_len > 0) {
			ret = send_data_frame(client,
					      dynamic_detail->data_buffer,
					      send_len,
					      frame->stream_identifier,
					      0);
			if (ret < 0) {
				break;
			}

			offset += copy_len;
			remaining -= copy_len;

			continue;
		}

		ret = send_data_frame(client, NULL, 0,
				      frame->stream_identifier,
				      HTTP2_FLAG_END_STREAM);
		if (ret < 0) {
			LOG_DBG("Cannot send last frame (%d)", ret);
		}

		dynamic_detail->holder = NULL;

		break;
	}

	return ret;
}

static int dynamic_post_req_v2(struct http_resource_detail_dynamic *dynamic_detail,
			       struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	size_t data_len;
	int copy_len;
	int ret = 0;

	if (dynamic_detail == NULL) {
		return -ENOENT;
	}

	data_len = MIN(frame->length, client->data_len);
	copy_len = MIN(data_len, dynamic_detail->data_buffer_len);

	while (copy_len > 0) {
		enum http_data_status status;
		int send_len;

		/* Read all the user data and pass it to application. After
		 * passing all the data, if application returns 0, it means
		 * that there is no more data to send to client.
		 */
		memcpy(dynamic_detail->data_buffer, client->cursor, copy_len);
		data_len -= copy_len;
		client->cursor += copy_len;
		client->data_len -= copy_len;
		frame->length -= copy_len;

		if (frame->length == 0 &&
		    is_header_flag_set(frame->flags, HTTP2_FLAG_END_STREAM)) {
			status = HTTP_SERVER_DATA_FINAL;
		} else {
			status = HTTP_SERVER_DATA_MORE;
		}

		send_len = dynamic_detail->cb(client, status,
					      dynamic_detail->data_buffer,
					      copy_len,
					      dynamic_detail->user_data);
		if (send_len > 0) {
			uint8_t flags = 0;

			if (!client->headers_sent) {
				ret = send_headers_frame(
					client, HTTP_200_OK, frame->stream_identifier,
					&dynamic_detail->common, 0);
				if (ret < 0) {
					LOG_DBG("Cannot write to socket (%d)", ret);
					return ret;
				}

				client->headers_sent = true;
			}

			/* In case no more data is available, that was the last
			 * callback call, so we can include END_STREAM flag.
			 */
			if (frame->length == 0 &&
			    is_header_flag_set(frame->flags, HTTP2_FLAG_END_STREAM)) {
				flags = HTTP2_FLAG_END_STREAM;
			}

			ret = send_data_frame(client,
					      dynamic_detail->data_buffer,
					      send_len,
					      frame->stream_identifier,
					      flags);
			if (ret < 0) {
				LOG_DBG("Cannot send data frame (%d)", ret);
				return ret;
			}
		}

		copy_len = MIN(data_len, dynamic_detail->data_buffer_len);
	};

	if (frame->length == 0 &&
	    is_header_flag_set(frame->flags, HTTP2_FLAG_END_STREAM)) {
		if (!client->headers_sent) {
			/* The callback did not report any data to send, therefore send
			 * headers frame now, including END_STREAM flag.
			 */
			ret = send_headers_frame(
				client, HTTP_200_OK, frame->stream_identifier,
				&dynamic_detail->common,
				HTTP2_FLAG_END_STREAM);
			if (ret < 0) {
				LOG_DBG("Cannot write to socket (%d)", ret);
				return ret;
			}

			client->headers_sent = true;
		}

		dynamic_detail->holder = NULL;
	}


	return ret;
}

static int handle_http2_dynamic_resource(
	struct http_resource_detail_dynamic *dynamic_detail,
	struct http2_frame *frame, struct http_client_ctx *client)
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
		ret = send_http2_409(client, frame);
		if (ret < 0) {
			return ret;
		}

		return enter_http_done_state(client);
	}

	dynamic_detail->holder = client;

	switch (client->method) {
	case HTTP_GET:
		if (user_method & BIT(HTTP_GET)) {
			return dynamic_get_req_v2(dynamic_detail, client);
		}

		goto not_supported;

	case HTTP_POST:
		/* The data will come in DATA frames. Remember the detail ptr
		 * which needs to be known when passing data to application.
		 */
		if (user_method & BIT(HTTP_POST)) {
			client->current_detail =
				(struct http_resource_detail *)dynamic_detail;
			break;
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

int enter_http2_request(struct http_client_ctx *client)
{
	int ret;

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
	client->data_len -= sizeof(HTTP2_PREFACE) - 1;
	client->cursor += sizeof(HTTP2_PREFACE) - 1;

	/* HTTP/2 client preface received, send server preface
	 * (settings frame).
	 */
	if (!client->preface_sent) {
		ret = send_settings_frame(client, false);
		if (ret < 0) {
			return ret;
		}

		client->preface_sent = true;
	}

	return 0;
}

static int enter_http_frame_settings_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_SETTINGS_STATE;

	return 0;
}

static int enter_http_frame_data_state(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	struct http2_stream_ctx *stream;

	if (frame->stream_identifier == 0) {
		LOG_DBG("Stream ID 0 is forbidden for data frames.");
		return -EBADMSG;
	}

	stream = find_http_stream_context(client, frame->stream_identifier);
	if (stream == NULL) {
		LOG_DBG("No stream context found for ID %d",
			frame->stream_identifier);
		return -EBADMSG;
	}

	if (stream->stream_state != HTTP2_STREAM_OPEN &&
	    stream->stream_state != HTTP2_STREAM_HALF_CLOSED_REMOTE) {
		LOG_DBG("Stream ID %d in a wrong state %d", stream->stream_id,
			stream->stream_state);
		return -EBADMSG;
	}

	stream->window_size -= frame->length;
	client->window_size -= frame->length;
	client->server_state = HTTP_SERVER_FRAME_DATA_STATE;

	return 0;
}

static int enter_http_frame_headers_state(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	struct http2_stream_ctx *stream;

	stream = find_http_stream_context(client, frame->stream_identifier);
	if (!stream) {
		LOG_DBG("|| stream ID ||  %d", frame->stream_identifier);

		stream = allocate_http_stream_context(client, frame->stream_identifier);
		if (!stream) {
			LOG_DBG("No available stream slots. Connection closed.");

			return -ENOMEM;
		}
	}

	if (!is_header_flag_set(frame->flags, HTTP2_FLAG_END_HEADERS)) {
		client->expect_continuation = true;
	} else {
		client->expect_continuation = false;
	}

	client->server_state = HTTP_SERVER_FRAME_HEADERS_STATE;

	return 0;
}

static int enter_http_frame_continuation_state(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;

	if (!is_header_flag_set(frame->flags, HTTP2_FLAG_END_HEADERS)) {
		client->expect_continuation = true;
	} else {
		client->expect_continuation = false;
	}

	client->server_state = HTTP_SERVER_FRAME_CONTINUATION_STATE;

	return 0;
}

static int enter_http_frame_window_update_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_WINDOW_UPDATE_STATE;

	return 0;
}

static int enter_http_frame_priority_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_PRIORITY_STATE;

	return 0;
}

static int enter_http_frame_rst_stream_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_RST_STREAM_STATE;

	return 0;
}

static int enter_http_frame_goaway_state(struct http_client_ctx *client)
{
	client->server_state = HTTP_SERVER_FRAME_GOAWAY_STATE;

	return 0;
}

int handle_http_frame_header(struct http_client_ctx *client)
{
	int ret;

	LOG_DBG("HTTP_SERVER_FRAME_HEADER");

	ret = parse_http_frame_header(client, client->cursor, client->data_len);
	if (ret < 0) {
		return ret;
	}

	client->cursor += HTTP2_FRAME_HEADER_SIZE;
	client->data_len -= HTTP2_FRAME_HEADER_SIZE;

	print_http_frames(client);

	if (client->expect_continuation &&
	    client->current_frame.type != HTTP2_CONTINUATION_FRAME) {
		LOG_ERR("Continuation frame expected");
		return -EBADMSG;
	}

	switch (client->current_frame.type) {
	case HTTP2_DATA_FRAME:
		return enter_http_frame_data_state(client);
	case HTTP2_HEADERS_FRAME:
		return enter_http_frame_headers_state(client);
	case HTTP2_CONTINUATION_FRAME:
		return enter_http_frame_continuation_state(client);
	case HTTP2_SETTINGS_FRAME:
		return enter_http_frame_settings_state(client);
	case HTTP2_WINDOW_UPDATE_FRAME:
		return enter_http_frame_window_update_state(client);
	case HTTP2_RST_STREAM_FRAME:
		return enter_http_frame_rst_stream_state(client);
	case HTTP2_GOAWAY_FRAME:
		return enter_http_frame_goaway_state(client);
	case HTTP2_PRIORITY_FRAME:
		return enter_http_frame_priority_state(client);
	default:
		return enter_http_done_state(client);
	}

	return 0;
}

/* This feature is theoretically obsoleted in RFC9113, but curl for instance
 * still uses it, so implement as described in RFC7540.
 */
int handle_http1_to_http2_upgrade(struct http_client_ctx *client)
{
	static const char switching_protocols[] =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Connection: Upgrade\r\n"
		"Upgrade: h2c\r\n"
		"\r\n";
	struct http2_frame *frame = &client->current_frame;
	struct http_resource_detail *detail;
	int path_len;
	int ret;

	/* Create an artificial Data frame, so that we can proceed with HTTP2
	 * processing. The HTTP/1.1 request that is sent prior to upgrade is
	 * assigned a stream identifier of 1.
	 */
	frame->stream_identifier = 1;
	frame->type = HTTP2_DATA_FRAME;
	frame->length = client->http1_frag_data_len;
	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		frame->flags = HTTP2_FLAG_END_STREAM;
	} else {
		frame->flags = 0;
	}

	if (!client->preface_sent) {
		ret = http_server_sendall(client, switching_protocols,
					  sizeof(switching_protocols) - 1);
		if (ret < 0) {
			goto error;
		}

		/* The first HTTP/2 frame sent by the server MUST be a server connection
		 * preface.
		 */
		ret = send_settings_frame(client, false);
		if (ret < 0) {
			goto error;
		}

		client->preface_sent = true;
	}

	detail = get_resource_detail(client->url_buffer, &path_len, false);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http2_static_resource(
				(struct http_resource_detail_static *)detail,
				frame, client);
			if (ret < 0) {
				goto error;
			}
		} else if (detail->type == HTTP_RESOURCE_TYPE_DYNAMIC) {
			ret = handle_http2_dynamic_resource(
				(struct http_resource_detail_dynamic *)detail,
				frame, client);
			if (ret < 0) {
				goto error;
			}

			if (client->method == HTTP_POST) {
				ret = dynamic_post_req_v2(
					(struct http_resource_detail_dynamic *)detail,
					client);
				if (ret < 0) {
					goto error;
				}
			}

		}
	} else {
		ret = send_http2_404(client, frame);
		if (ret < 0) {
			goto error;
		}
	}

	/* Only after the complete HTTP1 payload has been processed, switch
	 * to HTTP2.
	 */
	if (client->parser_state == HTTP1_MESSAGE_COMPLETE_STATE) {
		client->current_detail = NULL;
		client->server_state = HTTP_SERVER_PREFACE_STATE;
		client->cursor += client->data_len;
		client->data_len = 0;
	}

	return 0;

error:
	return ret;
}

static int parse_http_frame_padded_field(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;

	if (client->data_len == 0) {
		return -EAGAIN;
	}

	frame->padding_len = *client->cursor;
	client->cursor++;
	client->data_len--;
	frame->length--;

	if (frame->length <= frame->padding_len) {
		return -EBADMSG;
	}

	/* Subtract the padding length from frame length now to simplify
	 * payload processing. Padding will be handled based on
	 * frame->padding_len in a separate state.
	 */
	frame->length -= frame->padding_len;

	/* Clear the padded flag, this indicates that padding field was
	 * already parsed.
	 */
	clear_header_flag(&frame->flags, HTTP2_FLAG_PADDED);

	return 0;
}

static int parse_http_frame_priority_field(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;

	if (client->data_len < HTTP2_HEADERS_FRAME_PRIORITY_LEN) {
		return -EAGAIN;
	}

	/* Priority signalling is deprecated by RFC 9113, however it still
	 * should be expected to receive, just drop the bytes.
	 */
	client->cursor += HTTP2_HEADERS_FRAME_PRIORITY_LEN;
	client->data_len -= HTTP2_HEADERS_FRAME_PRIORITY_LEN;
	frame->length -= HTTP2_HEADERS_FRAME_PRIORITY_LEN;

	/* Clear the priority flag, this indicates that priority field was
	 * already parsed.
	 */
	clear_header_flag(&frame->flags, HTTP2_FLAG_PRIORITY);

	return 0;
}

int handle_http_frame_data(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	int ret;

	LOG_DBG("HTTP_SERVER_FRAME_DATA_STATE");

	if (client->current_detail == NULL) {
		/* There is no handler */
		LOG_DBG("No dynamic handler found.");
		(void)send_http2_404(client, frame);
		return -ENOENT;
	}

	if (is_header_flag_set(frame->flags, HTTP2_FLAG_PADDED)) {
		ret = parse_http_frame_padded_field(client);
		if (ret < 0) {
			return ret;
		}
	}

	ret = dynamic_post_req_v2(
		(struct http_resource_detail_dynamic *)client->current_detail,
		client);
	if (ret < 0 && ret == -ENOENT) {
		ret = send_http2_404(client, frame);
	}

	if (ret < 0) {
		return ret;
	}

	if (frame->length == 0) {
		struct http2_stream_ctx *stream =
			find_http_stream_context(client, frame->stream_identifier);

		if (stream == NULL) {
			LOG_DBG("No stream context found for ID %d",
				frame->stream_identifier);
			return -EBADMSG;
		}

		ret = send_window_update_frame(client, stream);
		if (ret < 0) {
			return ret;
		}

		ret = send_window_update_frame(client, NULL);
		if (ret < 0) {
			return ret;
		}

		if (is_header_flag_set(frame->flags, HTTP2_FLAG_END_STREAM)) {
			client->current_detail = NULL;
			release_http_stream_context(client, frame->stream_identifier);
		}

		/* Whole frame consumed, expect next one. */
		if (frame->padding_len > 0) {
			client->server_state = HTTP_SERVER_FRAME_PADDING_STATE;
		} else {
			client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
		}
	}

	return 0;
}

static int process_header(struct http_client_ctx *client,
			  struct http_hpack_header_buf *header)
{
	if (header->name_len == (sizeof(":method") - 1) &&
	    memcmp(header->name, ":method", header->name_len) == 0) {
		/* TODO Improve string to method conversion */
		if (header->value_len == (sizeof("GET") - 1) &&
			memcmp(header->value, "GET", header->value_len) == 0) {
			client->method = HTTP_GET;
		} else if (header->value_len == (sizeof("POST") - 1) &&
				memcmp(header->value, "POST", header->value_len) == 0) {
			client->method = HTTP_POST;
		} else {
			/* Unknown method */
			return -EBADMSG;
		}
	} else if (header->name_len == (sizeof(":path") - 1) &&
		   memcmp(header->name, ":path", header->name_len) == 0) {
		if (header->value_len > sizeof(client->url_buffer) - 1) {
			/* URL too long to handle */
			return -ENOBUFS;
		}

		memcpy(client->url_buffer, header->value, header->value_len);
		client->url_buffer[header->value_len] = '\0';
	} else if (header->name_len == (sizeof("content-type") - 1) &&
		   memcmp(header->name, "content-type", header->name_len) == 0) {
		if (header->value_len > sizeof(client->content_type) - 1) {
			/* Content-type too long to handle */
			return -ENOBUFS;
		}

		memcpy(client->content_type, header->value, header->value_len);
		client->content_type[header->value_len] = '\0';
	} else if (header->name_len == (sizeof("content-length") - 1) &&
		   memcmp(header->name, "content-length", header->name_len) == 0) {
		char len_str[16] = { 0 };
		char *endptr;
		unsigned long len;

		memcpy(len_str, header->value, MIN(sizeof(len_str), header->value_len));
		len_str[sizeof(len_str) - 1] = '\0';

		len = strtoul(len_str, &endptr, 10);
		if (*endptr != '\0') {
			return -EINVAL;
		}

		client->content_len = (size_t)len;
	} else {
		/* Just ignore for now. */
		LOG_DBG("Ignoring field %.*s", (int)header->name_len, header->name);
	}

	return 0;
}

static int handle_incomplete_http_header(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	size_t extra_len, prev_frame_len;
	int ret;

	if (client->data_len < frame->length) {
		/* Still did not receive entire frame content */
		return -EAGAIN;
	}

	if (!client->expect_continuation) {
		/* Failed to parse header field while the frame is complete
		 * and no continuation frame is expected - report protocol
		 * error.
		 */
		LOG_ERR("Incomplete header field");
		return -EBADMSG;
	}

	/* A header field can be split between two frames, i. e. header and
	 * continuation or two continuation frames. Because of this, the
	 * continuation frame header can be present in the stream in between
	 * header field data, so in such case we need to check for header here
	 * and remove it from the stream to unblock further processing of the
	 * header field.
	 */
	prev_frame_len = frame->length;
	extra_len = client->data_len - frame->length;
	ret = parse_http_frame_header(client, client->cursor + prev_frame_len,
				      extra_len);
	if (ret < 0) {
		return -EAGAIN;
	}

	/* Continuation frame expected. */
	if (frame->type != HTTP2_CONTINUATION_FRAME) {
		LOG_ERR("Continuation frame expected");
		return -EBADMSG;
	}

	print_http_frames(client);
	/* Now remove continuation frame header from the stream, and proceed
	 * with processing.
	 */
	extra_len -= HTTP2_FRAME_HEADER_SIZE;
	client->data_len -= HTTP2_FRAME_HEADER_SIZE;
	frame->length += prev_frame_len;
	memmove(client->cursor + prev_frame_len,
		client->cursor + prev_frame_len + HTTP2_FRAME_HEADER_SIZE,
		extra_len);

	return enter_http_frame_continuation_state(client);
}

int handle_http_frame_headers(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	struct http_resource_detail *detail;
	int ret, path_len;

	LOG_DBG("HTTP_SERVER_FRAME_HEADERS");

	if (is_header_flag_set(frame->flags, HTTP2_FLAG_PADDED)) {
		ret = parse_http_frame_padded_field(client);
		if (ret < 0) {
			return ret;
		}
	}

	if (is_header_flag_set(frame->flags, HTTP2_FLAG_PRIORITY)) {
		ret = parse_http_frame_priority_field(client);
		if (ret < 0) {
			return ret;
		}
	}

	while (frame->length > 0) {
		struct http_hpack_header_buf *header = &client->header_field;
		size_t datalen = MIN(client->data_len, frame->length);

		ret = http_hpack_decode_header(client->cursor, datalen, header);
		if (ret <= 0) {
			if (ret == -EAGAIN) {
				ret = handle_incomplete_http_header(client);
			} else if (ret == 0) {
				ret = -EBADMSG;
			}

			return ret;
		}

		if (ret > frame->length) {
			LOG_ERR("Protocol error, frame length exceeded");
			return -EBADMSG;
		}

		frame->length -= ret;
		client->cursor += ret;
		client->data_len -= ret;

		LOG_DBG("Parsed header: %.*s %.*s", (int)header->name_len,
			header->name, (int)header->value_len, header->value);

		ret = process_header(client, header);
		if (ret < 0) {
			return ret;
		}
	}

	if (client->expect_continuation) {
		/* More headers to come in the continuation frame. */
		client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
		return 0;
	}

	detail = get_resource_detail(client->url_buffer, &path_len, false);
	if (detail != NULL) {
		detail->path_len = path_len;

		if (detail->type == HTTP_RESOURCE_TYPE_STATIC) {
			ret = handle_http2_static_resource(
				(struct http_resource_detail_static *)detail,
				frame, client);
			if (ret < 0) {
				return ret;
			}
		} else if (detail->type == HTTP_RESOURCE_TYPE_DYNAMIC) {
			ret = handle_http2_dynamic_resource(
				(struct http_resource_detail_dynamic *)detail,
				frame, client);
			if (ret < 0) {
				return ret;
			}
		}

	} else {
		ret = send_http2_404(client, frame);
		if (ret < 0) {
			return ret;
		}
	}

	if (is_header_flag_set(frame->flags, HTTP2_FLAG_END_STREAM)) {
		release_http_stream_context(client, frame->stream_identifier);
	}

	if (frame->padding_len > 0) {
		client->server_state = HTTP_SERVER_FRAME_PADDING_STATE;
	} else {
		client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
	}

	return 0;
}

int handle_http_frame_priority(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;

	LOG_DBG("HTTP_SERVER_FRAME_PRIORITY_STATE");

	if (frame->length != HTTP2_PRIORITY_FRAME_LEN) {
		return -EBADMSG;
	}

	if (client->data_len < frame->length) {
		return -EAGAIN;
	}

	/* Priority signalling is deprecated by RFC 9113, however it still
	 * should be expected to receive, just drop the bytes.
	 */
	client->data_len -= HTTP2_PRIORITY_FRAME_LEN;
	client->cursor += HTTP2_PRIORITY_FRAME_LEN;

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_rst_stream(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	struct http2_stream_ctx *stream_ctx;
	uint32_t error_code;

	LOG_DBG("FRAME_RST_STREAM");

	if (frame->length != HTTP2_RST_STREAM_FRAME_LEN) {
		return -EBADMSG;
	}

	if (client->data_len < frame->length) {
		return -EAGAIN;
	}

	if (frame->stream_identifier == 0) {
		return -EBADMSG;
	}

	stream_ctx = find_http_stream_context(client, frame->stream_identifier);
	if (stream_ctx == NULL) {
		return -EBADMSG;
	}

	error_code = sys_get_be32(client->cursor);

	LOG_DBG("Stream %u reset with error code %u", stream_ctx->stream_id,
		error_code);

	release_http_stream_context(client, stream_ctx->stream_id);

	client->data_len -= HTTP2_RST_STREAM_FRAME_LEN;
	client->cursor += HTTP2_RST_STREAM_FRAME_LEN;

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_settings(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("HTTP_SERVER_FRAME_SETTINGS");

	if (client->data_len < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->data_len -= bytes_consumed;
	client->cursor += bytes_consumed;

	if (!is_header_flag_set(frame->flags, HTTP2_FLAG_SETTINGS_ACK)) {
		int ret;

		ret = send_settings_frame(client, true);
		if (ret < 0) {
			LOG_DBG("Cannot write to socket (%d)", ret);
			return ret;
		}
	}

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_goaway(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("HTTP_SERVER_FRAME_GOAWAY");

	if (client->data_len < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->data_len -= bytes_consumed;
	client->cursor += bytes_consumed;

	enter_http_done_state(client);

	return 0;
}

int handle_http_frame_window_update(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	int bytes_consumed;

	LOG_DBG("HTTP_SERVER_FRAME_WINDOW_UPDATE");

	/* TODO Implement flow control, for now just ignore. */

	if (client->data_len < frame->length) {
		return -EAGAIN;
	}

	bytes_consumed = client->current_frame.length;
	client->data_len -= bytes_consumed;
	client->cursor += bytes_consumed;

	client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;

	return 0;
}

int handle_http_frame_continuation(struct http_client_ctx *client)
{
	LOG_DBG("HTTP_SERVER_FRAME_CONTINUATION_STATE");
	client->server_state = HTTP_SERVER_FRAME_HEADERS_STATE;

	return 0;
}

int handle_http_frame_padding(struct http_client_ctx *client)
{
	struct http2_frame *frame = &client->current_frame;
	size_t bytes_consumed;

	if (client->data_len == 0) {
		return -EAGAIN;
	}

	bytes_consumed = MIN(client->data_len, frame->padding_len);
	client->data_len -= bytes_consumed;
	client->cursor += bytes_consumed;
	frame->padding_len -= bytes_consumed;

	if (frame->padding_len == 0) {
		client->server_state = HTTP_SERVER_FRAME_HEADER_STATE;
	}

	return 0;
}

const char *get_frame_type_name(enum http2_frame_type type)
{
	switch (type) {
	case HTTP2_DATA_FRAME:
		return "DATA";
	case HTTP2_HEADERS_FRAME:
		return "HEADERS";
	case HTTP2_PRIORITY_FRAME:
		return "PRIORITY";
	case HTTP2_RST_STREAM_FRAME:
		return "RST_STREAM";
	case HTTP2_SETTINGS_FRAME:
		return "SETTINGS";
	case HTTP2_PUSH_PROMISE_FRAME:
		return "PUSH_PROMISE";
	case HTTP2_PING_FRAME:
		return "PING";
	case HTTP2_GOAWAY_FRAME:
		return "GOAWAY";
	case HTTP2_WINDOW_UPDATE_FRAME:
		return "WINDOW_UPDATE";
	case HTTP2_CONTINUATION_FRAME:
		return "CONTINUATION";
	default:
		return "UNKNOWN";
	}
}

int parse_http_frame_header(struct http_client_ctx *client, const uint8_t *buffer,
			    size_t buflen)
{
	struct http2_frame *frame = &client->current_frame;

	if (buflen < HTTP2_FRAME_HEADER_SIZE) {
		return -EAGAIN;
	}

	frame->length = sys_get_be24(&buffer[HTTP2_FRAME_LENGTH_OFFSET]);
	frame->type = buffer[HTTP2_FRAME_TYPE_OFFSET];
	frame->flags = buffer[HTTP2_FRAME_FLAGS_OFFSET];
	frame->stream_identifier = sys_get_be32(
				&buffer[HTTP2_FRAME_STREAM_ID_OFFSET]);
	frame->stream_identifier &= HTTP2_FRAME_STREAM_ID_MASK;
	frame->padding_len = 0;

	LOG_DBG("Frame len %d type 0x%02x flags 0x%02x id %d",
		frame->length, frame->type, frame->flags, frame->stream_identifier);

	return 0;
}
