/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HTTP2_CLIENT_TEST_PEER_H_
#define HTTP2_CLIENT_TEST_PEER_H_

#include <zephyr/kernel.h>
#include <zephyr/net/http/frame.h>
#include <zephyr/net/http/server.h>

/* A scripted HTTP/2 peer driven from the test thread over a raw socket. It
 * produces the frames the real server never does: control frames, malformed
 * input, padding, split header blocks, stalls and abrupt closes.
 */

#define PEER_PORT 8081
#define PEER_PAYLOAD_MAX 16384
#define PEER_RECV_TIMEOUT_MS 2000
#define PEER_MAX_HEADERS 16
#define PEER_HEADER_NAME_LEN 64
#define PEER_HEADER_VALUE_LEN 128

struct peer_frame {
	uint32_t len;
	uint8_t type;
	uint8_t flags;
	uint32_t stream_id;
	uint8_t payload[PEER_PAYLOAD_MAX];
};

struct peer_header {
	char name[PEER_HEADER_NAME_LEN];
	char value[PEER_HEADER_VALUE_LEN];
};

/* Frames seen from the client, accumulated by every peer_recv_frame() call. */
struct peer_counters {
	int frames;
	int window_update_frames;
	uint32_t window_update_conn;
	uint32_t window_update_stream;
	bool rst_stream_seen;
	uint32_t rst_stream_id;
	uint32_t rst_error;
	bool goaway_seen;
	uint32_t goaway_last_stream;
	uint32_t goaway_error;
	int settings_ack_frames;
	bool ping_ack_seen;
	uint8_t ping_ack_data[8];
	size_t data_bytes;
	bool end_stream_seen;
};

extern struct peer_counters peer_stats;

void peer_stats_reset(void);

int peer_listen(void);
int peer_accept(int listen_fd);
int peer_set_recv_timeout(int conn, uint32_t timeout_ms);
/* Read the client's connection preface without answering it. */
int peer_recv_preface(int conn);
/* Read the client's preface and send an empty SETTINGS frame and the ACK. */
int peer_handshake(int conn);
const uint8_t *peer_client_settings(size_t *len);

int peer_recv_frame(int conn, struct peer_frame *frame);
int peer_recv_frame_type(int conn, uint8_t type, struct peer_frame *frame);
int peer_drain(int conn);

void peer_frame_header(uint8_t *buf, uint32_t len, uint8_t type, uint8_t flags,
		       uint32_t stream_id);
int peer_send_raw(int conn, const uint8_t *buf, size_t len);
int peer_send_sliced(int conn, const uint8_t *buf, size_t len, size_t slice, uint32_t gap_ms);
int peer_send_frame(int conn, uint8_t type, uint8_t flags, uint32_t stream_id,
		    const uint8_t *payload, size_t len);
int peer_encode_headers(uint8_t *buf, size_t buflen, const struct http_header *headers,
			size_t count);
int peer_send_headers(int conn, uint32_t stream_id, uint8_t flags,
		      const struct http_header *headers, size_t count);
int peer_send_data(int conn, uint32_t stream_id, uint8_t flags, const uint8_t *data,
		   size_t len);

int peer_decode_headers(const uint8_t *payload, size_t len, struct peer_header *headers,
			size_t max);
const char *peer_header_value(const struct peer_header *headers, int count, const char *name);

#endif /* HTTP2_CLIENT_TEST_PEER_H_ */
