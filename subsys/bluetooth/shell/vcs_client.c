/** @file
 *  @brief Bluetooth VCS client shell.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <bluetooth/conn.h>
#include <bluetooth/services/vcs.h>
#include <shell/shell.h>
#include <stdlib.h>

#include "bt.h"

static struct bt_vcs vcs;

static void vcs_discover_cb(struct bt_conn *conn, int err, uint8_t vocs_count,
			    uint8_t aics_count)
{
	if (err) {
		shell_error(ctx_shell, "VCS discover failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS discover done with %u AICS",
			    aics_count);

		if (bt_vcs_get(conn, &vcs)) {
			shell_error(ctx_shell, "Could not get VCS context");
		}
	}
}

static void vcs_vol_down_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS vol_down failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS vol_down done");
	}
}

static void vcs_vol_up_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS vol_up failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS vol_up done");
	}
}

static void vcs_mute_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS mute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS mute done");
	}
}

static void vcs_unmute_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS unmute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS unmute done");
	}
}

static void vcs_vol_down_unmute_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS vol_down_unmute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS vol_down_unmute done");
	}
}

static void vcs_vol_up_unmute_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS vol_up_unmute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS vol_up_unmute done");
	}
}

static void vcs_vol_set_cb(struct bt_conn *conn, int err)
{
	if (err) {
		shell_error(ctx_shell, "VCS vol_set failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS vol_set done");
	}
}

static void vcs_state_cb(struct bt_conn *conn, int err, uint8_t volume,
			    uint8_t mute)
{
	if (err) {
		shell_error(ctx_shell, "VCS state get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS volume %u, mute %u", volume, mute);
	}
}

static void vcs_flags_cb(struct bt_conn *conn, int err, uint8_t flags)
{
	if (err) {
		shell_error(ctx_shell, "VCS flags get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS flags 0x%02X", flags);
	}
}

#if CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0
static void aics_set_gain_cb(struct bt_conn *conn, struct bt_aics *inst,
			     int err)
{
	if (err) {
		shell_error(ctx_shell, "Set gain failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Gain set for inst %p", inst);
	}
}

static void aics_unmute_cb(struct bt_conn *conn, struct bt_aics *inst, int err)
{
	if (err) {
		shell_error(ctx_shell, "Unmute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Unmuted inst %p", inst);
	}
}

static void aics_mute_cb(struct bt_conn *conn, struct bt_aics *inst, int err)
{
	if (err) {
		shell_error(ctx_shell, "Mute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Muted inst %p", inst);
	}
}

static void aics_set_manual_mode_cb(struct bt_conn *conn, struct bt_aics *inst,
				    int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Set manual mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Manuel mode set for inst %p", inst);
	}
}

static void aics_automatic_mode_cb(struct bt_conn *conn, struct bt_aics *inst,
				   int err)
{
	if (err) {
		shell_error(ctx_shell,
			    "Set automatic mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Automatic mode set for inst %p",
			    inst);
	}
}

static void aics_state_cb(struct bt_conn *conn, struct bt_aics *inst, int err,
			  int8_t gain, uint8_t mute, uint8_t mode)
{
	if (err) {
		shell_error(ctx_shell, "AICS state get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell,
			    "AICS inst %p state gain %d, mute %u, mode %u",
			    inst, gain, mute, mode);
	}
}

static void aics_gain_setting_cb(struct bt_conn *conn, struct bt_aics *inst,
				 int err, uint8_t units, int8_t minimum,
				 int8_t maximum)
{
	if (err) {
		shell_error(ctx_shell,
			    "AICS gain settings get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell,
			    "AICS inst %p gain settings units %u, min %d, max %d",
			    inst, units, minimum, maximum);
	}
}

static void vcs_aics_input_type_cb(struct bt_conn *conn, struct bt_aics *inst,
				   int err, uint8_t input_type)
{
	if (err) {
		shell_error(ctx_shell,
			    "AICS input type get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p input type %u",
			    inst, input_type);
	}
}

static void vcs_aics_status_cb(struct bt_conn *conn, struct bt_aics *inst,
			       int err, bool active)
{
	if (err) {
		shell_error(ctx_shell,
			    "AICS status get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p status %s",
			    inst, active ? "active" : "inactive");
	}

}
static void vcs_aics_description_cb(struct bt_conn *conn, struct bt_aics *inst,
				    int err, char *description)
{
	if (err) {
		shell_error(ctx_shell,
			    "AICS description get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p description %s",
			    inst, description);
	}
}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 */

