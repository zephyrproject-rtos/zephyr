/* pbap_internal.h - Internal for Phone Book Access Protocol handling */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/** @brief PBAP connection state enumeration */
enum __packed bt_pbap_state {
	/** PBAP disconnected */
	BT_PBAP_STATE_DISCONNECTED = 0,
	/** PBAP disconnecting */
	BT_PBAP_STATE_DISCONNECTING = 1,
	/** PBAP ready for upper layer traffic on it */
	BT_PBAP_STATE_CONNECTED = 2,
	/** PBAP in connecting state */
	BT_PBAP_STATE_CONNECTING = 3,
};

/** @brief Transport layer state enumeration */
enum __packed bt_pbap_transport_state {
	/** @brief Transport is disconnected */
	BT_PBAP_TRANSPORT_STATE_DISCONNECTED = 0,
	/** @brief Transport is connecting */
	BT_PBAP_TRANSPORT_STATE_CONNECTING = 1,
	/** @brief Transport is connected */
	BT_PBAP_TRANSPORT_STATE_CONNECTED = 2,
	/** @brief Transport is disconnecting */
	BT_PBAP_TRANSPORT_STATE_DISCONNECTING = 3,
};
