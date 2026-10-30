/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>
#include <errno.h>
#include <strings.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/http/frame.h>
#include <zephyr/net/http/hpack.h>
#include <zephyr/net/http/parser.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_core.h>

#include "headers/client_internal.h"
#include "headers/http2_internal.h"

LOG_MODULE_DECLARE(net_http_client_mp, CONFIG_NET_HTTP_LOG_LEVEL);

#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"

/* HPACK dynamic table size update to 0 (RFC 7541, Section 6.3). */
#define HPACK_TABLE_SIZE_UPDATE_ZERO 0x20U

/* Receive window granted to the peer on the connection and on each stream. */
#define HTTP2_RX_WINDOW_SIZE CONFIG_HTTP_CLIENT_HTTP2_WINDOW_SIZE

/* Replenish a receive window with a WINDOW_UPDATE once it drops to (or below)
 * this level, so that the peer is not stalled waiting for flow-control credit.
 */
#define HTTP2_WINDOW_UPDATE_THRESHOLD (HTTP2_RX_WINDOW_SIZE / 2)

/* Fail the connection (RFC 9113, Section 5.4.1): the error code is sent in
 * the GOAWAY frame once the session is torn down.
 */
static int http2_connection_error(struct http2_client_parser *parser, enum http2_error error,
				  const char *reason)
{
	NET_ERR("HTTP/2 connection error %u: %s", error, reason);
	parser->goaway_error = error;

	return -EBADMSG;
}

/* Whether the server refers to a stream the client never opened: an even
 * identifier, as server push is disabled, or one above the last opened. Most
 * frames on such a stream are a connection error (RFC 9113, Section 5.1).
 */
static bool http2_stream_is_idle(const struct http2_client_parser *parser, uint32_t stream_id)
{
	return (stream_id % 2 == 0) || stream_id > parser->last_stream_id;
}

/* HTTP/2 multiplexes its streams itself, so everything goes out on the
 * connection. A failed send may leave part of a frame on the wire, after
 * which nothing more can be sent, not even a RST_STREAM or GOAWAY: the
 * session ends right away, without notifying the peer, every transaction in
 * flight completing with the error.
 */
static int http2_send(struct http_client *client, const uint8_t *data, size_t len)
{
	return client->transport_ops->send(client, NULL, data, len, false);
}

/* A send failed with ret: the session is unusable, close it. May be called
 * with or without the client lock held.
 */
static void http2_send_failed(struct http_client *client, int ret)
{
	k_mutex_lock(&client->lock, K_FOREVER);

	if (client->session.state == HTTP_CLIENT_SESSION_OPEN) {
		NET_ERR("Send failed (%d), closing the connection", ret);
		http_client_session_terminate(client, ret, false);
	}

	k_mutex_unlock(&client->lock);
}

static int send_window_update_now(struct http_client *client, uint32_t stream_id,
				  uint32_t increment);

/* Send a control frame at once if no request frame is going out, otherwise
 * return -EBUSY for the caller to defer it; never waits. Called with the
 * client lock held, so the TX lock is given back directly: closing the
 * transport is left to the request senders.
 */
static int http2_control_send(struct http_client *client, const uint8_t *frame, size_t len)
{
	int ret;

	if (client->session.state == HTTP_CLIENT_SESSION_CLOSED) {
		return -ENOTCONN;
	}

	if (k_mutex_lock(&client->tx_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	ret = http2_send(client, frame, len);
	k_mutex_unlock(&client->tx_lock);

	if (ret < 0) {
		http2_send_failed(client, ret);
	}

	return ret;
}

static int send_preface(struct http_client *client)
{
	return http2_send(client, HTTP2_PREFACE, sizeof(HTTP2_PREFACE) - 1);
}

static int send_settings(struct http_client *client)
{
	/* No dynamic header compression table, no server push, as many
	 * streams as the session has room for, and the configured window on
	 * each of them.
	 */
	static const struct http2_setting settings[] = {
		{ HTTP2_SETTINGS_HEADER_TABLE_SIZE, 0 },
		{ HTTP2_SETTINGS_ENABLE_PUSH, 0 },
		{ HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS, HTTP_CLIENT_MAX_STREAMS },
		{ HTTP2_SETTINGS_INITIAL_WINDOW_SIZE, HTTP2_RX_WINDOW_SIZE },
	};
	uint8_t frame[HTTP2_SETTINGS_FRAME_SIZE(ARRAY_SIZE(settings))];
	int len;

	len = http2_frame_settings_encode(frame, sizeof(frame), settings, ARRAY_SIZE(settings));

	return http2_send(client, frame, len);
}

static int send_settings_ack(struct http_client *client)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	uint8_t frame[HTTP2_FRAME_HEADER_SIZE];
	int len, ret;

	len = http2_frame_settings_ack_encode(frame, sizeof(frame));

	ret = http2_control_send(client, frame, len);
	if (ret == -EBUSY) {
		/* Each SETTINGS frame gets its own acknowledgment; a peer that
		 * sends them by the hundreds within one request frame is
		 * flooding the connection.
		 */
		if (parser->deferred.settings_acks == UINT8_MAX) {
			return http2_connection_error(parser, HTTP2_ERROR_ENHANCE_YOUR_CALM,
						      "Too many SETTINGS to acknowledge");
		}

		parser->deferred.settings_acks++;
		return 0;
	}

	return ret;
}

static int send_ping_ack(struct http_client *client, const uint8_t *opaque_data)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	uint8_t frame[HTTP2_PING_FRAME_SIZE];
	int len, ret;

	len = http2_frame_ping_encode(frame, sizeof(frame), true, opaque_data);

	ret = http2_control_send(client, frame, len);
	if (ret == -EBUSY) {
		struct http2_client_deferred *deferred = &parser->deferred;

		/* Each PING is answered with its own payload (RFC 9113,
		 * Section 6.7). Servers may send them with every burst of data,
		 * and a request frame may take a while to go out: once the
		 * queue is full, the oldest PING goes unanswered rather than
		 * the connection.
		 */
		if (deferred->ping_count == ARRAY_SIZE(deferred->ping_data)) {
			NET_DBG("Too many PINGs to acknowledge, dropping the oldest");
			memmove(deferred->ping_data[0], deferred->ping_data[1],
				sizeof(deferred->ping_data) - HTTP2_PING_FRAME_LEN);
			deferred->ping_count--;
		}

		memcpy(deferred->ping_data[deferred->ping_count], opaque_data,
		       HTTP2_PING_FRAME_LEN);
		deferred->ping_count++;
		return 0;
	}

	return ret;
}

/* The senders of request frames serialize on the TX lock, taken without the
 * client lock held so that the receive context is never blocked by a send.
 */
static void http2_tx_acquire(struct http_client *client)
{
	http_client_tx_acquire(client);
}

/* Give the TX lock up, after sending the control frames deferred while it
 * was held. Called with the TX lock held and the client lock released;
 * returns with both released.
 */
static void http2_tx_release(struct http_client *client)
{
	struct http2_client_parser *parser = &client->session.parser.http2;

	while (true) {
		struct http2_client_deferred deferred;
		struct {
			uint32_t id;
			uint32_t increment;
		} windows[HTTP_CLIENT_MAX_STREAMS];
		size_t window_count = 0;
		uint8_t frame[HTTP2_PING_FRAME_SIZE];
		bool pending;
		int ret = 0;

		k_mutex_lock(&client->lock, K_FOREVER);

		deferred = parser->deferred;
		memset(&parser->deferred, 0, sizeof(parser->deferred));

		ARRAY_FOR_EACH(client->session.streams, i) {
			struct http_client_stream *stream = &client->session.streams[i];

			if (stream->is_used && stream->proto.http2.window_update_pending > 0) {
				windows[window_count].id = (uint32_t)stream->stream_id;
				windows[window_count].increment =
					stream->proto.http2.window_update_pending;
				window_count++;
				stream->proto.http2.window_update_pending = 0;
			}
		}

		pending = deferred.settings_acks > 0 || deferred.ping_count > 0 ||
			  deferred.conn_window > 0 || deferred.rst_count > 0 || window_count > 0;

		if (!pending || client->session.state != HTTP_CLIENT_SESSION_OPEN) {
			k_mutex_unlock(&client->lock);
			http_client_tx_release(client);
			return;
		}

		k_mutex_unlock(&client->lock);

		/* SETTINGS ACK, one per SETTINGS frame received. */
		while (ret >= 0 && deferred.settings_acks > 0) {
			ret = http2_send(client, frame,
					 http2_frame_settings_ack_encode(frame, sizeof(frame)));
			deferred.settings_acks--;
		}

		/* PING ACK for each PING received, echoing its payload. */
		for (size_t i = 0; ret >= 0 && i < deferred.ping_count; i++) {
			ret = http2_send(client, frame,
					 http2_frame_ping_encode(frame, sizeof(frame), true,
								 deferred.ping_data[i]));
		}

		/* WINDOW_UPDATE for the connection, with the credit accumulated. */
		if (ret >= 0 && deferred.conn_window > 0) {
			ret = send_window_update_now(client, 0, deferred.conn_window);
		}

		/* WINDOW_UPDATE for each stream with credit accumulated. */
		for (size_t i = 0; ret >= 0 && i < window_count; i++) {
			ret = send_window_update_now(client, windows[i].id, windows[i].increment);
		}

		/* RST_STREAM for each stream failed meanwhile. */
		for (size_t i = 0; ret >= 0 && i < deferred.rst_count; i++) {
			ret = http2_send(client, frame,
					 http2_frame_rst_stream_encode(frame, sizeof(frame),
								       deferred.rst[i].id,
								       deferred.rst[i].error));
		}

		if (ret < 0) {
			http2_send_failed(client, ret);
		}
	}
}