#if CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0
static void vocs_set_offset_cb(struct bt_conn *conn, struct bt_vocs *inst,
			       int err)
{
	if (err) {
		shell_error(ctx_shell, "Set offset failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Offset set for inst %p", inst);
	}
}

static void vocs_state_cb(struct bt_conn *conn, struct bt_vocs *inst, int err,
			  int16_t offset)
{
	if (err) {
		shell_error(ctx_shell, "VOCS state get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p offset %d", inst, offset);
	}
}

static void vocs_location_cb(struct bt_conn *conn, struct bt_vocs *inst,
			     int err, uint32_t location)
{
	if (err) {
		shell_error(ctx_shell,
			    "VOCS location get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p location %u",
			    inst, location);
	}
}

static void vocs_description_cb(struct bt_conn *conn, struct bt_vocs *inst,
				int err, char *description)
{
	if (err) {
		shell_error(ctx_shell,
			    "VOCS description get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p description %s",
			    inst, description);
	}
}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0 */

static struct bt_vcs_cb vcs_cbs = {
	.discover = vcs_discover_cb,
	.vol_down = vcs_vol_down_cb,
	.vol_up = vcs_vol_up_cb,
	.mute = vcs_mute_cb,
	.unmute = vcs_unmute_cb,
	.vol_down_unmute = vcs_vol_down_unmute_cb,
	.vol_up_unmute = vcs_vol_up_unmute_cb,
	.vol_set = vcs_vol_set_cb,

	.state = vcs_state_cb,
	.flags = vcs_flags_cb,

	/* Audio Input Control Service */
#if CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0
	.aics_cb = {
		.state = aics_state_cb,
		.gain_setting = aics_gain_setting_cb,
		.type = aics_input_type_cb,
		.status = aics_status_cb,
		.description = aics_description_cb,
		.set_gain = aics_set_gain_cb,
		.unmute = aics_unmute_cb,
		.mute = aics_mute_cb,
		.set_manual_mode = aics_set_manual_mode_cb,
		.set_auto_mode = aics_automatic_mode_cb,
	},
#endif /* CONFIG_BT_VCS_CLIENT_MAX_AICS_INST > 0 */
#if CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0
	.vocs_cb = {
		.state = vocs_state_cb,
		.location = vocs_location_cb,
		.description = vocs_description_cb,
		.set_offset = vocs_set_offset_cb,
	}
#endif /* CONFIG_BT_VCS_CLIENT_MAX_VOCS_INST > 0 */
};

