/*  Bluetooth Audio Unicast Server */

/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/audio/audio.h>

#include "pacs_internal.h"
#include "endpoint.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_unicast_server, CONFIG_BT_AUDIO_UNICAST_SERVER_LOG_LEVEL);

const struct bt_audio_unicast_server_cb *unicast_server_cb;

int bt_audio_unicast_server_register_cb(const struct bt_audio_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
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

int bt_audio_unicast_server_unregister_cb(const struct bt_audio_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
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

int bt_unicast_server_reconfig(struct bt_audio_stream *stream,
			       const struct bt_codec *codec)
{
	struct bt_audio_ep *ep;
	int err;

	ep = stream->ep;

	if (unicast_server_cb != NULL &&
		unicast_server_cb->reconfig != NULL) {
		err = unicast_server_cb->reconfig(stream, ep->dir, codec,
						  &ep->qos_pref);
	} else {
		err = -ENOTSUP;
	}

	if (err != 0) {
		return err;
	}

	(void)memcpy(&ep->codec, &codec, sizeof(codec));

	ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_CODEC_CONFIGURED);

	return 0;
}

int bt_unicast_server_start(struct bt_audio_stream *stream)
{
	int err;

	if (unicast_server_cb != NULL && unicast_server_cb->start != NULL) {
		err = unicast_server_cb->start(stream);
	} else {
		err = -ENOTSUP;
	}

	if (err != 0) {
		return err;
	}

	ascs_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_STREAMING);

	return 0;
}

int bt_unicast_server_metadata(struct bt_audio_stream *stream,
			       struct bt_codec_data meta[],
			       size_t meta_count)
{
	struct bt_audio_ep *ep;
	int err;


	if (unicast_server_cb != NULL && unicast_server_cb->metadata != NULL) {
		err = unicast_server_cb->metadata(stream, meta, meta_count);
	} else {
		err = -ENOTSUP;
	}

	ep = stream->ep;
	for (size_t i = 0U; i < meta_count; i++) {
		(void)memcpy(&ep->codec.meta[i], &meta[i],
			     sizeof(ep->codec.meta[i]));
	}

	if (err) {
		return err;
	}

	/* Set the state to the same state to trigger the notifications */
	ascs_ep_set_state(ep, ep->status.state);

	return 0;
}

int bt_unicast_server_disable(struct bt_audio_stream *stream)
{
	struct bt_audio_ep *ep;
	int err;

	if (unicast_server_cb != NULL && unicast_server_cb->disable != NULL) {
		err = unicast_server_cb->disable(stream);
	} else {
		err = -ENOTSUP;
	}

	if (err != 0) {
		return err;
	}

	ep = stream->ep;

	/* The ASE state machine goes into different states from this operation
	 * based on whether it is a source or a sink ASE.
	 */
	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_DISABLING);
	} else {
		ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
	}

	return 0;
}

int bt_unicast_server_release(struct bt_audio_stream *stream)
{
	int err;

	if (unicast_server_cb != NULL && unicast_server_cb->release != NULL) {
		err = unicast_server_cb->release(stream);
	} else {
		err = -ENOTSUP;
	}

	if (err != 0) {
		return err;
	}

	/* ase_process will set the state to IDLE after sending the
	 * notification, finalizing the release
	 */
	ascs_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_RELEASING);

	return 0;
}