static int send_window_update_now(struct http_client *client, uint32_t stream_id,
				  uint32_t increment)
{
	uint8_t frame[HTTP2_WINDOW_UPDATE_FRAME_SIZE];
	int len;

	len = http2_frame_window_update_encode(frame, sizeof(frame), stream_id, increment);

	return http2_send(client, frame, len);
}

static int send_window_update(struct http_client *client, uint32_t stream_id,
			      uint32_t increment)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	uint8_t frame[HTTP2_WINDOW_UPDATE_FRAME_SIZE];
	int len, ret;

	len = http2_frame_window_update_encode(frame, sizeof(frame), stream_id, increment);

	ret = http2_control_send(client, frame, len);
	if (ret != -EBUSY) {
		return ret;
	}

	/* Credit accumulates until the sender flushes it. */
	if (stream_id == 0) {
		parser->deferred.conn_window += increment;
	} else {
		struct http_client_stream *stream = http_client_stream_lookup(client, stream_id);

		if (stream != NULL) {
			stream->proto.http2.window_update_pending += increment;
		}
	}

	return 0;
}

static int send_rst_stream(struct http_client *client, uint32_t stream_id,
			   enum http2_error error)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	uint8_t frame[HTTP2_RST_STREAM_FRAME_SIZE];
	int len, ret;

	stream_id = stream_id & HTTP2_FRAME_STREAM_ID_MASK;
	if (stream_id == 0) {
		return -EINVAL;
	}

	len = http2_frame_rst_stream_encode(frame, sizeof(frame), stream_id, error);

	ret = http2_control_send(client, frame, len);
	if (ret != -EBUSY) {
		return ret;
	}

	if (parser->deferred.rst_count >= ARRAY_SIZE(parser->deferred.rst)) {
		/* A courtesy to the peer; the stream is over here regardless. */
		NET_WARN("No room to defer RST_STREAM for stream %u", stream_id);
		return 0;
	}

	parser->deferred.rst[parser->deferred.rst_count].id = stream_id;
	parser->deferred.rst[parser->deferred.rst_count].error = error;
	parser->deferred.rst_count++;

	return 0;
}

/* Fail a stream (RFC 9113, Section 5.4.2): reset it and complete the
 * transaction with result. The connection and the other streams go on.
 */
static void http2_stream_error(struct http_client *client, struct http_client_stream *stream,
			       enum http2_error error, int result, const char *reason)
{
	NET_ERR("HTTP/2 stream %u error %u: %s", (uint32_t)stream->stream_id, error, reason);
	(void)send_rst_stream(client, (uint32_t)stream->stream_id, error);
	http_client_stream_set_complete(client, stream, result);
}

static int send_goaway(struct http_client *client, int reason)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	enum http2_error error;

	if (reason == 0) {
		error = HTTP2_ERROR_NO_ERROR;
	} else if (reason != -EBADMSG) {
		error = HTTP2_ERROR_INTERNAL_ERROR;
	} else if (parser->goaway_error != HTTP2_ERROR_NO_ERROR) {
		error = parser->goaway_error;
	} else {
		/* A malformed frame rejected by the shared frame code. */
		error = HTTP2_ERROR_PROTOCOL_ERROR;
	}
	uint8_t frame[HTTP2_GOAWAY_FRAME_SIZE];
	int len, ret;

	/* No server-initiated stream is ever accepted, so the last one is 0. */
	len = http2_frame_goaway_encode(frame, sizeof(frame), 0, error);

	/* The transport closes right after; a GOAWAY that cannot go out at
	 * once is not worth waiting for.
	 */
	ret = http2_control_send(client, frame, len);

	return (ret == -EBUSY) ? 0 : ret;
}

/* Request header block encoder. Fields are HPACK-encoded into the session TX
 * buffer; when a field does not fit, the buffer is sent as a HEADERS frame (the
 * first time) or a CONTINUATION frame and the field is retried, so the buffer
 * size bounds a single field rather than the whole block.
 */
struct http2_header_block_tx {
	struct http_client *client;
	struct http_client_stream *stream;
	struct http_hpack_header_buf field;
	uint32_t table_size_changes;
	bool end_stream;
	bool started;
	bool send_failed;
	size_t max_len;
	size_t frames;
	size_t len;
};

/* Number the stream as its HEADERS frame is about to go out: identifiers
 * then follow the order on the wire, and a request that never got that far
 * takes none. Called with the TX lock held.
 */
static int http2_stream_number(struct http_client *client, struct http_client_stream *stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	int ret = 0;

	k_mutex_lock(&client->lock, K_FOREVER);

	if (stream->completed) {
		/* Ended meanwhile, with the session when that is over. */
		ret = (stream->result < 0) ? stream->result : -EALREADY;
	} else if (client->session.state != HTTP_CLIENT_SESSION_OPEN) {
		ret = -ECONNABORTED;
	} else if (parser->going_away ||
		   parser->last_stream_id >= HTTP2_FRAME_STREAM_ID_MASK - 1) {
		ret = -ESHUTDOWN;
	} else {
		/* Client-initiated streams use odd identifiers (RFC 9113,
		 * Section 5.1.1).
		 */
		parser->last_stream_id = (parser->last_stream_id == 0) ?
						 1 : parser->last_stream_id + 2;
		stream->stream_id = parser->last_stream_id;
	}

	k_mutex_unlock(&client->lock);

	return ret;
}

static int send_data(struct http_client *client, struct http_client_stream *stream,
		     const uint8_t *data, size_t len, bool end_stream);

static int header_block_flush(struct http2_header_block_tx *tx, bool end_headers)
{
	uint8_t *frame = tx->client->session.tx_buf;
	uint8_t type = tx->started ? HTTP2_CONTINUATION_FRAME : HTTP2_HEADERS_FRAME;
	uint8_t flags = end_headers ? HTTP2_FLAG_END_HEADERS : 0;
	int ret;

	/* END_STREAM goes on the HEADERS frame only when the whole block fits
	 * in it. A block that spills into CONTINUATION frames is ended by an
	 * empty DATA frame instead, so that a block cut short by an error
	 * leaves the server with a request it cannot run.
	 */
	if (!tx->started && tx->end_stream && end_headers) {
		flags |= HTTP2_FLAG_END_STREAM;
	}

	if (!tx->started) {
		ret = http2_stream_number(tx->client, tx->stream);
		if (ret < 0) {
			return ret;
		}
	}

	http2_frame_header_encode(frame, tx->len, type, flags,
				  (uint32_t)tx->stream->stream_id);

	ret = http2_send(tx->client, frame, HTTP2_FRAME_HEADER_SIZE + tx->len);
	if (ret < 0) {
		tx->send_failed = true;
	}

	tx->started = true;
	tx->frames++;
	tx->len = 0;

	return ret;
}

static int http2_add_field(const char *name, const char *value, void *user_data)
{
	struct http2_header_block_tx *tx = user_data;
	uint8_t *buf = tx->client->session.tx_buf + HTTP2_FRAME_HEADER_SIZE;
	size_t buflen = tx->max_len;
	int ret;

	ret = http_hpack_encode_field(buf + tx->len, buflen - tx->len, &tx->field, name, value);
	if (ret == -ENOBUFS && tx->len > 0) {
		/* Make room by sending what is encoded so far. */
		ret = header_block_flush(tx, false);
		if (ret < 0) {
			return ret;
		}

		ret = http_hpack_encode_field(buf, buflen, &tx->field, name, value);
	}

	if (ret < 0) {
		return ret;
	}

	tx->len += ret;

	return 0;
}

static int send_headers(struct http_client *client, struct http_client_stream *stream,
			bool end_stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	struct http2_header_block_tx tx = {
		.client = client,
		.stream = stream,
		.end_stream = end_stream,
	};
	int ret;

	http2_tx_acquire(client);

	k_mutex_lock(&client->lock, K_FOREVER);

	/* A frame is bounded by the peer's SETTINGS_MAX_FRAME_SIZE as well. */
	tx.max_len = MIN(sizeof(client->session.tx_buf) - HTTP2_FRAME_HEADER_SIZE,
			 parser->peer_max_frame_size);

	tx.table_size_changes = parser->table_size_changes;
	if (parser->table_size_changes != parser->table_size_acked) {
		/* The encoder keeps no dynamic table, so the new limit is
		 * acknowledged with a size update to 0 (RFC 7541, Section 4.2).
		 */
		client->session.tx_buf[HTTP2_FRAME_HEADER_SIZE] = HPACK_TABLE_SIZE_UPDATE_ZERO;
		tx.len = 1;
	}

	k_mutex_unlock(&client->lock);

	ret = http_client_request_fields_foreach(client, stream, true, http2_add_field, &tx);
	if (ret == 0) {
		ret = header_block_flush(&tx, true);
	}

	if (tx.frames > 0 && !(tx.frames == 1 && tx.send_failed)) {
		/* The size update went out with the first frame. */
		k_mutex_lock(&client->lock, K_FOREVER);
		parser->table_size_acked = tx.table_size_changes;
		k_mutex_unlock(&client->lock);
	}

	if (ret < 0) {
		k_mutex_lock(&client->lock, K_FOREVER);

		if (tx.send_failed) {
			http2_send_failed(client, ret);
		} else if (tx.started && client->session.state == HTTP_CLIENT_SESSION_OPEN) {
			/* Part of the block is already on the wire, and nothing
			 * but its continuation may follow (RFC 9113, Section
			 * 6.10): ending the block would hand the server a
			 * request with fields missing, and no other frame may
			 * be sent. The transport is closed instead, with this
			 * request reporting the cause and the other ones the
			 * lost connection.
			 */
			NET_ERR("Request header block cut short, closing the connection");
			http_client_stream_set_complete(client, stream, ret);
			http_client_session_terminate(client, -ECONNABORTED, false);
		}

		k_mutex_unlock(&client->lock);
	}

	http2_tx_release(client);

	if (ret == 0 && end_stream && tx.frames > 1) {
		/* The empty DATA frame ending the stream, see header_block_flush(). */
		return send_data(client, stream, NULL, 0, true);
	}

	return ret;
}

