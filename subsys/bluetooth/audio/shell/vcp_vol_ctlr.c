/** @file
 *  @brief Bluetooth VCP client shell.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/vcp.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include "shell/bt.h"

static struct bt_vcp_vol_ctlr *vcp_vol_ctlr;
static struct bt_vcp_included vcp_included;

static void vcs_discover_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			    uint8_t vocs_count, uint8_t aics_count)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP discover failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP discover done with %u AICS",
			    aics_count);

		if (bt_vcp_vol_ctlr_included_get(vol_ctlr, &vcp_included)) {
			shell_error(ctx_shell, "Could not get VCP context");
		}
	}
}

static void vcs_vol_down_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP vol_down failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP vol_down done");
	}
}

static void vcs_vol_up_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP vol_up failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP vol_up done");
	}
}

static void vcs_mute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP mute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP mute done");
	}
}

static void vcs_unmute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP unmute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP unmute done");
	}
}

static void vcs_vol_down_unmute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP vol_down_unmute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP vol_down_unmute done");
	}
}

static void vcs_vol_up_unmute_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP vol_up_unmute failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP vol_up_unmute done");
	}
}

static void vcs_vol_set_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP vol_set failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP vol_set done");
	}
}

static void vcs_state_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			 uint8_t volume, uint8_t mute)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP state get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP volume %u, mute %u", volume, mute);
	}
}

static void vcs_flags_cb(struct bt_vcp_vol_ctlr *vol_ctlr, int err,
			 uint8_t flags)
{
	if (err != 0) {
		shell_error(ctx_shell, "VCP flags get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP flags 0x%02X", flags);
	}
}

#if CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST > 0
static void vcs_aics_set_gain_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Set gain failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Gain set for inst %p", inst);
	}
}

static void vcs_aics_unmute_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unmute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Unmuted inst %p", inst);
	}
}

static void vcs_aics_mute_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Muted inst %p", inst);
	}
}

static void vcs_aics_set_manual_mode_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "Set manual mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Manuel mode set for inst %p", inst);
	}
}

static void vcs_aics_automatic_mode_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "Set automatic mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Automatic mode set for inst %p",
			    inst);
	}
}

static void vcs_aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			      uint8_t mute, uint8_t mode)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS state get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell,
			    "AICS inst %p state gain %d, mute %u, mode %u",
			    inst, gain, mute, mode);
	}
}

static void vcs_aics_gain_setting_cb(struct bt_aics *inst, int err,
				     uint8_t units, int8_t minimum,
				     int8_t maximum)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "AICS gain settings get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell,
			    "AICS inst %p gain settings units %u, min %d, max %d",
			    inst, units, minimum, maximum);
	}
}

static void vcs_aics_input_type_cb(struct bt_aics *inst, int err,
				   uint8_t input_type)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "AICS input type get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p input type %u",
			    inst, input_type);
	}
}

static void vcs_aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "AICS status get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p status %s",
			    inst, active ? "active" : "inactive");
	}

}
static void vcs_aics_description_cb(struct bt_aics *inst, int err,
				    char *description)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "AICS description get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p description %s",
			    inst, description);
	}
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST > 0 */

#if CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST > 0
static void vcs_vocs_set_offset_cb(struct bt_vocs *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Set offset failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Offset set for inst %p", inst);
	}
}

static void vcs_vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	if (err != 0) {
		shell_error(ctx_shell, "VOCS state get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p offset %d", inst, offset);
	}
}

static void vcs_vocs_location_cb(struct bt_vocs *inst, int err,
				 uint32_t location)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "VOCS location get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p location %u",
			    inst, location);
	}
}

static void vcs_vocs_description_cb(struct bt_vocs *inst, int err,
				    char *description)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "VOCS description get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p description %s",
			    inst, description);
	}
}
#endif /* CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST > 0 */

