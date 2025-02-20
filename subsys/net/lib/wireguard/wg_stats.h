/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __WG_STATS_H__
#define __WG_STATS_H__

#include <zephyr/net/wireguard.h>

#if defined(CONFIG_NET_STATISTICS_VPN)

static inline void vpn_stats_update_keepalive_rx(struct wg_iface_context *ctx)
{
	ctx->stats.keepalive_rx++;
}

static inline void vpn_stats_update_keepalive_tx(struct wg_iface_context *ctx)
{
	ctx->stats.keepalive_tx++;
}

static inline void vpn_stats_update_peer_not_found(struct wg_iface_context *ctx)
{
	ctx->stats.peer_not_found++;
}

static inline void vpn_stats_update_key_expired(struct wg_iface_context *ctx)
{
	ctx->stats.key_expired++;
}

static inline void vpn_stats_update_invalid_packet(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_packet++;
}

static inline void vpn_stats_update_invalid_key(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_key++;
}

static inline void vpn_stats_update_invalid_mic(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_mic++;
}

static inline void vpn_stats_update_invalid_packet_len(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_packet_len++;
}

static inline void vpn_stats_update_invalid_keepalive(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_keepalive++;
}

static inline void vpn_stats_update_invalid_handshake(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_handshake++;
}

static inline void vpn_stats_update_invalid_cookie(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_cookie++;
}

static inline void vpn_stats_update_invalid_mac1(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_mac1++;
}

static inline void vpn_stats_update_invalid_mac2(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_mac2++;
}

static inline void vpn_stats_update_decrypt_failed(struct wg_iface_context *ctx)
{
	ctx->stats.decrypt_failed++;
}

static inline void vpn_stats_update_drop_rx(struct wg_iface_context *ctx)
{
	ctx->stats.drop_rx++;
}

static inline void vpn_stats_update_drop_tx(struct wg_iface_context *ctx)
{
	ctx->stats.drop_tx++;
}

static inline void vpn_stats_update_alloc_failed(struct wg_iface_context *ctx)
{
	ctx->stats.alloc_failed++;
}

static inline void vpn_stats_update_invalid_ip_version(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_ip_version++;
}

static inline void vpn_stats_update_denied_ip(struct wg_iface_context *ctx)
{
	ctx->stats.denied_ip++;
}

static inline void vpn_stats_update_replay_error(struct wg_iface_context *ctx)
{
	ctx->stats.replay_error++;
}

static inline void vpn_stats_update_invalid_ip_family(struct wg_iface_context *ctx)
{
	ctx->stats.invalid_ip_family++;
}

static inline void vpn_stats_update_valid_rx(struct wg_iface_context *ctx)
{
	ctx->stats.valid_rx++;
}

static inline void vpn_stats_update_valid_tx(struct wg_iface_context *ctx)
{
	ctx->stats.valid_tx++;
}

static inline void vpn_stats_update_handshake_init_rx(struct wg_iface_context *ctx)
{
	ctx->stats.handshake_init_rx++;
}

static inline void vpn_stats_update_handshake_init_tx(struct wg_iface_context *ctx)
{
	ctx->stats.handshake_init_tx++;
}

static inline void vpn_stats_update_handshake_resp_rx(struct wg_iface_context *ctx)
{
	ctx->stats.handshake_resp_rx++;
}

static inline void vpn_stats_update_handshake_resp_tx(struct wg_iface_context *ctx)
{
	ctx->stats.handshake_resp_tx++;
}
#else /* CONFIG_NET_STATISTICS_VPN */

#define vpn_stats_update_keepalive_rx(ctx)
#define vpn_stats_update_keepalive_tx(ctx)
#define vpn_stats_update_peer_not_found(ctx)
#define vpn_stats_update_key_expired(ctx)
#define vpn_stats_update_invalid_packet(ctx)
#define vpn_stats_update_invalid_key(ctx)
#define vpn_stats_update_invalid_mic(ctx)
#define vpn_stats_update_invalid_packet_len(ctx)
#define vpn_stats_update_invalid_keepalive(ctx)
#define vpn_stats_update_invalid_handshake(ctx)
#define vpn_stats_update_invalid_cookie(ctx)
#define vpn_stats_update_invalid_mac1(ctx)
#define vpn_stats_update_invalid_mac2(ctx)
#define vpn_stats_update_decrypt_failed(ctx)
#define vpn_stats_update_drop_rx(ctx)
#define vpn_stats_update_drop_tx(ctx)
#define vpn_stats_update_alloc_failed(ctx)
#define vpn_stats_update_invalid_ip_version(ctx)
#define vpn_stats_update_denied_ip(ctx)
#define vpn_stats_update_replay_error(ctx)
#define vpn_stats_update_invalid_ip_family(ctx)
#define vpn_stats_update_valid_rx(ctx)
#define vpn_stats_update_valid_tx(ctx)
#define vpn_stats_update_handshake_init_rx(ctx)
#define vpn_stats_update_handshake_init_tx(ctx)
#define vpn_stats_update_handshake_resp_rx(ctx)
#define vpn_stats_update_handshake_resp_tx(ctx)

#endif /* CONFIG_NET_STATISTICS_VPN */

#endif /* __WG_STATS_H__ */
