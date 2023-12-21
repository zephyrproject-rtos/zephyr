/*
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/check.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/bluetooth/audio/vocs.h>
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
	case BT_CAP_COMMON_PROC_TYPE_VOLUME_MUTE_CHANGE:
		if (cap_cb->volume_changed != NULL) {
			cap_cb->volume_mute_changed(failed_conn, err);
		}
		break;
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	case BT_CAP_COMMON_PROC_TYPE_VOLUME_OFFSET_CHANGE:
		if (cap_cb->volume_offset_changed != NULL) {
			cap_cb->volume_offset_changed(failed_conn, err);
		}
		break;
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	case BT_CAP_COMMON_PROC_TYPE_MICROPHONE_GAIN_CHANGE:
		if (cap_cb->microphone_gain_changed != NULL) {
			cap_cb->microphone_gain_changed(failed_conn, err);
		}
		break;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
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
static struct bt_vcp_vol_ctlr_cb vol_ctlr_cb;
static bool vcp_cb_registered;

static int cap_commander_register_vcp_cb(void)
{
	int err;

	err = bt_vcp_vol_ctlr_cb_register(&vol_ctlr_cb);
	if (err != 0) {
		LOG_DBG("Failed to register VCP callbacks: %d", err);

		return -ENOEXEC;
	}

	vcp_cb_registered = true;

	return 0;
}

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
		const struct bt_cap_common_client *client =
			bt_cap_common_get_client(param->type, member);

		if (client == NULL) {
			LOG_DBG("Invalid param->members[%zu]", i);
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
		active_proc->proc_initiated_cnt++;
		err = bt_vcp_vol_ctlr_set_vol(bt_vcp_vol_ctlr_get_by_conn(conn),
					      proc_param->change_volume.volume);
		if (err != 0) {
			LOG_DBG("Failed to set volume for conn %p: %d", (void *)conn, err);
			bt_cap_common_abort_proc(conn, err);
			cap_commander_unicast_audio_proc_complete();
		}
	} else {
		cap_commander_unicast_audio_proc_complete();
	}
}

int bt_cap_commander_change_volume(const struct bt_cap_commander_change_volume_param *param)
{
	const struct bt_cap_commander_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
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

	vol_ctlr_cb.vol_set = cap_commander_vcp_vol_set_cb;
	if (!vcp_cb_registered && cap_commander_register_vcp_cb() != 0) {
		LOG_DBG("Failed to register VCP callbacks");

		return -ENOEXEC;
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
	active_proc->proc_initiated_cnt++;
	err = bt_vcp_vol_ctlr_set_vol(bt_vcp_vol_ctlr_get_by_conn(conn),
				      proc_param->change_volume.volume);
	if (err != 0) {
		LOG_DBG("Failed to set volume for conn %p: %d", (void *)conn, err);
		return -ENOEXEC;
	}

	return 0;
}

static bool valid_change_volume_mute_state_param(
	const struct bt_cap_commander_change_volume_mute_state_param *param)
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
		const struct bt_cap_common_client *client =
			bt_cap_common_get_client(param->type, member);

		CHECKIF(client == NULL) {
			LOG_DBG("Invalid param->members[%zu]", i);
			return false;
		}

		CHECKIF(bt_vcp_vol_ctlr_get_by_conn(client->conn) == NULL) {
			LOG_DBG("Volume control not available for param->members[%zu]", i);
			return false;
		}

		for (size_t j = 0U; j < i; j++) {
			const union bt_cap_set_member *other = &param->members[j];

			CHECKIF(other == member) {
				LOG_DBG("param->members[%zu] (%p) is duplicated by "
					"param->members[%zu] (%p)",
					j, other, i, member);
				return false;
			}
		}
	}

	return true;
}

static void cap_commander_vcp_vol_mute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
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
		active_proc->proc_initiated_cnt++;
		if (proc_param->change_mute.mute) {
			err = bt_vcp_vol_ctlr_mute(bt_vcp_vol_ctlr_get_by_conn(conn));
		} else {
			err = bt_vcp_vol_ctlr_unmute(bt_vcp_vol_ctlr_get_by_conn(conn));
		}

		if (err != 0) {
			LOG_DBG("Failed to set volume for conn %p: %d", (void *)conn, err);
			bt_cap_common_abort_proc(conn, err);
			cap_commander_unicast_audio_proc_complete();
		}
	} else {
		cap_commander_unicast_audio_proc_complete();
	}
}

int bt_cap_commander_change_volume_mute_state(
	const struct bt_cap_commander_change_volume_mute_state_param *param)
{
	const struct bt_cap_commander_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
	struct bt_conn *conn;
	int err;

	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_change_volume_mute_state_param(param)) {
		return -EINVAL;
	}

	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_VOLUME_MUTE_CHANGE, param->count);

	vol_ctlr_cb.mute = cap_commander_vcp_vol_mute_cb;
	vol_ctlr_cb.unmute = cap_commander_vcp_vol_mute_cb;
	if (!vcp_cb_registered && cap_commander_register_vcp_cb() != 0) {
		LOG_DBG("Failed to register VCP callbacks");

		return -ENOEXEC;
	}

	active_proc = bt_cap_common_get_active_proc();

	for (size_t i = 0U; i < param->count; i++) {
		struct bt_conn *member_conn =
			bt_cap_common_get_member_conn(param->type, &param->members[i]);

		CHECKIF(member_conn == NULL) {
			LOG_DBG("Invalid param->members[%zu]", i);
			return -EINVAL;
		}

		/* Store the necessary parameters as we cannot assume that the supplied parameters
		 * are kept valid
		 */
		active_proc->proc_param.commander[i].conn = member_conn;
		active_proc->proc_param.commander[i].change_mute.mute = param->mute;
	}

	proc_param = &active_proc->proc_param.commander[0];
	conn = proc_param->conn;
	active_proc->proc_initiated_cnt++;

	if (proc_param->change_mute.mute) {
		err = bt_vcp_vol_ctlr_mute(bt_vcp_vol_ctlr_get_by_conn(conn));
	} else {
		err = bt_vcp_vol_ctlr_unmute(bt_vcp_vol_ctlr_get_by_conn(conn));
	}

	if (err != 0) {
		LOG_DBG("Failed to set volume mute state for conn %p: %d", (void *)conn, err);
		return -ENOEXEC;
	}

	return 0;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
