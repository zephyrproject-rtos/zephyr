/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_ASCS_H_
#define MOCKS_ASCS_H_
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/ascs.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/bap.h>

extern struct bt_ascs_cb mock_ascs_cb;

void mock_ascs_init(void);
void mock_ascs_cleanup(void);

DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_config, struct bt_conn *, const struct bt_bap_ep *,
			enum bt_audio_dir, const struct bt_audio_codec_cfg *,
			struct bt_bap_stream **, struct bt_bap_qos_cfg_pref *const,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_reconfig, struct bt_bap_stream *, enum bt_audio_dir,
			const struct bt_audio_codec_cfg *, struct bt_bap_qos_cfg_pref *const,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_qos, struct bt_bap_stream *,
			const struct bt_bap_qos_cfg *, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_enable, struct bt_bap_stream *, const uint8_t *, size_t,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_start, struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_metadata, struct bt_bap_stream *, const uint8_t *, size_t,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_disable, struct bt_bap_stream *,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_stop, struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_ascs_cb_release, struct bt_bap_stream *,
			struct bt_bap_ascs_rsp *);

#endif /* MOCKS_ASCS_H_ */
