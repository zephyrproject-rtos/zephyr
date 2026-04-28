/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_LORAWAN_NATIVE_MAC_MAC_INTERNAL_H_
#define SUBSYS_LORAWAN_NATIVE_MAC_MAC_INTERNAL_H_

#include "mac.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RX_BUF_SIZE		255

/* LoRaWAN field sizes in bytes */
#define LWAN_MIC_SIZE		4
#define MHDR_SIZE		1
#define DEV_ADDR_SIZE		4
#define FCTRL_SIZE		1
#define FCNT_SIZE		2
#define FPORT_SIZE		1
#define EUI_SIZE		8
#define JOIN_NONCE_SIZE		3
#define NET_ID_SIZE		3
#define DEV_NONCE_SIZE		2

#define LWAN_FCNT_NONE		UINT32_MAX
#define LWAN_PENDING_ACK	BIT(0)

#define MHDR_TYPE_MASK		GENMASK(7, 5)
#define MHDR_MAJOR_MASK		GENMASK(1, 0)
#define MHDR_MAJOR_R1		0

enum mac_rx_result {
	MAC_RX_DONE,
	MAC_RX_CONTINUE,
};

struct mac_tx_params {
	/* Pre-built uplink frame */
	uint8_t *frame;
	size_t frame_len;

	/* TX channel frequency in Hz */
	uint32_t tx_freq;
	/* TX datarate index */
	uint8_t tx_dr_idx;

	/* Delay from TX-done to RX1 open, in ms */
	uint32_t rx1_delay_ms;

	/* Called after TX, before RX windows; used to bump FCntUp (may be NULL) */
	void (*post_tx)(struct lwan_ctx *ctx);

	/* Called on each RX frame; MAC_RX_DONE to accept, MAC_RX_CONTINUE to keep going */
	enum mac_rx_result (*rx_handler)(struct lwan_ctx *ctx,
					  const uint8_t *rx_buf, size_t rx_len,
					  int16_t rssi, int8_t snr,
					  void *user_data);

	/* Opaque data passed to rx_handler */
	void *user_data;
};

int mac_do_tx_rx(struct lwan_ctx *ctx, const struct mac_tx_params *params);
void mac_do_join(struct lwan_ctx *ctx, const struct lwan_join_req *req);
void mac_do_send(struct lwan_ctx *ctx, const struct lwan_send_req *req);

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_LORAWAN_NATIVE_MAC_MAC_INTERNAL_H_ */
