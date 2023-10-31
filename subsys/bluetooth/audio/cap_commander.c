/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include "cap_internal.h"
#include "ccid_internal.h"
#include "csip_internal.h"
#include "bap_endpoint.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_cap_commander, CONFIG_BT_CAP_COMMANDER_LOG_LEVEL);

#include "common/bt_str.h"

int bt_cap_commander_unicast_discover(struct bt_conn *conn)
{
	return -ENOSYS;
}

int bt_cap_commander_broadcast_reception_start(
	const struct bt_cap_commander_broadcast_reception_start_param *param)
{
	return -ENOSYS;
}

int bt_cap_commander_broadcast_reception_stop(
	const struct bt_cap_commander_broadcast_reception_stop_param *param)
{
	return -ENOSYS;
}

int bt_cap_commander_change_volume(const struct bt_cap_commander_change_volume_param *param)
{
	return -ENOSYS;
}

int bt_cap_commander_change_volume_offset(
	const struct bt_cap_commander_change_volume_offset_param *param)
{
	return -ENOSYS;
}

int bt_cap_commander_change_volume_mute_state(
	const struct bt_cap_commander_change_volume_mute_state_param *param)
{
	return -ENOSYS;
}

int bt_cap_commander_change_microphone_mute_state(
	const struct bt_cap_commander_change_microphone_mute_state_param *param)
{
	return -ENOSYS;
}

int bt_cap_commander_change_microphone_gain_setting(
	const struct bt_cap_commander_change_microphone_gain_setting_param *param)
{

	return -ENOSYS;
}
