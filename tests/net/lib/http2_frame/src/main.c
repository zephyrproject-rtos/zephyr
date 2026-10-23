/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/http/frame.h>
#include <zephyr/ztest.h>

#include "headers/http2_internal.h"

ZTEST(http2_frame, test_header_round_trip)
{
	static const uint8_t expected[] = {
		0x12, 0x34, 0x56, 0x01, 0x05, 0x07, 0x65, 0x43, 0x21,
	};
	uint8_t buf[HTTP2_FRAME_HEADER_SIZE];
	struct http2_frame frame;
	int ret;

	http2_frame_header_encode(buf, 0x123456, HTTP2_HEADERS_FRAME,
				  HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, 0x7654321);

	zassert_mem_equal(buf, expected, sizeof(expected), "unexpected frame header bytes");

	memset(&frame, 0xff, sizeof(frame));
	ret = http2_frame_header_parse(buf, sizeof(buf), &frame);
	zassert_ok(ret, "parse failed (%d)", ret);
	zassert_equal(frame.length, 0x123456, "length 0x%x", frame.length);
	zassert_equal(frame.type, HTTP2_HEADERS_FRAME, "type %u", frame.type);
	zassert_equal(frame.flags, HTTP2_FLAG_END_HEADERS | HTTP2_FLAG_END_STREAM, "flags 0x%x",
		      frame.flags);
	zassert_equal(frame.stream_identifier, 0x7654321, "stream 0x%x",
		      frame.stream_identifier);
	zassert_equal(frame.padding_len, 0, "padding length not reset");
}

ZTEST(http2_frame, test_header_parse_reserved_bit)
{
	/* The most significant bit of the stream identifier is reserved. */
	static const uint8_t buf[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x03 };
	struct http2_frame frame;
	int ret;

	ret = http2_frame_header_parse(buf, sizeof(buf), &frame);
	zassert_ok(ret, "parse failed (%d)", ret);
	zassert_equal(frame.stream_identifier, 3, "stream 0x%x", frame.stream_identifier);
}

ZTEST(http2_frame, test_header_parse_incomplete)
{
	static const uint8_t buf[HTTP2_FRAME_HEADER_SIZE] = { 0 };
	struct http2_frame frame;

	for (size_t len = 0; len < sizeof(buf); len++) {
		zassert_equal(http2_frame_header_parse(buf, len, &frame), -EAGAIN,
			      "%zu bytes accepted", len);
	}
}

ZTEST(http2_frame, test_settings)
{
	static const struct http2_setting settings[] = {
		{ HTTP2_SETTINGS_HEADER_TABLE_SIZE, 0 },
		{ HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, 0x01020304 },
	};
	static const uint8_t expected[] = {
		0x00, 0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x03, 0x01, 0x02, 0x03, 0x04,
	};
	uint8_t buf[HTTP2_SETTINGS_FRAME_SIZE(ARRAY_SIZE(settings))];
	int ret;

	ret = http2_frame_settings_encode(buf, sizeof(buf), settings, ARRAY_SIZE(settings));
	zassert_equal(ret, sizeof(expected), "length %d", ret);
	zassert_mem_equal(buf, expected, sizeof(expected), "unexpected SETTINGS bytes");

	ret = http2_frame_settings_encode(buf, sizeof(buf) - 1, settings, ARRAY_SIZE(settings));
	zassert_equal(ret, -ENOBUFS, "short buffer accepted (%d)", ret);

	/* No parameters is a valid, empty SETTINGS frame. */
	ret = http2_frame_settings_encode(buf, sizeof(buf), NULL, 0);
	zassert_equal(ret, HTTP2_FRAME_HEADER_SIZE, "length %d", ret);
}

ZTEST(http2_frame, test_settings_ack)
{
	static const uint8_t expected[] = { 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00 };
	uint8_t buf[HTTP2_FRAME_HEADER_SIZE];
	int ret;

	ret = http2_frame_settings_ack_encode(buf, sizeof(buf));
	zassert_equal(ret, sizeof(expected), "length %d", ret);
	zassert_mem_equal(buf, expected, sizeof(expected), "unexpected SETTINGS ACK bytes");

	ret = http2_frame_settings_ack_encode(buf, sizeof(buf) - 1);
	zassert_equal(ret, -ENOBUFS, "short buffer accepted (%d)", ret);
}

