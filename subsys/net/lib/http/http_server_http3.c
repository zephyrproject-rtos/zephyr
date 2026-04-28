/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/http/service.h>
#include <zephyr/net/http/server.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/quic.h>

#if defined(CONFIG_FILE_SYSTEM)
#include <zephyr/fs/fs.h>
#endif

LOG_MODULE_DECLARE(net_http_server, CONFIG_NET_HTTP_SERVER_LOG_LEVEL);

#include "headers/server_internal.h"

/* Huffman decoder from http_huffman.c (same table used by HPACK and QPACK) */
int http_hpack_huffman_decode(const uint8_t *encoded_buf, size_t encoded_len,
			      uint8_t *buf, size_t buflen);

/* HTTP/3 frame types (RFC 9114 Section 7.2) */
#define H3_FRAME_DATA           0x00
#define H3_FRAME_HEADERS        0x01
#define H3_FRAME_CANCEL_PUSH    0x03
#define H3_FRAME_SETTINGS       0x04
#define H3_FRAME_PUSH_PROMISE   0x05
#define H3_FRAME_GOAWAY         0x07
#define H3_FRAME_MAX_PUSH_ID    0x0d

/* HTTP/3 Settings identifiers (RFC 9114 Section 7.2.4.1) */
#define H3_SETTINGS_QPACK_MAX_TABLE_CAPACITY  0x01
#define H3_SETTINGS_MAX_FIELD_SECTION_SIZE    0x06
#define H3_SETTINGS_QPACK_BLOCKED_STREAMS     0x07

/* HTTP/3 unidirectional stream types (RFC 9114 Section 6.2) */
#define H3_UNI_STREAM_CONTROL        0x00
#define H3_UNI_STREAM_PUSH           0x01
#define H3_UNI_STREAM_QPACK_ENCODER  0x02
#define H3_UNI_STREAM_QPACK_DECODER  0x03

/* HTTP/3 error codes (RFC 9114 Section 8.1) */
#define H3_NO_ERROR                  0x0100
#define H3_GENERAL_PROTOCOL_ERROR    0x0101
#define H3_INTERNAL_ERROR            0x0102
#define H3_STREAM_CREATION_ERROR     0x0103
#define H3_CLOSED_CRITICAL_STREAM    0x0104
#define H3_FRAME_UNEXPECTED          0x0105
#define H3_FRAME_ERROR               0x0106
#define H3_EXCESSIVE_LOAD            0x0107
#define H3_ID_ERROR                  0x0108
#define H3_SETTINGS_ERROR            0x0109
#define H3_MISSING_SETTINGS          0x010a
#define H3_REQUEST_REJECTED          0x010b
#define H3_REQUEST_CANCELLED         0x010c
#define H3_REQUEST_INCOMPLETE        0x010d
#define H3_MESSAGE_ERROR             0x010e
#define H3_CONNECT_ERROR             0x010f
#define H3_VERSION_FALLBACK          0x0110

/* QPACK error codes (RFC 9204 Section 6) */
#define QPACK_DECOMPRESSION_FAILED   0x0200
#define QPACK_ENCODER_STREAM_ERROR   0x0201
#define QPACK_DECODER_STREAM_ERROR   0x0202

/* QPACK static table indices (RFC 9204 Appendix A) */
#define QPACK_STATIC_AUTHORITY          0
#define QPACK_STATIC_PATH_SLASH         1
#define QPACK_STATIC_METHOD_CONNECT     15
#define QPACK_STATIC_METHOD_DELETE      16
#define QPACK_STATIC_METHOD_GET         17
#define QPACK_STATIC_METHOD_HEAD        18
#define QPACK_STATIC_METHOD_OPTIONS     19
#define QPACK_STATIC_METHOD_POST        20
#define QPACK_STATIC_METHOD_PUT         21
#define QPACK_STATIC_SCHEME_HTTP        22
#define QPACK_STATIC_SCHEME_HTTPS       23
#define QPACK_STATIC_STATUS_200         25
#define QPACK_STATIC_STATUS_304         26
#define QPACK_STATIC_STATUS_404         27
#define QPACK_STATIC_STATUS_503         28
#define QPACK_STATIC_CONTENT_TYPE_HTML  52
#define QPACK_STATIC_PATH_INDEX_HTML    63

/* QPACK header field representation prefixes (RFC 9204 Section 4.5) */
#define QPACK_INDEXED_STATIC         0xc0  /* 11xxxxxx: indexed, static table */
#define QPACK_INDEXED_STATIC_MASK    0xc0
#define QPACK_LIT_NAME_REF_STATIC    0x50  /* 0101xxxx: literal with name ref, static */
#define QPACK_LIT_NAME_REF_MASK      0xf0
#define QPACK_LIT_WITHOUT_NAME_REF   0x20  /* 001xxxxx: literal without name ref */
#define QPACK_LIT_WITHOUT_NAME_MASK  0xe0

/* Maximum size of an H3 response header block buffer.
 * 2 (QPACK prefix) + ~128 bytes of encoded headers should suffice.
 */
#define H3_MAX_HEADER_BUF_SIZE  192

/* Maximum size of a varint-encoded frame header (type + length).
 * Each varint can be up to 8 bytes, so 16 max.
 */
#define H3_FRAME_HEADER_MAX_SIZE 16

/* Maximum size of a Huffman decoded string */
#define H3_HUFFMAN_DECODE_BUF_SIZE 256

/* Per-connection H3 context (simplified: indexed by slot) */
static struct h3_conn_ctx h3_conn_contexts[CONFIG_HTTP_SERVER_MAX_CLIENTS];

/* Track which connection fd each context belongs to (-1 = unused) */
static int h3_conn_ctx_fds[CONFIG_HTTP_SERVER_MAX_CLIENTS] = {
	[0 ... CONFIG_HTTP_SERVER_MAX_CLIENTS - 1] = -1
};

struct h3_conn_ctx *h3_get_conn_ctx(struct http_client_ctx *client)
{
	int conn_fd = client->h3.conn_sock;
	int i;

	if (conn_fd < 0) {
		return NULL;
	}

	/* Find existing context for this connection */
	for (i = 0; i < CONFIG_HTTP_SERVER_MAX_CLIENTS; i++) {
		if (h3_conn_ctx_fds[i] == conn_fd && h3_conn_contexts[i].initialized) {
			return &h3_conn_contexts[i];
		}
	}

	/* Allocate new context */
	for (i = 0; i < CONFIG_HTTP_SERVER_MAX_CLIENTS; i++) {
		if (!h3_conn_contexts[i].initialized) {
			h3_conn_ctx_fds[i] = conn_fd;
			return &h3_conn_contexts[i];
		}
	}

	return NULL;
}

static void h3_init_conn_ctx(struct h3_conn_ctx *ctx)
{
	if (ctx == NULL || ctx->initialized) {
		return;
	}

	ctx->peer_control_stream = INVALID_SOCK;
	ctx->peer_qpack_encoder_stream = INVALID_SOCK;
	ctx->peer_qpack_decoder_stream = INVALID_SOCK;
	ctx->local_control_stream = INVALID_SOCK;
	ctx->local_qpack_encoder_stream = INVALID_SOCK;
	ctx->local_qpack_decoder_stream = INVALID_SOCK;
	ctx->peer_control_rx.data_len = 0;
	ctx->peer_qpack_encoder_rx.data_len = 0;
	ctx->peer_qpack_decoder_rx.data_len = 0;

	ctx->peer_settings.qpack_max_table_capacity = 0;
	ctx->peer_settings.max_field_section_size = UINT64_MAX;
	ctx->peer_settings.qpack_blocked_streams = 0;
	ctx->peer_settings.received = false;

	ctx->settings_sent = false;
	ctx->initialized = true;
}

static struct h3_uni_stream_rx_ctx *h3_get_peer_uni_stream_rx_ctx(struct h3_conn_ctx *ctx,
								  int fd)
{
	if (ctx == NULL) {
		return NULL;
	}

	if (fd == ctx->peer_control_stream) {
		return &ctx->peer_control_rx;
	}

	if (fd == ctx->peer_qpack_encoder_stream) {
		return &ctx->peer_qpack_encoder_rx;
	}

	if (fd == ctx->peer_qpack_decoder_stream) {
		return &ctx->peer_qpack_decoder_rx;
	}

	return NULL;
}

static int h3_store_peer_uni_stream_data(struct h3_uni_stream_rx_ctx *rx_ctx,
					 const uint8_t *buf, size_t buflen)
{
	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	if (buflen > sizeof(rx_ctx->buffer)) {
		return -ENOBUFS;
	}

	if (buflen > 0) {
		memcpy(rx_ctx->buffer, buf, buflen);
	}

	rx_ctx->data_len = buflen;

	return 0;
}

static void h3_compact_peer_uni_stream_data(struct h3_uni_stream_rx_ctx *rx_ctx,
					    size_t consumed)
{
	if (rx_ctx == NULL || consumed == 0) {
		return;
	}

	if (consumed >= rx_ctx->data_len) {
		rx_ctx->data_len = 0;
		return;
	}

	memmove(rx_ctx->buffer, rx_ctx->buffer + consumed,
		rx_ctx->data_len - consumed);
	rx_ctx->data_len -= consumed;
}

static int h3_recv_peer_uni_stream_data(struct h3_uni_stream_rx_ctx *rx_ctx, int fd)
{
	ssize_t len;

	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	if (rx_ctx->data_len == sizeof(rx_ctx->buffer)) {
		return 0;
	}

	len = zsock_recv(fd, rx_ctx->buffer + rx_ctx->data_len,
			 sizeof(rx_ctx->buffer) - rx_ctx->data_len,
			 ZSOCK_MSG_DONTWAIT);
	if (len < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			return 0;
		}

		return -errno;
	}

	if (len == 0) {
		return -H3_CLOSED_CRITICAL_STREAM;
	}

	rx_ctx->data_len += (size_t)len;

	return (int)len;
}

/**
 * Parse a QUIC variable-length integer (RFC 9000 Section 16).
 *
 * @param buf    Input buffer.
 * @param buflen Available bytes in buffer.
 * @param value  Output: parsed integer value.
 *
 * @return Number of bytes consumed, or -EINVAL on error.
 */
static int h3_parse_varint(const uint8_t *buf, size_t buflen, uint64_t *value)
{
	uint8_t prefix;
	int len;

	if (buflen == 0) {
		return -EINVAL;
	}

	prefix = buf[0] >> 6;
	len = 1 << prefix;

	if ((size_t)len > buflen) {
		return -EINVAL;
	}

	*value = buf[0] & (0x3f);

	for (int i = 1; i < len; i++) {
		*value = (*value << 8) | buf[i];
	}

	return len;
}