static bool
valid_change_offset_param(const struct bt_cap_commander_change_volume_offset_param *param)
{
	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->count == 0) {
		LOG_DBG("Invalid param->count: %u", param->count);
		return false;
	}

	CHECKIF(param->param == NULL) {
		LOG_DBG("param->param is NULL");
		return false;
	}

	CHECKIF(param->count > CONFIG_BT_MAX_CONN) {
		LOG_DBG("param->count (%zu) is larger than CONFIG_BT_MAX_CONN (%d)", param->count,
			CONFIG_BT_MAX_CONN);
		return false;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const struct bt_cap_commander_change_volume_offset_member_param *member_param =
			&param->param[i];
		const union bt_cap_set_member *member = &member_param->member;
		const struct bt_cap_common_client *client =
			bt_cap_common_get_client(param->type, member);
		struct bt_vcp_vol_ctlr *vol_ctlr;
		struct bt_vcp_included included;
		int err;

		if (client == NULL) {
			LOG_DBG("Invalid param->param[%zu].member", i);
			return false;
		}

		vol_ctlr = bt_vcp_vol_ctlr_get_by_conn(client->conn);
		if (vol_ctlr == NULL) {
			LOG_DBG("Volume control not available for param->param[%zu].member", i);
			return false;
		}

		err = bt_vcp_vol_ctlr_included_get(vol_ctlr, &included);
		if (err != 0 || included.vocs_cnt == 0) {
			LOG_DBG("Volume offset control not available for param->param[%zu].member",
				i);
			return -ENOEXEC;
		}

		if (!IN_RANGE(member_param->offset, BT_VOCS_MIN_OFFSET, BT_VOCS_MAX_OFFSET)) {
			LOG_DBG("Invalid offset %d for param->param[%zu].offset",
				member_param->offset, i);
			return false;
		}

		for (size_t j = 0U; j < i; j++) {
			const union bt_cap_set_member *other = &param->param[j].member;

			if (other == member) {
				LOG_DBG("param->param[%zu].member (%p) is duplicated by "
					"param->param[%zu].member (%p)",
					j, other, i, member);
				return false;
			}
		}
	}

	return true;
}

static void cap_commander_vcp_set_offset_cb(struct bt_vocs *inst, int err)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_conn *conn;
	int vocs_err;

	LOG_DBG("bt_vocs %p", (void *)inst);

	vocs_err = bt_vocs_client_conn_get(inst, &conn);
	if (vocs_err != 0) {
		LOG_ERR("Failed to get conn by inst: %d", vocs_err);
		return;
	}

	LOG_DBG("conn %p", (void *)conn);
	if (!bt_cap_common_conn_in_active_proc(conn)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (err != 0) {
		LOG_DBG("Failed to set offset: %d", err);
		bt_cap_common_abort_proc(conn, err);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Conn %p offset updated (%zu/%zu streams done)", (void *)conn,
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
		active_proc->proc_initiated_cnt++;

		err = bt_vocs_state_set(proc_param->change_offset.vocs,
					proc_param->change_offset.offset);
		if (err != 0) {
			LOG_DBG("Failed to set offset for conn %p: %d", (void *)conn, err);
			bt_cap_common_abort_proc(conn, err);
			cap_commander_unicast_audio_proc_complete();
		}
	} else {
		cap_commander_unicast_audio_proc_complete();
	}
}

