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

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_UNICAST_SERVER)
#define LOG_MODULE_NAME bt_unicast_server
#include "common/log.h"

const struct bt_audio_unicast_server_cb *unicast_server_cb;

int bt_audio_unicast_server_register_cb(const struct bt_audio_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
		BT_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != NULL) {
		BT_DBG("callback structure already registered");
		return -EALREADY;
	}

	unicast_server_cb = cb;

	return 0;
}

int bt_audio_unicast_server_unregister_cb(const struct bt_audio_unicast_server_cb *cb)
{
	CHECKIF(cb == NULL) {
		BT_DBG("cb is NULL");
		return -EINVAL;
	}

	if (unicast_server_cb != cb) {
		BT_DBG("callback structure not registered");
		return -EINVAL;
	}

	unicast_server_cb = NULL;

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

int bt_unicast_server_release(struct bt_audio_stream *stream, bool cache)
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

	if (cache) {
		ascs_ep_set_state(stream->ep,
				  BT_AUDIO_EP_STATE_CODEC_CONFIGURED);
	} else {
		/* ase_process will set the state to IDLE after sending the
		 * notification, finalizing the release
		 */
		ascs_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_RELEASING);
	}

	return 0;
}
