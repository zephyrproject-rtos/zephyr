/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <strings.h>

#include <zephyr/net/http/hpack.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>

#include "peer.h"

#define PEER_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define PEER_PREFACE_LEN (sizeof(PEER_PREFACE) - 1)
#define PEER_SEND_TIMEOUT K_SECONDS(5)
#define PEER_HEADERS_BUF_SIZE 1024
#define PEER_FLAG_ACK 0x01

struct peer_counters peer_stats;

static struct http_hpack_header_buf hpack_buf;
static uint8_t client_settings[64];
static size_t client_settings_len;
static uint8_t headers_buf[PEER_HEADERS_BUF_SIZE];

void peer_stats_reset(void)
{
	memset(&peer_stats, 0, sizeof(peer_stats));
}

int peer_listen(void)
{
	struct net_sockaddr_in6 sa = { 0 };
	int fd, ret;

	fd = zsock_socket(NET_AF_INET6, NET_SOCK_STREAM, NET_IPPROTO_TCP);
	if (fd < 0) {
		return -errno;
	}

	ret = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_REUSEADDR, &(int){ 1 }, sizeof(int));
	if (ret < 0) {
		ret = -errno;
		goto error;
	}

	sa.sin6_family = NET_AF_INET6;
	sa.sin6_port = net_htons(PEER_PORT);
	sa.sin6_addr = net_in6addr_loopback;

	ret = zsock_bind(fd, (struct net_sockaddr *)&sa, sizeof(sa));
	if (ret < 0) {
		ret = -errno;
		goto error;
	}

	ret = zsock_listen(fd, 2);
	if (ret < 0) {
		ret = -errno;
		goto error;
	}

	return fd;

error:
	zsock_close(fd);

	return ret;
}