/**
 * Encode a QUIC variable-length integer (RFC 9000 Section 16).
 *
 * @param buf    Output buffer.
 * @param buflen Available space.
 * @param value  Value to encode.
 *
 * @return Number of bytes written, or <0 on error.
 */
static int h3_encode_varint(uint8_t *buf, size_t buflen, uint64_t value)
{
	if (value <= 0x3f) {
		if (buflen < 1) {
			return -ENOSPC;
		}

		buf[0] = (uint8_t)value;
		return 1;
	}

	if (value <= 0x3fff) {
		if (buflen < 2) {
			return -ENOSPC;
		}

		buf[0] = (uint8_t)((value >> 8) | 0x40);
		buf[1] = (uint8_t)(value & 0xff);
		return 2;
	}

	if (value <= 0x3fffffff) {
		if (buflen < 4) {
			return -ENOSPC;
		}

		buf[0] = (uint8_t)((value >> 24) | 0x80);
		buf[1] = (uint8_t)((value >> 16) & 0xff);
		buf[2] = (uint8_t)((value >> 8) & 0xff);
		buf[3] = (uint8_t)(value & 0xff);
		return 4;
	}

	if (buflen < 8) {
		return -ENOSPC;
	}

	buf[0] = (uint8_t)((value >> 56) | 0xc0);
	buf[1] = (uint8_t)((value >> 48) & 0xff);
	buf[2] = (uint8_t)((value >> 40) & 0xff);
	buf[3] = (uint8_t)((value >> 32) & 0xff);
	buf[4] = (uint8_t)((value >> 24) & 0xff);
	buf[5] = (uint8_t)((value >> 16) & 0xff);
	buf[6] = (uint8_t)((value >> 8) & 0xff);
	buf[7] = (uint8_t)(value & 0xff);
	return 8;
}

/**
 * Parse an HTTP/3 frame header (type + length).
 *
 * @param buf       Input buffer.
 * @param buflen    Available bytes.
 * @param type      Output: frame type.
 * @param frame_len Output: frame payload length.
 *
 * @return Number of bytes consumed by the frame header, or <0 on error.
 */
static int h3_parse_frame_header(const uint8_t *buf, size_t buflen,
				 uint64_t *type, uint64_t *frame_len)
{
	int consumed = 0;
	int ret;

	ret = h3_parse_varint(buf, buflen, type);
	if (ret < 0) {
		return ret;
	}

	consumed += ret;

	ret = h3_parse_varint(buf + consumed, buflen - consumed, frame_len);
	if (ret < 0) {
		return ret;
	}

	consumed += ret;

	return consumed;
}

/**
 * Decode a QPACK integer with the given prefix length.
 *
 * @param buf       Input buffer.
 * @param buflen    Available bytes.
 * @param prefix_n  Number of prefix bits (e.g. 6, 4, 3).
 * @param value     Output: decoded integer.
 *
 * @return Number of bytes consumed, or <0 on error.
 */
static int qpack_decode_int(const uint8_t *buf, size_t buflen,
			    int prefix_n, uint64_t *value)
{
	uint8_t max_first;
	int consumed;
	uint64_t shift;
	uint8_t b;

	if (buflen == 0) {
		return -EINVAL;
	}

	max_first = (1 << prefix_n) - 1;
	*value = buf[0] & max_first;
	consumed = 1;

	if (*value < max_first) {
		return consumed;
	}

	/* Multi-byte integer */
	shift = 0;

	do {
		if (consumed >= (int)buflen) {
			return -EINVAL;
		}

		b = buf[consumed];
		*value += (uint64_t)(b & 0x7f) << shift;
		shift += 7;
		consumed++;
	} while (b & 0x80);

	return consumed;
}

/**
 * Encode a QPACK integer with the given prefix.
 *
 * @param buf       Output buffer.
 * @param buflen    Available space.
 * @param prefix_n  Number of prefix bits.
 * @param prefix    Prefix byte (high bits already set).
 * @param value     Value to encode.
 *
 * @return Number of bytes written, or <0 on error.
 */
static int qpack_encode_int(uint8_t *buf, size_t buflen,
			    int prefix_n, uint8_t prefix, uint64_t value)
{
	uint8_t max_first;
	int pos = 0;

	if (buflen == 0) {
		return -ENOSPC;
	}

	max_first = (1 << prefix_n) - 1;

	if (value < max_first) {
		buf[0] = prefix | (uint8_t)value;
		return 1;
	}

	buf[pos++] = prefix | max_first;
	value -= max_first;

	while (value >= 128) {
		if (pos >= (int)buflen) {
			return -ENOSPC;
		}

		buf[pos++] = (uint8_t)(0x80 | (value & 0x7f));
		value >>= 7;
	}

	if (pos >= (int)buflen) {
		return -ENOSPC;
	}

	buf[pos++] = (uint8_t)value;

	return pos;
}

/**
 * Look up a method string from a QPACK static table index.
 *
 * @param index  Static table index.
 * @param method Output: HTTP method enum value.
 *
 * @return 0 on success, -ENOENT if the index doesn't map to a method.
 */
static int qpack_static_method(uint64_t index, enum http_method *method)
{
	switch (index) {
	case QPACK_STATIC_METHOD_GET:
		*method = HTTP_GET;
		return 0;
	case QPACK_STATIC_METHOD_POST:
		*method = HTTP_POST;
		return 0;
	case QPACK_STATIC_METHOD_PUT:
		*method = HTTP_PUT;
		return 0;
	case QPACK_STATIC_METHOD_DELETE:
		*method = HTTP_DELETE;
		return 0;
	case QPACK_STATIC_METHOD_HEAD:
		*method = HTTP_HEAD;
		return 0;
	case QPACK_STATIC_METHOD_OPTIONS:
		*method = HTTP_OPTIONS;
		return 0;
	case QPACK_STATIC_METHOD_CONNECT:
		*method = HTTP_CONNECT;
		return 0;
	default:
		return -ENOENT;
	}
}

/**
 * Look up a path string from a QPACK static table index.
 *
 * @param index Static table index.
 *
 * @return Path string, or NULL if the index doesn't map to a known path.
 */
static const char *qpack_static_path(uint64_t index)
{
	switch (index) {
	case QPACK_STATIC_PATH_SLASH:
		return "/";
	case QPACK_STATIC_PATH_INDEX_HTML:
		return "/index.html";
	default:
		return NULL;
	}
}

/**
 * Get the QPACK static table index for a status code, if one exists.
 *
 * @param status HTTP status code.
 *
 * @return Static table index, or -1 if no match.
 */
static int qpack_static_status_index(enum http_status status)
{
	switch (status) {
	case HTTP_200_OK:
		return QPACK_STATIC_STATUS_200;
	case HTTP_304_NOT_MODIFIED:
		return QPACK_STATIC_STATUS_304;
	case HTTP_404_NOT_FOUND:
		return QPACK_STATIC_STATUS_404;
	case HTTP_503_SERVICE_UNAVAILABLE:
		return QPACK_STATIC_STATUS_503;
	default:
		return -1;
	}
}

/**
 * Encode QPACK response headers into a buffer.
 *
 * Encodes:
 * - Required Insert Count = 0, Delta Base = 0 (no dynamic table)
 * - :status via indexed if possible, else literal with name ref
 * - content-type from resource detail
 * - content-encoding from resource detail
 * - any extra headers
 *
 * @param buf           Output buffer.
 * @param buflen        Available space.
 * @param status        HTTP status code.
 * @param detail        Resource detail (may be NULL).
 * @param extra_headers Array of extra headers (may be NULL).
 * @param extra_count   Number of extra headers.
 *
 * @return Number of bytes written, or <0 on error.
 */
