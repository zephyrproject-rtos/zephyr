/*  Bluetooth Audio Unicast Server */

/*
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>

#include <zephyr/bluetooth/audio/audio.h>

#include "pacs_internal.h"

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

#if defined(CONFIG_BT_PAC_SNK_LOC) || defined(CONFIG_BT_PAC_SRC_LOC)
int bt_audio_unicast_server_location_changed(enum bt_audio_dir dir)
{
	return bt_pacs_location_changed(dir);
}
#endif /* CONFIG_BT_PAC_SNK_LOC || CONFIG_BT_PAC_SRC_LOC */

int bt_audio_unicast_server_available_contexts_changed(void)
{
	return bt_pacs_available_contexts_changed();
}
