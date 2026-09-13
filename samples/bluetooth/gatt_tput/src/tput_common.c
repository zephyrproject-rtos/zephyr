/*
 * Copyright (c) 2026 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared helpers: throughput counters, a once-per-second reporter thread, and
 * per-connection link tuning (2M PHY + Data Length Extension).
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_REGISTER(gatt_tput, LOG_LEVEL_INF);

atomic_t app_tx_bytes;
atomic_t app_rx_bytes;

static uint32_t total_tx; /* cumulative TX bytes for this session */
static uint32_t total_rx; /* cumulative RX bytes for this session */

void app_reset_totals(void)
{
	total_tx = 0U;
	total_rx = 0U;
	atomic_clear(&app_tx_bytes);
	atomic_clear(&app_rx_bytes);
}

uint16_t app_payload_len(struct bt_conn *conn)
{
	uint16_t mtu = bt_gatt_get_mtu(conn);
	uint16_t len;

	/* ATT notification/write payload = MTU - 3 byte ATT header. */
	if (mtu < 23U) {
		mtu = 23U;
	}

	len = mtu - 3U;

	return MIN(len, APP_MAX_PAYLOAD);
}

void app_link_optimize(struct bt_conn *conn)
{
	int err;

	err = bt_conn_le_phy_update(conn, BT_CONN_LE_PHY_PARAM_2M);
	if (err) {
		LOG_WRN("PHY update request failed (%d)", err);
	}

	err = bt_conn_le_data_len_update(conn, BT_LE_DATA_LEN_PARAM_MAX);
	if (err) {
		LOG_WRN("Data length update request failed (%d)", err);
	}
}

static void report_thread(void)
{
	while (1) {
		k_sleep(K_SECONDS(1));

		uint32_t tx = atomic_clear(&app_tx_bytes);
		uint32_t rx = atomic_clear(&app_rx_bytes);

		if (tx == 0U && rx == 0U) {
			continue;
		}

		total_tx += tx;
		total_rx += rx;

		/* bytes/s -> kbps with 3 decimals so sub-kbps rates aren't rounded to 0. */
		uint32_t tx_bits = tx * 8U;
		uint32_t rx_bits = rx * 8U;

		LOG_INF("Throughput  TX = %u.%03u kbps   RX = %u.%03u kbps   [TX=%u B  RX=%u B]",
			tx_bits / 1000U, tx_bits % 1000U, rx_bits / 1000U, rx_bits % 1000U,
			total_tx, total_rx);
	}
}

K_THREAD_DEFINE(app_report_tid, 1024, report_thread, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
