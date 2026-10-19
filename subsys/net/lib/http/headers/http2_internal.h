/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP2_INTERNAL_H_
#define HTTP2_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/net/http/frame.h>

/* HTTP/2 frame encoding and parsing shared by the HTTP server and client. All
 * functions work on caller-provided buffers and perform no I/O.
 */

/* Size of one SETTINGS parameter on the wire: 16-bit identifier, 32-bit value. */
#define HTTP2_SETTINGS_FIELD_LEN 6

/* Initial SETTINGS_MAX_FRAME_SIZE value (RFC 9113, Section 6.5.2), also the
 * smallest a peer may set, and the largest.
 */
#define HTTP2_DEFAULT_MAX_FRAME_SIZE 16384U

/* Dynamic table size an HPACK decoder has to accept until its own
 * SETTINGS_HEADER_TABLE_SIZE is acknowledged (RFC 7541, Section 6.3).
 */
#define HTTP2_DEFAULT_HEADER_TABLE_SIZE 4096U
#define HTTP2_MAX_FRAME_SIZE_LIMIT   16777215U

/* Initial flow-control window size (RFC 9113, Section 6.9.2) and the largest
 * one (Section 6.9.1).
 */
#define HTTP2_INITIAL_WINDOW_SIZE 65535
#define HTTP2_MAX_WINDOW_SIZE     0x7fffffff

/* Buffer sizes needed for the fixed-size frames built below. */
#define HTTP2_SETTINGS_FRAME_SIZE(count)                                                           \
	(HTTP2_FRAME_HEADER_SIZE + (count) * HTTP2_SETTINGS_FIELD_LEN)
#define HTTP2_WINDOW_UPDATE_FRAME_SIZE (HTTP2_FRAME_HEADER_SIZE + HTTP2_WINDOW_UPDATE_FRAME_LEN)
#define HTTP2_RST_STREAM_FRAME_SIZE    (HTTP2_FRAME_HEADER_SIZE + HTTP2_RST_STREAM_FRAME_LEN)
#define HTTP2_GOAWAY_FRAME_SIZE        (HTTP2_FRAME_HEADER_SIZE + HTTP2_GOAWAY_FRAME_LEN)
#define HTTP2_PING_FRAME_SIZE          (HTTP2_FRAME_HEADER_SIZE + HTTP2_PING_FRAME_LEN)

/* A SETTINGS parameter in host byte order. */
struct http2_setting {
	uint16_t id;
	uint32_t value;
};

/* Write a 9-byte frame header. buf must hold HTTP2_FRAME_HEADER_SIZE bytes. */
void http2_frame_header_encode(uint8_t *buf, uint32_t payload_len, enum http2_frame_type type,
			       uint8_t flags, uint32_t stream_id);

/* Parse a frame header into frame.
 *
 * Returns 0 on success, -EAGAIN when fewer than HTTP2_FRAME_HEADER_SIZE bytes
 * are available.
 */
int http2_frame_header_parse(const uint8_t *buf, size_t buflen, struct http2_frame *frame);

/* Frame builders. Each writes one complete frame into buf and returns its
 * length, or -ENOBUFS when buflen is too small.
 */
int http2_frame_settings_encode(uint8_t *buf, size_t buflen,
				const struct http2_setting *settings, size_t count);
int http2_frame_settings_ack_encode(uint8_t *buf, size_t buflen);
int http2_frame_window_update_encode(uint8_t *buf, size_t buflen, uint32_t stream_id,
				     uint32_t increment);
int http2_frame_rst_stream_encode(uint8_t *buf, size_t buflen, uint32_t stream_id,
				  enum http2_error error);
int http2_frame_goaway_encode(uint8_t *buf, size_t buflen, uint32_t last_stream_id,
			      enum http2_error error);
int http2_frame_ping_encode(uint8_t *buf, size_t buflen, bool ack, const uint8_t *opaque_data);

/* Consume the Pad Length field at the start of the payload of a frame with the
 * PADDED flag. On success frame->padding_len holds the amount of trailing
 * padding, frame->length the payload left without that field and the padding,
 * and the PADDED flag is cleared.
 *
 * Returns the number of bytes consumed, -EAGAIN when no data is available,
 * -EBADMSG when the padding does not fit the frame.
 */
int http2_frame_parse_padded_field(struct http2_frame *frame, const uint8_t *data, size_t len);

/* Consume the priority field at the start of the payload of a HEADERS frame
 * with the PRIORITY flag. Its content is ignored, priority signalling being
 * deprecated by RFC 9113. On success frame->length is reduced accordingly and
 * the PRIORITY flag is cleared.
 *
 * Returns the number of bytes consumed, -EAGAIN when the field is not fully
 * available, -EBADMSG when it does not fit the frame.
 */
int http2_frame_parse_priority_field(struct http2_frame *frame, const uint8_t *data, size_t len);

/* Name of a frame type for logging, "UNKNOWN" for types not defined by RFC 9113. */
const char *http2_frame_type_name(enum http2_frame_type type);

#endif /* HTTP2_INTERNAL_H_ */
