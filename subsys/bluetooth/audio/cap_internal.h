/*  Bluetooth Audio Common Audio Profile internal header */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/cap.h>

bool bt_cap_acceptor_ccid_exist(const struct bt_conn *conn, uint8_t ccid);

void bt_cap_initiator_codec_configured(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_qos_configured(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_enabled(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_started(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_metadata_updated(struct bt_cap_stream *cap_stream);
void bt_cap_initiator_released(struct bt_cap_stream *cap_stream);
void bt_cap_stream_ops_register_bap(struct bt_cap_stream *cap_stream);
