/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

static int quic_send_max_stream_data(struct quic_endpoint *ep,
				     struct quic_stream *stream);

/*
 * Handle STREAM frame (frame types 0x08 - 0x0f)
 *
 * STREAM Frame {
 *   Type (i) = 0x08..0x0f,
 *   Stream ID (i),
 *   [Offset (i)],
 *   [Length (i)],
 *   Stream Data (..),
 * }
 *
 * Bits in frame type:
 *   0x04 (OFF): Offset field present
 *   0x02 (LEN): Length field present
 *   0x01 (FIN): Final frame of stream
 */
static int handle_stream_frame(struct quic_endpoint *ep,
			       const uint8_t *buf, size_t len,
			       size_t *consumed)
{
	uint8_t frame_type = buf[0];
	bool has_offset = (frame_type & 0x04) != 0;
	bool has_length = (frame_type & 0x02) != 0;
	bool is_fin = (frame_type & 0x01) != 0;
	struct quic_context *conn = NULL;
	struct quic_stream *stream;
	uint64_t stream_id;
	uint64_t offset = 0;
	uint64_t data_len;
	size_t pos = 1;
	int ret;

	/* Stream ID */
	ret = quic_get_len(&buf[pos], len - pos, &stream_id);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Offset (optional) */
	if (has_offset) {
		ret = quic_get_len(&buf[pos], len - pos, &offset);
		if (ret < 0) {
			return ret;
		}

		pos += ret;
	}

	/* Length (optional) */
	if (has_length) {
		ret = quic_get_len(&buf[pos], len - pos, &data_len);
		if (ret < 0) {
			return ret;
		}

		pos += ret;
	} else {
		/* No length field means data extends to end of packet */
		data_len = len - pos;
	}

	if (pos + data_len > len) {
		NET_DBG("[EP:%p/%d] STREAM frame data exceeds packet length",
			ep, quic_get_by_ep(ep));
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] STREAM frame: id=%" PRIu64 ", offset=%" PRIu64
		", len=%" PRIu64 ", fin=%d",
		ep, quic_get_by_ep(ep), stream_id, offset, data_len, is_fin);

	/* Find the connection context for the accepting endpoint */
	conn = quic_find_context(ep);
	if (conn == NULL) {
		NET_DBG("[EP:%p/%d] No connection context for endpoint", ep, quic_get_by_ep(ep));
		return -ENOENT;
	}

	/* Find or create the stream */
	stream = quic_find_stream(conn, ep, stream_id);
	if (stream == NULL) {
		/*
		 * This is a new peer-initiated stream. Before allocating a slot,
		 * verify the peer has not exceeded the stream limit we advertised.
		 *
		 * RFC 9000 ch. 4.6: if a peer opens more streams than permitted,
		 * the endpoint MUST close the connection with STREAM_LIMIT_ERROR.
		 *
		 * Stream ID encoding (RFC 9000 ch. 2.1):
		 *   bit 0 = initiator: 0 = client, 1 = server
		 *   bit 1 = direction: 0 = bidi,   1 = uni
		 *   bits 63-2 = sequence number
		 *
		 * We only count peer-initiated streams against our rx_sl budget.
		 * Peer-initiated: bit0==0 when we are server, bit0==1 when client.
		 */
		bool peer_init = ((stream_id & 1u) == 0u) == ep->is_server;
		bool is_bidi = (stream_id & 2u) == 0u;
		uint64_t stream_seq = stream_id >> 2;

		if (peer_init) {
			uint64_t limit = is_bidi ? ep->rx_sl.max_bidi : ep->rx_sl.max_uni;

			if (stream_seq >= limit) {
				NET_ERR("[EP:%p/%d] Stream %" PRIu64
					" exceeds our %s limit %" PRIu64
					", sending STREAM_LIMIT_ERROR",
					ep, quic_get_by_ep(ep), stream_id,
					is_bidi ? "bidi" : "uni", limit);
				quic_endpoint_send_connection_close(
					ep,
					QUIC_ERROR_STREAM_LIMIT_ERROR,
					"stream limit exceeded");
				return -EPROTO;
			}

			/* Consume one slot from the open-stream budget */
			if (is_bidi) {
				ep->rx_sl.open_bidi++;
			} else {
				ep->rx_sl.open_uni++;
			}
		}

		/* New stream from peer, create and queue for accept */
		stream = quic_create_stream_from_peer(conn, ep, stream_id);
		if (stream == NULL) {
			/* Pool exhausted, roll back the counter we just incremented */
			if (peer_init) {
				if (is_bidi && ep->rx_sl.open_bidi > 0) {
					ep->rx_sl.open_bidi--;
				} else if (!is_bidi && ep->rx_sl.open_uni > 0) {
					ep->rx_sl.open_uni--;
				}
			}

			NET_DBG("[CO:%p/%d] Failed to create stream %" PRIu64,
				conn, quic_get_by_conn(conn), stream_id);
			return -ENOMEM;
		}

		sys_slist_find_and_remove(&conn->streams, &stream->node);
		sys_slist_prepend(&conn->streams, &stream->node);

		/* Queue for accept() call */
		k_fifo_put(&conn->incoming.stream_q, stream);

		NET_DBG("[CO:%p/%d] Queued new incoming stream %" PRIu64 " for accept on sock %d",
			conn, quic_get_by_conn(conn), stream_id, conn->sock);

		k_sem_give(&conn->incoming.stream_sem);

		k_yield();
	}

	/* Deliver data to the stream */
	ret = quic_stream_receive_data(stream, offset, &buf[pos], data_len, is_fin);
	if (ret < 0) {
		if (ret == -EAGAIN) {
			/* Out-of-order data. This is not fatal, consume the frame bytes */
			*consumed = pos + data_len;
			return pos + data_len;
		}

		NET_DBG("[CO:%p/%d] Failed to deliver stream data: %d",
			conn, quic_get_by_conn(conn), ret);
		return ret;
	}

	*consumed = pos + data_len;

	return pos + data_len;
}

/*
 * Stub handlers for other frame types
 */
static int handle_reset_stream_frame(struct quic_endpoint *ep,
				     const uint8_t *buf, size_t len)
{
	uint64_t stream_id, error_code, final_size;
	struct quic_stream *stream;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &stream_id);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	ret = quic_get_len(&buf[pos], len - pos, &error_code);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	ret = quic_get_len(&buf[pos], len - pos, &final_size);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	NET_DBG("[EP:%p/%d] RESET_STREAM: stream=%" PRIu64 ", error=0x%" PRIx64
		", final_size=%" PRIu64,
		ep, quic_get_by_ep(ep), stream_id, error_code, final_size);

	/*
	 * Wake any thread blocked in quic_stream_recv() so it gets
	 * -ECONNRESET rather than blocking forever. Also mark the stream's
	 * read half as closed so future recv() calls return immediately.
	 */
	stream = quic_find_stream_by_id(ep, stream_id);
	if (stream != NULL) {
		stream->read_closed = true;
		stream->rx_buf.fin_received = true;  /* unblocks the wait loop */

		k_poll_signal_raise(&stream->recv.signal, 0);
		k_condvar_signal(&stream->cond.recv);

		NET_DBG("[ST:%p/%d] RESET_STREAM: woke blocked readers on "
			"stream %" PRIu64, stream, quic_get_by_stream(stream),
			stream_id);
	}

	return pos;
}