static struct bt_vcp_vol_ctlr_cb vcp_cbs = {
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
#if CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST > 0
	.aics_cb = {
		.state = vcs_aics_state_cb,
		.gain_setting = vcs_aics_gain_setting_cb,
		.type = vcs_aics_input_type_cb,
		.status = vcs_aics_status_cb,
		.description = vcs_aics_description_cb,
		.set_gain = vcs_aics_set_gain_cb,
		.unmute = vcs_aics_unmute_cb,
		.mute = vcs_aics_mute_cb,
		.set_manual_mode = vcs_aics_set_manual_mode_cb,
		.set_auto_mode = vcs_aics_automatic_mode_cb,
	},
#endif /* CONFIG_BT_VCP_VOL_CTLR_MAX_AICS_INST > 0 */
#if CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST > 0
	.vocs_cb = {
		.state = vcs_vocs_state_cb,
		.location = vcs_vocs_location_cb,
		.description = vcs_vocs_description_cb,
		.set_offset = vcs_vocs_set_offset_cb,
	}
#endif /* CONFIG_BT_VCP_VOL_CTLR_MAX_VOCS_INST > 0 */
};

static int cmd_vcp_vol_ctlr_discover(const struct shell *sh, size_t argc,
				   char **argv)
{
	int result;

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	result = bt_vcp_vol_ctlr_cb_register(&vcp_cbs);
	if (result != 0) {
		shell_print(sh, "CB register failed: %d", result);
		return result;
	}

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_discover(default_conn, &vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_state_get(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_read_state(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_flags_get(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_read_flags(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_volume_down(const struct shell *sh, size_t argc,
				      char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_vol_down(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_volume_up(const struct shell *sh, size_t argc,
				    char **argv)

{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_vol_up(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_unmute_volume_down(const struct shell *sh,
					     size_t argc, char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_unmute_vol_down(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_unmute_volume_up(const struct shell *sh,
					   size_t argc, char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_unmute_vol_up(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_volume_set(const struct shell *sh, size_t argc,
				     char **argv)

{
	unsigned long volume;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	volume = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse volume: %d", result);

		return -ENOEXEC;
	}

	if (volume > UINT8_MAX) {
		shell_error(sh, "Volume shall be 0-255, was %lu", volume);

		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_set_vol(vcp_vol_ctlr, volume);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}


static int cmd_vcp_vol_ctlr_unmute(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_unmute(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_mute(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	result = bt_vcp_vol_ctlr_mute(vcp_vol_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_vocs_state_get(const struct shell *sh, size_t argc,
					 char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_vocs_state_get(vcp_included.vocs[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_vocs_location_get(const struct shell *sh,
					    size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_vocs_location_get(vcp_included.vocs[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_vocs_location_set(const struct shell *sh,
					    size_t argc, char **argv)
{
	unsigned long location;
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	location = shell_strtoul(argv[2], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse location: %d", result);

		return -ENOEXEC;
	}

	if (location > UINT32_MAX) {
		shell_error(sh, "Invalid location %lu", location);
		return -ENOEXEC;

	}

	result = bt_vocs_location_set(vcp_included.vocs[index], location);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_vocs_offset_set(const struct shell *sh,
					  size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;
	long offset;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	offset = shell_strtol(argv[2], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse offset: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(offset, BT_VOCS_MIN_OFFSET, BT_VOCS_MAX_OFFSET)) {
		shell_error(sh, "Offset shall be %d-%d, was %ld",
			    BT_VOCS_MIN_OFFSET, BT_VOCS_MAX_OFFSET, offset);
		return -ENOEXEC;
	}

	result = bt_vocs_state_set(vcp_included.vocs[index],
				       offset);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_vocs_output_description_get(const struct shell *sh,
						      size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_vocs_description_get(vcp_included.vocs[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_vocs_output_description_set(const struct shell *sh,
						      size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.vocs_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_vocs_description_set(vcp_included.vocs[index], argv[2]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_state_get(const struct shell *sh,
					       size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_state_get(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_gain_setting_get(const struct shell *sh,
						size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_gain_setting_get(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_type_get(const struct shell *sh,
					      size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_type_get(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_status_get(const struct shell *sh,
						size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_status_get(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_unmute(const struct shell *sh,
					    size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_unmute(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_mute(const struct shell *sh,
					  size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_mute(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_manual_input_gain_set(const struct shell *sh,
						     size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_manual_gain_set(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_auto_input_gain_set(const struct shell *sh,
						   size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_automatic_gain_set(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_gain_set(const struct shell *sh, size_t argc,
					char **argv)
{
	unsigned long index;
	int result = 0;
	long gain;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);
		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	gain = shell_strtol(argv[2], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse gain: %d", result);
		return -ENOEXEC;
	}

	if (!IN_RANGE(gain, INT8_MIN, INT8_MAX)) {
		shell_error(sh, "Gain shall be %d-%d, was %ld",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	result = bt_aics_gain_set(vcp_included.aics[index], gain);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_description_get(const struct shell *sh,
						     size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_description_get(vcp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr_aics_input_description_set(const struct shell *sh,
						     size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= vcp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    vcp_included.vocs_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_description_set(vcp_included.aics[index], argv[2]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_ctlr(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(vcp_vol_ctlr_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover VCP and included services for current "
		      "connection",
		      cmd_vcp_vol_ctlr_discover, 1, 0),
	SHELL_CMD_ARG(state_get, NULL,
		      "Get volume state of the VCP server. Should be done "
		      "before sending any control messages",
		      cmd_vcp_vol_ctlr_state_get, 1, 0),
	SHELL_CMD_ARG(flags_get, NULL,
		      "Read volume flags",
		      cmd_vcp_vol_ctlr_flags_get, 1, 0),
	SHELL_CMD_ARG(volume_down, NULL,
		      "Turn the volume down",
		      cmd_vcp_vol_ctlr_volume_down, 1, 0),
	SHELL_CMD_ARG(volume_up, NULL,
		      "Turn the volume up",
		      cmd_vcp_vol_ctlr_volume_up, 1, 0),
	SHELL_CMD_ARG(unmute_volume_down, NULL,
		      "Turn the volume down, and unmute",
		      cmd_vcp_vol_ctlr_unmute_volume_down, 1, 0),
	SHELL_CMD_ARG(unmute_volume_up, NULL,
		      "Turn the volume up, and unmute",
		      cmd_vcp_vol_ctlr_unmute_volume_up, 1, 0),
	SHELL_CMD_ARG(volume_set, NULL,
		      "Set an absolute volume <volume>",
		      cmd_vcp_vol_ctlr_volume_set, 2, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute",
		      cmd_vcp_vol_ctlr_unmute, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute",
		      cmd_vcp_vol_ctlr_mute, 1, 0),
	SHELL_CMD_ARG(vocs_state_get, NULL,
		      "Get the offset state of a VOCS instance <inst_index>",
		      cmd_vcp_vol_ctlr_vocs_state_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_get, NULL,
		      "Get the location of a VOCS instance <inst_index>",
		      cmd_vcp_vol_ctlr_vocs_location_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_set, NULL,
		      "Set the location of a VOCS instance <inst_index> "
		      "<location>",
		      cmd_vcp_vol_ctlr_vocs_location_set, 3, 0),
	SHELL_CMD_ARG(vocs_offset_set, NULL,
		      "Set the offset for a VOCS instance <inst_index> "
		      "<offset>",
		      cmd_vcp_vol_ctlr_vocs_offset_set, 3, 0),
	SHELL_CMD_ARG(vocs_output_description_get, NULL,
		      "Get the output description of a VOCS instance "
		      "<inst_index>",
		      cmd_vcp_vol_ctlr_vocs_output_description_get, 2, 0),
	SHELL_CMD_ARG(vocs_output_description_set, NULL,
		      "Set the output description of a VOCS instance "
		      "<inst_index> <description>",
		      cmd_vcp_vol_ctlr_vocs_output_description_set, 3, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_vcp_vol_ctlr_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_vcp_vol_ctlr_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_vcp_vol_ctlr_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_vcp_vol_ctlr_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_vcp_vol_ctlr_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_vcp_vol_ctlr_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_vcp_vol_ctlr_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_vcp_vol_ctlr_aics_auto_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain of a AICS instance <inst_index> <gain>",
		      cmd_vcp_vol_ctlr_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_vcp_vol_ctlr_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_vcp_vol_ctlr_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(vcp_vol_ctlr, &vcp_vol_ctlr_cmds,
		       "Bluetooth VCP client shell commands",
		       cmd_vcp_vol_ctlr, 1, 1);
