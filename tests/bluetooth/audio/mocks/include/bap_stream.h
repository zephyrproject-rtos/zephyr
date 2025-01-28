/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_STREAM_H_
#define MOCKS_BAP_STREAM_H_
#include <stdint.h>

#include <zephyr/bluetooth/iso.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/net_buf.h>

extern struct bt_bap_stream_ops mock_bap_stream_ops;

void mock_bap_stream_init(void);
void mock_bap_stream_cleanup(void);

DECLARE_FAKE_VOID_FUNC(mock_bap_stream_configured_cb, struct bt_bap_stream *,
		       const struct bt_bap_qos_cfg_pref *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_qos_set_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_enabled_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_metadata_updated_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_disabled_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_released_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_started_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_stopped_cb, struct bt_bap_stream *, uint8_t);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_recv_cb, struct bt_bap_stream *,
		       const struct bt_iso_recv_info *, struct net_buf *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_sent_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_connected_cb, struct bt_bap_stream *);
DECLARE_FAKE_VOID_FUNC(mock_bap_stream_disconnected_cb, struct bt_bap_stream *, uint8_t);

#endif /* MOCKS_BAP_STREAM_H_ */
