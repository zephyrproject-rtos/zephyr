/** @file
 *  @brief Bluetooth VCP server shell.
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
#include <stdio.h>
#include <string.h>

#include "shell/bt.h"

static struct bt_vcp_included vcp_included;

static void vcp_vol_rend_state_cb(int err, uint8_t volume, uint8_t mute)
{
	if (err) {
		shell_error(ctx_shell, "VCP state get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP volume %u, mute %u", volume, mute);
	}
}

static void vcp_vol_rend_flags_cb(int err, uint8_t flags)
{
	if (err) {
		shell_error(ctx_shell, "VCP flags get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCP flags 0x%02X", flags);
	}
}

static void aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			  uint8_t mute, uint8_t mode)
{
	if (err) {
		shell_error(ctx_shell,
			    "AICS state get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell,
			    "AICS inst %p state gain %d, mute %u, mode %u",
			    inst, gain, mute, mode);
	}
}

static void aics_gain_setting_cb(struct bt_aics *inst, int err, uint8_t units,
				 int8_t minimum, int8_t maximum)
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

static void aics_input_type_cb(struct bt_aics *inst, int err,
			       uint8_t input_type)
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

static void aics_status_cb(struct bt_aics *inst, int err, bool active)
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
static void aics_description_cb(struct bt_aics *inst, int err,
				char *description)
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
static void vocs_state_cb(struct bt_vocs *inst, int err, int16_t offset)
{
	if (err) {
		shell_error(ctx_shell, "VOCS state get failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "VOCS inst %p offset %d", inst, offset);
	}
}

static void vocs_location_cb(struct bt_vocs *inst, int err, uint32_t location)
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

static void vocs_description_cb(struct bt_vocs *inst, int err,
				char *description)
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

static struct bt_vcp_vol_rend_cb vcp_vol_rend_cbs = {
	.state = vcp_vol_rend_state_cb,
	.flags = vcp_vol_rend_flags_cb,
};

static struct bt_aics_cb aics_cbs = {
	.state = aics_state_cb,
	.gain_setting = aics_gain_setting_cb,
	.type = aics_input_type_cb,
	.status = aics_status_cb,
	.description = aics_description_cb
};

static struct bt_vocs_cb vocs_cbs = {
	.state = vocs_state_cb,
	.location = vocs_location_cb,
	.description = vocs_description_cb
};

static int cmd_vcp_vol_rend_init(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result = 0;
	struct bt_vcp_vol_rend_register_param vcp_register_param;
	char input_desc[CONFIG_BT_VCP_VOL_REND_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCP_VOL_REND_VOCS_INSTANCE_COUNT][16];
	static const char assignment_operator[] = "=";

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	memset(&vcp_register_param, 0, sizeof(vcp_register_param));

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND_VOCS)) {
		for (int i = 0; i < ARRAY_SIZE(vcp_register_param.vocs_param); i++) {
			vcp_register_param.vocs_param[i].location_writable = true;
			vcp_register_param.vocs_param[i].desc_writable = true;
			snprintf(output_desc[i], sizeof(output_desc[i]),
				"Output %d", i + 1);
			vcp_register_param.vocs_param[i].output_desc = output_desc[i];
			vcp_register_param.vocs_param[i].cb = &vocs_cbs;
		}
	}

	if (IS_ENABLED(CONFIG_BT_VCP_VOL_REND_AICS)) {
		for (int i = 0; i < ARRAY_SIZE(vcp_register_param.aics_param); i++) {
			vcp_register_param.aics_param[i].desc_writable = true;
			snprintf(input_desc[i], sizeof(input_desc[i]),
				"Input %d", i + 1);
			vcp_register_param.aics_param[i].description = input_desc[i];
			vcp_register_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
			vcp_register_param.aics_param[i].status = true;
			vcp_register_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
			vcp_register_param.aics_param[i].units = 1;
			vcp_register_param.aics_param[i].min_gain = -100;
			vcp_register_param.aics_param[i].max_gain = 100;
			vcp_register_param.aics_param[i].cb = &aics_cbs;
		}
	}

	/* Default values */
	vcp_register_param.step = 1;
	vcp_register_param.mute = BT_VCP_STATE_UNMUTED;
	vcp_register_param.volume = 100;

	for (int i = 1; i < argc; i++) {
		const char *operator = strstr(argv[i], assignment_operator);
		const char *kwarg = operator ? operator + 1 : NULL;

		if (kwarg) {
			if (!strncmp(argv[i], "step", 4)) {
				vcp_register_param.step = shell_strtoul(kwarg, 10, &result);
			} else if (!strncmp(argv[i], "mute", 4)) {
				vcp_register_param.mute = shell_strtobool(kwarg, 10, &result);
			} else if (!strncmp(argv[i], "volume", 6)) {
				vcp_register_param.volume = shell_strtoul(kwarg, 10, &result);
			} else {
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}

		if (result != 0) {
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	vcp_register_param.cb = &vcp_vol_rend_cbs;

	result = bt_vcp_vol_rend_register(&vcp_register_param);
	if (result) {
		shell_print(sh, "Fail: %d", result);
		return result;
	}

	result = bt_vcp_vol_rend_included_get(&vcp_included);
	if (result != 0) {
		shell_error(sh, "Failed to get included services: %d", result);
		return result;
	}

	return result;
}

static int cmd_vcp_vol_rend_volume_step(const struct shell *sh, size_t argc,
					char **argv)
{
	unsigned long step;
	int result = 0;

	step = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse step: %d", result);

		return -ENOEXEC;
	}

	if (!IN_RANGE(step, 1, UINT8_MAX)) {
		shell_error(sh, "Invalid step %lu", step);

		return -ENOEXEC;
	}

	result = bt_vcp_vol_rend_set_step(step);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_state_get(const struct shell *sh, size_t argc,
				      char **argv)
{
	int result = bt_vcp_vol_rend_get_state();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_flags_get(const struct shell *sh, size_t argc,
				      char **argv)
{
	int result = bt_vcp_vol_rend_get_flags();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_volume_down(const struct shell *sh, size_t argc,
					char **argv)
{
	int result = bt_vcp_vol_rend_vol_down();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_volume_up(const struct shell *sh, size_t argc,
				      char **argv)

{
	int result = bt_vcp_vol_rend_vol_up();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_unmute_volume_down(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result = bt_vcp_vol_rend_unmute_vol_down();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_unmute_volume_up(const struct shell *sh,
					     size_t argc, char **argv)
{
	int result = bt_vcp_vol_rend_unmute_vol_up();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_volume_set(const struct shell *sh, size_t argc,
				       char **argv)

{
	unsigned long volume;
	int result = 0;

	volume = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse volume: %d", result);

		return -ENOEXEC;
	}

	if (volume > UINT8_MAX) {
		shell_error(sh, "Invalid volume %lu", volume);

		return -ENOEXEC;
	}

	result = bt_vcp_vol_rend_set_vol(volume);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_unmute(const struct shell *sh, size_t argc,
				   char **argv)
{
	int result = bt_vcp_vol_rend_unmute();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_mute(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result = bt_vcp_vol_rend_mute();

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_VCP_VOL_REND_VOCS)
static int cmd_vcp_vol_rend_vocs_state_get(const struct shell *sh, size_t argc,
					   char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.vocs_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_vocs_state_get(vcp_included.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_vocs_location_get(const struct shell *sh,
					      size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.vocs_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_vocs_location_get(vcp_included.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_vocs_location_set(const struct shell *sh,
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
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_vocs_offset_set(const struct shell *sh, size_t argc,
					    char **argv)
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

	if (offset > BT_VOCS_MAX_OFFSET || offset < BT_VOCS_MIN_OFFSET) {
		shell_error(sh, "Offset shall be %d-%d, was %ld",
			    BT_VOCS_MIN_OFFSET, BT_VOCS_MAX_OFFSET, offset);
		return -ENOEXEC;
	}

	result = bt_vocs_state_set(vcp_included.vocs[index], offset);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_vocs_output_description_get(const struct shell *sh,
							size_t argc,
							char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.vocs_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_vocs_description_get(vcp_included.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_vocs_output_description_set(const struct shell *sh,
							size_t argc,
							char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.vocs_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_vocs_description_set(vcp_included.vocs[index], argv[2]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_VCP_VOL_REND_VOCS */

#if defined(CONFIG_BT_VCP_VOL_REND_AICS)
static int cmd_vcp_vol_rend_aics_input_state_get(const struct shell *sh,
						 size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_state_get(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_gain_setting_get(const struct shell *sh,
						  size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_gain_setting_get(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_input_type_get(const struct shell *sh,
						size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_type_get(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_input_status_get(const struct shell *sh,
						  size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_status_get(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_input_unmute(const struct shell *sh,
					      size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_unmute(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_input_mute(const struct shell *sh, size_t argc,
					    char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_mute(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_manual_input_gain_set(const struct shell *sh,
						       size_t argc,
						       char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_manual_gain_set(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_auto_input_gain_set(const struct shell *sh,
						     size_t argc,
						     char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_automatic_gain_set(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_gain_set(const struct shell *sh, size_t argc,
					  char **argv)
{
	unsigned long index;
	int result = 0;
	long gain;

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

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(sh, "Gain shall be %d-%d, was %ld",
			    INT8_MIN, INT8_MAX, gain);

		return -ENOEXEC;
	}

	result = bt_aics_gain_set(vcp_included.aics[index], gain);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_input_description_get(const struct shell *sh,
						       size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}
	result = bt_aics_description_get(vcp_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcp_vol_rend_aics_input_description_set(const struct shell *sh,
						       size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Failed to parse index: %d", result);

		return -ENOEXEC;
	}

	if (index > vcp_included.aics_cnt) {
		shell_error(sh, "Invalid index %lu", index);

		return -ENOEXEC;
	}

	result = bt_aics_description_set(vcp_included.aics[index], argv[2]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_VCP_VOL_REND_AICS */

static int cmd_vcp_vol_rend(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(vcp_vol_rend_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks "
		      "[step=<uint>] [mute=<bool>] [volume=<uint>]",
		      cmd_vcp_vol_rend_init, 1, 3),
	SHELL_CMD_ARG(state_get, NULL,
		      "Get volume state of the VCP server. Should be done "
		      "before sending any control messages",
		      cmd_vcp_vol_rend_state_get, 1, 0),
	SHELL_CMD_ARG(flags_get, NULL,
		      "Read volume flags",
		      cmd_vcp_vol_rend_flags_get, 1, 0),
	SHELL_CMD_ARG(volume_down, NULL,
		      "Turn the volume down",
		      cmd_vcp_vol_rend_volume_down, 1, 0),
	SHELL_CMD_ARG(volume_up, NULL,
		      "Turn the volume up",
		      cmd_vcp_vol_rend_volume_up, 1, 0),
	SHELL_CMD_ARG(unmute_volume_down, NULL,
		      "Turn the volume down, and unmute",
		      cmd_vcp_vol_rend_unmute_volume_down, 1, 0),
	SHELL_CMD_ARG(unmute_volume_up, NULL,
		      "Turn the volume up, and unmute",
		      cmd_vcp_vol_rend_unmute_volume_up, 1, 0),
	SHELL_CMD_ARG(volume_set, NULL,
		      "Set an absolute volume <volume>",
		      cmd_vcp_vol_rend_volume_set, 2, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute",
		      cmd_vcp_vol_rend_unmute, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute",
		      cmd_vcp_vol_rend_mute, 1, 0),
	SHELL_CMD_ARG(step, NULL,
		      "Set step size",
		      cmd_vcp_vol_rend_volume_step, 2, 0),
#if defined(CONFIG_BT_VCP_VOL_REND_VOCS)
	SHELL_CMD_ARG(vocs_state_get, NULL,
		      "Get the offset state of a VOCS instance <inst_index>",
		      cmd_vcp_vol_rend_vocs_state_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_get, NULL,
		      "Get the location of a VOCS instance <inst_index>",
		      cmd_vcp_vol_rend_vocs_location_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_set, NULL,
		      "Set the location of a VOCS instance <inst_index> "
		      "<location>",
		      cmd_vcp_vol_rend_vocs_location_set, 3, 0),
	SHELL_CMD_ARG(vocs_offset_set, NULL,
		      "Set the offset for a VOCS instance <inst_index> "
		      "<offset>",
		      cmd_vcp_vol_rend_vocs_offset_set, 3, 0),
	SHELL_CMD_ARG(vocs_output_description_get, NULL,
		      "Get the output description of a VOCS instance "
		      "<inst_index>",
		      cmd_vcp_vol_rend_vocs_output_description_get, 2, 0),
	SHELL_CMD_ARG(vocs_output_description_set, NULL,
		      "Set the output description of a VOCS instance "
		      "<inst_index> <description>",
		      cmd_vcp_vol_rend_vocs_output_description_set, 3, 0),
#endif /* CONFIG_BT_VCP_VOL_REND_VOCS */
#if defined(CONFIG_BT_VCP_VOL_REND_AICS)
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_vcp_vol_rend_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_vcp_vol_rend_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_vcp_vol_rend_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_vcp_vol_rend_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_vcp_vol_rend_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_vcp_vol_rend_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_vcp_vol_rend_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_vcp_vol_rend_aics_auto_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain in dB of a AICS instance <inst_index> "
		      "<gain (-128 to 127)>",
		      cmd_vcp_vol_rend_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_vcp_vol_rend_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_vcp_vol_rend_aics_input_description_set, 3, 0),
#endif /* CONFIG_BT_VCP_VOL_REND_AICS */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(vcp_vol_rend, &vcp_vol_rend_cmds,
		       "Bluetooth VCP Volume Renderer shell commands",
		       cmd_vcp_vol_rend, 1, 1);