static int h3_encode_response_headers(uint8_t *buf, size_t buflen,
				      enum http_status status,
				      struct http_resource_detail *detail,
				      const struct http_header *extra_headers,
				      size_t extra_count)
{
	int pos = 0;
	int ret;
	int static_idx;
	bool content_type_sent = false;
	bool content_encoding_sent = false;
	size_t i;

	/* QPACK prefix: Required Insert Count = 0, Delta Base = 0 */
	if (buflen < 2) {
		return -ENOSPC;
	}

	buf[pos++] = 0x00; /* Required Insert Count = 0 */
	buf[pos++] = 0x00; /* Delta Base = 0 */

	/* Encode :status */
	static_idx = qpack_static_status_index(status);

	if (static_idx >= 0) {
		/* Indexed field line, use static table */
		ret = qpack_encode_int(buf + pos, buflen - pos, 6,
				       QPACK_INDEXED_STATIC, static_idx);
		if (ret < 0) {
			return ret;
		}

		pos += ret;
	} else {
		/* Literal with name reference to :status (index 24 = :status 103,
		 * but we just need a :status name ref). Index 25 = :status 200.
		 * We use index 25 as name reference for :status.
		 */
		uint8_t status_str[4];
		int status_len;

		status_len = snprintf((char *)status_str, sizeof(status_str),
				      "%d", status);
		if (status_len < 0 || status_len >= (int)sizeof(status_str)) {
			return -EINVAL;
		}

		/* Literal with name ref, static, never-index=0: 0101NNNN */
		ret = qpack_encode_int(buf + pos, buflen - pos, 4,
				       QPACK_LIT_NAME_REF_STATIC,
				       QPACK_STATIC_STATUS_200);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		/* Value length + value (no Huffman) */
		ret = qpack_encode_int(buf + pos, buflen - pos, 7,
				       0x00, status_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + status_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, status_str, status_len);
		pos += status_len;
	}

	/* Encode extra headers */
	for (i = 0; i < extra_count; i++) {
		const struct http_header *hdr = &extra_headers[i];
		size_t name_len = strlen(hdr->name);
		size_t val_len = strlen(hdr->value);

		if (strcasecmp(hdr->name, "content-type") == 0) {
			content_type_sent = true;
		}

		if (strcasecmp(hdr->name, "content-encoding") == 0) {
			content_encoding_sent = true;
		}

		/* Literal without name reference: 001NNNNN */
		ret = qpack_encode_int(buf + pos, buflen - pos, 3,
				       QPACK_LIT_WITHOUT_NAME_REF, name_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + (int)name_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, hdr->name, name_len);
		pos += name_len;

		/* Value: no Huffman */
		ret = qpack_encode_int(buf + pos, buflen - pos, 7,
				       0x00, val_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + (int)val_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, hdr->value, val_len);
		pos += val_len;
	}

	/* content-type from resource detail */
	if (!content_type_sent && detail != NULL &&
	    detail->content_type != NULL) {
		size_t name_len = sizeof("content-type") - 1;
		size_t val_len = strlen(detail->content_type);

		ret = qpack_encode_int(buf + pos, buflen - pos, 3,
				       QPACK_LIT_WITHOUT_NAME_REF, name_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + (int)name_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, "content-type", name_len);
		pos += name_len;

		ret = qpack_encode_int(buf + pos, buflen - pos, 7,
				       0x00, val_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + (int)val_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, detail->content_type, val_len);
		pos += val_len;
	}

	/* content-encoding from resource detail */
	if (!content_encoding_sent && detail != NULL &&
	    detail->content_encoding != NULL) {
		size_t name_len = sizeof("content-encoding") - 1;
		size_t val_len = strlen(detail->content_encoding);

		ret = qpack_encode_int(buf + pos, buflen - pos, 3,
				       QPACK_LIT_WITHOUT_NAME_REF, name_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + (int)name_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, "content-encoding", name_len);
		pos += name_len;

		ret = qpack_encode_int(buf + pos, buflen - pos, 7,
				       0x00, val_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		if (pos + (int)val_len > (int)buflen) {
			return -ENOSPC;
		}

		memcpy(buf + pos, detail->content_encoding, val_len);
		pos += val_len;
	}

	return pos;
}

/**
 * Send an HTTP/3 HEADERS frame.
 *
 * @param client        Client context.
 * @param status        HTTP status code.
 * @param detail        Resource detail (may be NULL).
 * @param extra_headers Extra headers (may be NULL).
 * @param extra_count   Number of extra headers.
 *
 * @return 0 on success, <0 on error.
 */
static int h3_send_headers_frame(struct http_client_ctx *client,
				 enum http_status status,
				 struct http_resource_detail *detail,
				 const struct http_header *extra_headers,
				 size_t extra_count)
{
	uint8_t frame_buf[H3_FRAME_HEADER_MAX_SIZE + H3_MAX_HEADER_BUF_SIZE];
	uint8_t header_block[H3_MAX_HEADER_BUF_SIZE];
	int header_len;
	int frame_hdr_len;
	int ret;

	header_len = h3_encode_response_headers(header_block,
						sizeof(header_block),
						status, detail,
						extra_headers, extra_count);
	if (header_len < 0) {
		LOG_DBG("[%p] H3: failed to encode response headers (%d)",
			client, header_len);
		return header_len;
	}

	/* Encode frame header: type=HEADERS(0x01), length=header_len */
	frame_hdr_len = h3_encode_varint(frame_buf, sizeof(frame_buf),
					 H3_FRAME_HEADERS);
	if (frame_hdr_len < 0) {
		return frame_hdr_len;
	}

	ret = h3_encode_varint(frame_buf + frame_hdr_len,
			       sizeof(frame_buf) - frame_hdr_len,
			       header_len);
	if (ret < 0) {
		return ret;
	}

	frame_hdr_len += ret;

	/* Copy header block after frame header */
	if (frame_hdr_len + header_len > (int)sizeof(frame_buf)) {
		return -ENOSPC;
	}

	memcpy(frame_buf + frame_hdr_len, header_block, header_len);

	ret = http_server_sendall(client, frame_buf,
				  frame_hdr_len + header_len);
	if (ret < 0) {
		LOG_DBG("[%p] H3: cannot write headers to socket (%d)",
			client, ret);
		return ret;
	}

	LOG_DBG("[%p] H3: sent HEADERS frame (%d bytes, status=%d)",
		client, frame_hdr_len + header_len, status);

	return 0;
}

/**
 * Send an HTTP/3 DATA frame.
 *
 * @param client  Client context.
 * @param payload Data to send (may be NULL if length is 0).
 * @param length  Payload length.
 *
 * @return 0 on success, <0 on error.
 */
static int h3_send_data_frame(struct http_client_ctx *client,
			      const void *payload, size_t length)
{
	uint8_t frame_hdr[H3_FRAME_HEADER_MAX_SIZE];
	int frame_hdr_len;
	int ret;

	/* Encode frame header: type=DATA(0x00), length */
	frame_hdr_len = h3_encode_varint(frame_hdr, sizeof(frame_hdr),
					 H3_FRAME_DATA);
	if (frame_hdr_len < 0) {
		return frame_hdr_len;
	}

	ret = h3_encode_varint(frame_hdr + frame_hdr_len,
			       sizeof(frame_hdr) - frame_hdr_len,
			       length);
	if (ret < 0) {
		return ret;
	}

	frame_hdr_len += ret;

	/* Send frame header */
	ret = http_server_sendall(client, frame_hdr, frame_hdr_len);
	if (ret < 0) {
		LOG_DBG("[%p] H3: cannot write data frame header (%d)",
			client, ret);
		return ret;
	}

	/* Send payload */
	if (payload != NULL && length > 0) {
		ret = http_server_sendall(client, payload, length);
		if (ret < 0) {
			LOG_DBG("[%p] H3: cannot write data payload (%d)",
				client, ret);
			return ret;
		}
	}

	LOG_DBG("[%p] H3: sent DATA frame (%zu bytes)", client, length);

	return 0;
}

/**
 * Signal end-of-response on the current request stream.
 *
 * Sends a QUIC stream FIN so the client knows the response is complete
 * (RFC 9114 ch. 4.1). Must be called once, after the last DATA frame.
 * The caller is responsible for closing the fd afterwards.
 */
static int h3_end_response(struct http_client_ctx *client)
{
	/*
	 * Full half-close: send FIN (done sending) and STOP_SENDING (done
	 * reading). For GET/HEAD/DELETE the client typically sends no body,
	 * but SHUT_RDWR is harmless and guards against clients that
	 * erroneously send DATA frames after the HEADERS.
	 */
	int ret;
	uint64_t stop_code = H3_NO_ERROR;

	(void)zsock_setsockopt(client->fd, ZSOCK_SOL_QUIC,
			       ZSOCK_QUIC_SO_STOP_SENDING_CODE,
			       &stop_code, sizeof(stop_code));

	ret = zsock_shutdown(client->fd, ZSOCK_SHUT_RDWR);
	if (ret < 0) {
		LOG_DBG("[%p] H3: shutdown(SHUT_RDWR) failed (%d)", client, ret);
	}

	return ret;
}

/**
 * Send HTTP/3 404 Not Found response.
 */
static int h3_send_404(struct http_client_ctx *client)
{
	int ret;

	ret = h3_send_headers_frame(client, HTTP_404_NOT_FOUND,
				    NULL, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = h3_send_data_frame(client, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return h3_end_response(client);
}

/**
 * Send HTTP/3 405 Method Not Allowed response.
 */
static int h3_send_405(struct http_client_ctx *client)
{
	int ret;

	ret = h3_send_headers_frame(client, HTTP_405_METHOD_NOT_ALLOWED,
				    NULL, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return h3_end_response(client);
}

/**
 * Handle static resource for HTTP/3.
 */
static int h3_handle_static_resource(
	struct http_resource_detail_static *static_detail,
	struct http_client_ctx *client)
{
	int ret;

	if (client->method != HTTP_GET) {
		return h3_send_405(client);
	}

	ret = h3_send_headers_frame(client, HTTP_200_OK,
				    &static_detail->common, NULL, 0);
	if (ret < 0) {
		LOG_DBG("[%p] H3: cannot send headers (%d)", client, ret);
		return ret;
	}

	ret = h3_send_data_frame(client, static_detail->static_data,
				 static_detail->static_data_len);
	if (ret < 0) {
		LOG_DBG("[%p] H3: cannot send data (%d)", client, ret);
		return ret;
	}

	return h3_end_response(client);
}

#if defined(CONFIG_FILE_SYSTEM)
/**
 * Handle static filesystem resource for HTTP/3.
 */
static int h3_handle_static_fs_resource(
	struct http_resource_detail_static_fs *static_fs_detail,
	struct http_client_ctx *client)
{
	int ret;
	struct fs_file_t file;
	char fname[HTTP_SERVER_MAX_URL_LENGTH];
	char content_type[HTTP_SERVER_MAX_CONTENT_TYPE_LEN] = "text/html";
	struct http_resource_detail res_detail = {
		.bitmask_of_supported_http_methods =
			static_fs_detail->common.bitmask_of_supported_http_methods,
		.content_type = content_type,
		.path_len = static_fs_detail->common.path_len,
		.type = static_fs_detail->common.type,
	};
	size_t file_size;
	int len;
	int remaining;
	char tmp[64];

	if (client->method != HTTP_GET) {
		return h3_send_405(client);
	}

	/* Get filename and content-type from URL */
	len = strlen((const char *)client->url_buffer);

	if (len == 1) {
		snprintk(fname, sizeof(fname), "%s/index.html",
			 static_fs_detail->fs_path);
	} else {
		http_server_get_content_type_from_extension(
			(char *)client->url_buffer, content_type,
			sizeof(content_type));
		snprintk(fname, sizeof(fname), "%s%s",
			 static_fs_detail->fs_path, client->url_buffer);
	}

	ret = http_server_find_file(fname, sizeof(fname), &file_size, 0, NULL);
	if (ret < 0) {
		LOG_DBG("[%p] H3: file not found: %s (%d)", client, fname, ret);
		return h3_send_404(client);
	}

	fs_file_t_init(&file);

	ret = fs_open(&file, fname, FS_O_READ);
	if (ret < 0) {
		LOG_DBG("[%p] H3: fs_open %s: %d", client, fname, ret);
		return ret;
	}

	ret = h3_send_headers_frame(client, HTTP_200_OK, &res_detail,
				    NULL, 0);
	if (ret < 0) {
		LOG_DBG("[%p] H3: cannot send headers (%d)", client, ret);
		goto out;
	}

	remaining = file_size;

	while (remaining > 0) {
		len = fs_read(&file, tmp, sizeof(tmp));
		if (len < 0) {
			LOG_DBG("[%p] H3: filesystem read error (%d)", client, len);
			ret = len;
			goto out;
		}

		remaining -= len;

		ret = h3_send_data_frame(client, tmp, len);
		if (ret < 0) {
			LOG_DBG("[%p] H3: cannot send data (%d)", client, ret);
			goto out;
		}
	}

out:
	fs_close(&file);

	return ret;
}
#endif /* CONFIG_FILE_SYSTEM */

/**
 * Send an HTTP/3 dynamic response (headers + body).
 *
 * Mirrors http2_dynamic_response from the HTTP/2 implementation.
 */
static bool *h3_stream_headers_sent(struct http_client_ctx *client)
{
	for (int i = 0; i < ARRAY_SIZE(client->h3.stream_sock); i++) {
		if (client->h3.stream_sock[i] == client->fd) {
			return &client->h3.headers_sent[i];
		}
	}

	return NULL;
}

static int h3_dynamic_response(struct http_client_ctx *client,
				struct http_response_ctx *rsp,
				enum http_transaction_status status,
				struct http_resource_detail_dynamic *dynamic_detail,
				bool *headers_sent)
{
	int ret;
	bool final_response = http_response_is_final(rsp, status);

	if (*headers_sent && (rsp->header_count > 0 || rsp->status != 0)) {
		LOG_DBG("[%p] H3: already sent headers, dropping new headers", client);
	}

	/* Send headers if not yet sent */
	if (!*headers_sent) {
		if (rsp->status == 0) {
			rsp->status = 200;
		}

		if (rsp->status < HTTP_100_CONTINUE ||
		    rsp->status > HTTP_511_NETWORK_AUTHENTICATION_REQUIRED) {
			LOG_DBG("[%p] H3: invalid status code: %d", client, rsp->status);
			return -EINVAL;
		}

		if (rsp->headers == NULL && rsp->header_count > 0) {
			LOG_DBG("[%p] H3: NULL headers with count > 0", client);
			return -EINVAL;
		}

		ret = h3_send_headers_frame(client, rsp->status,
					    (struct http_resource_detail *)dynamic_detail,
					    rsp->headers, rsp->header_count);
		if (ret < 0) {
			return ret;
		}

		*headers_sent = true;
	}

	/* Send body data if provided */
	if (rsp->body != NULL && rsp->body_len > 0) {
		ret = h3_send_data_frame(client, rsp->body, rsp->body_len);
		if (ret < 0) {
			return ret;
		}
	}

	ARG_UNUSED(final_response);

	return 0;
}

/**
 * Handle dynamic resource for HTTP/3 GET/DELETE/OPTIONS
 */
static int h3_dynamic_get_del_opts(
	struct http_resource_detail_dynamic *dynamic_detail,
	struct http_client_ctx *client)
{
	int ret;
	int len;
	char *ptr;
	enum http_transaction_status status;
	struct http_request_ctx request_ctx;
	struct http_response_ctx response_ctx;
	bool *headers_sent;

	if (dynamic_detail->common.path_len >= sizeof(client->url_buffer) ||
	    dynamic_detail->common.path_len < 0) {
		LOG_DBG("[%p] H3: path length %d out of limits", client,
			dynamic_detail->common.path_len);
		return -EINVAL;
	}

	ptr = (char *)&client->url_buffer[dynamic_detail->common.path_len];
	len = strlen(ptr);
	status = HTTP_SERVER_REQUEST_DATA_FINAL;
	headers_sent = h3_stream_headers_sent(client);
	if (headers_sent == NULL) {
		return -ENOENT;
	}

	do {
		memset(&response_ctx, 0, sizeof(response_ctx));
		populate_request_ctx(&request_ctx, (uint8_t *)ptr, len, NULL);

		ret = dynamic_detail->cb(client, status, &request_ctx,
					 &response_ctx,
					 dynamic_detail->user_data);
		if (ret < 0) {
			return ret;
		}

		ret = h3_dynamic_response(client, &response_ctx, status,
					  dynamic_detail, headers_sent);
		if (ret < 0) {
			return ret;
		}

		/* URL params are passed in the first cb only */
		len = 0;
	} while (!http_response_is_final(&response_ctx, status));

	ret = dynamic_detail->cb(client, HTTP_SERVER_TRANSACTION_COMPLETE,
				 &request_ctx, &response_ctx,
				 dynamic_detail->user_data);
	if (ret < 0) {
		return ret;
	}

	dynamic_detail->holder = NULL;

	if (ret == 0) {
		ret = h3_end_response(client);
	}

	return ret;
}

/**
 * Handle dynamic resource for HTTP/3 POST/PUT/PATCH
 *
 * Unlike HTTP/2, in HTTP/3 request body data arrives in separate DATA frames
 * on the same stream. This function handles the initial HEADERS-only phase.
 * DATA frames are delivered later through h3_process_data_frame().
 */
static int h3_dynamic_post_put_start(
	struct http_resource_detail_dynamic *dynamic_detail,
	struct http_client_ctx *client)
{
	/* Store the detail for later DATA frame processing */
	client->current_detail = (struct http_resource_detail *)dynamic_detail;

	LOG_DBG("[%p] H3: POST/PUT waiting for DATA frames", client);

	return -EAGAIN;
}

/**
 * Handle HTTP/3 dynamic resource dispatch after HEADERS are parsed.
 */
static int h3_handle_dynamic_resource(
	struct http_resource_detail_dynamic *dynamic_detail,
	struct http_client_ctx *client)
{
	bool *headers_sent;
	uint32_t user_method;

	if (dynamic_detail->cb == NULL) {
		return -ESRCH;
	}

	user_method = dynamic_detail->common.bitmask_of_supported_http_methods;

	if (!(BIT(client->method) & user_method)) {
		return h3_send_405(client);
	}

	if (dynamic_detail->holder != NULL &&
	    dynamic_detail->holder != client) {
		int ret;

		ret = h3_send_headers_frame(client, HTTP_409_CONFLICT,
					    NULL, NULL, 0);
		if (ret < 0) {
			return ret;
		}

		return enter_http_done_state(client);
	}

	dynamic_detail->holder = client;
	headers_sent = h3_stream_headers_sent(client);
	if (headers_sent == NULL) {
		dynamic_detail->holder = NULL;
		return -ENOENT;
	}

	*headers_sent = false;

	switch (client->method) {
	case HTTP_GET:
	case HTTP_DELETE:
	case HTTP_OPTIONS:
		if (user_method & BIT(client->method)) {
			return h3_dynamic_get_del_opts(dynamic_detail, client);
		}

		break;

	case HTTP_POST:
	case HTTP_PUT:
	case HTTP_PATCH:
		if (user_method & BIT(client->method)) {
			return h3_dynamic_post_put_start(dynamic_detail,
							 client);
		}

		break;

	default:
		break;
	}

	LOG_DBG("[%p] H3: method %d not supported", client, client->method);

	return -ENOTSUP;
}

/**
 * Dispatch the request to the appropriate resource handler
 * after HTTP/3 HEADERS have been parsed.
 */
static int h3_dispatch_request(struct http_client_ctx *client,
			       struct http_resource_detail *detail)
{
	switch (detail->type) {
	case HTTP_RESOURCE_TYPE_STATIC:
		return h3_handle_static_resource(
			(struct http_resource_detail_static *)detail,
			client);

#if defined(CONFIG_FILE_SYSTEM)
	case HTTP_RESOURCE_TYPE_STATIC_FS:
		return h3_handle_static_fs_resource(
			(struct http_resource_detail_static_fs *)detail,
			client);
#endif

	case HTTP_RESOURCE_TYPE_DYNAMIC:
		return h3_handle_dynamic_resource(
			(struct http_resource_detail_dynamic *)detail,
			client);

	default:
		LOG_DBG("[%p] H3: unsupported resource type %d", client, detail->type);
		return h3_send_404(client);
	}
}

/**
 * Process an HTTP/3 DATA frame payload for dynamic resources.
 *
 * @param client  Client context.
 * @param payload DATA frame payload.
 * @param length  Payload length.
 * @param fin     True if this is the last DATA frame (stream FIN seen).
 *
 * @return 0 on success, <0 on error.
 */
static int h3_process_data_frame(struct http_client_ctx *client,
				 const uint8_t *payload, size_t length,
				 bool fin)
{
	struct http_resource_detail_dynamic *dynamic_detail;
	struct http_request_ctx request_ctx;
	struct http_response_ctx response_ctx;
	enum http_transaction_status status;
	bool *headers_sent;
	int ret;

	if (client->current_detail == NULL) {
		LOG_DBG("[%p] H3: DATA frame but no current resource", client);
		return 0;
	}

	if (client->current_detail->type != HTTP_RESOURCE_TYPE_DYNAMIC) {
		/* For non-dynamic resources, DATA frames are unexpected */
		NET_DBG("[%p] H3: DATA frame for non-dynamic resource (type=%d)",
			client, client->current_detail->type);
		return 0;
	}

	dynamic_detail = (struct http_resource_detail_dynamic *)client->current_detail;
	if (dynamic_detail->cb == NULL) {
		return -ESRCH;
	}

	headers_sent = h3_stream_headers_sent(client);
	if (headers_sent == NULL) {
		return -ENOENT;
	}

	status = fin ? HTTP_SERVER_REQUEST_DATA_FINAL
		     : HTTP_SERVER_REQUEST_DATA_MORE;

	memset(&response_ctx, 0, sizeof(response_ctx));
	populate_request_ctx(&request_ctx, (uint8_t *)payload, length, NULL);

	ret = dynamic_detail->cb(client, status, &request_ctx, &response_ctx,
				 dynamic_detail->user_data);
	if (ret < 0) {
		return ret;
	}

	if (http_response_is_provided(&response_ctx)) {
		ret = h3_dynamic_response(client, &response_ctx, status,
					  dynamic_detail, headers_sent);
		if (ret < 0) {
			return ret;
		}
	}

	/* Drain remaining response after final data */
	while (!http_response_is_final(&response_ctx, status) &&
	       status == HTTP_SERVER_REQUEST_DATA_FINAL) {
		memset(&response_ctx, 0, sizeof(response_ctx));
		populate_request_ctx(&request_ctx, NULL, 0, NULL);

		ret = dynamic_detail->cb(client, status, &request_ctx,
					 &response_ctx,
					 dynamic_detail->user_data);
		if (ret < 0) {
			return ret;
		}

		ret = h3_dynamic_response(client, &response_ctx, status,
					  dynamic_detail, headers_sent);
		if (ret < 0) {
			return ret;
		}
	}

	if (fin) {
		/* Notify completion */
		ret = dynamic_detail->cb(client,
					 HTTP_SERVER_TRANSACTION_COMPLETE,
					 &request_ctx, &response_ctx,
					 dynamic_detail->user_data);
		if (ret < 0) {
			return ret;
		}

		dynamic_detail->holder = NULL;

		return h3_end_response(client);
	}

	return 0;
}

/**
 * Parse QPACK-encoded header block from an HTTP/3 HEADERS frame.
 *
 * This decoder supports:
 * - Indexed field lines referencing the static table
 * - Literal field lines with name reference to the static table
 * - Literal field lines without name reference
 * - Huffman-encoded header names and values
 *
 * Dynamic table is not supported in this initial implementation.
 * We advertise QPACK_MAX_TABLE_CAPACITY=0 to disable it.
 *
 * @param client  Client context to populate with method, path, etc.
 * @param buf     QPACK header block data.
 * @param buflen  Length of header block data.
 *
 * @return 0 on success, <0 on error.
 */
static int h3_parse_qpack_headers(struct http_client_ctx *client,
				  const uint8_t *buf, size_t buflen)
{
	/* Buffer for Huffman decoding (stack-allocated for reentrancy) */
	uint8_t huffman_buf[H3_HUFFMAN_DECODE_BUF_SIZE];
	size_t pos = 0;
	uint64_t ric;
	uint64_t delta_base;
	int ret;
	uint8_t byte;
	uint64_t index;
	enum http_method method;
	const char *path;
	size_t path_len;
	uint64_t name_index;
	bool huffman;
	uint64_t val_len;
	bool name_huffman;
	uint64_t name_len;
	const uint8_t *val_ptr;
	size_t decoded_len;

	/* Required Insert Count (RFC 9204 Section 4.5.1) */
	ret = qpack_decode_int(buf, buflen, 8, &ric);
	if (ret < 0) {
		LOG_DBG("[%p] Failed to decode QPACK required insert count", client);
		return ret;
	}

	pos += ret;

	if (ric != 0) {
		LOG_DBG("[%p] QPACK dynamic table not supported (RIC=%llu)",
			client, ric);
		return -ENOTSUP;
	}

	/* Delta Base (RFC 9204 Section 4.5.1) */
	ret = qpack_decode_int(buf + pos, buflen - pos, 7, &delta_base);
	if (ret < 0) {
		LOG_DBG("[%p] Failed to decode QPACK delta base", client);
		return ret;
	}

	pos += ret;

	/* Parse header field lines */
	while (pos < buflen) {
		byte = buf[pos];

		if ((byte & QPACK_INDEXED_STATIC_MASK) == QPACK_INDEXED_STATIC) {
			/* Indexed field line, static table reference */

			ret = qpack_decode_int(buf + pos, buflen - pos, 6,
					       &index);
			if (ret < 0) {
				return ret;
			}

			pos += ret;

			/* Check if this is a method */
			if (qpack_static_method(index, &method) == 0) {
				client->method = method;
				LOG_DBG("[%p] H3: method=%d (static idx %llu)",
					client, method, index);
				continue;
			}

			/* Check if this is a path */
			path = qpack_static_path(index);

			if (path != NULL) {
				path_len = strlen(path);

				if (path_len < sizeof(client->url_buffer)) {
					memcpy(client->url_buffer, path,
					       path_len);
					client->url_buffer[path_len] = '\0';

					LOG_DBG("[%p] H3: path=%s (static idx %llu)",
						client, client->url_buffer, index);
				}
			}
		} else if ((byte & QPACK_LIT_NAME_REF_MASK) ==
			   QPACK_LIT_NAME_REF_STATIC) {
			/* Literal with name reference, static table */

			ret = qpack_decode_int(buf + pos, buflen - pos, 4,
					       &name_index);
			if (ret < 0) {
				return ret;
			}

			pos += ret;

			/* Read the value */
			huffman = (buf[pos] & 0x80) != 0;

			ret = qpack_decode_int(buf + pos, buflen - pos, 7, &val_len);
			if (ret < 0) {
				return ret;
			}

			pos += ret;

			if (pos + val_len > buflen) {
				return -EINVAL;
			}

			/* Decode value (Huffman or literal) */
			if (huffman) {
				ret = http_hpack_huffman_decode(buf + pos, val_len,
							       huffman_buf,
							       sizeof(huffman_buf) - 1);
				if (ret < 0) {
					LOG_DBG("[%p] H3: Huffman decode failed (%d)",
						client, ret);
					return ret;
				}

				huffman_buf[ret] = '\0';
				val_ptr = huffman_buf;
				decoded_len = ret;
			} else {
				val_ptr = buf + pos;
				decoded_len = val_len;
			}

			/* Extract known pseudo-headers by name index */
			if (name_index == QPACK_STATIC_PATH_SLASH ||
			    name_index == QPACK_STATIC_PATH_INDEX_HTML) {
				if (decoded_len < sizeof(client->url_buffer)) {
					memcpy(client->url_buffer, val_ptr,
					       decoded_len);
					client->url_buffer[decoded_len] = '\0';
					LOG_DBG("[%p] H3: path=%s", client, client->url_buffer);
				}
			} else if (name_index == QPACK_STATIC_AUTHORITY) {
				LOG_DBG("[%p] H3: authority=%.*s", client,
					(int)decoded_len, val_ptr);
			}

			/* Check method by name index */
			if (qpack_static_method(name_index, &method) == 0) {
				client->method = method;
			}

			pos += val_len;
		} else if ((byte & QPACK_LIT_WITHOUT_NAME_MASK) ==
			   QPACK_LIT_WITHOUT_NAME_REF) {
			/* Literal without name reference */
			name_huffman = (buf[pos] & 0x08) != 0;

			ret = qpack_decode_int(buf + pos, buflen - pos, 3,
					       &name_len);
			if (ret < 0) {
				return ret;
			}

			pos += ret;

			if (pos + name_len > buflen) {
				return -EINVAL;
			}

			/* Decode and log name */
			if (name_huffman) {
				ret = http_hpack_huffman_decode(buf + pos, name_len,
							       huffman_buf,
							       sizeof(huffman_buf) - 1);
				if (ret >= 0) {
					huffman_buf[ret] = '\0';
					LOG_DBG("[%p] H3: header name=%s", client, huffman_buf);
				}
			} else {
				LOG_DBG("[%p] H3: header name=%.*s", client,
					(int)name_len, buf + pos);
			}

			pos += name_len;

			/* Read value */
			if (pos >= buflen) {
				return -EINVAL;
			}

			huffman = (buf[pos] & 0x80) != 0;

			ret = qpack_decode_int(buf + pos, buflen - pos, 7,
					       &val_len);
			if (ret < 0) {
				return ret;
			}

			pos += ret;

			if (pos + val_len > buflen) {
				return -EINVAL;
			}

			/* Decode and log value */
			if (huffman) {
				ret = http_hpack_huffman_decode(buf + pos, val_len,
							       huffman_buf,
							       sizeof(huffman_buf) - 1);
				if (ret >= 0) {
					huffman_buf[ret] = '\0';
					LOG_DBG("[%p] H3: header value=%s", client, huffman_buf);
				}
			} else {
				LOG_DBG("[%p] H3: header value=%.*s", client,
					(int)val_len, buf + pos);
			}

			pos += val_len;
		} else {
			/* Unknown or post-base indexed reference */
			LOG_DBG("[%p] H3: unknown QPACK prefix byte 0x%02x", client, byte);
			return -ENOTSUP;
		}
	}

	return 0;
}

/**
 * Parse HTTP/3 SETTINGS frame payload.
 *
 * SETTINGS frame contains a list of identifier-value pairs encoded as varints.
 * We store recognized settings and ignore unknown ones per RFC 9114.
 *
 * @param client  Client context.
 * @param buf     SETTINGS frame payload.
 * @param buflen  Length of payload.
 *
 * @return 0 on success, <0 on error.
 */
static int h3_parse_settings(struct http_client_ctx *client,
			     const uint8_t *buf, size_t buflen)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
	size_t pos = 0;
	uint64_t identifier;
	uint64_t value;
	int ret;

	if (h3_ctx == NULL) {
		LOG_DBG("[%p] H3: No connection context for SETTINGS", client);
		return -EINVAL;
	}

	h3_init_conn_ctx(h3_ctx);

	/* Parse identifier-value pairs */
	while (pos < buflen) {
		/* Parse identifier (varint) */
		ret = h3_parse_varint(buf + pos, buflen - pos, &identifier);
		if (ret < 0) {
			LOG_DBG("[%p] H3: Failed to parse SETTINGS identifier", client);
			return -H3_SETTINGS_ERROR;
		}

		pos += ret;

		if (pos >= buflen) {
			LOG_DBG("[%p] H3: SETTINGS truncated after identifier", client);
			return -H3_SETTINGS_ERROR;
		}

		/* Parse value (varint) */
		ret = h3_parse_varint(buf + pos, buflen - pos, &value);
		if (ret < 0) {
			LOG_DBG("[%p] H3: Failed to parse SETTINGS value", client);
			return -H3_SETTINGS_ERROR;
		}
		pos += ret;

		/* Process known settings */
		switch (identifier) {
		case H3_SETTINGS_QPACK_MAX_TABLE_CAPACITY:
			h3_ctx->peer_settings.qpack_max_table_capacity = value;
			LOG_DBG("[%p] H3: SETTINGS QPACK_MAX_TABLE_CAPACITY=%llu",
				client, value);
			break;

		case H3_SETTINGS_MAX_FIELD_SECTION_SIZE:
			h3_ctx->peer_settings.max_field_section_size = value;
			LOG_DBG("[%p] H3: SETTINGS MAX_FIELD_SECTION_SIZE=%llu",
				client, value);
			break;

		case H3_SETTINGS_QPACK_BLOCKED_STREAMS:
			h3_ctx->peer_settings.qpack_blocked_streams = value;
			LOG_DBG("[%p] H3: SETTINGS QPACK_BLOCKED_STREAMS=%llu",
				client, value);
			break;

		default:
			/* Unknown settings MUST be ignored per RFC 9114 */
			LOG_DBG("[%p] H3: SETTINGS unknown id=0x%llx value=%llu",
				client, identifier, value);
			break;
		}
	}

	h3_ctx->peer_settings.received = true;

	LOG_DBG("[%p] H3: SETTINGS frame parsed successfully", client);

	return 0;
}

/**
 * Build and send our SETTINGS frame on the control stream.
 *
 * @param fd  Control stream socket fd.
 *
 * @return 0 on success, <0 on error.
 */
static int h3_send_settings(struct http_client_ctx *client, int fd)
{
	uint8_t buf[32];
	size_t payload_start;
	size_t payload_len;
	size_t len_pos;
	size_t pos = 0;
	int ret;

	/* Frame type: SETTINGS (0x04) */
	ret = h3_encode_varint(buf + pos, sizeof(buf) - pos, H3_FRAME_SETTINGS);
	if (ret < 0) {
		return ret;
	}
	pos += ret;

	/* We'll fill in the length after building the payload */
	len_pos = pos;
	pos++; /* Reserve 1 byte for length (our settings fit in 1 byte) */

	payload_start = pos;

	/* QPACK_MAX_TABLE_CAPACITY = 0 (disable dynamic table) */
	ret = h3_encode_varint(buf + pos, sizeof(buf) - pos,
			       H3_SETTINGS_QPACK_MAX_TABLE_CAPACITY);
	if (ret < 0) {
		return ret;
	}
	pos += ret;

	ret = h3_encode_varint(buf + pos, sizeof(buf) - pos, 0);
	if (ret < 0) {
		return ret;
	}
	pos += ret;

	/* QPACK_BLOCKED_STREAMS = 0 */
	ret = h3_encode_varint(buf + pos, sizeof(buf) - pos,
			       H3_SETTINGS_QPACK_BLOCKED_STREAMS);
	if (ret < 0) {
		return ret;
	}
	pos += ret;

	ret = h3_encode_varint(buf + pos, sizeof(buf) - pos, 0);
	if (ret < 0) {
		return ret;
	}
	pos += ret;

	/* Fill in the length */
	payload_len = pos - payload_start;
	buf[len_pos] = (uint8_t)payload_len;

	/* Send on control stream */
	ret = zsock_send(fd, buf, pos, 0);
	if (ret < 0) {
		LOG_DBG("[%p] H3: Failed to send SETTINGS (%d)", client, errno);
		return -errno;
	}

	LOG_DBG("[%p] H3: Sent SETTINGS frame (%zu bytes)", client, pos);

	return 0;
}

/**
 * Handle data received on the peer's control stream.
 *
 * @param client  Client context (used for connection context lookup).
 * @param fd      Control stream socket fd.
 *
 * @return 0 on success, <0 on error.
 */
int h3_handle_control_stream(struct http_client_ctx *client, int fd)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
	struct h3_uni_stream_rx_ctx *rx_ctx;
	size_t pos = 0;
	uint64_t frame_type;
	uint64_t frame_len;
	int ret;

	if (h3_ctx == NULL) {
		return -EINVAL;
	}

	rx_ctx = h3_get_peer_uni_stream_rx_ctx(h3_ctx, fd);
	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	ret = h3_recv_peer_uni_stream_data(rx_ctx, fd);
	if (ret == -H3_CLOSED_CRITICAL_STREAM) {
		/* Control stream closed, this is a connection error */
		LOG_DBG("[%p] H3: Control stream closed unexpectedly", client);
		return -H3_CLOSED_CRITICAL_STREAM;
	}

	if (ret < 0) {
		return ret;
	}

	if (rx_ctx->data_len == 0) {
		return 0;
	}

	LOG_DBG("[%p] H3: Control stream buffered %zu bytes", client, rx_ctx->data_len);

	/* Parse frames on control stream */
	while (pos < rx_ctx->data_len) {
		int hdr_bytes;

		ret = h3_parse_frame_header(rx_ctx->buffer + pos,
					    rx_ctx->data_len - pos,
					    &frame_type, &frame_len);
		if (ret < 0) {
			LOG_DBG("[%p] H3: Incomplete frame header on control stream", client);
			h3_compact_peer_uni_stream_data(rx_ctx, pos);

			if (rx_ctx->data_len == sizeof(rx_ctx->buffer)) {
				LOG_ERR("[%p] H3: Control stream RX buffer too small",
					client);
				return -ENOBUFS;
			}

			return 0;
		}

		hdr_bytes = ret;

		if (pos + hdr_bytes + frame_len > rx_ctx->data_len) {
			LOG_DBG("[%p] H3: Incomplete frame on control stream", client);
			h3_compact_peer_uni_stream_data(rx_ctx, pos);

			if (rx_ctx->data_len == sizeof(rx_ctx->buffer)) {
				LOG_ERR("[%p] H3: Control stream RX buffer too small",
					client);
				return -ENOBUFS;
			}

			return 0;
		}

		switch (frame_type) {
		case H3_FRAME_SETTINGS:
			LOG_DBG("[%p] H3: Control stream SETTINGS, len=%llu",
				client, frame_len);
			ret = h3_parse_settings(client, rx_ctx->buffer + pos + hdr_bytes,
						(size_t)frame_len);
			if (ret < 0) {
				return ret;
			}
			break;

		case H3_FRAME_GOAWAY:
			LOG_DBG("[%p] H3: Control stream GOAWAY", client);
			return -H3_NO_ERROR; /* Graceful shutdown */

		case H3_FRAME_MAX_PUSH_ID:
			LOG_DBG("[%p] H3: Control stream MAX_PUSH_ID (ignored)", client);
			break;

		case H3_FRAME_CANCEL_PUSH:
			LOG_DBG("[%p] H3: Control stream CANCEL_PUSH (ignored)", client);
			break;

		default:
			/* Unknown frame types on control stream are ignored */
			LOG_DBG("[%p] H3: Control stream unknown frame 0x%llx",
				client, frame_type);
			break;
		}

		pos += hdr_bytes + frame_len;
	}

	rx_ctx->data_len = 0;

	return 0;
}

/**
 * Handle data received on a QPACK encoder stream.
 * Since we don't support dynamic table, we just consume and ignore the data.
 *
 * @param fd  QPACK encoder stream socket fd.
 *
 * @return 0 on success, <0 on error.
 */
int h3_handle_qpack_encoder_stream(struct http_client_ctx *client, int fd)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
	struct h3_uni_stream_rx_ctx *rx_ctx;
	int ret;

	if (h3_ctx == NULL) {
		return -EINVAL;
	}

	rx_ctx = h3_get_peer_uni_stream_rx_ctx(h3_ctx, fd);
	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	ret = h3_recv_peer_uni_stream_data(rx_ctx, fd);
	if (ret == -H3_CLOSED_CRITICAL_STREAM) {
		LOG_DBG("[%p] H3: QPACK encoder stream closed", client);
		return -H3_CLOSED_CRITICAL_STREAM;
	}

	if (ret < 0) {
		return ret;
	}

	/* We ignore encoder instructions since we don't use dynamic table */
	if (rx_ctx->data_len > 0) {
		LOG_DBG("[%p] H3: QPACK encoder stream received %zu bytes (ignored)",
			client, rx_ctx->data_len);
		rx_ctx->data_len = 0;
	}

	return 0;
}

/**
 * Handle data received on a QPACK decoder stream.
 * Since we don't use dynamic table, we don't expect any data here.
 *
 * @param fd  QPACK decoder stream socket fd.
 *
 * @return 0 on success, <0 on error.
 */
int h3_handle_qpack_decoder_stream(struct http_client_ctx *client, int fd)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
	struct h3_uni_stream_rx_ctx *rx_ctx;
	int ret;

	if (h3_ctx == NULL) {
		return -EINVAL;
	}

	rx_ctx = h3_get_peer_uni_stream_rx_ctx(h3_ctx, fd);
	if (rx_ctx == NULL) {
		return -EINVAL;
	}

	ret = h3_recv_peer_uni_stream_data(rx_ctx, fd);
	if (ret == -H3_CLOSED_CRITICAL_STREAM) {
		LOG_DBG("[%p] H3: QPACK decoder stream closed", client);
		return -H3_CLOSED_CRITICAL_STREAM;
	}

	if (ret < 0) {
		return ret;
	}

	/* We ignore decoder feedback since we don't use dynamic table */
	if (rx_ctx->data_len > 0) {
		LOG_DBG("[%p] H3: QPACK decoder stream received %zu bytes (ignored)",
			client, rx_ctx->data_len);
		rx_ctx->data_len = 0;
	}

	return 0;
}

