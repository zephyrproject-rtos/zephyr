/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/http/frame.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "headers/http2_internal.h"

void http2_frame_header_encode(uint8_t *buf, uint32_t payload_len, enum http2_frame_type type,
			       uint8_t flags, uint32_t stream_id)
{
	sys_put_be24(payload_len, &buf[HTTP2_FRAME_LENGTH_OFFSET]);
	buf[HTTP2_FRAME_TYPE_OFFSET] = type;
	buf[HTTP2_FRAME_FLAGS_OFFSET] = flags;
	sys_put_be32(stream_id, &buf[HTTP2_FRAME_STREAM_ID_OFFSET]);
}

int http2_frame_header_parse(const uint8_t *buf, size_t buflen, struct http2_frame *frame)
{
	if (buflen < HTTP2_FRAME_HEADER_SIZE) {
		return -EAGAIN;
	}

	frame->length = sys_get_be24(&buf[HTTP2_FRAME_LENGTH_OFFSET]);
	frame->type = buf[HTTP2_FRAME_TYPE_OFFSET];
	frame->flags = buf[HTTP2_FRAME_FLAGS_OFFSET];
	frame->stream_identifier = sys_get_be32(&buf[HTTP2_FRAME_STREAM_ID_OFFSET]) &
				   HTTP2_FRAME_STREAM_ID_MASK;
	frame->padding_len = 0;

	return 0;
}

int http2_frame_settings_encode(uint8_t *buf, size_t buflen,
				const struct http2_setting *settings, size_t count)
{
	size_t payload_len = count * HTTP2_SETTINGS_FIELD_LEN;
	uint8_t *field = buf + HTTP2_FRAME_HEADER_SIZE;

	if (buflen < HTTP2_FRAME_HEADER_SIZE + payload_len) {
		return -ENOBUFS;
	}

	http2_frame_header_encode(buf, payload_len, HTTP2_SETTINGS_FRAME, 0, 0);

	for (size_t i = 0; i < count; i++) {
		sys_put_be16(settings[i].id, field);
		sys_put_be32(settings[i].value, field + sizeof(uint16_t));
		field += HTTP2_SETTINGS_FIELD_LEN;
	}

	return HTTP2_FRAME_HEADER_SIZE + payload_len;
}

int http2_frame_settings_ack_encode(uint8_t *buf, size_t buflen)
{
	if (buflen < HTTP2_FRAME_HEADER_SIZE) {
		return -ENOBUFS;
	}

	http2_frame_header_encode(buf, 0, HTTP2_SETTINGS_FRAME, HTTP2_FLAG_ACK, 0);

	return HTTP2_FRAME_HEADER_SIZE;
}

int http2_frame_window_update_encode(uint8_t *buf, size_t buflen, uint32_t stream_id,
				     uint32_t increment)
{
	if (buflen < HTTP2_WINDOW_UPDATE_FRAME_SIZE) {
		return -ENOBUFS;
	}

	http2_frame_header_encode(buf, HTTP2_WINDOW_UPDATE_FRAME_LEN, HTTP2_WINDOW_UPDATE_FRAME, 0,
				  stream_id);
	sys_put_be32(increment, buf + HTTP2_FRAME_HEADER_SIZE);

	return HTTP2_WINDOW_UPDATE_FRAME_SIZE;
}

int http2_frame_rst_stream_encode(uint8_t *buf, size_t buflen, uint32_t stream_id,
				  enum http2_error error)
{
	if (buflen < HTTP2_RST_STREAM_FRAME_SIZE) {
		return -ENOBUFS;
	}

	http2_frame_header_encode(buf, HTTP2_RST_STREAM_FRAME_LEN, HTTP2_RST_STREAM_FRAME, 0,
				  stream_id);
	sys_put_be32(error, buf + HTTP2_FRAME_HEADER_SIZE);

	return HTTP2_RST_STREAM_FRAME_SIZE;
}

int http2_frame_goaway_encode(uint8_t *buf, size_t buflen, uint32_t last_stream_id,
			      enum http2_error error)
{
	if (buflen < HTTP2_GOAWAY_FRAME_SIZE) {
		return -ENOBUFS;
	}

	http2_frame_header_encode(buf, HTTP2_GOAWAY_FRAME_LEN, HTTP2_GOAWAY_FRAME, 0, 0);
	sys_put_be32(last_stream_id, buf + HTTP2_FRAME_HEADER_SIZE);
	sys_put_be32(error, buf + HTTP2_FRAME_HEADER_SIZE + sizeof(uint32_t));

	return HTTP2_GOAWAY_FRAME_SIZE;
}

int http2_frame_ping_encode(uint8_t *buf, size_t buflen, bool ack, const uint8_t *opaque_data)
{
	if (buflen < HTTP2_PING_FRAME_SIZE) {
		return -ENOBUFS;
	}

	http2_frame_header_encode(buf, HTTP2_PING_FRAME_LEN, HTTP2_PING_FRAME,
				  ack ? HTTP2_FLAG_ACK : 0, 0);
	memcpy(buf + HTTP2_FRAME_HEADER_SIZE, opaque_data, HTTP2_PING_FRAME_LEN);

	return HTTP2_PING_FRAME_SIZE;
}

int http2_frame_parse_padded_field(struct http2_frame *frame, const uint8_t *data, size_t len)
{
	if (frame->length == 0) {
		return -EBADMSG;
	}

	if (len == 0) {
		return -EAGAIN;
	}

	frame->padding_len = data[0];
	frame->length--;

	/* The padding may take the whole payload, but not more (RFC 9113,
	 * Section 6.1).
	 */
	if (frame->length < frame->padding_len) {
		return -EBADMSG;
	}

	/* Take the padding out of the payload length, so that the payload can be
	 * processed without it. The padding itself is left for the caller to
	 * skip, based on frame->padding_len.
	 */
	frame->length -= frame->padding_len;
	frame->flags &= ~HTTP2_FLAG_PADDED;

	return 1;
}

int http2_frame_parse_priority_field(struct http2_frame *frame, const uint8_t *data, size_t len)
{
	ARG_UNUSED(data);

	if (frame->length < HTTP2_HEADERS_FRAME_PRIORITY_LEN) {
		return -EBADMSG;
	}

	if (len < HTTP2_HEADERS_FRAME_PRIORITY_LEN) {
		return -EAGAIN;
	}

	frame->length -= HTTP2_HEADERS_FRAME_PRIORITY_LEN;
	frame->flags &= ~HTTP2_FLAG_PRIORITY;

	return HTTP2_HEADERS_FRAME_PRIORITY_LEN;
}

const char *http2_frame_type_name(enum http2_frame_type type)
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