static int quic_send_reset_stream(struct quic_endpoint *ep,
				  uint64_t stream_id,
				  uint64_t error_code)
{
	uint8_t frame[32];
	int pos = 0;
	int n;

	frame[pos++] = QUIC_FRAME_TYPE_RESET_STREAM;

	n = quic_put_varint(&frame[pos], sizeof(frame) - pos, stream_id);
	if (n <= 0) {
		return -EINVAL;
	}
	pos += n;

	n = quic_put_varint(&frame[pos], sizeof(frame) - pos, error_code);
	if (n <= 0) {
		return -EINVAL;
	}
	pos += n;

	n = quic_put_varint(&frame[pos], sizeof(frame) - pos, 0 /* final_size */);
	if (n <= 0) {
		return -EINVAL;
	}
	pos += n;

	NET_DBG("[EP:%p/%d] Sending RESET_STREAM: stream=%" PRIu64
		" error=0x%" PRIx64,
		ep, quic_get_by_ep(ep), stream_id, error_code);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

/**
 * @brief Send STOP_SENDING frame (RFC 9000 ch. 19.5)
 *
 * Tells the peer to stop sending on this stream. The peer must respond
 * with RESET_STREAM. Used when we close the read half of a stream before
 * all data has been received (SHUT_RD).
 *
 * @param ep          The endpoint
 * @param stream_id   Stream to stop
 * @param error_code  Application error code (H3_NO_ERROR = 0x0100 for clean stop)
 * @return 0 on success, negative errno on error
 */
static int quic_send_stop_sending(struct quic_endpoint *ep,
				  uint64_t stream_id,
				  uint64_t error_code)
{
	uint8_t frame[32];
	int pos = 0;
	int varint_len;

	frame[pos++] = QUIC_FRAME_TYPE_STOP_SENDING;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos, stream_id);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos, error_code);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	NET_DBG("[EP:%p/%d] Sending STOP_SENDING: stream=%" PRIu64 " error=0x%" PRIx64,
		ep, quic_get_by_ep(ep), stream_id, error_code);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

static int handle_stop_sending_frame(struct quic_endpoint *ep,
				     const uint8_t *buf, size_t len)
{
	uint64_t stream_id, error_code;
	struct quic_stream *stream;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &stream_id);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	ret = quic_get_len(&buf[pos], len - pos, &error_code);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	NET_DBG("[EP:%p/%d] STOP_SENDING: stream=%" PRIu64 ", error=0x%" PRIx64,
		ep, quic_get_by_ep(ep), stream_id, error_code);

	/*
	 * RFC 9000 ch. 3.5: a sender that receives STOP_SENDING MUST respond
	 * with either RESET_STREAM or a stream FIN. Send RESET_STREAM to
	 * immediately release our TX side, then wake the stream so the HTTP
	 * layer can call zsock_close() and free the slot.
	 *
	 * Re-use the peer's error code so the HTTP layer (and any logging)
	 * can see why the stream was reset.
	 */
	stream = quic_find_stream_by_id(ep, stream_id);
	if (stream != NULL) {
		/* Send RESET_STREAM, our TX side is being abandoned */
		quic_send_reset_stream(ep, stream_id, error_code);

		/* Mark TX side as reset so future sends return an error */
		stream->tx_reset = true;

		/*
		 * Wake any thread blocked in zsock_recv() or waiting on
		 * POLLIN so the HTTP layer sees the stream is done and calls
		 * zsock_close().  Reuse the fin_received flag so
		 * quic_stream_recv() returns 0 (EOF) rather than blocking.
		 */
		stream->rx_buf.fin_received = true;
		k_poll_signal_raise(&stream->recv.signal, 0);
		k_condvar_signal(&stream->cond.recv);

		NET_DBG("[ST:%p/%d] STOP_SENDING: sent RESET_STREAM and woke "
			"stream %" PRIu64,
			stream, quic_get_by_stream(stream), stream_id);
	}

	return pos;
}

static int handle_new_token_frame(struct quic_endpoint *ep,
				  const uint8_t *buf, size_t len)
{
	uint64_t token_len;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &token_len);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	if (pos + token_len > len) {
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] NEW_TOKEN: len=%" PRIu64, ep, quic_get_by_ep(ep), token_len);

	/* TODO: Store token for future connections */

	return pos + token_len;
}

static int handle_max_data_frame(struct quic_endpoint *ep,
				 const uint8_t *buf, size_t len)
{
	uint64_t max_data;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &max_data);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Update connection-level TX flow control if increased */
	if (max_data > ep->tx_fc.max_data) {
		struct quic_stream *stream, *tmp;
		struct quic_context *ctx;

		NET_DBG("[EP:%p/%d] MAX_DATA: %" PRIu64 " -> %" PRIu64 " (delta=%" PRIu64 ")",
			ep, quic_get_by_ep(ep), ep->tx_fc.max_data, max_data,
			max_data - ep->tx_fc.max_data);

		ep->tx_fc.max_data = max_data;

		/* Signal all streams on this endpoint that they may be writable now.
		 * Connection-level flow control affects all streams.
		 */
		ctx = quic_find_context(ep);
		if (ctx != NULL) {
			SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ctx->streams, stream, tmp, node) {
				if (stream->ep == ep) {
					k_poll_signal_raise(&stream->send.signal, 0);
				}
			}
		}
	} else {
		NET_DBG("[EP:%p/%d] MAX_DATA: %" PRIu64 " (no change)",
			ep, quic_get_by_ep(ep), max_data);
	}

	return pos;
}

static int handle_max_stream_data_frame(struct quic_endpoint *ep,
					const uint8_t *buf, size_t len)
{
	uint64_t stream_id, max_stream_data;
	struct quic_stream *stream;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &stream_id);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	ret = quic_get_len(&buf[pos], len - pos, &max_stream_data);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Find the stream and update its TX flow control */
	stream = quic_find_stream_by_id(ep, stream_id);
	if (stream != NULL) {
		if (max_stream_data > stream->remote_max_data) {
			NET_DBG("[EP:%p/%d] MAX_STREAM_DATA: stream=%" PRIu64
				", %" PRIu64 " -> %" PRIu64,
				ep, quic_get_by_ep(ep), stream_id,
				stream->remote_max_data, max_stream_data);
			stream->remote_max_data = max_stream_data;
			/* Signal that stream may now be writable */
			k_poll_signal_raise(&stream->send.signal, 0);
		}
	} else {
		NET_DBG("[EP:%p/%d] MAX_STREAM_DATA: stream=%" PRIu64
			" not found, max=%" PRIu64,
			ep, quic_get_by_ep(ep), stream_id, max_stream_data);
	}

	return pos;
}

static int handle_max_streams_frame(struct quic_endpoint *ep, int frame_type,
				    const uint8_t *buf, size_t len)
{
	uint64_t max_streams;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &max_streams);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	if (frame_type == QUIC_FRAME_TYPE_MAX_STREAMS_BIDI) {
		if (max_streams > ep->peer_params.max_streams_bidi) {
			NET_DBG("[EP:%p/%d] MAX_STREAMS_BIDI: %" PRIu64 " -> %" PRIu64,
				ep, quic_get_by_ep(ep), ep->peer_params.max_streams_bidi,
				max_streams);
			ep->peer_params.max_streams_bidi = max_streams;
		} else {
			NET_DBG("[EP:%p/%d] MAX_STREAMS_BIDI: %" PRIu64 " (no change)",
				ep, quic_get_by_ep(ep), max_streams);
		}
	} else {
		if (max_streams > ep->peer_params.max_streams_uni) {
			NET_DBG("[EP:%p/%d] MAX_STREAMS_UNI: %" PRIu64 " -> %" PRIu64,
				ep, quic_get_by_ep(ep), ep->peer_params.max_streams_uni,
				max_streams);
			ep->peer_params.max_streams_uni = max_streams;
		} else {
			NET_DBG("[EP:%p/%d] MAX_STREAMS_UNI: %" PRIu64 " (no change)",
				ep, quic_get_by_ep(ep), max_streams);
		}
	}

	return pos;
}