/* Block until both the connection and the stream send window allow for some
 * data to be sent.
 */
static int http2_wait_send_window(struct http_client *client, struct http_client_stream *stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;

	while (parser->send_window <= 0 || stream->proto.http2.send_window <= 0) {
		int ret = http_client_stream_wait_tx(client, stream);

		if (ret == -EALREADY) {
			/* The response is complete. Tell the server that the
			 * rest of the request is not coming.
			 */
			(void)send_rst_stream(client, (uint32_t)stream->stream_id,
					      HTTP2_ERROR_NO_ERROR);
		}

		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int send_data(struct http_client *client, struct http_client_stream *stream,
		     const uint8_t *data, size_t len, bool end_stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	uint8_t frame_header[HTTP2_FRAME_HEADER_SIZE];
	uint32_t stream_id = stream->stream_id & HTTP2_FRAME_STREAM_ID_MASK;
	int ret;

	if (stream_id == 0 || (len > 0 && data == NULL)) {
		return -EINVAL;
	}

	/* The payload is sent directly from the application buffer, so only the
	 * frame header needs to be encoded here. A single call may need to be
	 * split into several frames to respect the peer's maximum frame size
	 * and the flow control windows. The loop runs at least once, so that a
	 * zero length frame can be used to terminate the stream. The credit
	 * for a frame is taken under the client lock with the TX lock already
	 * held: a SETTINGS the peer sends meanwhile is acknowledged by the
	 * holder of the TX lock, and the frame has to respect it from then on.
	 * The wait for credit happens with neither lock held.
	 */
	do {
		size_t frame_len;
		uint8_t flags = 0;

		http2_tx_acquire(client);
		k_mutex_lock(&client->lock, K_FOREVER);

		if (stream->completed) {
			/* Ended meanwhile, e. g. reset by the peer. */
			ret = (stream->result < 0) ? stream->result : -EALREADY;
			if (ret == -EALREADY) {
				(void)send_rst_stream(client, stream_id, HTTP2_ERROR_NO_ERROR);
			}

			k_mutex_unlock(&client->lock);
			http2_tx_release(client);
			return ret;
		}

		frame_len = MIN(len, parser->peer_max_frame_size);
		if (frame_len > 0 &&
		    (parser->send_window <= 0 || stream->proto.http2.send_window <= 0)) {
			k_mutex_unlock(&client->lock);
			http2_tx_release(client);

			k_mutex_lock(&client->lock, K_FOREVER);
			ret = http2_wait_send_window(client, stream);
			k_mutex_unlock(&client->lock);
			if (ret < 0) {
				return ret;
			}

			continue;
		}

		if (frame_len > 0) {
			frame_len = MIN(frame_len, (size_t)MIN(parser->send_window,
							       stream->proto.http2.send_window));
			parser->send_window -= (int)frame_len;
			stream->proto.http2.send_window -= (int)frame_len;
		}

		k_mutex_unlock(&client->lock);

		len -= frame_len;

		if (end_stream && len == 0) {
			flags |= HTTP2_FLAG_END_STREAM;
		}

		http2_frame_header_encode(frame_header, frame_len, HTTP2_DATA_FRAME, flags,
					  stream_id);

		ret = http2_send(client, frame_header, sizeof(frame_header));
		if (ret >= 0 && frame_len > 0) {
			ret = http2_send(client, data, frame_len);
		}

		if (ret < 0) {
			http2_send_failed(client, ret);
		}

		http2_tx_release(client);

		if (ret < 0) {
			return ret;
		}

		data += frame_len;
	} while (len > 0);

	return 0;
}

/* RX frame parser */

static void http2_parser_init(struct http2_client_parser *parser)
{
	parser->state = HTTP2_CLIENT_STATE_FRAME_HEADER;
	memset(&parser->frame, 0, sizeof(parser->frame));
	parser->expect_continuation = false;
	parser->window_size = HTTP2_RX_WINDOW_SIZE;
	parser->send_window = HTTP2_INITIAL_WINDOW_SIZE;
	parser->peer_initial_window = HTTP2_INITIAL_WINDOW_SIZE;
	parser->peer_max_frame_size = HTTP2_DEFAULT_MAX_FRAME_SIZE;
	parser->peer_max_streams = UINT32_MAX;
	parser->going_away = false;
	parser->last_stream_id = 0;
	parser->headers_stream_id = 0;
	parser->block_field_seen = false;
	parser->goaway_error = HTTP2_ERROR_NO_ERROR;
	parser->preface_received = false;
	parser->settings_acked = false;
	parser->table_size_changes = 0;
	parser->table_size_acked = 0;
	memset(&parser->deferred, 0, sizeof(parser->deferred));
}

/* HTTP2 operations */

static int http2_client_session_open(struct http_client *client)
{
	int ret;

	http2_parser_init(&client->session.parser.http2);

	ret = send_preface(client);
	if (ret < 0) {
		NET_ERR("Failed to send HTTP/2 %s, err %d", "PREFACE", ret);
		return ret;
	}

	ret = send_settings(client);
	if (ret < 0) {
		NET_ERR("Failed to send HTTP/2 %s, err %d", "SETTINGS", ret);
		return ret;
	}

	/* The connection window starts at the protocol's initial size no
	 * matter what SETTINGS says (RFC 9113, Section 6.9.2); the rest of
	 * the configured window is granted explicitly.
	 */
	if (HTTP2_RX_WINDOW_SIZE > HTTP2_INITIAL_WINDOW_SIZE) {
		ret = send_window_update_now(client, 0,
					     HTTP2_RX_WINDOW_SIZE - HTTP2_INITIAL_WINDOW_SIZE);
		if (ret < 0) {
			NET_ERR("Failed to send HTTP/2 %s, err %d", "WINDOW_UPDATE", ret);
		}
	}

	return ret;
}

static int http2_client_session_close(struct http_client *client, int reason)
{
	int ret;

	ret = send_goaway(client, reason);
	if (ret < 0) {
		NET_ERR("Failed to send HTTP/2 %s, err %d", "GO AWAY", ret);
	}

	return ret;
}

static int http2_client_stream_open(struct http_client *client,
				    struct http_client_stream *stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	uint32_t open_streams = 0;

	if (parser->going_away) {
		NET_DBG("Server is closing the connection, no new streams");
		return -ESHUTDOWN;
	}

	/* Identifiers are never reused: a connection that used them all up is
	 * done, like one the server is closing (RFC 9113, Section 5.1.1).
	 */
	if (parser->last_stream_id >= HTTP2_FRAME_STREAM_ID_MASK - 1) {
		NET_ERR("Out of stream identifiers, a new connection is needed");
		return -ESHUTDOWN;
	}

	/* The stream being opened is already counted as used. */
	ARRAY_FOR_EACH(client->session.streams, i) {
		if (client->session.streams[i].is_used) {
			open_streams++;
		}
	}

	if (open_streams > parser->peer_max_streams) {
		NET_ERR("Peer allows no more than %u concurrent streams",
			parser->peer_max_streams);
		return -EBUSY;
	}

	/* The identifier is assigned when the HEADERS frame goes out, so that
	 * identifiers follow the order on the wire (RFC 9113, Section 5.1.1).
	 */
	stream->stream_id = 0;
	stream->proto.http2.window_size = HTTP2_RX_WINDOW_SIZE;
	stream->proto.http2.send_window = (int)parser->peer_initial_window;
	stream->proto.http2.body_received = 0;

	return 0;
}

static int http2_client_stream_reset(struct http_client *client,
				     struct http_client_stream *stream)
{
	int ret;

	if (stream->stream_id == 0) {
		/* Never put on the wire, nothing to reset. */
		return 0;
	}

	ret = send_rst_stream(client, (uint32_t)stream->stream_id, HTTP2_ERROR_CANCEL);
	if (ret < 0) {
		NET_ERR("Failed to send HTTP/2 %s, err %d", "RST_STREAM", ret);
	}

	return ret;
}

static int http2_client_stream_send_headers(struct http_client *client,
					    struct http_client_stream *stream,
					    const struct http_client_request *req, bool end_stream)
{
	int ret;

	ARG_UNUSED(req);

	ret = send_headers(client, stream, end_stream);
	if (ret < 0) {
		NET_ERR("Failed to send HTTP/2 %s, err %d", "HEADERS", ret);
	}

	return ret;
}

static int http2_client_stream_send_data(struct http_client *client,
					 struct http_client_stream *stream, const uint8_t *data,
					 size_t len, bool end_stream)
{
	int ret;

	ret = send_data(client, stream, data, len, end_stream);
	if (ret < 0) {
		NET_ERR("Failed to send HTTP/2 %s, err %d", "DATA", ret);
	}

	return ret;
}

/* Apply a new SETTINGS_INITIAL_WINDOW_SIZE: the change also applies to the
 * send window of every open stream (RFC 9113, Section 6.9.2), which may turn
 * it negative.
 */
static int http2_apply_initial_window(struct http_client *client,
				      struct http2_client_parser *parser, uint32_t value)
{
	int64_t delta;

	if (value > HTTP2_MAX_WINDOW_SIZE) {
		return http2_connection_error(parser, HTTP2_ERROR_FLOW_CONTROL_ERROR,
					      "invalid SETTINGS_INITIAL_WINDOW_SIZE");
	}

	delta = (int64_t)value - parser->peer_initial_window;

	ARRAY_FOR_EACH(client->session.streams, i) {
		struct http_client_stream *stream = &client->session.streams[i];

		if (!stream->is_used) {
			continue;
		}

		if (stream->proto.http2.send_window + delta > HTTP2_MAX_WINDOW_SIZE) {
			return http2_connection_error(parser, HTTP2_ERROR_FLOW_CONTROL_ERROR,
						      "stream send window overflow");
		}

		stream->proto.http2.send_window += (int)delta;
	}

	parser->peer_initial_window = value;

	return 0;
}

static int http2_apply_setting(struct http_client *client, struct http2_client_parser *parser,
			       uint16_t id, uint32_t value)
{
	NET_DBG("HTTP/2 SETTINGS: id %u = %u", id, value);

	switch (id) {
	case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
		return http2_apply_initial_window(client, parser, value);
	case HTTP2_SETTINGS_MAX_FRAME_SIZE:
		if (value < HTTP2_DEFAULT_MAX_FRAME_SIZE || value > HTTP2_MAX_FRAME_SIZE_LIMIT) {
			return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
						      "invalid SETTINGS_MAX_FRAME_SIZE");
		}

		parser->peer_max_frame_size = value;
		break;
	case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
		parser->peer_max_streams = value;
		break;
	case HTTP2_SETTINGS_ENABLE_PUSH:
		/* A server may only set this to 0 (RFC 9113, Section 6.5.2). */
		if (value != 0) {
			return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
						      "invalid SETTINGS_ENABLE_PUSH");
		}

		break;
	case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
		/* Acknowledged at the start of the next header block sent. */
		parser->table_size_changes++;
		break;
	default:
		/* Unknown or irrelevant to a client, ignore. */
		break;
	}

	return 0;
}