/**
 * Open our unidirectional streams (control, QPACK encoder/decoder).
 *
 * @param client      Client context.
 * @param quic_sock   QUIC connection socket.
 *
 * @return 0 on success, <0 on error.
 */
int h3_open_uni_streams(struct http_client_ctx *client, int quic_sock)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
	uint8_t stream_type_buf[1];
	uint64_t id = 0ULL;
	int fd;
	int ret;

	if (h3_ctx == NULL) {
		return -EINVAL;
	}

	h3_init_conn_ctx(h3_ctx);

	if (h3_ctx->settings_sent) {
		return 0; /* Already completed */
	}

	/* Open control stream (if not already open) */
	if (h3_ctx->local_control_stream < 0) {
		fd = quic_stream_open(quic_sock, QUIC_STREAM_SERVER,
				      QUIC_STREAM_UNIDIRECTIONAL, 0);
		if (fd < 0) {
			LOG_DBG("[%p] H3: Failed to open control stream (%d)", client, fd);
			return fd;
		}

		h3_ctx->local_control_stream = fd;

		/* Send stream type */
		stream_type_buf[0] = H3_UNI_STREAM_CONTROL;
		ret = zsock_send(fd, stream_type_buf, 1, ZSOCK_MSG_DONTWAIT);
		if (ret < 0) {
			ret = errno;

			if (ret == EAGAIN || ret == EWOULDBLOCK) {
				return -EAGAIN;
			}

			LOG_DBG("[%p] H3: Failed to send control stream type (%d)",
				client, -ret);
			return -ret;
		}

		/* Send SETTINGS on control stream */
		ret = h3_send_settings(client, fd);
		if (ret < 0) {
			LOG_DBG("[%p] H3: Settings send (%d)", client, ret);
			return ret;
		}

		(void)quic_stream_get_id(fd, &id);

		LOG_DBG("[%p] H3: Control stream %" PRIu64 " opened (fd=%d)",
			client, id, fd);
	}

	/* Open QPACK encoder stream (if not already open) */
	if (h3_ctx->local_qpack_encoder_stream < 0) {
		fd = quic_stream_open(quic_sock, QUIC_STREAM_SERVER,
				      QUIC_STREAM_UNIDIRECTIONAL, 0);
		if (fd < 0) {
			LOG_DBG("[%p] H3: Failed to open QPACK encoder stream (%d)",
				client, fd);
			return fd;
		}

		h3_ctx->local_qpack_encoder_stream = fd;

		stream_type_buf[0] = H3_UNI_STREAM_QPACK_ENCODER;
		ret = zsock_send(fd, stream_type_buf, 1, ZSOCK_MSG_DONTWAIT);
		if (ret < 0) {
			ret = errno;

			if (ret == EAGAIN || ret == EWOULDBLOCK) {
				LOG_DBG("[%p] H3: QPACK encoder stream type send (%d)",
					client, -ret);
				return -EAGAIN;
			}

			LOG_DBG("[%p] H3: Failed to send QPACK encoder stream type (%d)",
				client, -ret);
			return -ret;
		}

		(void)quic_stream_get_id(fd, &id);

		LOG_DBG("[%p] H3: QPACK encoder stream %" PRIu64 " opened (fd=%d)",
			client, id, fd);
	}

	/* Open QPACK decoder stream (if not already open) */
	if (h3_ctx->local_qpack_decoder_stream < 0) {
		fd = quic_stream_open(quic_sock, QUIC_STREAM_SERVER,
				      QUIC_STREAM_UNIDIRECTIONAL, 0);
		if (fd < 0) {
			LOG_DBG("[%p] H3: Failed to open QPACK decoder stream (%d)",
				client, fd);
			return fd;
		}

		h3_ctx->local_qpack_decoder_stream = fd;

		stream_type_buf[0] = H3_UNI_STREAM_QPACK_DECODER;
		ret = zsock_send(fd, stream_type_buf, 1, ZSOCK_MSG_DONTWAIT);
		if (ret < 0) {
			ret = errno;

			if (ret == EAGAIN || ret == EWOULDBLOCK) {
				LOG_DBG("[%p] H3: QPACK decoder stream type send (%d)",
					client, -ret);
				return -EAGAIN;
			}

			LOG_DBG("[%p] H3: Failed to send QPACK decoder stream type (%d)",
				client, -ret);
			return -ret;
		}

		(void)quic_stream_get_id(fd, &id);

		LOG_DBG("[%p] H3: QPACK decoder stream %" PRIu64 " opened (fd=%d)",
			client, id, fd);
	}

	/* All streams opened successfully */
	h3_ctx->settings_sent = true;

	LOG_DBG("[%p] H3: All uni streams opened successfully", client);

	return 0;
}

