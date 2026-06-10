/*  Bluetooth Audio Unicast Server */

/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/ascs.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/logging/log.h>

#include "ascs_internal.h"
#include "bap_iso.h"
#include "bap_endpoint.h"
#include "pacs_internal.h"

LOG_MODULE_REGISTER(bt_bap_unicast_server, CONFIG_BT_BAP_UNICAST_SERVER_LOG_LEVEL);

static const struct bt_bap_unicast_server_cb *unicast_server_cb;

static int ascs_config_cb(struct bt_conn *conn, const struct bt_bap_ep *ep, enum bt_audio_dir dir,
			  const struct bt_audio_codec_cfg *codec_cfg, struct bt_bap_stream **stream,
			  struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->config != NULL) {
		return unicast_server_cb->config(conn, ep, dir, codec_cfg, stream, pref, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_reconfig_cb(struct bt_bap_stream *stream, enum bt_audio_dir dir,
			    const struct bt_audio_codec_cfg *codec_cfg,
			    struct bt_bap_qos_cfg_pref *const pref, struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->reconfig != NULL) {
		return unicast_server_cb->reconfig(stream, dir, codec_cfg, pref, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_qos_cb(struct bt_bap_stream *stream, const struct bt_bap_qos_cfg *qos,
		       struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->qos != NULL) {
		return unicast_server_cb->qos(stream, qos, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_enable_cb(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
			  struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->enable != NULL) {
		return unicast_server_cb->enable(stream, meta, meta_len, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_start_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->start != NULL) {
		return unicast_server_cb->start(stream, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_metadata_cb(struct bt_bap_stream *stream, const uint8_t meta[], size_t meta_len,
			    struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->metadata != NULL) {
		return unicast_server_cb->metadata(stream, meta, meta_len, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_disable_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->disable != NULL) {
		return unicast_server_cb->disable(stream, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_stop_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->stop != NULL) {
		return unicast_server_cb->stop(stream, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

static int ascs_release_cb(struct bt_bap_stream *stream, struct bt_bap_ascs_rsp *rsp)
{
	if (unicast_server_cb != NULL && unicast_server_cb->release != NULL) {
		return unicast_server_cb->release(stream, rsp);
	} else {
		return -EOPNOTSUPP;
	}
}

int bt_bap_unicast_server_register(const struct bt_bap_unicast_server_register_param *param)
{
	static struct bt_ascs_cb ascs_cb = {
		.config = ascs_config_cb,
		.reconfig = ascs_reconfig_cb,
		.qos = ascs_qos_cb,
		.enable = ascs_enable_cb,
		.start = ascs_start_cb,
		.metadata = ascs_metadata_cb,
		.disable = ascs_disable_cb,
		.stop = ascs_stop_cb,
		.release = ascs_release_cb,
	};

	if (param == NULL) {
		LOG_DBG("param is NULL");
		return -EINVAL;
	}

	struct bt_ascs_register_param ascs_param = {
		.snk_cnt = param->snk_cnt,
		.src_cnt = param->src_cnt,
		.cb = &ascs_cb,
	};

	return bt_ascs_register(&ascs_param);
}

int bt_bap_unicast_server_unregister(void)
{
	if (unicast_server_cb != NULL) {
		LOG_DBG("Callbacks are still registered");
		return -EAGAIN;
	}

	return bt_ascs_unregister();
}

int bt_bap_unicast_server_register_cb(const struct bt_bap_unicast_server_cb *cb)
{
	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != NULL) {
		LOG_DBG("callback structure already registered");
		return -EALREADY;
	}

	unicast_server_cb = cb;

	return 0;
}

int bt_bap_unicast_server_unregister_cb(const struct bt_bap_unicast_server_cb *cb)
{
	if (unicast_server_cb == NULL) {
		LOG_DBG("no callback is registered");
		return -EALREADY;
	}

	if (cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != cb) {
		LOG_DBG("callback structure not registered");
		return -EINVAL;
	}

	unicast_server_cb = NULL;

	return 0;
}

int bt_bap_unicast_server_reconfig(struct bt_bap_stream *stream,
				   const struct bt_audio_codec_cfg *codec_cfg)
{
	return bt_ascs_reconfig_ase(stream->ep, codec_cfg);
}

int bt_bap_unicast_server_start(struct bt_bap_stream *stream)
{
	return bt_ascs_start_ase(stream->ep);
}

int bt_bap_unicast_server_metadata(struct bt_bap_stream *stream, const uint8_t meta[],
				   size_t meta_len)
{
	return bt_ascs_metadata_ase(stream->ep, meta, meta_len);
}

int bt_bap_unicast_server_disable(struct bt_bap_stream *stream)
{
	return bt_ascs_disable_ase(stream->ep);
}

int bt_bap_unicast_server_release(struct bt_bap_stream *stream)
{
	return bt_ascs_release_ase(stream->ep);
}

int bt_bap_unicast_server_config_ase(struct bt_conn *conn, struct bt_bap_stream *stream,
				     const struct bt_audio_codec_cfg *codec_cfg,
				     const struct bt_bap_qos_cfg_pref *qos_pref)
{
	return bt_ascs_config_ase(conn, stream, codec_cfg, qos_pref);
}

int bt_bap_unicast_server_foreach_ep(struct bt_conn *conn, bt_bap_ep_func_t func, void *user_data)
{
	return bt_ascs_foreach_ep(conn, func, user_data);
}

bool bt_bap_unicast_server_has_ep(const struct bt_bap_ep *ep)
{
	return bt_ascs_has_ep(ep);
}

struct bt_conn *bt_bap_unicast_server_ep_get_conn(const struct bt_bap_ep *ep)
{
	return bt_ascs_ep_get_conn(ep);
}