/* Enter the SETTINGS state on a SETTINGS frame, or acknowledge it right away
 * when it carries nothing. SETTINGS frames are always associated with the
 * connection (RFC 9113, Section 6.5).
 */
static int http2_enter_settings(struct http_client *client)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	int ret;

	if (parser->frame.stream_identifier != 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "SETTINGS on a stream");
	}

	if (parser->frame.flags & HTTP2_FLAG_ACK) {
		/* Peer acknowledged our SETTINGS; must carry no payload. */
		if (parser->frame.length != 0) {
			return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
						      "SETTINGS ACK with a payload");
		}

		parser->settings_acked = true;

		return HTTP2_FRAME_HEADER_SIZE;
	}

	if ((parser->frame.length % HTTP2_SETTINGS_FIELD_LEN) != 0) {
		return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
					      "invalid SETTINGS length");
	}

	if (parser->frame.length == 0) {
		ret = send_settings_ack(client);
		if (ret < 0) {
			return ret;
		}

		return HTTP2_FRAME_HEADER_SIZE;
	}

	parser->state = HTTP2_CLIENT_STATE_SETTINGS;

	return HTTP2_FRAME_HEADER_SIZE;
}

/* Apply the parameters of the current SETTINGS frame one by one, as they are
 * buffered, so that the frame need not fit in the receive buffer as a whole.
 */
static int http2_process_settings(struct http_client *client, uint8_t *data, size_t len)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	size_t consumed = 0;
	int ret;

	while (parser->frame.length > 0 && len - consumed >= HTTP2_SETTINGS_FIELD_LEN) {
		uint16_t id = sys_get_be16(data + consumed);
		uint32_t value = sys_get_be32(data + consumed + sizeof(uint16_t));

		ret = http2_apply_setting(client, parser, id, value);
		if (ret < 0) {
			return ret;
		}

		consumed += HTTP2_SETTINGS_FIELD_LEN;
		parser->frame.length -= HTTP2_SETTINGS_FIELD_LEN;
	}

	if (parser->frame.length > 0) {
		return (consumed > 0) ? (int)consumed : -EAGAIN;
	}

	parser->state = HTTP2_CLIENT_STATE_FRAME_HEADER;

	/* A larger initial window may let a blocked request proceed. */
	http_client_tx_wakeup(client);

	ret = send_settings_ack(client);
	if (ret < 0) {
		return ret;
	}

	return (int)consumed;
}

static int handle_rx_window_update(struct http_client *client,
				   struct http2_client_parser *parser, const uint8_t *payload)
{
	struct http_client_stream *stream;
	uint32_t increment;

	if (parser->frame.stream_identifier != 0 &&
	    http2_stream_is_idle(parser, parser->frame.stream_identifier)) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "WINDOW_UPDATE on an idle stream");
	}

	/* The most significant bit is reserved (RFC 9113, Section 6.9). */
	increment = sys_get_be32(payload) & HTTP2_FRAME_STREAM_ID_MASK;

	NET_DBG("HTTP/2 WINDOW_UPDATE: stream %u, increment %u",
		parser->frame.stream_identifier, increment);

	if (increment == 0 && parser->frame.stream_identifier == 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "WINDOW_UPDATE with a zero increment");
	}

	if (parser->frame.stream_identifier == 0) {
		if ((int64_t)parser->send_window + increment > HTTP2_MAX_WINDOW_SIZE) {
			return http2_connection_error(parser, HTTP2_ERROR_FLOW_CONTROL_ERROR,
						      "connection send window overflow");
		}

		parser->send_window += (int)increment;
	} else {
		stream = http_client_stream_lookup(client, parser->frame.stream_identifier);
		if (stream == NULL) {
			/* A stream we no longer track, nothing to credit. */
			return 0;
		}

		/* An error of the stream only (RFC 9113, Section 6.9). */
		if (increment == 0) {
			http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
					   "WINDOW_UPDATE with a zero increment");
			return 0;
		}

		if ((int64_t)stream->proto.http2.send_window + increment > HTTP2_MAX_WINDOW_SIZE) {
			/* An error of the stream only (RFC 9113, Section 6.9.1). */
			http2_stream_error(client, stream, HTTP2_ERROR_FLOW_CONTROL_ERROR, -EPROTO,
					   "send window overflow");
			return 0;
		}

		stream->proto.http2.send_window += (int)increment;
	}

	http_client_tx_wakeup(client);

	return 0;
}

static int handle_rx_ping(struct http_client *client,
			  struct http2_client_parser *parser, const uint8_t *payload)
{
	/* PING frames are always associated with the connection (RFC 9113,
	 * Section 6.7).
	 */
	if (parser->frame.stream_identifier != 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "PING on a stream");
	}

	if (parser->frame.flags & HTTP2_FLAG_ACK) {
		/* Response to a PING we sent; we do not send any, so ignore. */
		return 0;
	}

	return send_ping_ack(client, payload);
}

static int handle_rx_priority(struct http_client *client,
			      struct http2_client_parser *parser, const uint8_t *payload)
{
	ARG_UNUSED(client);
	ARG_UNUSED(payload);

	/* PRIORITY is only ever about a stream (RFC 9113, Section 6.3). */
	if (parser->frame.stream_identifier == 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "PRIORITY on stream 0");
	}

	/* PRIORITY signalling is deprecated by RFC 9113; ignore the contents. */
	NET_DBG("HTTP/2 PRIORITY frame on stream %u, ignoring", parser->frame.stream_identifier);

	return 0;
}

static int handle_rx_rst_stream(struct http_client *client,
				struct http2_client_parser *parser, const uint8_t *payload)
{
	struct http_client_stream *stream;
	uint32_t error_code;

	/* RST_STREAM must reference an open stream (RFC 9113, Section 6.4). */
	if (parser->frame.stream_identifier == 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "RST_STREAM on stream 0");
	}

	if (http2_stream_is_idle(parser, parser->frame.stream_identifier)) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "RST_STREAM on an idle stream");
	}

	error_code = sys_get_be32(payload);

	stream = http_client_stream_lookup(client, parser->frame.stream_identifier);
	if (stream == NULL) {
		/* Reset for a stream we no longer track (already completed or
		 * never opened); nothing to do.
		 */
		NET_DBG("HTTP/2 RST_STREAM for unknown stream %u (error %u)",
			parser->frame.stream_identifier, error_code);
		return 0;
	}

	NET_DBG("HTTP/2 RST_STREAM: stream %u reset with error %u",
		parser->frame.stream_identifier, error_code);

	http_client_stream_set_complete(client, stream, -ECONNRESET);

	return 0;
}

/* The payload length a fixed-size control frame must have. */
static uint32_t http2_control_frame_len(uint8_t type)
{
	switch (type) {
	case HTTP2_WINDOW_UPDATE_FRAME:
		return HTTP2_WINDOW_UPDATE_FRAME_LEN;
	case HTTP2_PING_FRAME:
		return HTTP2_PING_FRAME_LEN;
	case HTTP2_PRIORITY_FRAME:
		return HTTP2_PRIORITY_FRAME_LEN;
	case HTTP2_RST_STREAM_FRAME:
		return HTTP2_RST_STREAM_FRAME_LEN;
	default:
		return 0;
	}
}

/* Handle a small frame whose whole payload is already buffered. */
static int handle_control_frame(struct http_client *client,
				struct http2_client_parser *parser, const uint8_t *payload)
{
	switch (parser->frame.type) {
	case HTTP2_WINDOW_UPDATE_FRAME:
		return handle_rx_window_update(client, parser, payload);
	case HTTP2_PING_FRAME:
		return handle_rx_ping(client, parser, payload);
	case HTTP2_PRIORITY_FRAME:
		return handle_rx_priority(client, parser, payload);
	case HTTP2_RST_STREAM_FRAME:
		return handle_rx_rst_stream(client, parser, payload);
	default:
		return -EINVAL;
	}
}

