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
#include <zephyr/ztest_assert.h>

bool bt_bap_ep_is_unicast_client(const struct bt_bap_ep *ep)
{
	return false;
}

int bt_bap_unicast_client_config(struct bt_bap_stream *stream,
				 const struct bt_audio_codec_cfg *codec_cfg)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_qos(struct bt_conn *conn, struct bt_bap_unicast_group *group)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_enable(struct bt_bap_stream *stream, uint8_t meta[],
				 size_t meta_len)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_metadata(struct bt_bap_stream *stream, uint8_t meta[],
				   size_t meta_len)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_disable(struct bt_bap_stream *stream)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_connect(struct bt_bap_stream *stream)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_start(struct bt_bap_stream *stream)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_stop(struct bt_bap_stream *stream)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}

int bt_bap_unicast_client_release(struct bt_bap_stream *stream)
{
	zassert_unreachable("Unexpected call to '%s()' occurred", __func__);
	return 0;
}
