/**
 * @file
 * @brief Shell APIs for Bluetooth CAP commander
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/vocs.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "host/shell/bt.h"
#include "audio.h"

static void cap_discover_cb(struct bt_conn *conn, int err,
			    const struct bt_csip_set_coordinator_set_member *member,
			    const struct bt_csip_set_coordinator_csis_inst *csis_inst)
{
	if (err != 0) {
		bt_shell_error("discover failed (%d)", err);
		return;
	}

	bt_shell_print("discovery completed%s", csis_inst == NULL ? "" : " with CSIS");
}

#if defined(CONFIG_BT_VCP_VOL_CTLR)
static void cap_volume_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Volume change failed (%d)", err);
		return;
	}

	bt_shell_print("Volume change completed");
}

static void cap_volume_mute_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Volume mute change failed (%d)", err);
		return;
	}

	bt_shell_print("Volume mute change completed");
}
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
static void cap_volume_offset_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Volume offset change failed (%d)", err);
		return;
	}

	bt_shell_print("Volume offset change completed");
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */

#if defined(CONFIG_BT_MICP_MIC_CTLR)
static void cap_microphone_mute_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Microphone mute change failed (%d)", err);
		return;
	}

	bt_shell_print("Microphone mute change completed");
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static void cap_microphone_gain_changed_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Microphone gain change failed (%d)", err);
		return;
	}

	bt_shell_print("Microphone gain change completed");
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */

#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
static void cap_broadcast_reception_start_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Broadcast reception start failed (%d)", err);
		return;
	}

	bt_shell_print("Broadcast reception start completed");
}

static void cap_broadcast_reception_stop_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Broadcast reception stop failed (%d) for conn %p", err,
			       (void *)conn);
		return;
	}

	bt_shell_print("Broadcast reception stop completed");
}

static void cap_distribute_broadcast_code_cb(struct bt_conn *conn, int err)
{
	if (err != 0) {
		bt_shell_error("Distribute broadcast code failed (%d) for conn %p", err,
			       (void *)conn);
		return;
	}

	bt_shell_error("Distribute broadcast code completed");
}
#endif

static struct bt_cap_commander_cb cbs = {
	.discovery_complete = cap_discover_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	.volume_changed = cap_volume_changed_cb,
	.volume_mute_changed = cap_volume_mute_changed_cb,
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	.volume_offset_changed = cap_volume_offset_changed_cb,
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
	.microphone_mute_changed = cap_microphone_mute_changed_cb,
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	.microphone_gain_changed = cap_microphone_gain_changed_cb,
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
	.broadcast_reception_start = cap_broadcast_reception_start_cb,
	.broadcast_reception_stop = cap_broadcast_reception_stop_cb,
	.distribute_broadcast_code = cap_distribute_broadcast_code_cb,
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */
};

