/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * LoRaWAN Class A TX/RX timing
 *
 * The device transmits an uplink and opens two short receive windows
 * at fixed delays from the end of TX.  If no downlink is received in
 * RX1 the device tries RX2 on a different frequency/datarate.
 *
 *            TX done
 *            |
 *            |<--- rx1_delay_ms --->|<-- RX2_DELAY_OFFSET_MS -->|
 *            |                      |                            |
 *            v                      v                            v
 *  +---------+                +-----+-----+                +-----+-----+
 *  |   TX    |                |    RX1    |                |    RX2    |
 *  +---------+                +-----+-----+                +-----+-----+
 *                             ^                            ^
 *                             |                            |
 *                       wake up early by             wake up early by
 *                       RX_SETUP_MARGIN_MS           RX_SETUP_MARGIN_MS
 *
 * rx1_delay_ms is set per transaction:
 *   - Join accept:  5000 ms  (JOIN_ACCEPT_DELAY1_MS in join.c)
 *   - Data frames:  rx_delay from join accept (default 1000 ms)
 *
 * Each RX window listens for at most the airtime of a maximum-size
 * frame at that window's datarate, plus RX_TIMEOUT_MARGIN_MS.
 */

#include <zephyr/kernel.h>

#include "mac_internal.h"
#include "radio.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lorawan_native_mac, CONFIG_LORAWAN_LOG_LEVEL);

#define RX_SETUP_MARGIN_MS	50
#define RX2_DELAY_OFFSET_MS	1000
#define RX_TIMEOUT_MARGIN_MS	100

/* PHY frame overhead: MHDR(1) + FHDR(7) + FPort(1) + MIC(4) */
#define PHY_OVERHEAD		13

static int mac_try_rx_window(struct lwan_ctx *ctx,
			     const struct mac_tx_params *params,
			     uint32_t delay_ms, uint32_t rx_freq,
			     const struct lwan_dr_params *rx_dr,
			     int64_t tx_done, const char *win_name)
{
	uint8_t rx_buf[MAX_RX_BUF_SIZE];
	int64_t wake_at = tx_done + delay_ms - RX_SETUP_MARGIN_MS;
	uint32_t timeout;
	int64_t elapsed;
	int16_t rssi;
	int8_t snr;
	int rx_ret;

	k_sleep(K_TIMEOUT_ABS_MS(wake_at));

	elapsed = k_uptime_get() - tx_done;
	LOG_INF("%s at TX+%lld ms: freq=%u sf=%u bw=%u",
		win_name, elapsed, rx_freq, rx_dr->sf, rx_dr->bw);

	timeout = radio_airtime_params(rx_dr->sf, rx_dr->bw,
				       rx_dr->max_payload + PHY_OVERHEAD);
	timeout += RX_TIMEOUT_MARGIN_MS;

	rx_ret = radio_rx(rx_freq, rx_dr, timeout,
			  rx_buf, sizeof(rx_buf), &rssi, &snr);

	if (rx_ret < 0) {
		return rx_ret;
	}

	if (rx_ret > 0 &&
	    params->rx_handler(ctx, rx_buf, rx_ret, rssi, snr,
			       params->user_data) == MAC_RX_DONE) {
		LOG_INF("Response received in %s", win_name);
		return 0;
	}

	return -EAGAIN;
}

int mac_do_tx_rx(struct lwan_ctx *ctx, const struct mac_tx_params *params)
{
	const struct lwan_region_ops *region = ctx->region;
	struct lwan_dr_params tx_dr;
	struct lwan_dr_params rx_dr;
	uint32_t rx_freq;
	int8_t tx_power;
	int64_t tx_done_time;
	int ret;

	ret = region->get_tx_params(params->tx_dr_idx, &tx_dr, &tx_power);
	if (ret != 0) {
		return ret;
	}

	LOG_INF("TX: freq=%u dr=%u power=%d", params->tx_freq, params->tx_dr_idx,
		tx_power);

	ret = radio_tx(params->frame, params->frame_len, params->tx_freq, &tx_dr, tx_power);
	if (ret != 0) {
		LOG_ERR("TX failed: %d", ret);
		return ret;
	}

	tx_done_time = k_uptime_get();

	if (IS_ENABLED(CONFIG_LORAWAN_NATIVE_DUTY_CYCLE)) {
		uint32_t airtime = radio_airtime(params->frame_len);

		region->record_tx(params->tx_freq, airtime);
	}

	if (params->post_tx != NULL) {
		params->post_tx(ctx);
	}

	/* --- RX1 window --- */
	ret = region->get_rx1_params(params->tx_freq, params->tx_dr_idx,
				     ctx->session.rx1_dr_offset,
				     &rx_freq, &rx_dr);
	if (ret != 0) {
		return ret;
	}

	ret = mac_try_rx_window(ctx, params, params->rx1_delay_ms, rx_freq,
				&rx_dr, tx_done_time, "RX1");
	if (ret == 0) {
		return 0;
	}
	if (ret != -EAGAIN) {
		return ret;
	}

	/* --- RX2 window --- */
	ret = region->get_rx2_params(ctx->session.rx2_datarate, &rx_freq,
				       &rx_dr);
	if (ret != 0) {
		return ret;
	}

	ret = mac_try_rx_window(ctx, params,
				params->rx1_delay_ms + RX2_DELAY_OFFSET_MS,
				rx_freq, &rx_dr, tx_done_time, "RX2");
	if (ret == 0) {
		return 0;
	}
	if (ret != -EAGAIN) {
		return ret;
	}

	return -ETIMEDOUT;
}

void mac_process_req(struct lwan_ctx *ctx, const struct lwan_req *req)
{
	switch (req->type) {
	case LWAN_REQ_JOIN:
		mac_do_join(ctx, req->data);
		break;
	case LWAN_REQ_SEND:
		mac_do_send(ctx, req->data);
		break;
	default:
		LOG_WRN("Unknown request type: %d", req->type);
		break;
	}
}
