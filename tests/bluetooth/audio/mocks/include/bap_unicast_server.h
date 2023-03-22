/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BAP_UNICAST_SERVER_H_
#define MOCKS_BAP_UNICAST_SERVER_H_

#include <zephyr/fff.h>
#include <zephyr/bluetooth/audio/bap.h>

extern const struct bt_bap_unicast_server_cb mock_bap_unicast_server_cb;

void mock_bap_unicast_server_init(void);
void mock_bap_unicast_server_cleanup(void);

DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_config, struct bt_conn *,
			const struct bt_bap_ep *, enum bt_audio_dir, const struct bt_codec *,
			struct bt_bap_stream **, struct bt_codec_qos_pref *const,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_reconfig, struct bt_bap_stream *,
			enum bt_audio_dir, const struct bt_codec *,
			struct bt_codec_qos_pref *const, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_qos, struct bt_bap_stream *,
			const struct bt_codec_qos *, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_enable, struct bt_bap_stream *,
			const struct bt_codec_data *, size_t, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_start, struct bt_bap_stream *,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_metadata, struct bt_bap_stream *,
			const struct bt_codec_data *, size_t, struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_disable, struct bt_bap_stream *,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_stop, struct bt_bap_stream *,
			struct bt_bap_ascs_rsp *);
DECLARE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_release, struct bt_bap_stream *,
			struct bt_bap_ascs_rsp *);

#endif /* MOCKS_BAP_UNICAST_SERVER_H_ */