/* Handle a GOAWAY frame. Only the mandatory 8-byte prefix (last stream ID and
 * error code) is parsed; any additional debug data is discarded via the SKIP
 * state so that an oversized debug blob does not need to be buffered.
 */
static int handle_rx_goaway(struct http_client *client, const uint8_t *data, size_t len)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	const uint8_t *payload = data + HTTP2_FRAME_HEADER_SIZE;
	size_t payload_len = len - HTTP2_FRAME_HEADER_SIZE;
	uint32_t last_stream_id;
	uint32_t error_code;

	/* GOAWAY is always about the connection (RFC 9113, Section 6.8). */
	if (parser->frame.stream_identifier != 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "GOAWAY on a stream");
	}

	if (parser->frame.length < HTTP2_GOAWAY_FRAME_LEN) {
		return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
					      "invalid GOAWAY length");
	}

	if (payload_len < HTTP2_GOAWAY_FRAME_LEN) {
		/* Wait for the mandatory fields to be fully buffered. */
		return -EAGAIN;
	}

	last_stream_id = sys_get_be32(&payload[0]) & HTTP2_FRAME_STREAM_ID_MASK;
	error_code = sys_get_be32(&payload[sizeof(uint32_t)]);

	NET_DBG("HTTP/2 GOAWAY: last stream %u, error %u", last_stream_id, error_code);

	if (error_code != HTTP2_ERROR_NO_ERROR) {
		NET_WARN("Server is closing the connection, error %u", error_code);
	}

	/* The server accepts no new streams, and has not acted on the ones
	 * above last_stream_id: those can be repeated on another connection.
	 * The others run to completion.
	 */
	parser->going_away = true;

	ARRAY_FOR_EACH(client->session.streams, i) {
		struct http_client_stream *stream = &client->session.streams[i];

		if (stream->is_used && !stream->completed &&
		    (uint32_t)stream->stream_id > last_stream_id) {
			http_client_stream_set_complete(client, stream, -ESHUTDOWN);
		}
	}

	parser->frame.length -= HTTP2_GOAWAY_FRAME_LEN;
	parser->state = (parser->frame.length > 0) ? HTTP2_CLIENT_STATE_SKIP :
							HTTP2_CLIENT_STATE_FRAME_HEADER;

	return HTTP2_FRAME_HEADER_SIZE + HTTP2_GOAWAY_FRAME_LEN;
}

/* Abort a stream locally: tell the peer via RST_STREAM and complete the
 * transaction with an error. Used when the application asks to abort from its
 * response callback.
 */
static void http2_abort_stream(struct http_client *client,
			       struct http_client_stream *stream, int error)
{
	(void)send_rst_stream(client, (uint32_t)stream->stream_id, HTTP2_ERROR_CANCEL);
	http_client_stream_set_complete(client, stream, error);
}

static bool header_name_is(const struct http_hpack_header_buf *header, const char *name)
{
	size_t len = strlen(name);

	return header->name_len == len && strncasecmp(header->name, name, len) == 0;
}

static void copy_header_value(char *dst, size_t dst_size, const char *value, size_t value_len)
{
	size_t copy = MIN(value_len, dst_size - 1);

	memcpy(dst, value, copy);
	dst[copy] = '\0';
}

static bool all_digits(const char *value, size_t value_len)
{
	for (size_t i = 0; i < value_len; i++) {
		if (value[i] < '0' || value[i] > '9') {
			return false;
		}
	}

	return true;
}

/* Parse an unsigned decimal number. Returns false on a non-digit or when the
 * value does not fit a size_t.
 */
static bool parse_decimal(const char *value, size_t value_len, size_t *result)
{
	size_t acc = 0;

	for (size_t i = 0; i < value_len; i++) {
		size_t digit;

		if (value[i] < '0' || value[i] > '9') {
			return false;
		}

		digit = (size_t)(value[i] - '0');
		if (acc > (SIZE_MAX - digit) / 10U) {
			return false;
		}

		acc = acc * 10U + digit;
	}

	*result = acc;

	return true;
}

/* A field as HTTP/2 allows it on the wire (RFC 9113, Section 8.2.1): the name
 * has no control characters, space, uppercase or non-ASCII bytes and a colon
 * only as the leading one of a pseudo-header; the value has no NUL, CR or LF
 * and neither starts nor ends with a space or tab. Anything else is passed on
 * to the application as it is.
 */
static bool field_is_valid(const struct http_hpack_header_buf *header)
{
	for (size_t i = 0; i < header->name_len; i++) {
		unsigned char c = (unsigned char)header->name[i];

		if (c <= 0x20U || c >= 0x7fU || (c >= 'A' && c <= 'Z') || (c == ':' && i > 0)) {
			return false;
		}
	}

	if (header->value_len > 0) {
		unsigned char first = (unsigned char)header->value[0];
		unsigned char last = (unsigned char)header->value[header->value_len - 1];

		if (first == ' ' || first == '\t' || last == ' ' || last == '\t') {
			return false;
		}
	}

	for (size_t i = 0; i < header->value_len; i++) {
		unsigned char c = (unsigned char)header->value[i];

		if (c == 0x00U || c == '\n' || c == '\r') {
			return false;
		}
	}

	return true;
}

/* Store a single decoded response header field in the stream response context. */
static int process_response_header(struct http_client_stream *stream,
				   const struct http_hpack_header_buf *header)
{
	struct http_client_response *resp = &stream->response;
	struct http2_client_stream *h2 = &stream->proto.http2;
	size_t status;

	if (header->name_len == 0) {
		NET_ERR("Empty header field name");
		return -EBADMSG;
	}

	if (!field_is_valid(header)) {
		NET_ERR("Malformed header field");
		return -EBADMSG;
	}

	if (header->name[0] == ':') {
		/* The only valid response pseudo-header is :status, once and
		 * ahead of the regular fields (RFC 9113, Sections 8.3 and
		 * 8.3.2), and trailers carry none (Section 8.1).
		 */
		if (stream->headers_received) {
			NET_ERR("Pseudo-header in trailers");
			return -EBADMSG;
		}

		if (h2->regular_field_seen) {
			NET_ERR("Pseudo-header after a regular field");
			return -EBADMSG;
		}

		if (header_name_is(header, ":status")) {
			if (h2->status_seen) {
				NET_ERR("Duplicate :status");
				return -EBADMSG;
			}

			/* Always three digits (RFC 9113, Section 8.3.2). */
			if (header->value_len != 3 || !all_digits(header->value, 3)) {
				NET_ERR("Invalid :status value");
				return -EBADMSG;
			}

			(void)parse_decimal(header->value, header->value_len, &status);
			resp->status_code = (uint16_t)status;
			h2->status_seen = true;
			return 0;
		}

		NET_ERR("Unexpected response pseudo-header");
		return -EBADMSG;
	}

	h2->regular_field_seen = true;

	/* Connection-specific fields have no place here (RFC 9113,
	 * Section 8.2.2).
	 */
	if (http_client_field_is_connection_specific(header->name, header->name_len) ||
	    header_name_is(header, "te")) {
		NET_ERR("Connection-specific header field");
		return -EBADMSG;
	}

	if (header_name_is(header, "content-type")) {
		copy_header_value(resp->content_type, sizeof(resp->content_type),
				  header->value, header->value_len);
	} else if (header_name_is(header, "content-encoding")) {
		copy_header_value(resp->content_encoding, sizeof(resp->content_encoding),
				  header->value, header->value_len);
	} else if (header_name_is(header, "content-length")) {
		size_t content_len;

		/* A framing field has no place in trailers (RFC 9110, Section 6.5.1). */
		if (stream->headers_received) {
			NET_ERR("content-length in trailers");
			return -EBADMSG;
		}

		if (header->value_len == 0 ||
		    !parse_decimal(header->value, header->value_len, &content_len)) {
			NET_ERR("Invalid content-length value");
			return -EBADMSG;
		}

		/* A repeat may only say the same (RFC 9110, Section 8.6). */
		if (resp->content_len_present && content_len != resp->content_len) {
			NET_ERR("Conflicting content-length values");
			return -EBADMSG;
		}

		resp->content_len = content_len;
		resp->content_len_present = true;
	}

	return 0;
}

/* Forget the fields summarized from a response's header block. */
static void http2_reset_response_summary(struct http_client_response *resp)
{
	resp->status_code = 0;
	resp->content_type[0] = '\0';
	resp->content_encoding[0] = '\0';
	resp->content_len = 0;
	resp->content_len_present = false;
}

/* Reset the response header accumulator at the start of a header block. The
 * header array is carved from the start of stream->headers_buf and the
 * NUL-terminated name/value strings are packed from its end, so they meet in
 * the middle as headers are captured.
 */
static void http2_reset_captured_headers(struct http_client_stream *stream,
					 struct http2_client_parser *parser)
{
	stream->response.headers = NULL;
	stream->response.header_count = 0;
	parser->rx_headers_str_top = sizeof(stream->headers_buf);
}

/* Store a single decoded response header (name and value) in the stream headers
 * buffer so that it can be exposed to the application via response.headers.
 * Pseudo-headers are skipped (they are surfaced through dedicated fields such as
 * response.status_code). Returns -ENOMEM when the buffer is full.
 */