static int quic_send_max_data(struct quic_endpoint *ep)
{
	uint8_t frame[16];
	int pos = 0;
	int varint_len;

	frame[pos++] = QUIC_FRAME_TYPE_MAX_DATA;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos,
				     ep->rx_fc.max_data);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	NET_DBG("[EP:%p/%d] Sending MAX_DATA: %" PRIu64, ep, quic_get_by_ep(ep),
		ep->rx_fc.max_data);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

static int quic_send_max_stream_data(struct quic_endpoint *ep,
				     struct quic_stream *stream)
{
	uint8_t frame[24];
	int pos = 0;
	int varint_len;

	frame[pos++] = QUIC_FRAME_TYPE_MAX_STREAM_DATA;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos, stream->id);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos,
				     stream->local_max_data);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	stream->local_max_data_sent = stream->local_max_data;

	NET_DBG("[EP:%p/%d] Sending MAX_STREAM_DATA: stream=%" PRIu64 ", max=%" PRIu64,
		ep, quic_get_by_ep(ep), stream->id, stream->local_max_data);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

/**
 * @brief Send DATA_BLOCKED frame to notify peer that we are blocked
 *
 * This is sent when the sender is blocked by connection-level flow control.
 * The receiver should respond with MAX_DATA to increase the limit.
 *
 * @param ep The endpoint
 * @return 0 on success, negative errno on error
 */
static int quic_send_data_blocked(struct quic_endpoint *ep)
{
	uint8_t frame[16];
	int pos = 0;
	int varint_len;

	/* Only send if we haven't already sent DATA_BLOCKED for this limit */
	if (ep->tx_fc.blocked_sent >= ep->tx_fc.max_data) {
		return 0;
	}

	frame[pos++] = QUIC_FRAME_TYPE_DATA_BLOCKED;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos,
				     ep->tx_fc.max_data);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	ep->tx_fc.blocked_sent = ep->tx_fc.max_data;

	NET_DBG("[EP:%p/%d] Sending DATA_BLOCKED: limit=%" PRIu64, ep, quic_get_by_ep(ep),
		ep->tx_fc.max_data);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

/**
 * @brief Send STREAM_DATA_BLOCKED frame when stream flow control blocks sending
 *
 * This notifies the peer that we are blocked by stream-level flow control.
 * The peer should respond with MAX_STREAM_DATA to increase the limit.
 *
 * @param ep The endpoint
 * @param stream The stream that is blocked
 * @return 0 on success, negative errno on error
 */
static int quic_send_stream_data_blocked(struct quic_endpoint *ep,
					 struct quic_stream *stream)
{
	uint8_t frame[24];
	int pos = 0;
	int varint_len;

	/* Only send if we haven't already sent STREAM_DATA_BLOCKED for this limit */
	if (stream->blocked_sent >= stream->remote_max_data) {
		return 0;
	}

	frame[pos++] = QUIC_FRAME_TYPE_STREAM_DATA_BLOCKED;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos, stream->id);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos,
				     stream->remote_max_data);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	stream->blocked_sent = stream->remote_max_data;

	NET_DBG("[EP:%p/%d] Sending STREAM_DATA_BLOCKED: stream=%" PRIu64 ", limit=%" PRIu64,
		ep, quic_get_by_ep(ep), stream->id, stream->remote_max_data);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

static int handle_data_blocked_frame(struct quic_endpoint *ep,
				     const uint8_t *buf, size_t len)
{
	uint64_t max_data;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &max_data);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	NET_DBG("[EP:%p/%d] DATA_BLOCKED: limit=%" PRIu64,
		ep, quic_get_by_ep(ep), max_data);

	/* Peer is blocked, send MAX_DATA to increase their limit */
	if (ep->rx_fc.max_data > max_data) {
		quic_send_max_data(ep);
	}

	return pos;
}

static int handle_stream_data_blocked_frame(struct quic_endpoint *ep,
					    const uint8_t *buf, size_t len)
{
	struct quic_stream *stream;
	uint64_t stream_id, max_stream_data;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &stream_id);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	ret = quic_get_len(&buf[pos], len - pos, &max_stream_data);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	NET_DBG("[EP:%p/%d] STREAM_DATA_BLOCKED: stream=%" PRIu64 ", limit=%" PRIu64,
		ep, quic_get_by_ep(ep), stream_id, max_stream_data);

	/* Find the stream and send MAX_STREAM_DATA */
	stream = quic_find_stream_by_id(ep, stream_id);
	if (stream != NULL && stream->local_max_data > max_stream_data) {
		quic_send_max_stream_data(ep, stream);
	}

	return pos;
}

static int quic_send_max_streams(struct quic_endpoint *ep, bool bidi)
{
	uint8_t frame[16];
	int pos = 0;
	int varint_len;
	uint64_t limit = bidi ? ep->rx_sl.max_bidi : ep->rx_sl.max_uni;

	frame[pos++] = bidi ? QUIC_FRAME_TYPE_MAX_STREAMS_BIDI
			    : QUIC_FRAME_TYPE_MAX_STREAMS_UNI;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos, limit);
	if (varint_len <= 0) {
		return -EINVAL;
	}

	pos += varint_len;

	NET_DBG("[EP:%p/%d] Sending MAX_STREAMS_%s: %" PRIu64,
		ep, quic_get_by_ep(ep),
		bidi ? "BIDI" : "UNI", limit);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

static int handle_streams_blocked_frame(struct quic_endpoint *ep,
					const uint8_t *buf, size_t len)
{
	bool is_bidi = (buf[0] == QUIC_FRAME_TYPE_STREAMS_BLOCKED_BIDI);
	uint64_t max_streams;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &max_streams);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	NET_DBG("[EP:%p/%d] STREAMS_BLOCKED: limit=%" PRIu64,
		ep, quic_get_by_ep(ep), max_streams);

	/*
	 * Only extend the peer's stream credit if we have free pool slots.
	 * The number of free bidi slots = pool_capacity - currently open.
	 * Handing out more credit than we have slots would cause the pool
	 * to run dry and produce the -ENOMEM error.
	 */
	if (is_bidi) {
		uint64_t free_slots = (uint64_t)CONFIG_QUIC_MAX_STREAMS_BIDI
							- ep->rx_sl.open_bidi;

		if (free_slots > 0 && ep->rx_sl.max_bidi <= max_streams) {
			ep->rx_sl.max_bidi += free_slots;
			quic_send_max_streams(ep, true);

			NET_DBG("[EP:%p/%d] STREAMS_BLOCKED: advanced bidi limit to "
				"%" PRIu64 " (%" PRIu64 " free slots)",
				ep, quic_get_by_ep(ep),
				ep->rx_sl.max_bidi, free_slots);
		} else {
			NET_DBG("[EP:%p/%d] STREAMS_BLOCKED: pool full "
				"(open=%" PRIu64 "), not advancing limit",
				ep, quic_get_by_ep(ep), ep->rx_sl.open_bidi);
		}
	} else {
		uint64_t free_slots = (uint64_t)CONFIG_QUIC_MAX_STREAMS_UNI - ep->rx_sl.open_uni;

		if (free_slots > 0 && ep->rx_sl.max_uni <= max_streams) {
			ep->rx_sl.max_uni += free_slots;
			quic_send_max_streams(ep, false);
		}
	}

	return pos;
}