int peer_set_recv_timeout(int conn, uint32_t timeout_ms)
{
	struct zsock_timeval tv = {
		.tv_sec = timeout_ms / MSEC_PER_SEC,
		.tv_usec = (timeout_ms % MSEC_PER_SEC) * USEC_PER_MSEC,
	};

	if (zsock_setsockopt(conn, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		return -errno;
	}

	return 0;
}

int peer_accept(int listen_fd)
{
	int conn, ret;

	conn = zsock_accept(listen_fd, NULL, NULL);
	if (conn < 0) {
		return -errno;
	}

	ret = peer_set_recv_timeout(conn, PEER_RECV_TIMEOUT_MS);
	if (ret < 0) {
		zsock_close(conn);
		return ret;
	}

	return conn;
}

static int recv_exact(int conn, uint8_t *buf, size_t len)
{
	size_t got = 0;

	while (got < len) {
		int ret = zsock_recv(conn, buf + got, len - got, 0);

		if (ret < 0) {
			return -errno;
		}

		if (ret == 0) {
			return -ENOTCONN;
		}

		got += ret;
	}

	return 0;
}

static void stats_update(const struct peer_frame *frame)
{
	peer_stats.frames++;

	switch (frame->type) {
	case HTTP2_WINDOW_UPDATE_FRAME: {
		uint32_t inc = sys_get_be32(frame->payload) & HTTP2_FRAME_STREAM_ID_MASK;

		peer_stats.window_update_frames++;
		if (frame->stream_id == 0) {
			peer_stats.window_update_conn += inc;
		} else {
			peer_stats.window_update_stream += inc;
		}
		break;
	}
	case HTTP2_RST_STREAM_FRAME:
		peer_stats.rst_stream_seen = true;
		peer_stats.rst_stream_id = frame->stream_id;
		peer_stats.rst_error = sys_get_be32(frame->payload);
		break;
	case HTTP2_GOAWAY_FRAME:
		peer_stats.goaway_seen = true;
		peer_stats.goaway_last_stream = sys_get_be32(frame->payload) &
						HTTP2_FRAME_STREAM_ID_MASK;
		peer_stats.goaway_error = sys_get_be32(frame->payload + sizeof(uint32_t));
		break;
	case HTTP2_SETTINGS_FRAME:
		if (frame->flags & PEER_FLAG_ACK) {
			peer_stats.settings_ack_frames++;
		}
		break;
	case HTTP2_PING_FRAME:
		if (frame->flags & PEER_FLAG_ACK) {
			peer_stats.ping_ack_seen = true;
			memcpy(peer_stats.ping_ack_data, frame->payload,
			       sizeof(peer_stats.ping_ack_data));
		}
		break;
	case HTTP2_DATA_FRAME:
		peer_stats.data_bytes += frame->len;
		if (frame->flags & HTTP2_FLAG_END_STREAM) {
			peer_stats.end_stream_seen = true;
		}
		break;
	default:
		break;
	}
}

int peer_recv_frame(int conn, struct peer_frame *frame)
{
	uint8_t hdr[HTTP2_FRAME_HEADER_SIZE];
	int ret;

	ret = recv_exact(conn, hdr, sizeof(hdr));
	if (ret < 0) {
		return ret;
	}

	frame->len = sys_get_be24(&hdr[HTTP2_FRAME_LENGTH_OFFSET]);
	frame->type = hdr[HTTP2_FRAME_TYPE_OFFSET];
	frame->flags = hdr[HTTP2_FRAME_FLAGS_OFFSET];
	frame->stream_id = sys_get_be32(&hdr[HTTP2_FRAME_STREAM_ID_OFFSET]) &
			   HTTP2_FRAME_STREAM_ID_MASK;

	if (frame->len > sizeof(frame->payload)) {
		return -EMSGSIZE;
	}

	ret = recv_exact(conn, frame->payload, frame->len);
	if (ret < 0) {
		return ret;
	}

	stats_update(frame);

	return 0;
}

int peer_recv_frame_type(int conn, uint8_t type, struct peer_frame *frame)
{
	while (true) {
		int ret = peer_recv_frame(conn, frame);

		if (ret < 0) {
			return ret;
		}

		if (frame->type == type) {
			return 0;
		}
	}
}

int peer_drain(int conn)
{
	static struct peer_frame frame;
	int count = 0;

	while (peer_recv_frame(conn, &frame) == 0) {
		count++;
	}

	return count;
}

int peer_recv_preface(int conn)
{
	static struct peer_frame frame;
	uint8_t preface[PEER_PREFACE_LEN];
	int ret;

	ret = recv_exact(conn, preface, sizeof(preface));
	if (ret < 0) {
		return ret;
	}

	if (memcmp(preface, PEER_PREFACE, sizeof(preface)) != 0) {
		return -EBADMSG;
	}

	ret = peer_recv_frame(conn, &frame);
	if (ret < 0) {
		return ret;
	}

	if (frame.type != HTTP2_SETTINGS_FRAME || (frame.flags & PEER_FLAG_ACK) != 0) {
		return -EBADMSG;
	}

	client_settings_len = MIN(frame.len, sizeof(client_settings));
	memcpy(client_settings, frame.payload, client_settings_len);

	return 0;
}

int peer_handshake(int conn)
{
	int ret;

	ret = peer_recv_preface(conn);
	if (ret < 0) {
		return ret;
	}

	ret = peer_send_frame(conn, HTTP2_SETTINGS_FRAME, 0, 0, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* The client's acknowledgment of our SETTINGS is not awaited here: a
	 * synchronous session only reads the socket while a request is in
	 * flight, so it arrives with the first request. It is counted in
	 * peer_stats whenever it is read.
	 */
	return peer_send_frame(conn, HTTP2_SETTINGS_FRAME, PEER_FLAG_ACK, 0, NULL, 0);
}

const uint8_t *peer_client_settings(size_t *len)
{
	*len = client_settings_len;

	return client_settings;
}

void peer_frame_header(uint8_t *buf, uint32_t len, uint8_t type, uint8_t flags,
		       uint32_t stream_id)
{
	sys_put_be24(len, &buf[HTTP2_FRAME_LENGTH_OFFSET]);
	buf[HTTP2_FRAME_TYPE_OFFSET] = type;
	buf[HTTP2_FRAME_FLAGS_OFFSET] = flags;
	sys_put_be32(stream_id, &buf[HTTP2_FRAME_STREAM_ID_OFFSET]);
}

int peer_send_raw(int conn, const uint8_t *buf, size_t len)
{
	return zsock_send_all(conn, buf, len, 0, PEER_SEND_TIMEOUT, NULL);
}

int peer_send_sliced(int conn, const uint8_t *buf, size_t len, size_t slice, uint32_t gap_ms)
{
	for (size_t off = 0; off < len; off += slice) {
		int ret = peer_send_raw(conn, buf + off, MIN(slice, len - off));

		if (ret < 0) {
			return ret;
		}

		k_sleep(K_MSEC(gap_ms));
	}

	return 0;
}

int peer_send_frame(int conn, uint8_t type, uint8_t flags, uint32_t stream_id,
		    const uint8_t *payload, size_t len)
{
	uint8_t hdr[HTTP2_FRAME_HEADER_SIZE];
	int ret;

	peer_frame_header(hdr, len, type, flags, stream_id);

	ret = peer_send_raw(conn, hdr, sizeof(hdr));
	if (ret < 0 || len == 0) {
		return ret;
	}

	return peer_send_raw(conn, payload, len);
}

int peer_encode_headers(uint8_t *buf, size_t buflen, const struct http_header *headers,
			size_t count)
{
	size_t off = 0;

	for (size_t i = 0; i < count; i++) {
		int ret;

		hpack_buf.name = headers[i].name;
		hpack_buf.name_len = strlen(headers[i].name);
		hpack_buf.value = headers[i].value;
		hpack_buf.value_len = strlen(headers[i].value);

		ret = http_hpack_encode_header(buf + off, buflen - off, &hpack_buf);
		if (ret < 0) {
			return ret;
		}

		off += ret;
	}

	return (int)off;
}

int peer_send_headers(int conn, uint32_t stream_id, uint8_t flags,
		      const struct http_header *headers, size_t count)
{
	int len;

	len = peer_encode_headers(headers_buf, sizeof(headers_buf), headers, count);
	if (len < 0) {
		return len;
	}

	return peer_send_frame(conn, HTTP2_HEADERS_FRAME, flags, stream_id, headers_buf, len);
}

int peer_send_data(int conn, uint32_t stream_id, uint8_t flags, const uint8_t *data,
		   size_t len)
{
	return peer_send_frame(conn, HTTP2_DATA_FRAME, flags, stream_id, data, len);
}

int peer_decode_headers(const uint8_t *payload, size_t len, struct peer_header *headers,
			size_t max)
{
	size_t off = 0;
	int count = 0;

	while (off < len) {
		int ret;

		hpack_buf.name_len = 0;

		ret = http_hpack_decode_header(payload + off, len - off, &hpack_buf);
		if (ret <= 0) {
			return (ret == 0) ? -EBADMSG : ret;
		}

		off += ret;

		if (hpack_buf.name_len == 0) {
			/* Dynamic table size update, not a field. */
			continue;
		}

		if (count >= max) {
			return -ENOMEM;
		}

		snprintk(headers[count].name, sizeof(headers[count].name), "%.*s",
			 (int)hpack_buf.name_len, hpack_buf.name);
		snprintk(headers[count].value, sizeof(headers[count].value), "%.*s",
			 (int)hpack_buf.value_len, hpack_buf.value);
		count++;
	}

	return count;
}

const char *peer_header_value(const struct peer_header *headers, int count, const char *name)
{
	for (int i = 0; i < count; i++) {
		if (strcasecmp(headers[i].name, name) == 0) {
			return headers[i].value;
		}
	}

	return NULL;
}
