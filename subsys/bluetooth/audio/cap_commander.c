/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include "cap_internal.h"
#include "ccid_internal.h"
#include "csip_internal.h"
#include "bap_endpoint.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bt_cap_commander, CONFIG_BT_CAP_COMMANDER_LOG_LEVEL);

#include "common/bt_str.h"

static const struct bt_cap_commander_cb *cap_cb;

int bt_cap_commander_register_cb(const struct bt_cap_commander_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	CHECKIF(cap_cb != NULL) {
		LOG_DBG("callbacks already registered");
		return -EALREADY;
	}

	cap_cb = cb;

	return 0;
}

int bt_cap_commander_unregister_cb(const struct bt_cap_commander_cb *cb)
{
	CHECKIF(cb == NULL) {
		LOG_DBG("cb is NULL");
		return -EINVAL;
	}

	CHECKIF(cap_cb != cb) {
		LOG_DBG("cb is not registered");
		return -EINVAL;
	}

	cap_cb = NULL;

	return 0;
}

static void
cap_commander_discover_complete(struct bt_conn *conn, int err,
				const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (cap_cb && cap_cb->discovery_complete) {
		cap_cb->discovery_complete(conn, err, csis_inst);
	}
}

int bt_cap_commander_discover(struct bt_conn *conn)
{
	CHECKIF(conn == NULL) {
		LOG_DBG("NULL conn");
		return -EINVAL;
	}

	return bt_cap_common_discover(conn, cap_commander_discover_complete);
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
static void cap_commander_unicast_audio_proc_complete(void)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	enum bt_cap_common_proc_type proc_type;
	struct bt_conn *failed_conn;
	int err;

	failed_conn = active_proc->failed_conn;
	err = active_proc->err;
	proc_type = active_proc->proc_type;
	bt_cap_common_clear_active_proc();

	if (cap_cb == NULL) {
		return;
	}

	switch (proc_type) {
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	case BT_CAP_COMMON_PROC_TYPE_VOLUME_CHANGE:
		if (cap_cb->volume_changed != NULL) {
			cap_cb->volume_changed(failed_conn, err);
		}
		break;
#endif /* CONFIG_BT_VCP_VOL_CTLR */
	case BT_CAP_COMMON_PROC_TYPE_NONE:
	default:
		__ASSERT(false, "Invalid proc_type: %u", proc_type);
	}
}

int bt_cap_commander_cancel(void)
{
	if (!bt_cap_common_proc_is_active() && !bt_cap_common_proc_is_aborted()) {
		LOG_DBG("No CAP procedure is in progress");

		return -EALREADY;
	}

	bt_cap_common_abort_proc(NULL, -ECANCELED);
	cap_commander_unicast_audio_proc_complete();

	return 0;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR)
static bool valid_change_volume_param(const struct bt_cap_commander_change_volume_param *param)
{
	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->count == 0) {
		LOG_DBG("Invalid param->count: %u", param->count);
		return false;
	}

	CHECKIF(param->members == NULL) {
		LOG_DBG("param->members is NULL");
		return false;
	}

	CHECKIF(param->count > CONFIG_BT_MAX_CONN) {
		LOG_DBG("param->count (%zu) is larger than CONFIG_BT_MAX_CONN (%d)", param->count,
			CONFIG_BT_MAX_CONN);
		return false;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const union bt_cap_set_member *member = &param->members[i];
		struct bt_cap_common_client *client = NULL;

		if (param->type == BT_CAP_SET_TYPE_AD_HOC) {

			CHECKIF(member->member == NULL) {
				LOG_DBG("param->members[%zu].member is NULL", i);
				return false;
			}

			client = bt_cap_common_get_client_by_acl(member->member);
			if (client == NULL || !client->cas_found) {
				LOG_DBG("CAS was not found for param->members[%zu]", i);
				return false;
			}
		} else if (param->type == BT_CAP_SET_TYPE_CSIP) {
			CHECKIF(member->csip == NULL) {
				LOG_DBG("param->members[%zu].csip is NULL", i);
				return false;
			}

			client = bt_cap_common_get_client_by_csis(member->csip);
			if (client == NULL) {
				LOG_DBG("CSIS was not found for param->members[%zu]", i);
				return false;
			}
		}

		if (client == NULL || !client->cas_found) {
			LOG_DBG("CAS was not found for param->members[%zu]", i);
			return false;
		}

		if (bt_vcp_vol_ctlr_get_by_conn(client->conn) == NULL) {
			LOG_DBG("Volume control not available for param->members[%zu]", i);
			return false;
		}

		for (size_t j = 0U; j < i; j++) {
			const union bt_cap_set_member *other = &param->members[j];

			if (other == member) {
				LOG_DBG("param->members[%zu] (%p) is duplicated by "
					"param->members[%zu] (%p)",
					j, other, i, member);
				return false;
			}
		}
	}

	return true;
}