static int http2_capture_header(struct http_client_stream *stream,
				struct http2_client_parser *parser,
				const struct http_hpack_header_buf *header)
{
	struct http_client_response *resp = &stream->response;
	struct http_client_header *entries = (struct http_client_header *)stream->headers_buf;
	char *buf = (char *)stream->headers_buf;
	size_t array_bytes;
	size_t str_bytes;
	size_t name_off;
	size_t value_off;

	if (header->name_len == 0 || header->name[0] == ':') {
		/* Not a regular header field (pseudo-header, or a dynamic table
		 * size update carrying no field).
		 */
		return 0;
	}

	array_bytes = (resp->header_count + 1) * sizeof(struct http_client_header);
	str_bytes = header->name_len + 1 + header->value_len + 1;

	if (parser->rx_headers_str_top < str_bytes ||
	    parser->rx_headers_str_top - str_bytes < array_bytes) {
		return -ENOMEM;
	}

	value_off = parser->rx_headers_str_top - (header->value_len + 1);
	name_off = value_off - (header->name_len + 1);
	parser->rx_headers_str_top = name_off;

	memcpy(buf + name_off, header->name, header->name_len);
	buf[name_off + header->name_len] = '\0';
	memcpy(buf + value_off, header->value, header->value_len);
	buf[value_off + header->value_len] = '\0';

	entries[resp->header_count].name = buf + name_off;
	entries[resp->header_count].value = buf + value_off;
	resp->header_count++;
	resp->headers = entries;

	return 0;
}

/* Whether the response carries no content whatever its headers say: the
 * response to a HEAD request, or a 204 or 304 one (RFC 9110, Sections 9.3.2,
 * 15.3.5 and 15.4.5).
 */
static bool http2_response_without_content(const struct http_client_stream *stream)
{
	return (stream->request != NULL && stream->request->method == HTTP_HEAD) ||
	       stream->response.status_code == 204 || stream->response.status_code == 304;
}

/* Whether the body received so far agrees with content-length. A response
 * without content may still describe one (RFC 9110, Section 8.6); otherwise
 * the lengths must match once the stream ends (RFC 9113, Section 8.1.1).
 */
static bool http2_body_length_ok(const struct http_client_stream *stream, bool final)
{
	const struct http_client_response *resp = &stream->response;
	size_t received = stream->proto.http2.body_received;

	if (!resp->content_len_present || http2_response_without_content(stream)) {
		return true;
	}

	return final ? (received == resp->content_len) : (received <= resp->content_len);
}

/* Deliver a completed response header block to the application. When the HEADERS
 * frame carried END_STREAM there is no body, so the transaction is completed
 * here; otherwise the headers are delivered as a non-final chunk and the body is
 * expected in following DATA frames.
 */
static int http2_deliver_headers(struct http_client *client,
				 struct http_client_stream *stream, bool end_stream,
				 bool block_done)
{
	int ret;

	if (!stream->headers_received && stream->response.status_code == 0) {
		/* RFC 9113, Section 8.3.2. */
		http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
				   ":status missing");
		return 0;
	}

	if (end_stream && stream->response.status_code < 200) {
		/* An interim response does not end the stream (RFC 9113,
		 * Section 8.1).
		 */
		http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
				   "interim response with END_STREAM");
		return 0;
	}

	if (end_stream && !http2_body_length_ok(stream, true)) {
		http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
				   "body length does not match content-length");
		return 0;
	}

	if (end_stream) {
		/* The stream may be released in here. */
		return http_client_stream_deliver(client, stream, true);
	}

	ret = http_client_stream_deliver(client, stream, false);
	if (ret < 0) {
		return ret;
	}

	/* Any header block after the one of the final, non-interim response is
	 * trailers.
	 */
	if (block_done && stream->response.status_code >= 200) {
		stream->headers_received = true;
	}

	return 0;
}

/* The payload of the current frame is consumed: skip its padding, if any,
 * before expecting the next frame.
 */
static void http2_frame_payload_done(struct http2_client_parser *parser)
{
	if (parser->frame.padding_len > 0) {
		parser->frame.length = parser->frame.padding_len;
		parser->frame.padding_len = 0;
		parser->state = HTTP2_CLIENT_STATE_SKIP;
	} else {
		parser->state = HTTP2_CLIENT_STATE_FRAME_HEADER;
	}
}

/* The header block fragment of the current HEADERS or CONTINUATION frame is
 * decoded. Deliver the header block if this frame ends it.
 */
static void http2_headers_frame_done(struct http_client *client,
				     struct http_client_stream *stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	int ret;

	http2_frame_payload_done(parser);

	if (!(parser->frame.flags & HTTP2_FLAG_END_HEADERS)) {
		/* The header block continues in a CONTINUATION frame. */
		parser->expect_continuation = true;
		return;
	}

	if (stream != NULL) {
		ret = http2_deliver_headers(client, stream, parser->headers_end_stream, true);
		if (ret < 0) {
			/* The application asked to abort the transaction. */
			http2_abort_stream(client, stream, ret);
		}
	}
}

/* Enter the HEADERS state on a HEADERS or CONTINUATION frame. Only the header
 * consumed here; the header block itself is decoded incrementally in
 * http2_process_headers().
 */
static int http2_enter_headers(struct http_client *client)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	struct http_client_stream *stream;

	/* HEADERS and CONTINUATION are always associated with a stream. */
	if (parser->frame.stream_identifier == 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "header block on stream 0");
	}

	if (parser->frame.type == HTTP2_HEADERS_FRAME) {
		if (http2_stream_is_idle(parser, parser->frame.stream_identifier)) {
			return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
						      "HEADERS on an idle stream");
		}

		parser->headers_stream_id = parser->frame.stream_identifier;
		parser->headers_end_stream =
			(parser->frame.flags & HTTP2_FLAG_END_STREAM) != 0;
		parser->header_field.datalen = 0;
		parser->block_field_seen = false;

		/* A HEADERS frame starts a fresh header block (initial response
		 * headers or trailers); reset the captured-header accumulator.
		 */
		stream = http_client_stream_lookup(client, parser->frame.stream_identifier);
		if (stream != NULL) {
			if (stream->headers_received && !parser->headers_end_stream) {
				/* Trailers end the stream (RFC 9113, Section 8.1).
				 * The block is still decoded, for the HPACK state.
				 */
				http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR,
						   -EPROTO, "trailers without END_STREAM");
			} else {
				http2_reset_captured_headers(stream, parser);
				stream->proto.http2.status_seen = false;
				stream->proto.http2.regular_field_seen = false;
				if (!stream->headers_received) {
					/* Nothing of an interim response carries
					 * over into the next one.
					 */
					http2_reset_response_summary(&stream->response);
				}
			}
		}
	} else if (!parser->expect_continuation) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "unexpected CONTINUATION frame");
	} else if (parser->frame.stream_identifier != parser->headers_stream_id) {
		/* RFC 9113, Section 6.10. */
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "CONTINUATION on another stream");
	}

	parser->expect_continuation = false;
	parser->state = HTTP2_CLIENT_STATE_HEADERS;

	if (parser->frame.length == 0) {
		/* No fragment to wait for, e. g. an empty CONTINUATION frame. */
		if (parser->frame.type == HTTP2_HEADERS_FRAME &&
		    (parser->frame.flags & (HTTP2_FLAG_PADDED | HTTP2_FLAG_PRIORITY)) != 0) {
			return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
						      "HEADERS too short for its fields");
		}

		http2_headers_frame_done(client, http_client_stream_lookup(
						 client, parser->frame.stream_identifier));
	}

	return HTTP2_FRAME_HEADER_SIZE;
}

/* The header field at the start of data runs past the current frame. Join
 * it with the fragment of the CONTINUATION frame that has to follow, by moving
 * it up over that frame's header and over the padding of the current frame.
 * The two frames are then processed as one.
 *
 * Returns the number of bytes consumed at the start of data, or -EAGAIN when
 * the CONTINUATION frame header is not buffered yet.
 */
static int http2_join_continuation(struct http2_client_parser *parser, uint8_t *data,
				   size_t len)
{
	size_t partial = parser->frame.length;
	size_t skip = parser->frame.padding_len + HTTP2_FRAME_HEADER_SIZE;
	struct http2_frame next;

	if (parser->frame.flags & HTTP2_FLAG_END_HEADERS) {
		return http2_connection_error(parser, HTTP2_ERROR_COMPRESSION_ERROR,
					      "header block ends within a field");
	}

	if (len < partial + skip) {
		return -EAGAIN;
	}

	(void)http2_frame_header_parse(data + partial + parser->frame.padding_len,
				       HTTP2_FRAME_HEADER_SIZE, &next);
	if (next.type != HTTP2_CONTINUATION_FRAME ||
	    next.stream_identifier != parser->frame.stream_identifier) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "CONTINUATION frame expected");
	}

	if (next.length > HTTP2_DEFAULT_MAX_FRAME_SIZE) {
		return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
					      "frame exceeds SETTINGS_MAX_FRAME_SIZE");
	}

	memmove(data + skip, data, partial);

	parser->frame.type = HTTP2_CONTINUATION_FRAME;
	parser->frame.flags = next.flags;
	parser->frame.length = partial + next.length;
	parser->frame.padding_len = 0;

	return (int)skip;
}

/* Incrementally HPACK-decode the header block of the current HEADERS or
 * CONTINUATION frame. Decodes as many complete header fields as the buffered
 * data allows and returns the number of bytes consumed.
 */
