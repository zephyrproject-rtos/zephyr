/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/audio/bap.h>

#include "bap_stream.h"

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                                       \
	FAKE(mock_bap_stream_configured_cb)                                                        \
	FAKE(mock_bap_stream_qos_set_cb)                                                           \
	FAKE(mock_bap_stream_enabled_cb)                                                           \
	FAKE(mock_bap_stream_metadata_updated_cb)                                                  \
	FAKE(mock_bap_stream_disabled_cb)                                                          \
	FAKE(mock_bap_stream_released_cb)                                                          \
	FAKE(mock_bap_stream_started_cb)                                                           \
	FAKE(mock_bap_stream_stopped_cb)                                                           \
	FAKE(mock_bap_stream_recv_cb)                                                              \
	FAKE(mock_bap_stream_sent_cb)                                                              \

struct bt_bap_stream_ops mock_bap_stream_ops;

DEFINE_FAKE_VOID_FUNC(mock_bap_stream_configured_cb, struct bt_bap_stream *,
		      const struct bt_codec_qos_pref *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_qos_set_cb, struct bt_bap_stream *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_enabled_cb, struct bt_bap_stream *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_metadata_updated_cb, struct bt_bap_stream *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_disabled_cb, struct bt_bap_stream *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_released_cb, struct bt_bap_stream *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_started_cb, struct bt_bap_stream *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_stopped_cb, struct bt_bap_stream *, uint8_t);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_recv_cb, struct bt_bap_stream *,
		      const struct bt_iso_recv_info *, struct net_buf *);
DEFINE_FAKE_VOID_FUNC(mock_bap_stream_sent_cb, struct bt_bap_stream *);

void mock_bap_stream_init(void)
{
	FFF_FAKES_LIST(RESET_FAKE);

	mock_bap_stream_ops.configured = mock_bap_stream_configured_cb;
	mock_bap_stream_ops.qos_set = mock_bap_stream_qos_set_cb;
	mock_bap_stream_ops.enabled = mock_bap_stream_enabled_cb;
	mock_bap_stream_ops.metadata_updated = mock_bap_stream_metadata_updated_cb;
	mock_bap_stream_ops.disabled = mock_bap_stream_disabled_cb;
	mock_bap_stream_ops.released = mock_bap_stream_released_cb;
	mock_bap_stream_ops.started = mock_bap_stream_started_cb;
	mock_bap_stream_ops.stopped = mock_bap_stream_stopped_cb;
#if defined(CONFIG_BT_AUDIO_RX)
	mock_bap_stream_ops.recv = mock_bap_stream_recv_cb;
#endif /* CONFIG_BT_AUDIO_RX */
#if defined(CONFIG_BT_AUDIO_TX)
	mock_bap_stream_ops.sent = mock_bap_stream_sent_cb;
#endif /* CONFIG_BT_AUDIO_TX */
}

void mock_bap_stream_cleanup(void)
{

}
