/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _NRF5340_AUDIO_COMMON_H_
#define _NRF5340_AUDIO_COMMON_H_

#include <zephyr/bluetooth/audio/audio.h>

#define ZBUS_READ_TIMEOUT_MS	K_MSEC(100)
#define ZBUS_ADD_OBS_TIMEOUT_MS K_MSEC(200)

/***** Messages for zbus ******/

enum button_action {
	BUTTON_PRESS = 1,
};

struct button_msg {
	uint32_t button_pin;
	enum button_action button_action;
};

enum le_audio_evt_type {
	LE_AUDIO_EVT_CONFIG_RECEIVED = 1,
	LE_AUDIO_EVT_PRES_DELAY_SET,
	LE_AUDIO_EVT_STREAMING,
	LE_AUDIO_EVT_NOT_STREAMING,
	LE_AUDIO_EVT_SYNC_LOST,
	LE_AUDIO_EVT_NO_VALID_CFG,
};

struct le_audio_msg {
	enum le_audio_evt_type event;
	struct bt_conn *conn;
	struct bt_le_per_adv_sync *pa_sync;
	enum bt_audio_dir dir;
};

/**
 * tx_sync_ts_us	The timestamp from get_tx_sync.
 * curr_ts_us		The current time. This must be in the controller frame of reference.
 */
struct sdu_ref_msg {
	uint32_t tx_sync_ts_us;
	uint32_t curr_ts_us;
	bool adjust;
};

enum bt_mgmt_evt_type {
	BT_MGMT_EXT_ADV_WITH_PA_READY = 1,
	BT_MGMT_CONNECTED,
	BT_MGMT_SECURITY_CHANGED,
	BT_MGMT_PA_SYNCED,
	BT_MGMT_PA_SYNC_LOST,
	BT_MGMT_DISCONNECTED,
};

struct bt_mgmt_msg {
	enum bt_mgmt_evt_type event;
	struct bt_conn *conn;
	struct bt_le_ext_adv *ext_adv;
	struct bt_le_per_adv_sync *pa_sync;
	uint32_t broadcast_id;
	uint8_t pa_sync_term_reason;
};

enum volume_evt_type {
	VOLUME_UP = 1,
	VOLUME_DOWN,
	VOLUME_SET,
	VOLUME_MUTE,
	VOLUME_UNMUTE,
};

struct volume_msg {
	enum volume_evt_type event;
	uint8_t volume;
};

enum content_control_evt_type {
	MEDIA_START = 1,
	MEDIA_STOP,
};

struct content_control_msg {
	enum content_control_evt_type event;
};

/**
 * @brief	Initialize the software modules that are common for all the audio samples.
 *
 * @return	0 if successful, error otherwise.
 */
int nrf5340_audio_common_init(void);

#endif /* _NRF5340_AUDIO_COMMON_H_ */