static int send_retire_connection_id(struct quic_endpoint *ep, uint64_t seq_num)
{
	uint8_t frame[16]; /* 1 (type) + up to 15 (varint) */
	int pos = 0;
	int varint_len;

	frame[pos++] = QUIC_FRAME_TYPE_RETIRE_CONNECTION_ID;

	varint_len = quic_put_varint(&frame[pos], sizeof(frame) - pos, seq_num);
	if (varint_len <= 0) {
		return -EINVAL;
	}
	pos += varint_len;

	NET_DBG("[EP:%p/%d] Sending RETIRE_CONNECTION_ID: seq=%" PRIu64,
		ep, quic_get_by_ep(ep), seq_num);

	return quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, frame, pos);
}

static int handle_new_connection_id_frame(struct quic_endpoint *ep,
					  const uint8_t *buf, size_t len)
{
	uint64_t seq_num, retire_prior_to;
	uint8_t cid_len;
	const uint8_t *cid;
	const uint8_t *stateless_reset_token;
	int pos = 1;
	int ret;
	int i;

	ret = quic_get_len(&buf[pos], len - pos, &seq_num);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	ret = quic_get_len(&buf[pos], len - pos, &retire_prior_to);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	if (pos >= len) {
		return -EINVAL;
	}

	if (retire_prior_to > seq_num) {
		NET_DBG("[EP:%p/%d] NEW_CONNECTION_ID: retire_prior_to (%"
			PRIu64 ") > seq_num (%" PRIu64 ")",
			ep, quic_get_by_ep(ep), retire_prior_to, seq_num);
		return -EPROTO;  /* should trigger CONNECTION_CLOSE */
	}

	cid_len = buf[pos++];

	if (cid_len > MAX_CONN_ID_LEN || pos + cid_len + QUIC_RESET_TOKEN_LEN > len) {
		return -EINVAL;
	}

	cid = &buf[pos];
	pos += cid_len;

	stateless_reset_token = &buf[pos];
	pos += QUIC_RESET_TOKEN_LEN;

	NET_DBG("[EP:%p/%d] NEW_CONNECTION_ID: seq=%" PRIu64
		", retire_prior=%" PRIu64 ", cid_len=%u",
		ep, quic_get_by_ep(ep), seq_num, retire_prior_to, cid_len);

	/* Update retire_prior_to tracking */
	if (retire_prior_to > ep->peer_cid_retire_prior_to) {
		ep->peer_cid_retire_prior_to = retire_prior_to;

		/* Retire CIDs with seq_num < retire_prior_to */
		for (i = 0; i < CONFIG_QUIC_MAX_PEER_CIDS; i++) {
			if (ep->peer_cid_pool[i].active &&
			    ep->peer_cid_pool[i].seq_num < retire_prior_to) {
				NET_DBG("[EP:%p/%d] Retiring peer CID seq=%" PRIu64,
					ep, quic_get_by_ep(ep), ep->peer_cid_pool[i].seq_num);
				/* Send RETIRE_CONNECTION_ID frame */
				send_retire_connection_id(ep, ep->peer_cid_pool[i].seq_num);
				ep->peer_cid_pool[i].active = false;
			}
		}
	}

	/* Store the new connection ID in the pool */
	for (i = 0; i < CONFIG_QUIC_MAX_PEER_CIDS; i++) {
		if (!ep->peer_cid_pool[i].active) {
			ep->peer_cid_pool[i].seq_num = seq_num;
			ep->peer_cid_pool[i].cid_len = cid_len;
			memcpy(ep->peer_cid_pool[i].cid, cid, cid_len);
			memcpy(ep->peer_cid_pool[i].stateless_reset_token,
			       stateless_reset_token, QUIC_RESET_TOKEN_LEN);
			ep->peer_cid_pool[i].active = true;
			NET_DBG("[EP:%p/%d] Stored peer CID seq=%" PRIu64 " in slot %d",
				ep, quic_get_by_ep(ep), seq_num, i);
			break;
		}
	}

	if (i == CONFIG_QUIC_MAX_PEER_CIDS) {
		NET_WARN("[EP:%p/%d] Peer CID pool full, ignoring new CID seq=%" PRIu64,
			 ep, quic_get_by_ep(ep), seq_num);
	}

	return pos;
}

static int handle_retire_connection_id_frame(struct quic_endpoint *ep,
					     const uint8_t *buf, size_t len)
{
	uint64_t seq_num;
	int pos = 1;
	int ret;

	ret = quic_get_len(&buf[pos], len - pos, &seq_num);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	NET_DBG("[EP:%p/%d] RETIRE_CONNECTION_ID: seq=%" PRIu64, ep, quic_get_by_ep(ep), seq_num);

	/* TODO: Handle retirement of our own CIDs that the peer wants to retire */

	return pos;
}

static int handle_path_challenge_frame(struct quic_endpoint *ep,
				       const uint8_t *buf, size_t len)
{
	uint8_t response[9];
	int pos = 1;
	int ret;

	if (pos + 8 > len) {
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] PATH_CHALLENGE received", ep, quic_get_by_ep(ep));

	/* Must respond with PATH_RESPONSE containing same 8 bytes */
	response[0] = QUIC_FRAME_TYPE_PATH_RESPONSE;
	memcpy(&response[1], &buf[pos], 8);

	ret = quic_send_packet(ep, QUIC_SECRET_LEVEL_APPLICATION, response, 9);
	if (ret != 0) {
		NET_WARN("Failed to send PATH_RESPONSE: %d", ret);
	}

	return pos + 8;
}

static int handle_path_response_frame(struct quic_endpoint *ep,
				      const uint8_t *buf, size_t len)
{
	int pos = 1;

	if (pos + 8 > len) {
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] PATH_RESPONSE received", ep, quic_get_by_ep(ep));

	/* TODO: Validate against pending PATH_CHALLENGE */

	return pos + 8;
}

/*
 * Parse and handle ACK frame
 *
 * ACK Frame {
 *   Type (i) = 0x02..0x03,
 *   Largest Acknowledged (i),
 *   ACK Delay (i),
 *   ACK Range Count (i),
 *   First ACK Range (i),
 *   ACK Range (. .) ...,
 *   [ECN Counts (. .)],
 * }
 */