/**
 * Identify and register an incoming unidirectional stream.
 *
 * @param client  Client context.
 * @param fd      Accepted unidirectional stream socket fd.
 *
 * @return 0 on success, <0 on error.
 */
int h3_identify_uni_stream(struct http_client_ctx *client, int fd)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);
	struct h3_uni_stream_rx_ctx *rx_ctx = NULL;
	uint64_t stream_type;
	uint8_t buf[8];
	ssize_t len;
	int ret;

	if (h3_ctx == NULL) {
		return -EINVAL;
	}

	h3_init_conn_ctx(h3_ctx);

	/* Read stream type (varint, usually 1 byte) */
	errno = 0;  /* Clear errno before recv */
	len = zsock_recv(fd, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT);
	if (len < 0) {
		int err = errno;

		if (err == EAGAIN || err == EWOULDBLOCK) {
			return -EAGAIN; /* Need to wait for data */
		}
		LOG_DBG("[%p] H3: recv failed on uni stream fd=%d, errno=%d, len=%zd",
			client, fd, err, len);
		return (err != 0) ? -err : -EIO;
	}

	if (len == 0) {
		LOG_DBG("[%p] H3: Unidirectional stream closed before type", client);
		return -ECONNRESET;
	}

	ret = h3_parse_varint(buf, len, &stream_type);
	if (ret < 0) {
		LOG_DBG("[%p] H3: Failed to parse uni stream type", client);
		return ret;
	}

	switch (stream_type) {
	case H3_UNI_STREAM_CONTROL:
		if (h3_ctx->peer_control_stream >= 0) {
			LOG_DBG("[%p] H3: Duplicate control stream", client);
			return -H3_STREAM_CREATION_ERROR;
		}

		h3_ctx->peer_control_stream = fd;
		rx_ctx = &h3_ctx->peer_control_rx;
		LOG_DBG("[%p] H3: Peer control stream identified (fd=%d)", client, fd);
		break;

	case H3_UNI_STREAM_QPACK_ENCODER:
		if (h3_ctx->peer_qpack_encoder_stream >= 0) {
			LOG_DBG("[%p] H3: Duplicate QPACK encoder stream", client);
			return -H3_STREAM_CREATION_ERROR;
		}

		h3_ctx->peer_qpack_encoder_stream = fd;
		rx_ctx = &h3_ctx->peer_qpack_encoder_rx;
		LOG_DBG("[%p] H3: Peer QPACK encoder stream identified (fd=%d)",
			client, fd);
		break;

	case H3_UNI_STREAM_QPACK_DECODER:
		if (h3_ctx->peer_qpack_decoder_stream >= 0) {
			LOG_DBG("[%p] H3: Duplicate QPACK decoder stream", client);
			return -H3_STREAM_CREATION_ERROR;
		}

		h3_ctx->peer_qpack_decoder_stream = fd;
		rx_ctx = &h3_ctx->peer_qpack_decoder_rx;
		LOG_DBG("[%p] H3: Peer QPACK decoder stream identified (fd=%d)",
			client, fd);
		break;

	case H3_UNI_STREAM_PUSH:
		/* Server should not receive push streams */
		LOG_DBG("[%p] H3: Received push stream (unexpected for server)", client);
		zsock_close(fd);
		return H3_STREAM_IGNORED;

	default:
		/* Unknown stream types MUST be ignored per RFC 9114 */
		LOG_DBG("[%p] H3: Unknown uni stream type 0x%llx", client, stream_type);
		zsock_close(fd);
		return H3_STREAM_IGNORED;
	}

	ret = h3_store_peer_uni_stream_data(rx_ctx, buf + ret, (size_t)len - ret);
	if (ret < 0) {
		LOG_ERR("[%p] H3: Failed to buffer peer uni stream data (%d)",
			client, ret);
		return ret;
	}

	return 0;
}