static void cap_commander_vcp_vol_set_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_conn *conn;
	int vcp_err;

	LOG_DBG("vol_ctlr %p", (void *)vol_ctlr);

	vcp_err = bt_vcp_vol_ctlr_conn_get(vol_ctlr, &conn);
	if (vcp_err != 0) {
		LOG_ERR("Failed to get conn by vol_ctrl: %d", vcp_err);
		return;
	}

	LOG_DBG("conn %p", (void *)conn);
	if (!bt_cap_common_conn_in_active_proc(conn)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (err != 0) {
		LOG_DBG("Failed to set volume: %d", err);
		bt_cap_common_abort_proc(conn, err);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Conn %p volume updated (%zu/%zu streams done)", (void *)conn,
			active_proc->proc_done_cnt, active_proc->proc_cnt);
	}

	if (bt_cap_common_proc_is_aborted()) {
		LOG_DBG("Proc is aborted");
		if (bt_cap_common_proc_all_handled()) {
			LOG_DBG("All handled");
			cap_commander_unicast_audio_proc_complete();
		}

		return;
	}

	if (!bt_cap_common_proc_is_done()) {
		const struct bt_cap_commander_proc_param *proc_param;

		proc_param = &active_proc->proc_param.commander[active_proc->proc_done_cnt];
		conn = proc_param->conn;
		err = bt_vcp_vol_ctlr_set_vol(bt_vcp_vol_ctlr_get_by_conn(conn),
					      proc_param->change_volume.volume);
		if (err != 0) {
			LOG_DBG("Failed to set volume for conn %p: %d", (void *)conn, err);
			bt_cap_common_abort_proc(conn, err);
			cap_commander_unicast_audio_proc_complete();
		} else {
			active_proc->proc_initiated_cnt++;
		}
	} else {
		cap_commander_unicast_audio_proc_complete();
	}
}

int bt_cap_commander_change_volume(const struct bt_cap_commander_change_volume_param *param)
{
	const struct bt_cap_commander_proc_param *proc_param;
	static struct bt_vcp_vol_ctlr_cb vol_ctlr_cb = {
		.vol_set = cap_commander_vcp_vol_set_cb,
	};
	struct bt_cap_common_proc *active_proc;
	static bool cb_registered;
	struct bt_conn *conn;
	int err;

	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_change_volume_param(param)) {
		return -EINVAL;
	}

	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_VOLUME_CHANGE, param->count);

	if (!cb_registered) {
		/* Ensure that ops are registered before any procedures are started */
		err = bt_vcp_vol_ctlr_cb_register(&vol_ctlr_cb);
		if (err != 0) {
			LOG_DBG("Failed to register VCP callbacks: %d", err);

			return -ENOEXEC;
		}

		cb_registered = true;
	}

	active_proc = bt_cap_common_get_active_proc();

	for (size_t i = 0U; i < param->count; i++) {
		struct bt_conn *member_conn =
			bt_cap_common_get_member_conn(param->type, &param->members[i]);

		if (member_conn == NULL) {
			LOG_DBG("Invalid param->members[%zu]", i);
			return -EINVAL;
		}

		/* Store the necessary parameters as we cannot assume that the supplied parameters
		 * are kept valid
		 */
		active_proc->proc_param.commander[i].conn = member_conn;
		active_proc->proc_param.commander[i].change_volume.volume = param->volume;
	}

	proc_param = &active_proc->proc_param.commander[0];
	conn = proc_param->conn;
	err = bt_vcp_vol_ctlr_set_vol(bt_vcp_vol_ctlr_get_by_conn(conn),
				      proc_param->change_volume.volume);
	if (err != 0) {
		LOG_DBG("Failed to set volume for conn %p: %d", (void *)conn, err);
		return -ENOEXEC;
	}

	active_proc->proc_initiated_cnt++;

	return 0;
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
#endif /* CONFIG_BT_VCP_VOL_CTLR */

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