static int handle_ack_frame(struct quic_endpoint *ep,
			    enum quic_secret_level level,
			    const uint8_t *data,
			    size_t len,
			    size_t *consumed)
{
	size_t pos = 0;
	uint64_t largest_ack;
	uint64_t ack_delay;
	uint64_t ack_range_count;
	uint64_t first_ack_range;
	uint64_t ack_delay_us;
	struct quic_ack_range ranges[QUIC_MAX_ACK_RANGES];
	int range_idx = 0;
	int ret;
	bool has_ecn = (data[pos] == QUIC_FRAME_TYPE_ACK_ECN);

	/* Skip frame type */
	pos++;

	/* Parse Largest Acknowledged */
	ret = quic_get_len(&data[pos], len - pos, &largest_ack);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Parse ACK Delay */
	ret = quic_get_len(&data[pos], len - pos, &ack_delay);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Parse ACK Range Count */
	ret = quic_get_len(&data[pos], len - pos, &ack_range_count);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Parse First ACK Range */
	ret = quic_get_len(&data[pos], len - pos, &first_ack_range);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Build the first ACK range: [largest_ack - first_ack_range, largest_ack] */
	if (first_ack_range > largest_ack) {
		return -EPROTO;
	}

	ranges[range_idx].end = largest_ack;
	ranges[range_idx].start = largest_ack - first_ack_range;
	range_idx++;

	NET_DBG("[EP:%p/%d] ACK frame: largest=%" PRIu64 ", delay=%" PRIu64
		", ranges=%" PRIu64 ", first_range=%" PRIu64,
		ep, quic_get_by_ep(ep), largest_ack, ack_delay, ack_range_count,
		first_ack_range);

	/* Parse additional ACK ranges and build range entries.
	 * Per RFC 9000 Section 19.3.1:
	 *   smallest = previous_range.start
	 *   gap means (gap + 1) unacknowledged packets
	 *   range_end = smallest - gap - 2
	 *   range_start = range_end - ack_range
	 */
	for (uint64_t i = 0; i < ack_range_count; i++) {
		uint64_t gap, ack_range;

		/* Gap */
		ret = quic_get_len(&data[pos], len - pos, &gap);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		/* ACK Range */
		ret = quic_get_len(&data[pos], len - pos, &ack_range);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		/* Build range entry if we have room */
		if (range_idx < QUIC_MAX_ACK_RANGES) {
			uint64_t smallest = ranges[range_idx - 1].start;

			if (smallest < gap + 2) {
				/* Malformed: gap exceeds available PN space */
				break;
			}

			ranges[range_idx].end = smallest - gap - 2;
			ranges[range_idx].start = ranges[range_idx].end - ack_range;
			range_idx++;
		}
	}

	/* Parse ECN counts if present */
	if (has_ecn) {
		uint64_t ect0, ect1, ecn_ce;

		ret = quic_get_len(&data[pos], len - pos, &ect0);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		ret = quic_get_len(&data[pos], len - pos, &ect1);
		if (ret < 0) {
			return ret;
		}

		pos += ret;

		ret = quic_get_len(&data[pos], len - pos, &ecn_ce);
		if (ret < 0) {
			return ret;
		}

		pos += ret;
	}

	/* Convert ACK delay to microseconds.
	 * The ack_delay field is encoded in units of 2^ack_delay_exponent microseconds.
	 * Default ack_delay_exponent is 3, so default unit is 8 microseconds.
	 * TODO: Use peer's ack_delay_exponent from transport params.
	 */
	ack_delay_us = ack_delay << 3; /* Default exponent = 3 */

	/* Update RTT and bytes_in_flight based on ACK ranges */
	quic_recovery_on_ack_received(ep, level, ranges, range_idx, ack_delay_us);

	*consumed = pos;

	return 0;
}

/*
 * Store out-of-order CRYPTO segment metadata
 */
static int crypto_store_ooo_segment(struct quic_endpoint *ep,
				    uint32_t offset, uint16_t len)
{
	int i;

	/* Check if this segment overlaps or is adjacent to existing ones */
	for (i = 0; i < CONFIG_QUIC_CRYPTO_OOO_SLOTS; i++) {
		struct quic_crypto_ooo_seg *seg = &ep->crypto.ooo[i];

		if (!seg->valid) {
			continue;
		}

		/* Check for overlap or adjacency */
		if (offset <= seg->offset + seg->len &&
		    offset + len >= seg->offset) {
			/* Merge: extend existing segment */
			uint32_t new_start = MIN(seg->offset, offset);
			uint32_t new_end = MAX(seg->offset + seg->len, offset + len);

			seg->offset = new_start;
			seg->len = new_end - new_start;
			return 0;
		}
	}

	/* Find empty slot */
	for (i = 0; i < CONFIG_QUIC_CRYPTO_OOO_SLOTS; i++) {
		if (!ep->crypto.ooo[i].valid) {
			ep->crypto.ooo[i].offset = offset;
			ep->crypto.ooo[i].len = len;
			ep->crypto.ooo[i].valid = true;
			return 0;
		}
	}

	NET_WARN("[EP:%p/%d] CRYPTO OOO slots exhausted", ep, quic_get_by_ep(ep));

	return -ENOMEM;
}

/*
 * Try to advance rx_offset by consuming contiguous data from the buffer
 * Returns bytes of new contiguous data available (0 if none)
 */
static uint32_t crypto_try_advance_offset(struct quic_endpoint *ep,
					  enum quic_secret_level level)
{
	uint32_t rx_offset = ep->crypto.stream[level].rx_offset;
	uint32_t contiguous_end = rx_offset;
	bool progress;
	int i;

	/* Keep merging OOO segments that are now contiguous */
	do {
		progress = false;

		for (i = 0; i < CONFIG_QUIC_CRYPTO_OOO_SLOTS; i++) {
			struct quic_crypto_ooo_seg *seg = &ep->crypto.ooo[i];

			if (!seg->valid) {
				continue;
			}

			/* If segment starts at or before contiguous_end and extends past it */
			if (seg->offset <= contiguous_end &&
			    seg->offset + seg->len > contiguous_end) {
				contiguous_end = seg->offset + seg->len;
				seg->valid = false; /* Consumed */
				progress = true;
			}
		}
	} while (progress);

	return contiguous_end - rx_offset;
}

/*
 * Reset CRYPTO reassembly buffer for a new encryption level
 */
static void crypto_reset_rx_buffer(struct quic_endpoint *ep,
				   enum quic_secret_level level)
{
	int i;

	ep->crypto.rx_buf_level = level;
	ep->crypto.rx_buf_len = 0;

	for (i = 0; i < CONFIG_QUIC_CRYPTO_OOO_SLOTS; i++) {
		ep->crypto.ooo[i].valid = false;
	}
}

/*
 * Parse and handle CRYPTO frame
 *
 * CRYPTO Frame {
 *   Type (i) = 0x06,
 *   Offset (i),
 *   Length (i),
 *   Crypto Data (..),
 * }
 */
