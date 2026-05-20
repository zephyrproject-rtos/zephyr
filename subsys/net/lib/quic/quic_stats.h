/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __QUIC_STATS_H__
#define __QUIC_STATS_H__

#if defined(CONFIG_NET_STATISTICS_QUIC)

static inline void quic_stats_update_peer_not_found(struct quic_context *ctx)
{
	ctx->stats.peer_not_found++;
}

static inline void quic_stats_update_invalid_packet(struct quic_context *ctx)
{
	ctx->stats.invalid_packet++;
}

static inline void quic_stats_update_invalid_key(struct quic_context *ctx)
{
	ctx->stats.invalid_key++;
}

static inline void quic_stats_update_invalid_packet_len(struct quic_context *ctx)
{
	ctx->stats.invalid_packet_len++;
}

static inline void quic_stats_update_invalid_handshake(struct quic_context *ctx)
{
	ctx->stats.invalid_handshake++;
}

static inline void quic_stats_update_decrypt_failed(struct quic_context *ctx)
{
	ctx->stats.decrypt_failed++;
}

static inline void quic_stats_update_drop_rx(struct quic_context *ctx)
{
	ctx->stats.drop_rx++;
}

static inline void quic_stats_update_drop_tx(struct quic_context *ctx)
{
	ctx->stats.drop_tx++;
}

static inline void quic_stats_update_alloc_failed(struct quic_context *ctx)
{
	ctx->stats.alloc_failed++;
}

static inline void quic_stats_update_valid_rx(struct quic_context *ctx)
{
	ctx->stats.valid_rx++;
}

static inline void quic_stats_update_valid_tx(struct quic_context *ctx)
{
	ctx->stats.valid_tx++;
}

static inline void quic_stats_update_handshake_init_rx(struct quic_context *ctx)
{
	ctx->stats.handshake_init_rx++;
}

static inline void quic_stats_update_handshake_init_tx(struct quic_context *ctx)
{
	ctx->stats.handshake_init_tx++;
}

static inline void quic_stats_update_handshake_resp_rx(struct quic_context *ctx)
{
	ctx->stats.handshake_resp_rx++;
}

static inline void quic_stats_update_handshake_resp_tx(struct quic_context *ctx)
{
	ctx->stats.handshake_resp_tx++;
}

extern struct net_stats_quic_global *quic_stats;

static inline void quic_stats_update_packets_rx(uint32_t count)
{
	quic_stats->packets_rx += count;
}

static inline void quic_stats_update_connection_opened(void)
{
	quic_stats->connections_opened++;
}

static inline void quic_stats_update_connection_closed(void)
{
	quic_stats->connections_closed++;
}

static inline void quic_stats_update_stream_opened(void)
{
	quic_stats->streams_opened++;
}

static inline void quic_stats_update_stream_closed(void)
{
	quic_stats->streams_closed++;
}

static inline void quic_stats_update_connection_open_failed(void)
{
	quic_stats->connection_open_failed++;
}

static inline void quic_stats_update_stream_open_failed(void)
{
	quic_stats->stream_open_failed++;
}

/* Close failures are typically programming errors as close operation always
 * succeeds from application perspective.
 */
static inline void quic_stats_update_connection_close_failed(void)
{
	quic_stats->connection_close_failed++;
}

static inline void quic_stats_update_stream_close_failed(void)
{
	quic_stats->stream_close_failed++;
}

#else /* CONFIG_NET_STATISTICS_QUIC */

#define quic_stats_update_peer_not_found(ctx)
#define quic_stats_update_invalid_packet(ctx)
#define quic_stats_update_invalid_key(ctx)
#define quic_stats_update_invalid_packet_len(ctx)
#define quic_stats_update_invalid_handshake(ctx)
#define quic_stats_update_decrypt_failed(ctx)
#define quic_stats_update_drop_rx(ctx)
#define quic_stats_update_drop_tx(ctx)
#define quic_stats_update_alloc_failed(ctx)
#define quic_stats_update_valid_rx(ctx)
#define quic_stats_update_valid_tx(ctx)
#define quic_stats_update_handshake_init_rx(ctx)
#define quic_stats_update_handshake_init_tx(ctx)
#define quic_stats_update_handshake_resp_rx(ctx)
#define quic_stats_update_handshake_resp_tx(ctx)

#define quic_stats_update_packets_rx(count)
#define quic_stats_update_connection_opened()
#define quic_stats_update_connection_closed()
#define quic_stats_update_stream_opened()
#define quic_stats_update_stream_closed()
#define quic_stats_update_connection_open_failed()
#define quic_stats_update_stream_open_failed()
#define quic_stats_update_connection_close_failed()
#define quic_stats_update_stream_close_failed()
#endif /* CONFIG_NET_STATISTICS_QUIC */

#endif /* __QUIC_STATS_H__ */