static int http2_process_headers(struct http_client *client, uint8_t *data, size_t len)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	struct http_client_stream *stream;
	size_t consumed = 0;
	int ret;

	stream = http_client_stream_lookup(client, parser->frame.stream_identifier);

	/* A HEADERS frame may carry a Pad Length and a priority field ahead of
	 * the header block fragment.
	 */
	if (parser->frame.type == HTTP2_HEADERS_FRAME) {
		if (parser->frame.flags & HTTP2_FLAG_PADDED) {
			ret = http2_frame_parse_padded_field(&parser->frame, data, len);
			if (ret < 0) {
				return ret;
			}

			consumed += ret;
		}

		if (parser->frame.flags & HTTP2_FLAG_PRIORITY) {
			ret = http2_frame_parse_priority_field(&parser->frame, data + consumed,
							       len - consumed);
			if (ret == -EAGAIN && consumed > 0) {
				return (int)consumed;
			}

			if (ret == -EBADMSG) {
				/* RFC 9113, Section 4.2. */
				return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
							      "HEADERS too short for its fields");
			}

			if (ret < 0) {
				return ret;
			}

			consumed += ret;
		}
	}

	while (parser->frame.length > 0) {
		size_t datalen = MIN(len - consumed, parser->frame.length);

		/* Reset the name length so that an entry which does not set it
		 * (a dynamic table size update) can be detected afterwards.
		 */
		parser->header_field.name_len = 0;

		ret = http_hpack_decode_header(data + consumed, datalen, &parser->header_field);
		if (ret == -EAGAIN) {
			if (consumed > 0) {
				/* Advance past the fields decoded so far and retry
				 * the partial field once more data is buffered.
				 */
				return (int)consumed;
			}

			if (datalen >= parser->frame.length) {
				/* The field runs past this frame, i. e. it is
				 * split across a CONTINUATION boundary.
				 */
				return http2_join_continuation(parser, data, len);
			}

			/* Need more buffered data to decode the current field. */
			return -EAGAIN;
		}

		if (ret <= 0) {
			NET_ERR("HPACK decode error %d", ret);
			return http2_connection_error(parser, HTTP2_ERROR_COMPRESSION_ERROR,
						      "HPACK decode error");
		}

		if ((uint32_t)ret > parser->frame.length) {
			return http2_connection_error(parser, HTTP2_ERROR_COMPRESSION_ERROR,
						      "HPACK field exceeds frame length");
		}

		if (parser->header_field.type == HTTP_HPACK_ENTRY_TABLE_SIZE_UPDATE) {
			/* The client advertises no dynamic table; until the
			 * server acknowledges that, the protocol default is the
			 * limit (RFC 7541, Section 6.3). A size update only
			 * belongs at the start of a block (Section 4.2).
			 */
			uint32_t limit = parser->settings_acked ? 0U :
							   HTTP2_DEFAULT_HEADER_TABLE_SIZE;

			if (parser->header_field.table_size > limit) {
				return http2_connection_error(
					parser, HTTP2_ERROR_COMPRESSION_ERROR,
					"dynamic table size update above the limit");
			}

			if (parser->block_field_seen) {
				return http2_connection_error(
					parser, HTTP2_ERROR_COMPRESSION_ERROR,
					"dynamic table size update after a field");
			}

			parser->frame.length -= ret;
			consumed += ret;
			continue;
		}

		parser->block_field_seen = true;

		if (stream != NULL && parser->header_field.oversized) {
			/* A limit of this client, not an error of the peer: give
			 * up on the response, not on the connection.
			 */
			http2_stream_error(client, stream, HTTP2_ERROR_INTERNAL_ERROR, -ENOBUFS,
					   "response header field too large");
			stream = NULL;
		}

		if (stream != NULL) {
			int hret = process_response_header(stream, &parser->header_field);

			if (hret < 0) {
				/* A malformed response fails the stream only
				 * (RFC 9113, Section 8.1.1). The rest of the
				 * block is decoded for the HPACK state.
				 */
				http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR,
						   -EPROTO, "malformed response headers");
				stream = NULL;
			}
		}

		if (stream != NULL && stream->request != NULL &&
		    !stream->request->skip_response_headers) {
			int hret = http2_capture_header(stream, parser, &parser->header_field);

			if (hret == -ENOMEM && stream->response.header_count > 0) {
				/* Buffer full: hand the headers captured so far to
				 * the application and start over.
				 */
				hret = http2_deliver_headers(client, stream, false, false);
				if (hret < 0) {
					http2_abort_stream(client, stream, hret);
					stream = NULL;
				} else if (!stream->is_used || stream->completed) {
					/* The delivery failed the stream. */
					stream = NULL;
				} else {
					http2_reset_captured_headers(stream, parser);
					hret = http2_capture_header(stream, parser,
								    &parser->header_field);
				}
			}

			if (hret == -ENOMEM && stream != NULL) {
				/* Larger than the whole headers buffer: a limit
				 * of this client, reported like the other ones.
				 */
				http2_stream_error(client, stream, HTTP2_ERROR_INTERNAL_ERROR,
						   -ENOBUFS,
						   "response header field too large");
				stream = NULL;
			}
		}

		parser->frame.length -= ret;
		consumed += ret;
	}

	http2_headers_frame_done(client, stream);

	return (int)consumed;
}

/* Account for received DATA payload, padding included, against the connection
 * and stream receive windows, sending a WINDOW_UPDATE to replenish whichever
 * drops to the refill threshold. The stream window is not refilled once the
 * stream ends. Returns 1 when the stream exceeded its window and was failed,
 * 0 otherwise, or a negative errno on a connection error.
 */
static int http2_flow_control_consume(struct http_client *client,
				      struct http_client_stream *stream, uint32_t len,
				      bool stream_ends)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	int ret;

	if ((int)len > parser->window_size) {
		return http2_connection_error(parser, HTTP2_ERROR_FLOW_CONTROL_ERROR,
					      "DATA exceeds the connection window");
	}

	parser->window_size -= (int)len;
	if (parser->window_size <= HTTP2_WINDOW_UPDATE_THRESHOLD) {
		ret = send_window_update(client, 0,
					 HTTP2_RX_WINDOW_SIZE - parser->window_size);
		if (ret < 0) {
			return ret;
		}

		parser->window_size = HTTP2_RX_WINDOW_SIZE;
	}

	if (stream != NULL) {
		struct http2_client_stream *h2 = &stream->proto.http2;

		if ((int)len > h2->window_size) {
			/* RFC 9113, Section 6.9.1. */
			http2_stream_error(client, stream, HTTP2_ERROR_FLOW_CONTROL_ERROR, -EPROTO,
					   "DATA exceeds the stream window");
			return 1;
		}

		h2->window_size -= (int)len;
		if (!stream_ends && h2->window_size <= HTTP2_WINDOW_UPDATE_THRESHOLD) {
			ret = send_window_update(client, (uint32_t)stream->stream_id,
						 HTTP2_RX_WINDOW_SIZE - h2->window_size);
			if (ret < 0) {
				return ret;
			}

			h2->window_size = HTTP2_RX_WINDOW_SIZE;
		}
	}

	return 0;
}

/* Look up the stream of a DATA frame. DATA before the final response headers
 * is a malformed response (RFC 9113, Section 8.1), which fails the stream;
 * the frame is then discarded like one on an unknown stream.
 */
static struct http_client_stream *http2_data_stream_lookup(struct http_client *client,
							   struct http2_client_parser *parser)
{
	struct http_client_stream *stream;

	stream = http_client_stream_lookup(client, parser->frame.stream_identifier);
	if (stream == NULL) {
		return NULL;
	}

	if (!stream->headers_received) {
		http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
				   "DATA before the response headers");
		return NULL;
	}

	return stream;
}

/* Deliver a body chunk to the application. When final is set this is the
 * last chunk of the response and the transaction is completed.
 */
static int http2_deliver_body(struct http_client *client, struct http_client_stream *stream,
			      const uint8_t *body, size_t body_len, bool final)
{
	stream->proto.http2.body_received += body_len;
	if (!http2_body_length_ok(stream, final)) {
		http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
				   "body length does not match content-length");
		return 0;
	}

	stream->response.body = body;
	stream->response.body_len = body_len;

	/* The stream may be released in here when final. */
	return http_client_stream_deliver(client, stream, final);
}

/* Enter the DATA state on a DATA frame. An empty DATA frame carries no body and
 * only matters when it terminates the stream, which is handled here directly.
 */
static int http2_enter_data(struct http_client *client)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	struct http_client_stream *stream;

	/* DATA frames are always associated with a stream. */
	if (parser->frame.stream_identifier == 0) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "DATA on stream 0");
	}

	if (http2_stream_is_idle(parser, parser->frame.stream_identifier)) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "DATA on an idle stream");
	}

	if (parser->frame.length == 0) {
		if (parser->frame.flags & HTTP2_FLAG_PADDED) {
			/* Not even the Pad Length field fits. */
			return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
						      "DATA too short for its padding");
		}

		if (parser->frame.flags & HTTP2_FLAG_END_STREAM) {
			stream = http2_data_stream_lookup(client, parser);
			if (stream != NULL) {
				int ret = http2_deliver_body(client, stream, NULL, 0, true);

				if (ret < 0) {
					http2_abort_stream(client, stream, ret);
				}
			}
		}

		parser->state = HTTP2_CLIENT_STATE_FRAME_HEADER;
		return HTTP2_FRAME_HEADER_SIZE;
	}

	parser->state = HTTP2_CLIENT_STATE_DATA;

	return HTTP2_FRAME_HEADER_SIZE;
}