int h3_handle_uni_stream_data(struct http_client_ctx *client, int fd)
{
	struct h3_conn_ctx *h3_ctx = h3_get_conn_ctx(client);

	if (h3_ctx == NULL) {
		return -EINVAL;
	}

	if (fd == h3_ctx->peer_control_stream) {
		return h3_handle_control_stream(client, fd);
	} else if (fd == h3_ctx->peer_qpack_encoder_stream) {
		return h3_handle_qpack_encoder_stream(client, fd);
	} else if (fd == h3_ctx->peer_qpack_decoder_stream) {
		return h3_handle_qpack_decoder_stream(client, fd);
	}

	return 0;
}

/**
 * Process HTTP/3 frames from the client buffer.
 *
 * @param client Client context.
 *
 * @return 0 once the current request stream has been fully handled,
 *         -EAGAIN if more request data is needed, <0 on error to close
 *         the connection.
 */
static bool h3_request_body_pending(const struct http_client_ctx *client)
{
	if (client->current_detail == NULL) {
		return false;
	}

	if (client->current_detail->type != HTTP_RESOURCE_TYPE_DYNAMIC) {
		return false;
	}

	return client->method == HTTP_POST ||
	       client->method == HTTP_PUT ||
	       client->method == HTTP_PATCH;
}

