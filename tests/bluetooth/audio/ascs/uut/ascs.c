/*
 * Copyright (c) 2023 Codecoup
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/ascs.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/fff.h>

#include "ascs.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_ascs_cb_config)                                                                  \
	FAKE(mock_ascs_cb_reconfig)                                                                \
	FAKE(mock_ascs_cb_qos)                                                                     \
	FAKE(mock_ascs_cb_enable)                                                                  \
	FAKE(mock_ascs_cb_start)                                                                   \
	FAKE(mock_ascs_cb_metadata)                                                                \
	FAKE(mock_ascs_cb_disable)                                                                 \
	FAKE(mock_ascs_cb_stop)                                                                    \
	FAKE(mock_ascs_cb_release)

DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_config, struct bt_conn *, const struct bt_bap_ep *,
		       enum bt_audio_dir, const struct bt_audio_codec_cfg *,
		       struct bt_bap_stream **, struct bt_bap_qos_cfg_pref *const,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_reconfig, struct bt_bap_stream *, enum bt_audio_dir,
		       const struct bt_audio_codec_cfg *, struct bt_bap_qos_cfg_pref *const,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_qos, struct bt_bap_stream *, const struct bt_bap_qos_cfg *,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_enable, struct bt_bap_stream *, const uint8_t *, size_t,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_start, struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_metadata, struct bt_bap_stream *, const uint8_t *, size_t,
		       struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_disable, struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_stop, struct bt_bap_stream *, struct bt_bap_ascs_rsp *);
DEFINE_FAKE_VALUE_FUNC(int, mock_ascs_cb_release, struct bt_bap_stream *, struct bt_bap_ascs_rsp *);

struct bt_ascs_cb mock_ascs_cb;

void mock_ascs_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);

	mock_ascs_cb.config = mock_ascs_cb_config;
	mock_ascs_cb.reconfig = mock_ascs_cb_reconfig;
	mock_ascs_cb.qos = mock_ascs_cb_qos;
	mock_ascs_cb.enable = mock_ascs_cb_enable;
	mock_ascs_cb.start = mock_ascs_cb_start;
	mock_ascs_cb.metadata = mock_ascs_cb_metadata;
	mock_ascs_cb.disable = mock_ascs_cb_disable;
	mock_ascs_cb.stop = mock_ascs_cb_stop;
	mock_ascs_cb.release = mock_ascs_cb_release;
}

void mock_ascs_cleanup(void)
{
	/* No op */
}
