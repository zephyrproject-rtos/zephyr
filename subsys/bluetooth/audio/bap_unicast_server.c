/*  Bluetooth Audio Unicast Server */

/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>

#include "bap_iso.h"
#include "pacs_internal.h"
#include "bap_endpoint.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_bap_unicast_server, CONFIG_BT_BAP_UNICAST_SERVER_LOG_LEVEL);

static const struct bt_bap_unicast_server_cb *unicast_server_cb;

int bt_bap_unicast_server_register_cb(const struct bt_bap_unicast_server_cb *cb)
{
	int err;

	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != NULL) {
		LOG_DBG("callback structure already registered");
		return -EALREADY;
	}

	err = bt_ascs_init(cb);
	if (err != 0) {
		return err;
	}

	unicast_server_cb = cb;

	return 0;
}

int bt_bap_unicast_server_unregister_cb(const struct bt_bap_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != cb) {
		LOG_DBG("callback structure not registered");
		return -EINVAL;
	}

	bt_ascs_cleanup();
	unicast_server_cb = NULL;

	return 0;
}

int bt_bap_unicast_server_reconfig(struct bt_bap_stream *stream, const struct bt_codec *codec)
{
	return bt_ascs_ase_reconfig(stream, codec);
}

int bt_bap_unicast_server_start(struct bt_bap_stream *stream)
{
	return bt_ascs_ase_start(stream);
}

int bt_bap_unicast_server_metadata(struct bt_bap_stream *stream, struct bt_codec_data meta[],
				   size_t meta_count)
{
	return bt_ascs_ase_metadata(stream, meta, meta_count);
}

int bt_bap_unicast_server_disable(struct bt_bap_stream *stream)
{
	return bt_ascs_ase_disable(stream);
}

int bt_bap_unicast_server_release(struct bt_bap_stream *stream)
{
	return bt_ascs_ase_release(stream);
}

int bt_bap_unicast_server_config_ase(struct bt_conn *conn, struct bt_bap_stream *stream,
				     struct bt_codec *codec,
				     const struct bt_codec_qos_pref *qos_pref)
{
	return bt_ascs_ase_config(conn, stream, codec, qos_pref);
}

void bt_bap_unicast_server_foreach_ep(struct bt_conn *conn, bt_bap_ep_func_t func, void *user_data)
{
	bt_ascs_foreach_ep(conn, func, user_data);
}