ZTEST(http2_frame, test_window_update)
{
	static const uint8_t expected[] = {
		0x00, 0x00, 0x04, 0x08, 0x00, 0x00, 0x00, 0x00, 0x05,
		0x00, 0x00, 0x80, 0x00,
	};
	uint8_t buf[HTTP2_WINDOW_UPDATE_FRAME_SIZE];
	int ret;

	ret = http2_frame_window_update_encode(buf, sizeof(buf), 5, 32768);
	zassert_equal(ret, sizeof(expected), "length %d", ret);
	zassert_mem_equal(buf, expected, sizeof(expected), "unexpected WINDOW_UPDATE bytes");

	ret = http2_frame_window_update_encode(buf, sizeof(buf) - 1, 5, 32768);
	zassert_equal(ret, -ENOBUFS, "short buffer accepted (%d)", ret);
}

ZTEST(http2_frame, test_rst_stream)
{
	static const uint8_t expected[] = {
		0x00, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x00, 0x07,
		0x00, 0x00, 0x00, 0x08,
	};
	uint8_t buf[HTTP2_RST_STREAM_FRAME_SIZE];
	int ret;

	ret = http2_frame_rst_stream_encode(buf, sizeof(buf), 7, HTTP2_ERROR_CANCEL);
	zassert_equal(ret, sizeof(expected), "length %d", ret);
	zassert_mem_equal(buf, expected, sizeof(expected), "unexpected RST_STREAM bytes");

	ret = http2_frame_rst_stream_encode(buf, sizeof(buf) - 1, 7, HTTP2_ERROR_CANCEL);
	zassert_equal(ret, -ENOBUFS, "short buffer accepted (%d)", ret);
}

ZTEST(http2_frame, test_goaway)
{
	static const uint8_t expected[] = {
		0x00, 0x00, 0x08, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x09,
		0x00, 0x00, 0x00, 0x02,
	};
	uint8_t buf[HTTP2_GOAWAY_FRAME_SIZE];
	int ret;

	ret = http2_frame_goaway_encode(buf, sizeof(buf), 9, HTTP2_ERROR_INTERNAL_ERROR);
	zassert_equal(ret, sizeof(expected), "length %d", ret);
	zassert_mem_equal(buf, expected, sizeof(expected), "unexpected GOAWAY bytes");

	ret = http2_frame_goaway_encode(buf, sizeof(buf) - 1, 9, HTTP2_ERROR_INTERNAL_ERROR);
	zassert_equal(ret, -ENOBUFS, "short buffer accepted (%d)", ret);
}

ZTEST(http2_frame, test_ping)
{
	static const uint8_t opaque[HTTP2_PING_FRAME_LEN] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	static const uint8_t expected_ack[] = {
		0x00, 0x00, 0x08, 0x06, 0x01, 0x00, 0x00, 0x00, 0x00,
		1, 2, 3, 4, 5, 6, 7, 8,
	};
	uint8_t buf[HTTP2_PING_FRAME_SIZE];
	int ret;

	ret = http2_frame_ping_encode(buf, sizeof(buf), true, opaque);
	zassert_equal(ret, sizeof(expected_ack), "length %d", ret);
	zassert_mem_equal(buf, expected_ack, sizeof(expected_ack), "unexpected PING ACK bytes");

	ret = http2_frame_ping_encode(buf, sizeof(buf), false, opaque);
	zassert_equal(ret, sizeof(expected_ack), "length %d", ret);
	zassert_equal(buf[HTTP2_FRAME_FLAGS_OFFSET], 0, "ACK flag set on a request");

	ret = http2_frame_ping_encode(buf, sizeof(buf) - 1, true, opaque);
	zassert_equal(ret, -ENOBUFS, "short buffer accepted (%d)", ret);
}

ZTEST(http2_frame, test_padded_field)
{
	static const uint8_t data[] = { 3 };
	struct http2_frame frame = {
		.length = 10,
		.flags = HTTP2_FLAG_PADDED | HTTP2_FLAG_END_STREAM,
	};
	int ret;

	ret = http2_frame_parse_padded_field(&frame, data, 0);
	zassert_equal(ret, -EAGAIN, "field parsed without data (%d)", ret);
	zassert_equal(frame.length, 10, "frame modified on -EAGAIN");

	ret = http2_frame_parse_padded_field(&frame, data, sizeof(data));
	zassert_equal(ret, 1, "consumed %d", ret);
	zassert_equal(frame.padding_len, 3, "padding length %u", frame.padding_len);
	/* 10 bytes of payload: the field, 6 bytes of content, 3 of padding. */
	zassert_equal(frame.length, 6, "content length %u", frame.length);
	zassert_equal(frame.flags, HTTP2_FLAG_END_STREAM, "flags 0x%x", frame.flags);
}

