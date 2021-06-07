/*
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_
#include <zephyr/types.h>
#include <bluetooth/gatt.h>

int bt_mics_client_included_get(struct bt_conn *conn,
				struct bt_mics_included  *included);
int bt_mics_client_mute_get(struct bt_conn *conn);
int bt_mics_client_mute(struct bt_conn *conn);
int bt_mics_client_unmute(struct bt_conn *conn);
bool bt_mics_client_valid_aics_inst(struct bt_conn *conn, struct bt_aics *aics);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MICS_INTERNAL_ */