static int h3_process_frames(struct http_client_ctx *client, bool stream_fin)
{
	bool need_more_data = false;
	size_t pos = 0;

	while (pos < client->data_len) {
		const uint8_t *payload;
		uint64_t frame_type;
		uint64_t frame_len;
		int hdr_bytes;
		int ret;

		hdr_bytes = h3_parse_frame_header(client->buffer + pos,
						  client->data_len - pos,
						  &frame_type, &frame_len);
		if (hdr_bytes < 0) {
			/* Not enough data for frame header */
			break;
		}

		if (pos + hdr_bytes + frame_len > client->data_len) {
			/* Not enough data for complete frame */
			break;
		}

		payload = client->buffer + pos + hdr_bytes;

		switch (frame_type) {
		case H3_FRAME_HEADERS:
			LOG_DBG("[%p] H3: HEADERS frame, len=%llu", client, frame_len);

			ret = h3_parse_qpack_headers(client, payload,
						     (size_t)frame_len);
			if (ret < 0 && ret != -ENOTSUP) {
				LOG_DBG("[%p] H3: QPACK parse error (%d)", client, ret);
				return ret;
			}

			/* Look up the resource and dispatch */
			if (HTTP_SERVER_MAX_URL_LENGTH > 0 && client->url_buffer[0] != '\0') {
				struct http_resource_detail *detail;
				int path_len;

				http_server_remove_dot_segments(client->url_buffer);

				detail = get_resource_detail(client->service,
							     (const char *)client->url_buffer,
							     &path_len, false);
				if (detail != NULL) {
					client->current_detail = detail;

					ret = h3_dispatch_request(client, detail);
					if (ret < 0 && ret != -EAGAIN) {
						return ret;
					}

					if (ret == -EAGAIN) {
						need_more_data = true;
					}
				} else {
					LOG_DBG("[%p] H3: no resource for path=%s",
						client, client->url_buffer);
					ret = h3_send_404(client);
					if (ret < 0) {
						return ret;
					}
				}
			}

			break;

		case H3_FRAME_DATA:
			LOG_DBG("[%p] H3: DATA frame, len=%llu", client, frame_len);

			ret = h3_process_data_frame(client, payload,
						    (size_t)frame_len, false);
			if (ret < 0) {
				return ret;
			}

			if (h3_request_body_pending(client)) {
				need_more_data = true;
			}

			break;

		case H3_FRAME_SETTINGS:
			LOG_DBG("[%p] H3: SETTINGS frame, len=%llu", client, frame_len);
			ret = h3_parse_settings(client, payload,
						(size_t)frame_len);
			if (ret < 0) {
				LOG_DBG("[%p] H3: SETTINGS parse error (%d)", client, ret);
				return ret;
			}
			break;

		case H3_FRAME_GOAWAY:
			LOG_DBG("[%p] H3: GOAWAY frame, len=%llu", client, frame_len);
			return enter_http_done_state(client);

		default:
			/* Unknown frame types MUST be ignored per RFC 9114 */
			LOG_DBG("[%p] H3: unknown frame type=0x%llx, len=%llu",
				client, frame_type, frame_len);
			break;
		}

		pos += hdr_bytes + frame_len;
	}

	/* Move any remaining data to start of buffer */
	if (pos > 0 && pos < client->data_len) {
		memmove(client->buffer, client->buffer + pos,
			client->data_len - pos);
		client->data_len -= pos;
	} else if (pos >= client->data_len) {
		client->data_len = 0;
	}

	if (stream_fin) {
		int ret;

		if (client->data_len > 0) {
			LOG_DBG("[%p] H3: stream FIN with incomplete frame (%zu bytes buffered)",
				client, client->data_len);
			return -EINVAL;
		}

		if (!h3_request_body_pending(client)) {
			return 0;
		}

		ret = h3_process_data_frame(client, NULL, 0, true);
		if (ret < 0) {
			return ret;
		}

		client->current_detail = NULL;

		return 0;
	}

	return (need_more_data || client->data_len > 0) ? -EAGAIN : 0;
}