/* Consume and deliver the payload of the current DATA frame in chunks. */
static int http2_process_data(struct http_client *client, uint8_t *data, size_t len)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	bool end_stream = (parser->frame.flags & HTTP2_FLAG_END_STREAM) != 0;
	struct http_client_stream *stream;
	size_t consumed = 0;
	size_t chunk;
	bool final;
	int ret;

	stream = http2_data_stream_lookup(client, parser);

	if (parser->frame.flags & HTTP2_FLAG_PADDED) {
		ret = http2_frame_parse_padded_field(&parser->frame, data, len);
		if (ret < 0) {
			return ret;
		}

		consumed = ret;

		/* The Pad Length field and the padding count against flow
		 * control like the data itself (RFC 9113, Section 6.1).
		 */
		ret = http2_flow_control_consume(client, stream,
						 consumed + parser->frame.padding_len, end_stream);
		if (ret < 0) {
			return ret;
		} else if (ret == 1) {
			stream = NULL;
		}
	}

	if (parser->frame.length == 0) {
		/* Nothing but padding. Only the end of the stream, if flagged,
		 * is of interest.
		 */
		if (end_stream && stream != NULL) {
			ret = http2_deliver_body(client, stream, NULL, 0, true);
			if (ret < 0) {
				http2_abort_stream(client, stream, ret);
			}
		}

		http2_frame_payload_done(parser);

		return (int)consumed;
	}

	/* A frame of padding alone may still end the stream, but a body is
	 * malformed on a response without content.
	 */
	if (stream != NULL && http2_response_without_content(stream)) {
		http2_stream_error(client, stream, HTTP2_ERROR_PROTOCOL_ERROR, -EPROTO,
				   "body on a response without content");
		stream = NULL;
	}

	chunk = MIN(len - consumed, parser->frame.length);
	if (chunk == 0) {
		/* Only the Pad Length field was buffered so far. */
		return (int)consumed;
	}

	final = (chunk == parser->frame.length) && end_stream;

	/* Replenish flow control before delivery, since delivery of the final
	 * chunk may complete (and, in async mode, release) the stream.
	 */
	ret = http2_flow_control_consume(client, stream, chunk, final);
	if (ret < 0) {
		return ret;
	} else if (ret == 1) {
		stream = NULL;
	}

	if (stream != NULL) {
		ret = http2_deliver_body(client, stream, data + consumed, chunk, final);
		if (ret < 0) {
			http2_abort_stream(client, stream, ret);
		}
	}

	parser->frame.length -= chunk;
	if (parser->frame.length == 0) {
		http2_frame_payload_done(parser);
	}

	return (int)(consumed + chunk);
}

/* Parse a frame header and dispatch on the frame type. Connection-level control
 * frames are handled here as soon as their (small) payload is fully buffered;
 * stream frames only consume the header and hand the payload to a follow-up
 * state.
 */
static int http2_process_frame_header(struct http_client *client, uint8_t *data, size_t len)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	const uint8_t *payload = data + HTTP2_FRAME_HEADER_SIZE;
	size_t payload_len;
	int ret;

	ret = http2_frame_header_parse(data, len, &parser->frame);
	if (ret < 0) {
		return ret;
	}

	/* The server preface opens with a SETTINGS frame (RFC 9113, Section 3.4). */
	if (!parser->preface_received) {
		if (parser->frame.type != HTTP2_SETTINGS_FRAME ||
		    (parser->frame.flags & HTTP2_FLAG_ACK) != 0) {
			return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
						      "SETTINGS expected first");
		}

		parser->preface_received = true;
	}

	/* Nothing but its continuation may follow an unfinished header block
	 * (RFC 9113, Section 6.10).
	 */
	if (parser->expect_continuation && parser->frame.type != HTTP2_CONTINUATION_FRAME) {
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "CONTINUATION frame expected");
	}

	/* No larger frame size was advertised (RFC 9113, Section 4.2). */
	if (parser->frame.length > HTTP2_DEFAULT_MAX_FRAME_SIZE) {
		return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
					      "frame exceeds SETTINGS_MAX_FRAME_SIZE");
	}

	payload_len = len - HTTP2_FRAME_HEADER_SIZE;

	NET_DBG("HTTP/2 RX frame: type %u, flags 0x%02x, stream %u, len %u",
		parser->frame.type, parser->frame.flags, parser->frame.stream_identifier,
		parser->frame.length);

	switch (parser->frame.type) {
	case HTTP2_SETTINGS_FRAME:
		return http2_enter_settings(client);

	case HTTP2_PRIORITY_FRAME:
		if (parser->frame.length != HTTP2_PRIORITY_FRAME_LEN) {
			struct http_client_stream *stream;

			/* An error of the stream only (RFC 9113, Section 6.3), and
			 * the frame is dropped like every PRIORITY frame.
			 */
			if (parser->frame.stream_identifier == 0) {
				return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
							      "PRIORITY on stream 0");
			}

			stream = http_client_stream_lookup(client, parser->frame.stream_identifier);
			if (stream != NULL) {
				http2_stream_error(client, stream, HTTP2_ERROR_FRAME_SIZE_ERROR,
						   -EPROTO, "invalid PRIORITY length");
			}

			if (parser->frame.length > 0) {
				parser->state = HTTP2_CLIENT_STATE_SKIP;
			}

			return HTTP2_FRAME_HEADER_SIZE;
		}

		__fallthrough;
	case HTTP2_WINDOW_UPDATE_FRAME:
	case HTTP2_PING_FRAME:
	case HTTP2_RST_STREAM_FRAME:
		/* Fixed-size frames, processed once their payload is buffered. */
		if (parser->frame.length != http2_control_frame_len(parser->frame.type)) {
			return http2_connection_error(parser, HTTP2_ERROR_FRAME_SIZE_ERROR,
						      "invalid control frame length");
		}

		if (payload_len < parser->frame.length) {
			/* Wait until the whole payload is buffered so that it can
			 * be processed in one pass.
			 */
			return -EAGAIN;
		}

		ret = handle_control_frame(client, parser, payload);
		if (ret < 0) {
			return ret;
		}

		return HTTP2_FRAME_HEADER_SIZE + parser->frame.length;

	case HTTP2_GOAWAY_FRAME:
		return handle_rx_goaway(client, data, len);

	case HTTP2_HEADERS_FRAME:
	case HTTP2_CONTINUATION_FRAME:
		return http2_enter_headers(client);

	case HTTP2_DATA_FRAME:
		return http2_enter_data(client);

	case HTTP2_PUSH_PROMISE_FRAME:
		/* Server push is disabled (RFC 9113, Section 8.4). */
		return http2_connection_error(parser, HTTP2_ERROR_PROTOCOL_ERROR,
					      "unexpected PUSH_PROMISE frame");

	default:
		/* Unknown frame types are discarded (RFC 9113, Section 4.1).
		 * Consume the header and skip the payload.
		 */
		parser->state = (parser->frame.length > 0) ? HTTP2_CLIENT_STATE_SKIP :
								HTTP2_CLIENT_STATE_FRAME_HEADER;
		return HTTP2_FRAME_HEADER_SIZE;
	}
}

/* Incrementally process received HTTP/2 data. Called repeatedly with the bytes
 * currently buffered; returns the number of bytes consumed, -EAGAIN when more
 * data is needed to make progress, or another negative errno on error. A frame
 * (or its payload) may be larger than the RX buffer, so state is kept across
 * calls in client->session.parser.http2 and each call makes as much progress as
 * the available data allows.
 */
static bool http2_has_active_streams(struct http_client *client)
{
	ARRAY_FOR_EACH(client->session.streams, i) {
		const struct http_client_stream *stream = &client->session.streams[i];

		if (stream->is_used && !stream->completed) {
			return true;
		}
	}

	return false;
}

static int http2_client_process_data(struct http_client *client, void *transport_stream,
				     uint8_t *data, size_t len, bool end_stream)
{
	struct http2_client_parser *parser = &client->session.parser.http2;
	int ret;

	ARG_UNUSED(transport_stream);
	ARG_UNUSED(end_stream);

	if (len == 0) {
		return 0;
	}

	switch (parser->state) {
	case HTTP2_CLIENT_STATE_FRAME_HEADER:
		ret = http2_process_frame_header(client, data, len);
		break;

	case HTTP2_CLIENT_STATE_HEADERS:
		ret = http2_process_headers(client, data, len);
		break;

	case HTTP2_CLIENT_STATE_SETTINGS:
		ret = http2_process_settings(client, data, len);
		break;

	case HTTP2_CLIENT_STATE_DATA:
		ret = http2_process_data(client, data, len);
		break;

	case HTTP2_CLIENT_STATE_SKIP: {
		/* Discard the frame payload in chunks until it is consumed. The
		 * outer loop guarantees len > 0 and this state is only entered
		 * with a non-zero frame length, so consumed is always > 0.
		 */
		size_t consumed = MIN(len, parser->frame.length);

		parser->frame.length -= consumed;
		if (parser->frame.length == 0) {
			parser->state = HTTP2_CLIENT_STATE_FRAME_HEADER;
		}

		ret = (int)consumed;
		break;
	}

	default:
		NET_ERR("Unexpected HTTP/2 parser state %d", parser->state);
		return -EPROTO;
	}

	if (ret >= 0 && parser->going_away && !http2_has_active_streams(client)) {
		/* Nothing is left to wait for on a connection the server is
		 * closing.
		 */
		http_client_session_terminate(client, 0, true);
	}

	return ret;
}

static struct http_client_ops http2_client_ops = {
	.session_open = http2_client_session_open,
	.session_close = http2_client_session_close,
	.stream_open = http2_client_stream_open,
	.stream_reset = http2_client_stream_reset,
	.stream_send_headers = http2_client_stream_send_headers,
	.stream_send_data = http2_client_stream_send_data,
	.process_data = http2_client_process_data,
};

const struct http_client_ops *http_client_get_http2_ops(void)
{
	return &http2_client_ops;
}
