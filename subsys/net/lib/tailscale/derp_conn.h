/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_NET_TAILSCALE_DERP_CONN_H_
#define ZEPHYR_NET_TAILSCALE_DERP_CONN_H_

#include <zephyr/net/net_if.h>
#include "tailscale_internal.h"
#include <zephyr/net/tls_credentials.h>
#include "control_internal.h"

#define DERP_FRAME_MAX (TAILSCALE_DATAGRAM_MAX + 32U)

enum derp_frame_type {
	DERP_SERVER_KEY = 1,
	DERP_CLIENT_INFO,
	DERP_SERVER_INFO,
	DERP_SEND,
	DERP_RECV,
	DERP_KEEP_ALIVE,
	DERP_PREFERRED,
	DERP_PEER_GONE,
	DERP_PING = 0x12,
	DERP_PONG,
	DERP_HEALTH,
	DERP_RESTARTING,
};

struct derp_conn {
	struct control_io io;
	uint8_t pinned_hash[32];
	uint32_t rate;
	uint32_t burst;
	int64_t credit;
	int64_t credit_time;
	int64_t last_rx;
};

int derp_conn_open(struct derp_conn *conn, const struct tailscale_derp_node *node,
		   struct net_if *lan, sec_tag_t ca, mbedtls_svc_key_id_t private_key, bool home,
		   uint8_t scratch[DERP_FRAME_MAX]);
int derp_conn_read(struct derp_conn *conn, uint8_t *type, uint8_t data[DERP_FRAME_MAX],
		   size_t *len);
int derp_conn_write(struct derp_conn *conn, uint8_t type, const uint8_t *data, size_t len);
#endif