static int handle_crypto_frame(struct quic_endpoint *ep,
			       enum quic_secret_level level,
			       const uint8_t *data,
			       size_t len,
			       size_t *consumed)
{
	size_t pos = 0;
	uint64_t offset;
	uint64_t crypto_len;
	uint32_t contiguous_bytes;
	int ret;

	/* Skip frame type (0x06) */
	pos++;

	/* Parse Offset */
	ret = quic_get_len(&data[pos], len - pos, &offset);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Parse Length */
	ret = quic_get_len(&data[pos], len - pos, &crypto_len);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Validate we have enough data */
	if (pos + crypto_len > len) {
		NET_DBG("[EP:%p/%d] CRYPTO frame truncated: need %" PRIu64 " bytes, have %zu",
			ep, quic_get_by_ep(ep), crypto_len, len - pos);
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] CRYPTO frame: offset=%" PRIu64 ", len=%" PRIu64,
		ep, quic_get_by_ep(ep), offset, crypto_len);

	/* Check for duplicate/already-received CRYPTO data */
	if (offset + crypto_len <= ep->crypto.stream[level].rx_offset) {
		NET_DBG("[EP:%p/%d] Duplicate CRYPTO data: offset=%" PRIu64 " len=%" PRIu64
			", already have up to %u",
			ep, quic_get_by_ep(ep), offset, crypto_len,
			ep->crypto.stream[level].rx_offset);
		*consumed = pos + crypto_len;
		return 0;
	}

	/* Reset buffer if switching to a new encryption level */
	if (ep->crypto.rx_buf_level != level) {
		crypto_reset_rx_buffer(ep, level);
	}

	/* Check if data fits in reassembly buffer */
	if (offset + crypto_len > CONFIG_QUIC_CRYPTO_RX_BUFFER_SIZE) {
		NET_ERR("CRYPTO data exceeds buffer: offset=%" PRIu64
			" len=%" PRIu64 " max=%d",
			offset, crypto_len, CONFIG_QUIC_CRYPTO_RX_BUFFER_SIZE);
		return -ENOMEM;
	}

	/* Copy data into reassembly buffer at the correct offset */
	memcpy(&ep->crypto.rx_buffer[offset], &data[pos], crypto_len);

	/* Track highest byte written */
	if (offset + crypto_len > ep->crypto.rx_buf_len) {
		ep->crypto.rx_buf_len = offset + crypto_len;
	}

	/* Record this segment (for tracking contiguity) */
	if (offset != ep->crypto.stream[level].rx_offset) {
		/* Out-of-order: store segment info */
		ret = crypto_store_ooo_segment(ep, (uint32_t)offset, (uint16_t)crypto_len);
		if (ret < 0) {
			*consumed = pos + crypto_len;
			return 0; /* Continue anyway, may reassemble later */
		}
	} else {
		/* In-order: directly advances contiguous region */
		ep->crypto.stream[level].rx_offset += crypto_len;
	}

	/* Try to advance rx_offset with any newly-contiguous OOO segments */
	contiguous_bytes = crypto_try_advance_offset(ep, level);
	if (contiguous_bytes > 0) {
		ep->crypto.stream[level].rx_offset += contiguous_bytes;
	}

	*consumed = pos + crypto_len;

	/* Initialize crypto context if needed (first CRYPTO frame) */
	if (!ep->crypto.initial.initialized) {
		if (!quic_conn_init_setup(ep, ep->my_cid, ep->my_cid_len)) {
			NET_DBG("[EP:%p/%d] Failed to setup Initial crypto context",
				ep, quic_get_by_ep(ep));
			return -EINVAL;
		}
		NET_DBG("[EP:%p/%d] Initialized Initial crypto context for endpoint",
			ep, quic_get_by_ep(ep));
	}

	if (!ep->crypto.tls.is_initialized) {
		ret = quic_tls_init(&ep->crypto.tls, ep->is_server);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] Failed to initialize TLS context: %d",
				ep, quic_get_by_ep(ep), ret);
			return ret;
		}
	}

	/* Process all contiguous data from the reassembly buffer
	 * that TLS hasn't consumed yet.
	 */
	uint32_t tls_consumed = ep->crypto.stream[level].tls_offset;
	uint32_t contiguous_end = ep->crypto.stream[level].rx_offset;
	uint32_t process_len = contiguous_end - tls_consumed;

	if (process_len > 0) {
		size_t tls_bytes_consumed = 0;

		NET_DBG("[EP:%p/%d] Processing %u bytes of CRYPTO data (offset %u to %u)",
			ep, quic_get_by_ep(ep), process_len, tls_consumed, contiguous_end);
		ret = quic_tls_process(&ep->crypto.tls, level,
				       &ep->crypto.rx_buffer[tls_consumed],
				       process_len, &tls_bytes_consumed);
		if (ret < 0) {
			NET_DBG("[EP:%p/%d] TLS processing failed: %d",
				ep, quic_get_by_ep(ep), ret);
			return ret;
		}
		/* Update how much TLS has actually consumed */
		ep->crypto.stream[level].tls_offset += tls_bytes_consumed;

		NET_DBG("[EP:%p/%d] TLS consumed %zu bytes, tls_offset now %u",
			ep, quic_get_by_ep(ep), tls_bytes_consumed,
			ep->crypto.stream[level].tls_offset);
	}

	return ret;
}

/*
 * Parse CONNECTION_CLOSE frame
 */
static int handle_connection_close_frame(struct quic_endpoint *ep,
					 const uint8_t *data,
					 size_t len,
					 size_t *consumed)
{
	size_t pos = 0;
	uint8_t frame_type = data[pos++];
	uint64_t error_code;
	uint64_t frame_type_field = 0;
	uint64_t reason_len;
	int ret;

	/* Parse Error Code */
	ret = quic_get_len(&data[pos], len - pos, &error_code);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* For transport errors, there's a Frame Type field */
	if (frame_type == QUIC_FRAME_TYPE_CONNECTION_CLOSE_TRANSPORT) {
		ret = quic_get_len(&data[pos], len - pos, &frame_type_field);
		if (ret < 0) {
			return ret;
		}

		pos += ret;
	}

	/* Parse Reason Phrase Length */
	ret = quic_get_len(&data[pos], len - pos, &reason_len);
	if (ret < 0) {
		return ret;
	}

	pos += ret;

	/* Skip reason phrase */
	if (pos + reason_len > len) {
		return -EINVAL;
	}

	NET_DBG("[EP:%p/%d] CONNECTION_CLOSE:  error=0x%" PRIx64 ", frame_type=0x%" PRIx64
		", reason_len=%" PRIu64,
		ep, quic_get_by_ep(ep), error_code, frame_type_field, reason_len);

	if (reason_len > 0) {
#define MAX_REASON_LEN 128
		char reason[MAX_REASON_LEN + 1];
		size_t reason_len_copy = MIN(reason_len,
					     sizeof(reason) - 1);

		memcpy(reason, &data[pos], reason_len_copy);
		reason[reason_len_copy] = '\0';

		NET_DBG("[EP:%p/%d] Close reason: \"%s\"", ep, quic_get_by_ep(ep), reason);
	}

	pos += reason_len;
	*consumed = pos;

	return 0;
}

static int quic_send_ack(struct quic_endpoint *ep,
			 enum quic_secret_level level,
			 uint64_t largest_pn)
{
	uint8_t frame[32];
	size_t pos = 0;

	/* ACK frame type */
	frame[pos++] = QUIC_FRAME_TYPE_ACK;

	/* Largest Acknowledged */
	pos += quic_put_varint(&frame[pos], sizeof(frame) - pos, largest_pn);

	/* ACK Delay (0 for now) */
	frame[pos++] = 0;

	/* ACK Range Count (0 = just one contiguous range) */
	frame[pos++] = 0;

	/* First ACK Range (acknowledging just this packet) */
	frame[pos++] = 0;

	return quic_send_packet(ep, level, frame, pos);
}

/*
 * Handle 1-RTT (short header) packet
 *
 * These packets carry application data after the handshake is complete.
 * They can contain various frame types including STREAM, ACK, PING, etc.
 */