ZTEST(http2_frame, test_padded_field_invalid)
{
	static const uint8_t data[] = { 9 };
	struct http2_frame frame = {
		.length = 0,
		.flags = HTTP2_FLAG_PADDED,
	};
	int ret;

	/* No room for the Pad Length field itself. */
	ret = http2_frame_parse_padded_field(&frame, data, sizeof(data));
	zassert_equal(ret, -EBADMSG, "empty padded frame accepted (%d)", ret);

	/* Padding that leaves no content is valid. */
	frame.length = 10;
	frame.flags = HTTP2_FLAG_PADDED;
	ret = http2_frame_parse_padded_field(&frame, data, sizeof(data));
	zassert_equal(ret, 1, "padding-only frame rejected (%d)", ret);
	zassert_equal(frame.length, 0, "content length %u", frame.length);
	zassert_equal(frame.padding_len, 9, "padding length %u", frame.padding_len);

	/* Padding larger than the payload is not. */
	frame.length = 9;
	frame.flags = HTTP2_FLAG_PADDED;
	ret = http2_frame_parse_padded_field(&frame, data, sizeof(data));
	zassert_equal(ret, -EBADMSG, "oversized padding accepted (%d)", ret);
}

ZTEST(http2_frame, test_priority_field)
{
	static const uint8_t data[HTTP2_HEADERS_FRAME_PRIORITY_LEN] = { 0 };
	struct http2_frame frame = {
		.length = 20,
		.flags = HTTP2_FLAG_PRIORITY | HTTP2_FLAG_END_HEADERS,
	};
	int ret;

	ret = http2_frame_parse_priority_field(&frame, data, sizeof(data) - 1);
	zassert_equal(ret, -EAGAIN, "partial field parsed (%d)", ret);
	zassert_equal(frame.length, 20, "frame modified on -EAGAIN");

	ret = http2_frame_parse_priority_field(&frame, data, sizeof(data));
	zassert_equal(ret, HTTP2_HEADERS_FRAME_PRIORITY_LEN, "consumed %d", ret);
	zassert_equal(frame.length, 15, "content length %u", frame.length);
	zassert_equal(frame.flags, HTTP2_FLAG_END_HEADERS, "flags 0x%x", frame.flags);

	frame.length = HTTP2_HEADERS_FRAME_PRIORITY_LEN - 1;
	frame.flags = HTTP2_FLAG_PRIORITY;
	ret = http2_frame_parse_priority_field(&frame, data, sizeof(data));
	zassert_equal(ret, -EBADMSG, "field larger than the frame accepted (%d)", ret);
}

ZTEST(http2_frame, test_type_name)
{
	zassert_str_equal(http2_frame_type_name(HTTP2_DATA_FRAME), "DATA");
	zassert_str_equal(http2_frame_type_name(HTTP2_HEADERS_FRAME), "HEADERS");
	zassert_str_equal(http2_frame_type_name(HTTP2_PRIORITY_FRAME), "PRIORITY");
	zassert_str_equal(http2_frame_type_name(HTTP2_RST_STREAM_FRAME), "RST_STREAM");
	zassert_str_equal(http2_frame_type_name(HTTP2_SETTINGS_FRAME), "SETTINGS");
	zassert_str_equal(http2_frame_type_name(HTTP2_PUSH_PROMISE_FRAME), "PUSH_PROMISE");
	zassert_str_equal(http2_frame_type_name(HTTP2_PING_FRAME), "PING");
	zassert_str_equal(http2_frame_type_name(HTTP2_GOAWAY_FRAME), "GOAWAY");
	zassert_str_equal(http2_frame_type_name(HTTP2_WINDOW_UPDATE_FRAME), "WINDOW_UPDATE");
	zassert_str_equal(http2_frame_type_name(HTTP2_CONTINUATION_FRAME), "CONTINUATION");
	zassert_str_equal(http2_frame_type_name(0x7f), "UNKNOWN");
}

ZTEST_SUITE(http2_frame, NULL, NULL, NULL, NULL, NULL);
