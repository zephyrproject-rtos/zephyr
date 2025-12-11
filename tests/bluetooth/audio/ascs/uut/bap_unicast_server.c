/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

#include "bap_unicast_server.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_bap_unicast_server_cb_config)                                                    \
	FAKE(mock_bap_unicast_server_cb_reconfig)                                                  \
	FAKE(mock_bap_unicast_server_cb_qos)                                                       \
	FAKE(mock_bap_unicast_server_cb_enable)                                                    \
	FAKE(mock_bap_unicast_server_cb_start)                                                     \
	FAKE(mock_bap_unicast_server_cb_metadata)                                                  \
	FAKE(mock_bap_unicast_server_cb_disable)                                                   \
	FAKE(mock_bap_unicast_server_cb_stop)                                                      \
	FAKE(mock_bap_unicast_server_cb_release)                                                   \

void mock_bap_unicast_server_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);
}

void mock_bap_unicast_server_cleanup(void)
{

}

DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_config, struct bt_conn *,
		       const struct bt_bap_ep *, enum bt_audio_dir,
		       const struct bt_audio_codec_cfg *, struct bt_bap_stream **,
		       struct bt_bap_qos_cfg_pref *const, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_reconfig, struct bt_bap_stream *,
		       enum bt_audio_dir, const struct bt_audio_codec_cfg *,
		       struct bt_bap_qos_cfg_pref *const, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_qos, struct bt_bap_stream *,
		       const struct bt_bap_qos_cfg *, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_enable, struct bt_bap_stream *,
		       const uint8_t *, size_t, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_start, struct bt_bap_stream *,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_metadata, struct bt_bap_stream *,
		       const uint8_t *, size_t, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_disable, struct bt_bap_stream *,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_stop, struct bt_bap_stream *,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_bap_unicast_server_cb_release, struct bt_bap_stream *,
		       struct bt_bap_ascs_rsp *);

const struct bt_bap_unicast_server_cb mock_bap_unicast_server_cb = {
	.config = mock_bap_unicast_server_cb_config,
	.reconfig = mock_bap_unicast_server_cb_reconfig,
	.qos = mock_bap_unicast_server_cb_qos,
	.enable = mock_bap_unicast_server_cb_enable,
	.start = mock_bap_unicast_server_cb_start,
	.metadata = mock_bap_unicast_server_cb_metadata,
	.disable = mock_bap_unicast_server_cb_disable,
	.stop = mock_bap_unicast_server_cb_stop,
	.release = mock_bap_unicast_server_cb_release,
};