int bt_cap_commander_change_volume_offset(
	const struct bt_cap_commander_change_volume_offset_param *param)
{
	const struct bt_cap_commander_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
	struct bt_vcp_vol_ctlr *vol_ctlr;
	struct bt_conn *conn;
	int err;

	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_change_offset_param(param)) {
		return -EINVAL;
	}

	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_VOLUME_OFFSET_CHANGE, param->count);

	vol_ctlr_cb.vocs_cb.set_offset = cap_commander_vcp_set_offset_cb;
	if (!vcp_cb_registered && cap_commander_register_vcp_cb() != 0) {
		LOG_DBG("Failed to register VCP callbacks");

		return -ENOEXEC;
	}

	active_proc = bt_cap_common_get_active_proc();

	for (size_t i = 0U; i < param->count; i++) {
		const struct bt_cap_commander_change_volume_offset_member_param *member_param =
			&param->param[i];
		struct bt_conn *member_conn =
			bt_cap_common_get_member_conn(param->type, &member_param->member);
		struct bt_vcp_included included;

		if (member_conn == NULL) {
			LOG_DBG("Invalid param->members[%zu]", i);
			return -EINVAL;
		}

		vol_ctlr = bt_vcp_vol_ctlr_get_by_conn(member_conn);
		if (vol_ctlr == NULL) {
			LOG_DBG("Invalid param->members[%zu] vol_ctlr", i);
			return -EINVAL;
		}

		err = bt_vcp_vol_ctlr_included_get(vol_ctlr, &included);
		if (err != 0 || included.vocs_cnt == 0) {
			LOG_DBG("Invalid param->members[%zu] vocs", i);
			return -EINVAL;
		}

		/* Store the necessary parameters as we cannot assume that the supplied parameters
		 * are kept valid
		 */
		active_proc->proc_param.commander[i].conn = member_conn;
		active_proc->proc_param.commander[i].change_offset.offset = member_param->offset;
		/* TODO: For now we just use the first VOCS instance
		 * - How should we handle multiple?
		 */
		active_proc->proc_param.commander[i].change_offset.vocs = included.vocs[0];
	}

	proc_param = &active_proc->proc_param.commander[0];
	conn = proc_param->conn;
	active_proc->proc_initiated_cnt++;

	err = bt_vocs_state_set(proc_param->change_offset.vocs, proc_param->change_offset.offset);
	if (err != 0) {
		LOG_DBG("Failed to set volume for conn %p: %d", (void *)conn, err);
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */

#if defined(CONFIG_BT_MICP_MIC_CTLR)
static struct bt_micp_mic_ctlr_cb mic_ctlr_cb;
static bool micp_callbacks_registered;

static int cap_commander_register_micp_callbacks(void)
{
	int err;

	err = bt_micp_mic_ctlr_cb_register(&mic_ctlr_cb);
	if (err != 0) {
		LOG_DBG("Failed to register MICP callbacks: %d", err);

		return -ENOEXEC;
	}

	micp_callbacks_registered = true;

	return 0;
}

int bt_cap_commander_change_microphone_mute_state(
	const struct bt_cap_commander_change_microphone_mute_state_param *param)
{
	if (!micp_callbacks_registered && cap_commander_register_micp_callbacks() != 0) {
		LOG_DBG("Failed to register MICP callbacks");

		return -ENOEXEC;
	}

	return -ENOSYS;
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static bool valid_change_microphone_gain_param(
	const struct bt_cap_commander_change_microphone_gain_setting_param *param)
{
	CHECKIF(param == NULL) {
		LOG_DBG("param is NULL");
		return false;
	}

	CHECKIF(param->count == 0) {
		LOG_DBG("Invalid param->count: %u", param->count);
		return false;
	}

	CHECKIF(param->param == NULL) {
		LOG_DBG("param->param is NULL");
		return false;
	}

	CHECKIF(param->count > CONFIG_BT_MAX_CONN) {
		LOG_DBG("param->count (%zu) is larger than CONFIG_BT_MAX_CONN (%d)", param->count,
			CONFIG_BT_MAX_CONN);
		return false;
	}

	for (size_t i = 0U; i < param->count; i++) {
		const union bt_cap_set_member *member = &param->param[i].member;
		const struct bt_cap_common_client *client =
			bt_cap_common_get_client(param->type, member);
		struct bt_micp_mic_ctlr *mic_ctlr;
		struct bt_micp_included included;
		int err;

		if (client == NULL) {
			LOG_DBG("Invalid param->param[%zu].member", i);
			return false;
		}

		mic_ctlr = bt_micp_mic_ctlr_get_by_conn(client->conn);
		if (mic_ctlr == NULL) {
			LOG_DBG("Microphone control not available for param->param[%zu].member", i);
			return false;
		}

		err = bt_micp_mic_ctlr_included_get(mic_ctlr, &included);
		if (err != 0 || included.aics_cnt == 0) {
			LOG_DBG("Microphone audio input control not available for "
				"param->param[%zu].member",
				i);
			return -ENOEXEC;
		}

		for (size_t j = 0U; j < i; j++) {
			const union bt_cap_set_member *other = &param->param[j].member;

			if (other == member) {
				LOG_DBG("param->param[%zu].member (%p) is duplicated by "
					"param->param[%zu].member (%p)",
					j, other, i, member);
				return false;
			}
		}
	}

	return true;
}

static void cap_commander_micp_gain_set_cb(struct bt_aics *inst, int err)
{
	struct bt_cap_common_proc *active_proc = bt_cap_common_get_active_proc();
	struct bt_conn *conn;
	int micp_err;

	LOG_DBG("bt_aics %p", (void *)inst);

	micp_err = bt_aics_client_conn_get(inst, &conn);
	if (micp_err != 0) {
		LOG_ERR("Failed to get conn by aics: %d", micp_err);
		return;
	}

	LOG_DBG("conn %p", (void *)conn);
	if (!bt_cap_common_conn_in_active_proc(conn)) {
		/* State change happened outside of a procedure; ignore */
		return;
	}

	if (err != 0) {
		LOG_DBG("Failed to set gain: %d", err);
		bt_cap_common_abort_proc(conn, err);
	} else {
		active_proc->proc_done_cnt++;

		LOG_DBG("Conn %p gain updated (%zu/%zu streams done)", (void *)conn,
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
		active_proc->proc_initiated_cnt++;
		err = bt_aics_gain_set(proc_param->change_gain.aics, proc_param->change_gain.gain);
		if (err != 0) {
			LOG_DBG("Failed to set gain for conn %p: %d", (void *)conn, err);
			bt_cap_common_abort_proc(conn, err);
			cap_commander_unicast_audio_proc_complete();
		}
	} else {
		cap_commander_unicast_audio_proc_complete();
	}
}

int bt_cap_commander_change_microphone_gain_setting(
	const struct bt_cap_commander_change_microphone_gain_setting_param *param)
{
	const struct bt_cap_commander_proc_param *proc_param;
	struct bt_cap_common_proc *active_proc;
	struct bt_conn *conn;
	int err;

	if (bt_cap_common_proc_is_active()) {
		LOG_DBG("A CAP procedure is already in progress");

		return -EBUSY;
	}

	if (!valid_change_microphone_gain_param(param)) {
		return -EINVAL;
	}

	bt_cap_common_start_proc(BT_CAP_COMMON_PROC_TYPE_MICROPHONE_GAIN_CHANGE, param->count);

	mic_ctlr_cb.aics_cb.set_gain = cap_commander_micp_gain_set_cb;
	if (!micp_callbacks_registered && cap_commander_register_micp_callbacks() != 0) {
		LOG_DBG("Failed to register MICP callbacks");

		return -ENOEXEC;
	}

	active_proc = bt_cap_common_get_active_proc();

	for (size_t i = 0U; i < param->count; i++) {
		const union bt_cap_set_member *member = &param->param[i].member;
		struct bt_conn *member_conn = bt_cap_common_get_member_conn(param->type, member);
		struct bt_micp_mic_ctlr *mic_ctlr;
		struct bt_micp_included included;

		if (member_conn == NULL) {
			LOG_DBG("Invalid param->param[%zu].member", i);
			return -EINVAL;
		}

		mic_ctlr = bt_micp_mic_ctlr_get_by_conn(member_conn);
		if (mic_ctlr == NULL) {
			LOG_DBG("Invalid param->param[%zu].member mic_ctlr", i);
			return -EINVAL;
		}

		err = bt_micp_mic_ctlr_included_get(mic_ctlr, &included);
		if (err != 0 || included.aics_cnt == 0) {
			LOG_DBG("Invalid param->param[%zu].member aics", i);
			return -EINVAL;
		}

		/* Store the necessary parameters as we cannot assume that the supplied parameters
		 * are kept valid
		 */
		active_proc->proc_param.commander[i].conn = member_conn;
		active_proc->proc_param.commander[i].change_gain.gain = param->param[i].gain;
		/* TODO: For now we just use the first AICS instance
		 * - How should we handle multiple?
		 */
		active_proc->proc_param.commander[i].change_gain.aics = included.aics[0];
	}

	proc_param = &active_proc->proc_param.commander[0];
	conn = proc_param->conn;
	active_proc->proc_initiated_cnt++;

	err = bt_aics_gain_set(proc_param->change_gain.aics, proc_param->change_gain.gain);
	if (err != 0) {
		LOG_DBG("Failed to set gain for conn %p: %d", (void *)conn, err);
		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