static int handle_1rtt_packet(struct quic_pkt *pkt)
{
	struct quic_endpoint *ep = pkt->ep;
	int pending_pos = pkt->pos;
	size_t len = pkt->len;
	uint8_t *buf = &pkt->data[pending_pos];
	size_t consumed;
	int pos = 0;
	int ret = 0;
	bool ack_eliciting = true;  /* Track if packet has ack-eliciting frames */

	NET_DBG("[EP:%p/%d] Processing 1-RTT packet, pn=%" PRIu64 ", len=%zu",
		ep, quic_get_by_ep(ep), pkt->pkt_num, len);

	/* Parse frames in the packet */
	while (pos < (int)len) {
		uint8_t frame_type = buf[pos];

		/* Check for STREAM frames (0x08 - 0x0f) */
		if ((frame_type & 0xF8) == QUIC_FRAME_TYPE_STREAM_BASE) {
			ret = handle_stream_frame(ep, &buf[pos], len - pos, &consumed);
			if (ret < 0) {
				NET_DBG("[EP:%p/%d] Failed to handle STREAM frame: %d",
					ep, quic_get_by_ep(ep), ret);
				goto fail;
			}

			pos += consumed;
			continue;
		}

		switch (frame_type) {
		case QUIC_FRAME_TYPE_PADDING:
			/* Skip padding bytes, not ack-eliciting */
			ack_eliciting = false;
			pos++;
			break;

		case QUIC_FRAME_TYPE_PING:
			/* PING frame has no payload, just acknowledge it */
			NET_DBG("[EP:%p/%d] Received PING frame", ep, quic_get_by_ep(ep));
			pos++;
			break;

		case QUIC_FRAME_TYPE_ACK:
		case QUIC_FRAME_TYPE_ACK_ECN:
			/* ACK frames are NOT ack-eliciting per RFC 9000 Section 13.2 */
			ret = handle_ack_frame(ep, QUIC_SECRET_LEVEL_APPLICATION,
					       &buf[pos], len - pos, &consumed);
			if (ret < 0) {
				NET_DBG("[EP:%p/%d] Failed to handle ACK frame: %d",
					ep, quic_get_by_ep(ep), ret);
				goto fail;
			}

			pos += consumed;

			/* Do NOT set ack_eliciting as ACK frames don't need ACKs */
			ack_eliciting = false;
			break;

		case QUIC_FRAME_TYPE_RESET_STREAM:
			ret = handle_reset_stream_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_STOP_SENDING:
			ret = handle_stop_sending_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_CRYPTO:
			/* Post-handshake CRYPTO (e.g., NewSessionTicket) */
			ret = handle_crypto_frame(ep, QUIC_SECRET_LEVEL_APPLICATION,
						  &buf[pos], len - pos, &consumed);
			if (ret < 0) {
				goto fail;
			}

			pos += consumed;
			break;

		case QUIC_FRAME_TYPE_NEW_TOKEN:
			ret = handle_new_token_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_MAX_DATA:
			ret = handle_max_data_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_MAX_STREAM_DATA:
			ret = handle_max_stream_data_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_MAX_STREAMS_BIDI:
		case QUIC_FRAME_TYPE_MAX_STREAMS_UNI:
			ret = handle_max_streams_frame(ep, frame_type, &buf[pos],
						       len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_DATA_BLOCKED:
			ret = handle_data_blocked_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_STREAM_DATA_BLOCKED:
			ret = handle_stream_data_blocked_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_STREAMS_BLOCKED_BIDI:
		case QUIC_FRAME_TYPE_STREAMS_BLOCKED_UNI:
			ret = handle_streams_blocked_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_NEW_CONNECTION_ID:
			ret = handle_new_connection_id_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_RETIRE_CONNECTION_ID:
			ret = handle_retire_connection_id_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_PATH_CHALLENGE:
			ret = handle_path_challenge_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_PATH_RESPONSE:
			ret = handle_path_response_frame(ep, &buf[pos], len - pos);
			if (ret < 0) {
				goto fail;
			}

			pos += ret;
			break;

		case QUIC_FRAME_TYPE_CONNECTION_CLOSE_TRANSPORT:
		case QUIC_FRAME_TYPE_CONNECTION_CLOSE_APPLICATION:
			ret = handle_connection_close_frame(ep, &buf[pos], len - pos,
							    &consumed);
			if (ret < 0) {
				goto fail;
			}

			/* Connection is being closed by peer */
			NET_DBG("[EP:%p/%d] Connection closed by peer", ep, quic_get_by_ep(ep));
			pos += ret;
			ack_eliciting = false;
			goto close;

		case QUIC_FRAME_TYPE_HANDSHAKE_DONE:
			/* Client receives this from server */
			NET_DBG("[EP:%p/%d] Received HANDSHAKE_DONE frame", ep, quic_get_by_ep(ep));
			pos++;

			/* Mark handshake as complete on client side */
			if (!ep->is_server) {
				ep->crypto.tls.state = QUIC_TLS_STATE_CONNECTED;
			}

			break;

		default:
			NET_DBG("[EP:%p/%d] Unknown frame type 0x%02x in 1-RTT packet",
				ep, quic_get_by_ep(ep), frame_type);

			/* Unknown frame. We should close the connection with error,
			 * but for now just skip it until we have support for all the
			 * messages.
			 */
			goto fail;
		}
	}

	/* Send ACK only for packets containing ack-eliciting frames.
	 * Per RFC 9000 Section 13.2.1: "A sender MUST NOT send an ACK frame
	 * in response to a packet containing only ACK frames."
	 */
	if (ack_eliciting) {
		ret = quic_send_ack(ep, QUIC_SECRET_LEVEL_APPLICATION, pkt->pkt_num);
		if (ret != 0) {
			NET_DBG("[EP:%p/%d] Failed to send 1-RTT ACK: %d",
				ep, quic_get_by_ep(ep), ret);
		}
	}

	return ret;

close:
	/* Connection closed by peer. Notify streams and release endpoint */
	quic_endpoint_notify_streams_closed(ep);
	quic_endpoint_unref(ep);
	return ret;

fail:
	quic_endpoint_unref(ep);
	return ret;
}

/*
 * Handle frames in Initial/Handshake/Application packets.
 *
 * This function processes all frames in the decrypted payload.
 * For Initial packets, valid frames are:
 * - CRYPTO frames (0x06): TLS handshake data
 * - ACK frames (0x02, 0x03): Acknowledgments
 * - PADDING frames (0x00): Padding
 * - PING frames (0x01): Keep-alive
 * - CONNECTION_CLOSE frames (0x1c, 0x1d): Connection termination
 */
static int handle_crypto_level_packet(struct quic_endpoint *ep,
				      enum quic_secret_level level,
				      const uint8_t *payload,
				      size_t payload_len,
				      size_t total_packet_len,
				      bool *ack_only)
{
	size_t pos = 0;
	int ret;
	bool saw_ack_frame = false;
	bool saw_other_frame = false;

	NET_DBG("[EP:%p/%d] Processing %s packet, payload %zu bytes",
		ep, quic_get_by_ep(ep),
		level == QUIC_SECRET_LEVEL_INITIAL ? "Initial" :
		level == QUIC_SECRET_LEVEL_HANDSHAKE ? "Handshake" : "Application",
		payload_len);

	while (pos < payload_len) {
		uint8_t frame_type = payload[pos];

		/* Handle PADDING frames (type 0x00), just skip them. */
		/* PADDING is not ack-eliciting */
		if (frame_type == QUIC_FRAME_TYPE_PADDING) {
			pos++;
			continue;
		}

		/* Handle PING frames (type 0x01), no content, just acknowledge */
		if (frame_type == QUIC_FRAME_TYPE_PING) {
			NET_DBG("[EP:%p/%d] PING frame at offset %zu",
				ep, quic_get_by_ep(ep), pos);
			pos++;
			saw_other_frame = true;
			continue;
		}

		/* Handle ACK frames (type 0x02 or 0x03) */
		/* ACK is NOT ack-eliciting per RFC 9000 Section 13.2 */
		if (frame_type == QUIC_FRAME_TYPE_ACK ||
		    frame_type == QUIC_FRAME_TYPE_ACK_ECN) {
			size_t consumed = 0;

			ret = handle_ack_frame(ep, level, &payload[pos],
					       payload_len - pos, &consumed);
			if (ret < 0) {
				NET_DBG("[EP:%p/%d] Failed to parse %s frame: %d",
					ep, quic_get_by_ep(ep), "ACK", ret);
				return ret;
			}
			pos += consumed;
			saw_ack_frame = true;
			continue;
		}

		/* Handle CRYPTO frames (type 0x06) */
		if (frame_type == QUIC_FRAME_TYPE_CRYPTO) {
			size_t consumed = 0;

			ret = handle_crypto_frame(ep, level, &payload[pos],
						  payload_len - pos, &consumed);
			NET_DBG("[EP:%p/%d] CRYPTO frame processed: pos=%zu, consumed=%zu, "
				"payload_len=%zu, ret=%d",
				ep, quic_get_by_ep(ep), pos, consumed, payload_len, ret);
			if (ret < 0) {
				NET_DBG("[EP:%p/%d] Failed to handle %s frame: %d",
					ep, quic_get_by_ep(ep), "CRYPTO", ret);
				return ret;
			}
			pos += consumed;
			saw_other_frame = true;

			if (ret == QUIC_HANDSHAKE_COMPLETED) {
				NET_DBG("[EP:%p/%d] TLS handshake complete, deriving "
					"application secrets", ep, quic_get_by_ep(ep));
				ret = quic_handshake_complete(ep);
				if (ret != 0) {
					NET_DBG("[EP:%p/%d] Failed to complete handshake: %d",
						ep, quic_get_by_ep(ep), ret);
					return ret;
				}
			}

			continue;
		}

		/* Handle STREAM frames (type 0x08-0x0f) */
		if ((frame_type & 0xF8) == QUIC_FRAME_TYPE_STREAM_BASE) {
			size_t consumed = 0;

			ret = handle_stream_frame(ep, &payload[pos],
						  payload_len - pos, &consumed);
			if (ret < 0) {
				NET_DBG("[EP:%p/%d] Failed to handle %s frame: %d",
					ep, quic_get_by_ep(ep), "STREAM", ret);
				return ret;
			}
			pos += consumed;
			saw_other_frame = true;

			continue;
		}

		/* Handle CONNECTION_CLOSE frames */
		if (frame_type == QUIC_FRAME_TYPE_CONNECTION_CLOSE_TRANSPORT ||
		    frame_type == QUIC_FRAME_TYPE_CONNECTION_CLOSE_APPLICATION) {
			uint64_t error_code = 0;
			uint64_t cc_frame_type = 0;
			uint64_t reason_len = 0;
			int n;

			NET_DBG("[EP:%p/%d] CONNECTION_CLOSE frame received",
				ep, quic_get_by_ep(ep));

			n = quic_get_len(&payload[pos], payload_len - pos, &error_code);
			if (n > 0) {
				pos += n;
			}

			n = quic_get_len(&payload[pos], payload_len - pos, &cc_frame_type);
			if (n > 0) {
				pos += n;
			}

			n = quic_get_len(&payload[pos], payload_len - pos, &reason_len);
			if (n > 0) {
				pos += n;
			}

			NET_DBG("[EP:%p/%d] Connection closed by peer: error=0x%" PRIx64
				" (frame_type=0x%" PRIx64 ", reason_len=%" PRIu64 ")",
				ep, quic_get_by_ep(ep), error_code, cc_frame_type, reason_len);

			/* TLS alerts come in as 0x100 + alert_code.
			 * TRANSPORT_PARAMETER_ERROR = 0xa
			 * missing_extension TLS alert = 0x116d
			 */

			/* Connection close is handled in the caller. */
			return 1; /* Signal connection closing */
		}

		/* For Initial packets, only specific frame types are allowed */
		if (level == QUIC_SECRET_LEVEL_INITIAL) {
			/* Only CRYPTO, ACK, PADDING, PING, CONNECTION_CLOSE are valid */
			break;
		}

		/* Handle Application-level frame types */
		if (level == QUIC_SECRET_LEVEL_APPLICATION) {
			switch (frame_type) {
			case QUIC_FRAME_TYPE_NEW_CONNECTION_ID:
				ret = handle_new_connection_id_frame(ep, &payload[pos],
								     payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_RETIRE_CONNECTION_ID:
				ret = handle_retire_connection_id_frame(ep, &payload[pos],
									payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_PATH_CHALLENGE:
				ret = handle_path_challenge_frame(ep, &payload[pos],
								  payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_PATH_RESPONSE:
				ret = handle_path_response_frame(ep, &payload[pos],
								 payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_HANDSHAKE_DONE:
				/* Client receives this from server */
				NET_DBG("[EP:%p/%d] Received HANDSHAKE_DONE frame",
					ep, quic_get_by_ep(ep));

				pos++;
				saw_other_frame = true;
				if (!ep->is_server) {
					ep->crypto.tls.state = QUIC_TLS_STATE_CONNECTED;

					/* Discard Initial and Handshake spaces */
					quic_recovery_discard_pn_space(ep,
						level_to_pn_space(QUIC_SECRET_LEVEL_INITIAL));
					quic_recovery_discard_pn_space(ep,
						level_to_pn_space(QUIC_SECRET_LEVEL_HANDSHAKE));
				}
				continue;

			case QUIC_FRAME_TYPE_NEW_TOKEN:
				ret = handle_new_token_frame(ep, &payload[pos],
							     payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_MAX_DATA:
				ret = handle_max_data_frame(ep, &payload[pos],
							    payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_MAX_STREAM_DATA:
				ret = handle_max_stream_data_frame(ep, &payload[pos],
								   payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_MAX_STREAMS_BIDI:
			case QUIC_FRAME_TYPE_MAX_STREAMS_UNI:
				ret = handle_max_streams_frame(ep, frame_type, &payload[pos],
							       payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_DATA_BLOCKED:
				ret = handle_data_blocked_frame(ep, &payload[pos],
								payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_STREAM_DATA_BLOCKED:
				ret = handle_stream_data_blocked_frame(ep, &payload[pos],
								       payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_STREAMS_BLOCKED_BIDI:
			case QUIC_FRAME_TYPE_STREAMS_BLOCKED_UNI:
				ret = handle_streams_blocked_frame(ep, &payload[pos],
								   payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_RESET_STREAM:
				ret = handle_reset_stream_frame(ep, &payload[pos],
								payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			case QUIC_FRAME_TYPE_STOP_SENDING:
				ret = handle_stop_sending_frame(ep, &payload[pos],
								payload_len - pos);
				if (ret < 0) {
					return ret;
				}
				pos += ret;
				saw_other_frame = true;
				continue;

			default:
				break;
			}
		}

		/* Unhandled frame type */
		NET_DBG("[EP:%p/%d] Unhandled frame type 0x%02x at offset %zu",
			ep, quic_get_by_ep(ep), frame_type, pos);
		return -ENOTSUP;
	}

	/* Set ack_only flag: true if we saw ACK frame(s) but no other frames */
	if (ack_only != NULL) {
		*ack_only = saw_ack_frame && !saw_other_frame;
	}

	return 0;
}
