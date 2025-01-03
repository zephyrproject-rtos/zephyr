/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/net_buf.h>

void bap_stream_rx_recv_cb(struct bt_bap_stream *stream, const struct bt_iso_recv_info *info,
			   struct net_buf *buf);

/**
 * @brief Test if the provided stream has been configured for RX
 *
 * @param bap_stream The stream to test for RX support
 *
 * @returns true if it has been configured for RX, and false if not
 */
bool bap_stream_rx_can_recv(const struct bt_bap_stream *stream);