static int cmd_vcs_client_discover(const struct shell *sh, size_t argc,
				   char **argv)
{
	int result;

	if (!ctx_shell) {
		ctx_shell = shell;
	}

	result = bt_vcs_client_cb_register(&vcs_cbs);
	if (result) {
		shell_print(sh, "CB register failed: %d", result);
		return result;
	}

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_discover(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_state_get(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_volume_get(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_flags_get(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_flags_get(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_volume_down(const struct shell *sh, size_t argc,
				      char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_volume_down(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_volume_up(const struct shell *sh, size_t argc,
				    char **argv)

{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_volume_up(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_unmute_volume_down(const struct shell *sh,
					     size_t argc, char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_unmute_volume_down(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_unmute_volume_up(const struct shell *sh,
					   size_t argc, char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_unmute_volume_up(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_volume_set(const struct shell *sh, size_t argc,
				     char **argv)

{
	int result;
	int volume = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (volume > UINT8_MAX) {
		shell_error(sh, "Volume shall be 0-255, was %d", volume);
		return -ENOEXEC;
	}

	result = bt_vcs_volume_set(default_conn, volume);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}


static int cmd_vcs_client_unmute(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_unmute(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_mute(const struct shell *sh, size_t argc, char **argv)
{
	int result;

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcs_mute(default_conn);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_vocs_state_get(const struct shell *sh, size_t argc,
					 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.vocs_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_state_get(default_conn, vcs.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_vocs_location_get(const struct shell *sh,
					    size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.vocs_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_location_get(default_conn, vcs.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_vocs_location_set(const struct shell *sh,
					    size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int location = strtol(argv[2], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.vocs_cnt, index);
		return -ENOEXEC;
	}

	if (location > UINT16_MAX || location < 0) {
		shell_error(sh, "Invalid location (%u-%u), was %u",
			    0, UINT16_MAX, location);
		return -ENOEXEC;

	}

	result = bt_vcs_vocs_location_set(default_conn, vcs.vocs[index],
					  location);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_vocs_offset_set(const struct shell *sh,
					  size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int offset = strtol(argv[2], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.vocs_cnt, index);
		return -ENOEXEC;
	}

	if (offset > UINT8_MAX || offset < -UINT8_MAX) {
		shell_error(sh, "Offset shall be %d-%d, was %d",
			    -UINT8_MAX, UINT8_MAX, offset);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_state_set(default_conn, vcs.vocs[index], offset);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_vocs_output_description_get(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.vocs_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_description_get(default_conn, vcs.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_vocs_output_description_set(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.vocs_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_description_set(default_conn, vcs.vocs[index],
					     description);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_state_get(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_state_get(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_gain_setting_get(const struct shell *sh,
						size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_gain_setting_get(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_type_get(const struct shell *sh,
					      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_type_get(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_status_get(const struct shell *sh,
						size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_status_get(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_unmute(const struct shell *sh,
					    size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_unmute(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_mute(const struct shell *sh,
					  size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_mute(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_manual_input_gain_set(const struct shell *sh,
						     size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_manual_gain_set(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_auto_input_gain_set(const struct shell *sh,
						   size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_automatic_gain_set(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_gain_set(const struct shell *sh, size_t argc,
					char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int gain = strtol(argv[2], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(sh, "Offset shall be %d-%d, was %d",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_gain_set(default_conn, vcs.aics[index], gain);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_description_get(const struct shell *sh,
						     size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_description_get(default_conn, vcs.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client_aics_input_description_set(const struct shell *sh,
						     size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (!default_conn) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	if (index >= vcs.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_description_set(default_conn, vcs.aics[index],
					     description);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_client(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(vcs_client_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover VCS and included services for current "
		      "connection",
		      cmd_vcs_client_discover, 1, 0),
	SHELL_CMD_ARG(state_get, NULL,
		      "Get volume state of the VCS server. Should be done "
		      "before sending any control messages",
		      cmd_vcs_client_state_get, 1, 0),
	SHELL_CMD_ARG(flags_get, NULL,
		      "Read volume flags",
		      cmd_vcs_client_flags_get, 1, 0),
	SHELL_CMD_ARG(volume_down, NULL,
		      "Turn the volume down",
		      cmd_vcs_client_volume_down, 1, 0),
	SHELL_CMD_ARG(volume_up, NULL,
		      "Turn the volume up",
		      cmd_vcs_client_volume_up, 1, 0),
	SHELL_CMD_ARG(unmute_volume_down, NULL,
		      "Turn the volume down, and unmute",
		      cmd_vcs_client_unmute_volume_down, 1, 0),
	SHELL_CMD_ARG(unmute_volume_up, NULL,
		      "Turn the volume up, and unmute",
		      cmd_vcs_client_unmute_volume_up, 1, 0),
	SHELL_CMD_ARG(volume_set, NULL,
		      "Set an absolute volume <volume>",
		      cmd_vcs_client_volume_set, 2, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute",
		      cmd_vcs_client_unmute, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute",
		      cmd_vcs_client_mute, 1, 0),
	SHELL_CMD_ARG(vocs_state_get, NULL,
		      "Get the offset state of a VOCS instance <inst_index>",
		      cmd_vcs_client_vocs_state_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_get, NULL,
		      "Get the location of a VOCS instance <inst_index>",
		      cmd_vcs_client_vocs_location_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_set, NULL,
		      "Set the location of a VOCS instance <inst_index> "
		      "<location>",
		      cmd_vcs_client_vocs_location_set, 3, 0),
	SHELL_CMD_ARG(vocs_offset_set, NULL,
		      "Set the offset for a VOCS instance <inst_index> "
		      "<offset>",
		      cmd_vcs_client_vocs_offset_set, 3, 0),
	SHELL_CMD_ARG(vocs_output_description_get, NULL,
		      "Get the output description of a VOCS instance "
		      "<inst_index>",
		      cmd_vcs_client_vocs_output_description_get, 2, 0),
	SHELL_CMD_ARG(vocs_output_description_set, NULL,
		      "Set the output description of a VOCS instance "
		      "<inst_index> <description>",
		      cmd_vcs_client_vocs_output_description_set, 3, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_vcs_client_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_vcs_client_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_vcs_client_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_vcs_client_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_vcs_client_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_vcs_client_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_vcs_client_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_vcs_client_aics_auto_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain of a AICS instance <inst_index> <gain>",
		      cmd_vcs_client_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_vcs_client_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_vcs_client_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(vcs_client, &vcs_client_cmds,
		       "Bluetooth VCS client shell commands",
		       cmd_vcs_client, 1, 1);