static int cmd_cap_commander_cancel(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = bt_cap_commander_cancel();
	if (err != 0) {
		shell_print(sh, "Failed to cancel CAP commander procedure: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_commander_discover(const struct shell *sh, size_t argc, char *argv[])
{
	static bool cbs_registered;
	int err;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (!cbs_registered) {
		bt_cap_commander_register_cb(&cbs);
		cbs_registered = true;
	}

	err = bt_cap_commander_discover(default_conn);
	if (err != 0) {
		shell_error(sh, "Fail: %d", err);
	}

	return err;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR) || defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static void populate_connected_conns(struct bt_conn *conn, void *data)
{
	struct bt_conn **connected_conns = (struct bt_conn **)data;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (connected_conns[i] == NULL) {
			connected_conns[i] = conn;
			return;
		}
	}
}
#endif /* CONFIG_BT_VCP_VOL_CTLR || CONFIG_BT_MICP_MIC_CTLR_AICS */

#if defined(CONFIG_BT_VCP_VOL_CTLR)
static int cmd_cap_commander_change_volume(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN] = {0};
	struct bt_cap_commander_change_volume_param param = {
		.members = members,
	};
	unsigned long volume;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	volume = shell_strtoul(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse volume from %s", argv[1]);

		return -ENOEXEC;
	}

	if (volume > UINT8_MAX) {
		shell_error(sh, "Invalid volume %lu", volume);

		return -ENOEXEC;
	}
	param.volume = (uint8_t)volume;

	param.type = BT_CAP_SET_TYPE_AD_HOC;
	/* TODO: Add support for coordinated sets */

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);

	param.count = 0U;
	param.members = members;
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		param.members[i].member = conn;
		param.count++;
	}

	shell_print(sh, "Setting volume to %u on %zu connections", param.volume, param.count);

	err = bt_cap_commander_change_volume(&param);
	if (err != 0) {
		shell_print(sh, "Failed to change volume: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_commander_change_volume_mute(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN] = {0};
	struct bt_cap_commander_change_volume_mute_state_param param = {
		.members = members,
		.type = BT_CAP_SET_TYPE_AD_HOC, /* TODO: Add support for coordinated sets */
	};
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.mute = shell_strtobool(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse volume mute from %s", argv[1]);

		return -ENOEXEC;
	}

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);

	param.count = 0U;
	param.members = members;
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		param.members[i].member = conn;
		param.count++;
	}

	shell_print(sh, "Setting volume mute to %d on %zu connections", param.mute, param.count);

	err = bt_cap_commander_change_volume_mute_state(&param);
	if (err != 0) {
		shell_print(sh, "Failed to change volume mute: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
static int cmd_cap_commander_change_volume_offset(const struct shell *sh, size_t argc, char *argv[])
{
	struct bt_cap_commander_change_volume_offset_member_param member_params[CONFIG_BT_MAX_CONN];
	const size_t cap_args = argc - 1; /* First argument is the command itself */
	struct bt_cap_commander_change_volume_offset_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
	};
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	size_t conn_cnt = 0U;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		conn_cnt++;
	}

	if (cap_args > conn_cnt) {
		shell_error(sh, "Cannot use %zu arguments for %zu connections", argc, conn_cnt);

		return -ENOEXEC;
	}

	/* TODO: Add support for coordinated sets */

	for (size_t i = 0U; i < cap_args; i++) {
		const char *arg = argv[i + 1];
		long volume_offset;

		volume_offset = shell_strtol(arg, 10, &err);
		if (err != 0) {
			shell_error(sh, "Failed to parse volume offset from %s", arg);

			return -ENOEXEC;
		}

		if (!IN_RANGE(volume_offset, BT_VOCS_MIN_OFFSET, BT_VOCS_MAX_OFFSET)) {
			shell_error(sh, "Invalid volume_offset %lu", volume_offset);

			return -ENOEXEC;
		}

		member_params[i].offset = (int16_t)volume_offset;
		member_params[i].member.member = connected_conns[i];
		param.count++;
	}

	shell_print(sh, "Setting volume offset on %zu connections", param.count);

	err = bt_cap_commander_change_volume_offset(&param);
	if (err != 0) {
		shell_print(sh, "Failed to change volume offset: %d", err);

		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */

#if defined(CONFIG_BT_MICP_MIC_CTLR)
static int cmd_cap_commander_change_microphone_mute(const struct shell *sh, size_t argc,
						    char *argv[])
{
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	union bt_cap_set_member members[CONFIG_BT_MAX_CONN] = {0};
	struct bt_cap_commander_change_microphone_mute_state_param param = {
		.members = members,
		.type = BT_CAP_SET_TYPE_AD_HOC, /* TODO: Add support for coordinated sets */
	};
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	param.mute = shell_strtobool(argv[1], 10, &err);
	if (err != 0) {
		shell_error(sh, "Failed to parse microphone mute from %s", argv[1]);

		return -ENOEXEC;
	}

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);

	param.count = 0U;
	param.members = members;
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		param.members[i].member = conn;
		param.count++;
	}

	shell_print(sh, "Setting microphone mute to %d on %zu connections", param.mute,
		    param.count);

	err = bt_cap_commander_change_microphone_mute_state(&param);
	if (err != 0) {
		shell_print(sh, "Failed to change microphone mute: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static int cmd_cap_commander_change_microphone_gain(const struct shell *sh, size_t argc,
						    char *argv[])
{
	struct bt_cap_commander_change_microphone_gain_setting_member_param
		member_params[CONFIG_BT_MAX_CONN];
	const size_t cap_args = argc - 1; /* First argument is the command itself */
	struct bt_cap_commander_change_microphone_gain_setting_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
	};
	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	size_t conn_cnt = 0U;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		conn_cnt++;
	}

	if (cap_args > conn_cnt) {
		shell_error(sh, "Cannot use %zu arguments for %zu connections", argc, conn_cnt);

		return -ENOEXEC;
	}

	/* TODO: Add support for coordinated sets */

	for (size_t i = 0U; i < cap_args; i++) {
		const char *arg = argv[i + 1];
		long gain;

		gain = shell_strtol(arg, 10, &err);
		if (err != 0) {
			shell_error(sh, "Failed to parse volume offset from %s", arg);

			return -ENOEXEC;
		}

		if (!IN_RANGE(gain, INT8_MIN, INT8_MAX)) {
			shell_error(sh, "Invalid gain %lu", gain);

			return -ENOEXEC;
		}

		member_params[i].gain = (int8_t)gain;
		member_params[i].member.member = connected_conns[i];
		param.count++;
	}

	shell_print(sh, "Setting microphone gain on %zu connections", param.count);

	err = bt_cap_commander_change_microphone_gain_setting(&param);
	if (err != 0) {
		shell_print(sh, "Failed to change microphone gain: %d", err);

		return -ENOEXEC;
	}

	return 0;
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */

#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
static int cmd_cap_commander_broadcast_reception_start(const struct shell *sh, size_t argc,
						       char *argv[])
{
	struct bt_cap_commander_broadcast_reception_start_member_param
		member_params[CONFIG_BT_MAX_CONN] = {0};

	struct bt_cap_commander_broadcast_reception_start_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
	};

	struct bt_cap_commander_broadcast_reception_start_member_param *member_param =
		&member_params[0];
	struct bt_bap_bass_subgroup subgroup = {0};

	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	size_t conn_cnt = 0U;
	unsigned long broadcast_id;
	unsigned long adv_sid;

	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	/* TODO: Add support for coordinated sets */

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		conn_cnt++;
	}

	err = bt_addr_le_from_str(argv[1], argv[2], &member_param->addr);
	if (err) {
		shell_error(sh, "Invalid peer address (err %d)", err);

		return -ENOEXEC;
	}

	adv_sid = shell_strtoul(argv[3], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse adv_sid: %d", err);

		return -ENOEXEC;
	}

	if (adv_sid > BT_GAP_SID_MAX) {
		shell_error(sh, "Invalid adv_sid: %lu", adv_sid);

		return -ENOEXEC;
	}

	member_param->adv_sid = adv_sid;

	broadcast_id = shell_strtoul(argv[4], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse broadcast_id: %d", err);

		return -ENOEXEC;
	}

	if (broadcast_id > BT_AUDIO_BROADCAST_ID_MAX) {
		shell_error(sh, "Invalid broadcast_id: %lu", broadcast_id);

		return -ENOEXEC;
	}

	member_param->broadcast_id = broadcast_id;

	if (argc > 5) {
		unsigned long pa_interval;

		pa_interval = shell_strtoul(argv[5], 0, &err);
		if (err) {
			shell_error(sh, "Could not parse pa_interval: %d", err);

			return -ENOEXEC;
		}

		if (!IN_RANGE(pa_interval, BT_GAP_PER_ADV_MIN_INTERVAL,
			      BT_GAP_PER_ADV_MAX_INTERVAL)) {
			shell_error(sh, "Invalid pa_interval: %lu", pa_interval);

			return -ENOEXEC;
		}

		member_param->pa_interval = pa_interval;
	} else {
		member_param->pa_interval = BT_BAP_PA_INTERVAL_UNKNOWN;
	}

	/* TODO: Support multiple subgroups */
	if (argc > 6) {
		unsigned long bis_sync;

		bis_sync = shell_strtoul(argv[6], 0, &err);
		if (err) {
			shell_error(sh, "Could not parse bis_sync: %d", err);

			return -ENOEXEC;
		}

		if (!BT_BAP_BASS_VALID_BIT_BITFIELD(bis_sync)) {
			shell_error(sh, "Invalid bis_sync: %lu", bis_sync);

			return -ENOEXEC;
		}

		subgroup.bis_sync = bis_sync;
	} else {
		subgroup.bis_sync = BT_BAP_BIS_SYNC_NO_PREF;
	}

	if (argc > 7) {
		size_t metadata_len;

		metadata_len = hex2bin(argv[7], strlen(argv[7]), subgroup.metadata,
				       sizeof(subgroup.metadata));

		if (metadata_len == 0U) {
			shell_error(sh, "Could not parse metadata");

			return -ENOEXEC;
		}

		/* sizeof(subgroup.metadata) can always fit in uint8_t */

		subgroup.metadata_len = metadata_len;
	}

	member_param->num_subgroups = 1;
	memcpy(member_param->subgroups, &subgroup, sizeof(struct bt_bap_bass_subgroup));

	member_param->member.member = connected_conns[0];

	/* each connection has its own member_params field
	 * here we use the same values for all connections, so we copy
	 * the parameters
	 */
	for (size_t i = 1U; i < conn_cnt; i++) {
		memcpy(&member_params[i], member_param, sizeof(*member_param));

		/* the member value is different for each, so we can not just copy this value */
		member_params[i].member.member = connected_conns[i];
	}

	param.count = conn_cnt;

	shell_print(sh, "Starting broadcast reception on %zu connection(s)", param.count);

	err = bt_cap_commander_broadcast_reception_start(&param);
	if (err != 0) {
		shell_print(sh, "Failed to start broadcast reception: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_commander_broadcast_reception_stop(const struct shell *sh, size_t argc,
						      char *argv[])
{
	struct bt_cap_commander_broadcast_reception_stop_member_param
		member_params[CONFIG_BT_MAX_CONN] = {0};
	const size_t cap_args = argc - 1; /* First argument is the command itself */
	struct bt_cap_commander_broadcast_reception_stop_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
	};

	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	size_t conn_cnt = 0U;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	/* TODO: Add support for coordinated sets */

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}

		conn_cnt++;
	}

	if (cap_args > conn_cnt) {
		shell_error(sh, "Cannot use %zu arguments for %zu connections", argc, conn_cnt);

		return -ENOEXEC;
	}

	for (size_t i = 0U; i < cap_args; i++) {
		const char *arg = argv[i + 1];
		unsigned long src_id;

		src_id = shell_strtoul(arg, 0, &err);
		if (err != 0) {
			shell_error(sh, "Could not parse src_id: %d", err);

			return -ENOEXEC;
		}

		if (src_id > UINT8_MAX) {
			shell_error(sh, "Invalid src_id: %lu", src_id);

			return -ENOEXEC;
		}

		/* TODO: Allow for multiple subgroups */
		member_params[i].num_subgroups = 1;
		member_params[i].src_id = src_id;
		member_params[i].member.member = connected_conns[i];
		param.count++;
	}

	shell_print(sh, "Stopping broadcast reception on %zu connection(s)", param.count);

	err = bt_cap_commander_broadcast_reception_stop(&param);
	if (err != 0) {
		shell_print(sh, "Failed to initiate broadcast reception stop: %d", err);

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_cap_commander_distribute_broadcast_code(const struct shell *sh, size_t argc,
						       char *argv[])
{
	struct bt_cap_commander_distribute_broadcast_code_member_param
		member_params[CONFIG_BT_MAX_CONN] = {0};
	const size_t cap_argc = argc - 1; /* First argument is the command itself */
	struct bt_cap_commander_distribute_broadcast_code_param param = {
		.type = BT_CAP_SET_TYPE_AD_HOC,
		.param = member_params,
	};

	struct bt_conn *connected_conns[CONFIG_BT_MAX_CONN] = {0};
	size_t conn_cnt = 0U;
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	/* TODO Add support for coordinated sets */

	/* Populate the array of connected connections */
	bt_conn_foreach(BT_CONN_TYPE_LE, populate_connected_conns, (void *)connected_conns);
	for (size_t i = 0; i < ARRAY_SIZE(connected_conns); i++) {
		struct bt_conn *conn = connected_conns[i];

		if (conn == NULL) {
			break;
		}
		conn_cnt++;
	}

	/* The number of cap_args needs to be the number of connections + 1 since the last argument
	 * is the broadcast code
	 */
	if (cap_argc != conn_cnt + 1) {
		shell_error(sh, "Cannot use %zu arguments for %zu connections", argc, conn_cnt);
		return -ENOEXEC;
	}

	/* the last argument is the broadcast code */
	if (strlen(argv[cap_argc]) > BT_ISO_BROADCAST_CODE_SIZE) {
		shell_error(sh, "Broadcast code can be maximum %d characters",
			    BT_ISO_BROADCAST_CODE_SIZE);
		return -ENOEXEC;
	}

	for (size_t i = 0; i < conn_cnt; i++) {
		const char *arg = argv[i + 1];
		unsigned long src_id;

		src_id = shell_strtoul(arg, 0, &err);
		if (err != 0) {
			shell_error(sh, "Could not parce src_id: %d", err);
			return -ENOEXEC;
		}
		if (src_id > UINT8_MAX) {
			shell_error(sh, "Invalid src_id: %lu", src_id);
			return -ENOEXEC;
		}

		member_params[i].src_id = src_id;
		member_params[i].member.member = connected_conns[i];
		param.count++;
	}

	memcpy(param.broadcast_code, argv[cap_argc], strlen(argv[cap_argc]));
	shell_print(sh, "Distributing broadcast code on %zu connection(s)", param.count);
	err = bt_cap_commander_distribute_broadcast_code(&param);
	if (err != 0) {
		shell_print(sh, "Failed to initiate distribute broadcast code: %d", err);
		return -ENOEXEC;
	}

	return 0;
}

#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */

static int cmd_cap_commander(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	cap_commander_cmds,
	SHELL_CMD_ARG(discover, NULL, "Discover CAS", cmd_cap_commander_discover, 1, 0),
	SHELL_CMD_ARG(cancel, NULL, "CAP commander cancel current procedure",
		      cmd_cap_commander_cancel, 1, 0),
#if defined(CONFIG_BT_VCP_VOL_CTLR)
	SHELL_CMD_ARG(change_volume, NULL, "Change volume on all connections <volume>",
		      cmd_cap_commander_change_volume, 2, 0),
	SHELL_CMD_ARG(change_volume_mute, NULL,
		      "Change volume mute state on all connections <mute>",
		      cmd_cap_commander_change_volume_mute, 2, 0),
#if defined(CONFIG_BT_VCP_VOL_CTLR_VOCS)
	SHELL_CMD_ARG(change_volume_offset, NULL,
		      "Change volume offset per connection <volume_offset [volume_offset [...]]>",
		      cmd_cap_commander_change_volume_offset, 2, CONFIG_BT_MAX_CONN - 1),
#endif /* CONFIG_BT_VCP_VOL_CTLR_VOCS */
#endif /* CONFIG_BT_VCP_VOL_CTLR */
#if defined(CONFIG_BT_MICP_MIC_CTLR)
	SHELL_CMD_ARG(change_microphone_mute, NULL,
		      "Change microphone mute state on all connections <mute>",
		      cmd_cap_commander_change_microphone_mute, 2, 0),
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	SHELL_CMD_ARG(change_microphone_gain, NULL,
		      "Change microphone gain per connection <gain [gain [...]]>",
		      cmd_cap_commander_change_microphone_gain, 2, CONFIG_BT_MAX_CONN - 1),
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
#endif /* CONFIG_BT_MICP_MIC_CTLR */
#if defined(CONFIG_BT_BAP_BROADCAST_ASSISTANT)
	SHELL_CMD_ARG(broadcast_reception_start, NULL,
		      "Start broadcast reception "
		      "with source <address: XX:XX:XX:XX:XX:XX> "
		      "<type: public/random> <adv_sid> "
		      "<broadcast_id> [<pa_interval>] [<sync_bis>] "
		      "[<metadata>]",
		      cmd_cap_commander_broadcast_reception_start, 5, 3),
	SHELL_CMD_ARG(broadcast_reception_stop, NULL,
		      "Stop broadcast reception "
		      "<src_id [...]>",
		      cmd_cap_commander_broadcast_reception_stop, 2, 0),
	SHELL_CMD_ARG(distribute_broadcast_code, NULL,
		      "Distribute broadcast code <src_id [...]> <broadcast_code>",
		      cmd_cap_commander_distribute_broadcast_code, 2, CONFIG_BT_MAX_CONN - 1),
#endif /* CONFIG_BT_BAP_BROADCAST_ASSISTANT */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(cap_commander, &cap_commander_cmds, "Bluetooth CAP commander shell commands",
		       cmd_cap_commander, 1, 1);