/**
 * Check if a stream socket is unidirectional.
 *
 * @param fd  Stream socket file descriptor.
 *
 * @return true if unidirectional, false if bidirectional or on error.
 */
bool h3_is_unidirectional_stream(int fd)
{
	int stream_type;
	net_socklen_t optlen = sizeof(stream_type);

	if (zsock_getsockopt(fd, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_STREAM_TYPE,
			     &stream_type, &optlen) < 0) {
		return false;
	}

	return (stream_type & QUIC_STREAM_UNIDIRECTIONAL) != 0;
}

/**
 * Check if a stream is server-initiated (i.e., initiated by us).
 * Server-initiated streams should not be read from in accept path.
 *
 * @param fd  Stream socket file descriptor.
 *
 * @return true if server-initiated, false if client-initiated or on error.
 */
static bool h3_is_server_initiated_stream(int fd)
{
	int stream_type;
	net_socklen_t optlen = sizeof(stream_type);

	if (zsock_getsockopt(fd, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_STREAM_TYPE,
			     &stream_type, &optlen) < 0) {
		return false;
	}

	return (stream_type & QUIC_STREAM_SERVER) != 0;
}

/**
 * Handle accepted HTTP/3 stream.
 * For peer unidirectional streams, defer stream-type parsing to the poll loop
 * so any bytes already queued on the stream remain available to the normal
 * buffered handling path. For bidirectional streams, the caller should always
 * register the accepted fd for request processing.
 */
static int handle_accepted_stream(struct http_client_ctx *client,
				  int conn_sock,
				  int stream_sock,
				  struct net_sockaddr_in6 *addr,
				  net_socklen_t addrlen,
				  int idx)
{
	if (IS_ENABLED(CONFIG_NET_HTTP_SERVER_LOG_LEVEL_DBG)) {
		char addr_str[NET_INET6_ADDRSTRLEN];

		zsock_inet_ntop(addr->sin6_family, &addr->sin6_addr,
				addr_str, sizeof(addr_str));

		LOG_DBG("[%p] H3: Accepted stream socket %d on connection %d from %s%s%s:%d "
			"for client #%d",
			client, stream_sock, conn_sock,
			addr->sin6_family == NET_AF_INET6 ? "[" : "",
			addr_str,
			addr->sin6_family == NET_AF_INET6 ? "]" : "",
			net_ntohs(addr->sin6_port), idx);
	}

	/* Check if this is a unidirectional stream */
	if (h3_is_unidirectional_stream(stream_sock)) {
		/* Server-initiated uni streams (our control/QPACK streams) should not be
		 * handled in accept path, they're already tracked in h3_conn_ctx.
		 * Only process peer (client) initiated uni streams.
		 */
		if (h3_is_server_initiated_stream(stream_sock)) {
			LOG_DBG("[%p] H3: Ignoring server-initiated uni stream (fd=%d)",
				client, stream_sock);
			/* Don't close as this is our stream that we already track */
			return H3_STREAM_IGNORED;
		}

		LOG_DBG("[%p] H3: Accepted peer unidirectional stream (fd=%d)", client,
			stream_sock);

		if (client == NULL) {
			/* No client context yet, close and let it reconnect */
			zsock_close(stream_sock);
			return H3_STREAM_IGNORED;
		}

		/*
		 * Register the peer uni stream so the poll loop can identify it
		 * and process any subsequent control/QPACK data.
		 */
		return 0;
	}

	/* Bidirectional request stream needs normal poll handling */
	return 0;
}

int accept_h3_connection(int h3_listen_sock)
{
	int new_socket;
	net_socklen_t addrlen;
	struct net_sockaddr_in6 addr;

	memset(&addr, 0, sizeof(addr));
	addrlen = sizeof(addr);

	if (!quic_is_connection_socket(h3_listen_sock)) {
		LOG_DBG("H3 not a connection socket (%d)", h3_listen_sock);
		return -EINVAL;
	}

	new_socket = zsock_accept(h3_listen_sock, (struct net_sockaddr *)&addr,
				  &addrlen);
	if (new_socket < 0) {
		new_socket = -errno;
		LOG_DBG("H3 conn accept for socket %d failed (%d)",
			h3_listen_sock, new_socket);
	}

	return new_socket;
}

int accept_h3_stream(int h3_conn_sock, int *stream_sock)
{
	int new_socket;
	net_socklen_t addrlen;
	struct net_sockaddr_in6 addr;
	int ret;

	*stream_sock = INVALID_SOCK;

	memset(&addr, 0, sizeof(addr));
	addrlen = sizeof(addr);

	if (!quic_is_connection_socket(h3_conn_sock)) {
		LOG_DBG("H3 not a connection socket (%d)", h3_conn_sock);
		return -EINVAL;
	}

	new_socket = zsock_accept(h3_conn_sock, (struct net_sockaddr *)&addr,
				  &addrlen);
	if (new_socket < 0) {
		ret = -errno;
		LOG_DBG("H3 stream accept for socket %d failed (%d)",
			h3_conn_sock, ret);
	} else {
		struct http_client_ctx *client;
		int idx;

		client = h3_find_client(h3_conn_sock, &idx);
		if (client == NULL) {
			zsock_close(new_socket);
			return -ENOENT;
		}

		ret = handle_accepted_stream(client, h3_conn_sock,
					     new_socket,
					     &addr, addrlen, idx);
		if (ret == 0) {
			*stream_sock = new_socket;
		}
	}

	return ret;
}

int handle_http3_request(struct http_client_ctx *client)
{
	return h3_process_frames(client, false);
}

int handle_http3_stream_fin(struct http_client_ctx *client)
{
	return h3_process_frames(client, true);
}

static void remove_from_polled(int fd,
			       struct zsock_pollfd fds[],
			       int max_fds_count)
{
	for (int i = 0; i < max_fds_count; i++) {
		if (fds[i].fd == fd) {
			fds[i].fd = INVALID_SOCK;
			fds[i].events = 0;
			fds[i].revents = 0;
			break;
		}
	}
}

int h3_client_cleanup(struct http_client_ctx *client,
		      struct zsock_pollfd fds[],
		      int max_fds_count)
{
	struct h3_conn_ctx *ctx;

	ctx = h3_get_conn_ctx(client);
	if (ctx == NULL) {
		LOG_DBG("[%p] H3 connection context not found", client);
	} else {
		if (ctx->peer_control_stream >= 0) {
			remove_from_polled(ctx->peer_control_stream,
					   fds, max_fds_count);
			(void)zsock_close(ctx->peer_control_stream);
			ctx->peer_control_stream = INVALID_SOCK;
			ctx->peer_control_rx.data_len = 0;
		}

		if (ctx->peer_qpack_encoder_stream >= 0) {
			remove_from_polled(ctx->peer_qpack_encoder_stream,
					   fds, max_fds_count);
			(void)zsock_close(ctx->peer_qpack_encoder_stream);
			ctx->peer_qpack_encoder_stream = INVALID_SOCK;
			ctx->peer_qpack_encoder_rx.data_len = 0;
		}

		if (ctx->peer_qpack_decoder_stream >= 0) {
			remove_from_polled(ctx->peer_qpack_decoder_stream,
					   fds, max_fds_count);
			(void)zsock_close(ctx->peer_qpack_decoder_stream);
			ctx->peer_qpack_decoder_stream = INVALID_SOCK;
			ctx->peer_qpack_decoder_rx.data_len = 0;
		}

		if (ctx->local_control_stream >= 0) {
			remove_from_polled(ctx->local_control_stream,
					   fds, max_fds_count);
			(void)zsock_close(ctx->local_control_stream);
			ctx->local_control_stream = INVALID_SOCK;
		}

		if (ctx->local_qpack_encoder_stream >= 0) {
			remove_from_polled(ctx->local_qpack_encoder_stream,
					   fds, max_fds_count);
			(void)zsock_close(ctx->local_qpack_encoder_stream);
			ctx->local_qpack_encoder_stream = INVALID_SOCK;
		}

		if (ctx->local_qpack_decoder_stream >= 0) {
			remove_from_polled(ctx->local_qpack_decoder_stream,
					   fds, max_fds_count);
			(void)zsock_close(ctx->local_qpack_decoder_stream);
			ctx->local_qpack_decoder_stream = INVALID_SOCK;
		}

		/*
		 * Close all bidi request stream fds tracked in stream_sock[].
		 * This triggers quic_stream_close_vmeth() -> quic_stream_unref()
		 * which is the only path that returns QUIC stream pool slots.
		 * Without this, leaked streams prevent new connections from
		 * opening streams (STREAMS_BLOCKED with pool full).
		 */
		for (int j = 0; j < ARRAY_SIZE(client->h3.stream_sock); j++) {
			int sfd = client->h3.stream_sock[j];

			if (sfd == INVALID_SOCK) {
				continue;
			}

			remove_from_polled(sfd, fds, max_fds_count);
			(void)zsock_close(sfd);
			client->h3.stream_sock[j] = INVALID_SOCK;
			client->h3.headers_sent[j] = false;
			memset(&client->h3.streams[j], 0,
			       sizeof(client->h3.streams[j]));

			LOG_DBG("[%p] H3: closed stream fd %d on cleanup", client, sfd);
		}

		ctx->initialized = false;
	}

	remove_from_polled(client->h3.conn_sock, fds, max_fds_count);

	return 0;
}

struct http_client_ctx *h3_find_client_for_uni_stream(int fd)
{
	for (int i = 0; i < CONFIG_HTTP_SERVER_MAX_CLIENTS; i++) {
		struct h3_conn_ctx *ctx = &h3_conn_contexts[i];

		if (!ctx->initialized) {
			continue;
		}

		if (fd == ctx->peer_control_stream ||
		    fd == ctx->peer_qpack_encoder_stream ||
		    fd == ctx->peer_qpack_decoder_stream) {
			int idx;

			return h3_find_client(h3_conn_ctx_fds[i], &idx);
		}
	}

	return NULL;
}
