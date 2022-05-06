/** @file
 *  @brief Bluetooth VCS server shell.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/vcs.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "bt.h"

static struct bt_vcs *vcs;
static struct bt_vcs_included vcs_included;

static void vcs_state_cb(struct bt_vcs *vcs, int err, uint8_t volume,
			 uint8_t mute)
{
	if (err) {
		shell_error(ctx_shell, "VCS state get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS volume %u, mute %u", volume, mute);
	}
}

static void vcs_flags_cb(struct bt_vcs *vcs, int err, uint8_t flags)
{
	if (err) {
		shell_error(ctx_shell, "VCS flags get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "VCS flags 0x%02X", flags);
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

static struct bt_vcs_cb vcs_cbs = {
	.state = vcs_state_cb,
	.flags = vcs_flags_cb,
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

static int cmd_vcs_init(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	struct bt_vcs_register_param vcs_param;
	char input_desc[CONFIG_BT_VCS_AICS_INSTANCE_COUNT][16];
	char output_desc[CONFIG_BT_VCS_VOCS_INSTANCE_COUNT][16];

	if (!ctx_shell) {
		ctx_shell = sh;
	}

	memset(&vcs_param, 0, sizeof(vcs_param));

	for (int i = 0; i < ARRAY_SIZE(vcs_param.vocs_param); i++) {
		vcs_param.vocs_param[i].location_writable = true;
		vcs_param.vocs_param[i].desc_writable = true;
		snprintf(output_desc[i], sizeof(output_desc[i]),
			 "Output %d", i + 1);
		vcs_param.vocs_param[i].output_desc = output_desc[i];
		vcs_param.vocs_param[i].cb = &vocs_cbs;
	}

	for (int i = 0; i < ARRAY_SIZE(vcs_param.aics_param); i++) {
		vcs_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		vcs_param.aics_param[i].description = input_desc[i];
		vcs_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		vcs_param.aics_param[i].status = true;
		vcs_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		vcs_param.aics_param[i].units = 1;
		vcs_param.aics_param[i].min_gain = -100;
		vcs_param.aics_param[i].max_gain = 100;
		vcs_param.aics_param[i].cb = &aics_cbs;
	}

	vcs_param.step = 1;
	vcs_param.mute = BT_VCS_STATE_UNMUTED;
	vcs_param.volume = 100;

	vcs_param.cb = &vcs_cbs;

	result = bt_vcs_register(&vcs_param, &vcs);
	if (result) {
		shell_print(sh, "Fail: %d", result);
		return result;
	}

	result = bt_vcs_included_get(vcs, &vcs_included);
	if (result != 0) {
		shell_error(sh, "Failed to get included services: %d", result);
		return result;
	}

	return result;
}

static int cmd_vcs_volume_step(const struct shell *sh, size_t argc,
			       char **argv)
{
	int result;
	int step = strtol(argv[1], NULL, 0);

	if (step > UINT8_MAX || step == 0) {
		shell_error(sh, "Step size out of range; 1-255, was %u",
			    step);
		return -ENOEXEC;
	}

	result = bt_vcs_vol_step_set(step);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_state_get(const struct shell *sh, size_t argc,
			     char **argv)
{
	int result = bt_vcs_vol_get(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_flags_get(const struct shell *sh, size_t argc,
			     char **argv)
{
	int result = bt_vcs_flags_get(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_volume_down(const struct shell *sh, size_t argc,
			       char **argv)
{
	int result = bt_vcs_vol_down(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_volume_up(const struct shell *sh, size_t argc,
			     char **argv)

{
	int result = bt_vcs_vol_up(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_unmute_volume_down(const struct shell *sh, size_t argc,
				      char **argv)
{
	int result = bt_vcs_unmute_vol_down(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_unmute_volume_up(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result = bt_vcs_unmute_vol_up(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_volume_set(const struct shell *sh, size_t argc,
			      char **argv)

{
	int result;
	int volume = strtol(argv[1], NULL, 0);

	if (volume > UINT8_MAX) {
		shell_error(sh, "Volume shall be 0-255, was %d", volume);
		return -ENOEXEC;
	}

	result = bt_vcs_vol_set(vcs, volume);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_unmute(const struct shell *sh, size_t argc, char **argv)
{
	int result = bt_vcs_unmute(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_mute(const struct shell *sh, size_t argc, char **argv)
{
	int result = bt_vcs_mute(vcs);

	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_vocs_state_get(const struct shell *sh, size_t argc,
				  char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	/* TODO: For here, and the following VOCS and AICS, default index to 0 */

	if (index > CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
		shell_error(sh, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_VCS_VOCS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_state_get(NULL, vcs_included.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_vocs_location_get(const struct shell *sh, size_t argc,
				     char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
		shell_error(sh, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_VCS_VOCS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_location_get(NULL, vcs_included.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_vocs_location_set(const struct shell *sh, size_t argc,
				     char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int location = strtol(argv[2], NULL, 0);

	if (index > CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
		shell_error(sh, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_VCS_VOCS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}
	if (location > UINT16_MAX || location < 0) {
		shell_error(sh, "Invalid location (%u-%u), was %u",
			    0, UINT16_MAX, location);
		return -ENOEXEC;

	}

	result = bt_vcs_vocs_location_set(NULL, vcs_included.vocs[index],
					  location);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_vocs_offset_set(const struct shell *sh, size_t argc,
				   char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int offset = strtol(argv[2], NULL, 0);

	if (index > CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
		shell_error(sh, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_VCS_VOCS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	if (offset > BT_VOCS_MAX_OFFSET || offset < BT_VOCS_MIN_OFFSET) {
		shell_error(sh, "Offset shall be %d-%d, was %d",
			    BT_VOCS_MIN_OFFSET, BT_VOCS_MAX_OFFSET, offset);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_state_set(NULL, vcs_included.vocs[index], offset);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_vocs_output_description_get(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index > CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
		shell_error(sh, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_VCS_VOCS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_description_get(NULL, vcs_included.vocs[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_vocs_output_description_set(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (index > CONFIG_BT_VCS_VOCS_INSTANCE_COUNT) {
		shell_error(sh, "Index out of range; 0-%u, was %u",
			    CONFIG_BT_VCS_VOCS_INSTANCE_COUNT, index);
		return -ENOEXEC;
	}

	result = bt_vcs_vocs_description_set(NULL, vcs_included.vocs[index],
					     description);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_state_get(const struct shell *sh, size_t argc,
					char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_state_get(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_gain_setting_get(const struct shell *sh, size_t argc,
					 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_gain_setting_get(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_type_get(const struct shell *sh, size_t argc,
				       char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_type_get(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_status_get(const struct shell *sh, size_t argc,
					 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_status_get(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_unmute(const struct shell *sh, size_t argc,
				     char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_unmute(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_mute(const struct shell *sh, size_t argc,
				   char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_mute(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_manual_input_gain_set(const struct shell *sh,
					      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_manual_gain_set(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_automatic_input_gain_set(const struct shell *sh,
						 size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_automatic_gain_set(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_gain_set(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int gain = strtol(argv[2], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(sh, "Offset shall be %d-%d, was %d",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_gain_set(NULL, vcs_included.aics[index], gain);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_description_get(const struct shell *sh,
					      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_description_get(NULL, vcs_included.aics[index]);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs_aics_input_description_set(const struct shell *sh,
					      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (index >= vcs_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    vcs_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_vcs_aics_description_set(NULL, vcs_included.aics[index],
					     description);
	if (result) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_vcs(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(vcs_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_vcs_init, 1, 0),
	SHELL_CMD_ARG(state_get, NULL,
		      "Get volume state of the VCS server. Should be done "
		      "before sending any control messages",
		      cmd_vcs_state_get, 1, 0),
	SHELL_CMD_ARG(flags_get, NULL,
		      "Read volume flags",
		      cmd_vcs_flags_get, 1, 0),
	SHELL_CMD_ARG(volume_down, NULL,
		      "Turn the volume down",
		      cmd_vcs_volume_down, 1, 0),
	SHELL_CMD_ARG(volume_up, NULL,
		      "Turn the volume up",
		      cmd_vcs_volume_up, 1, 0),
	SHELL_CMD_ARG(unmute_volume_down, NULL,
		      "Turn the volume down, and unmute",
		      cmd_vcs_unmute_volume_down, 1, 0),
	SHELL_CMD_ARG(unmute_volume_up, NULL,
		      "Turn the volume up, and unmute",
		      cmd_vcs_unmute_volume_up, 1, 0),
	SHELL_CMD_ARG(volume_set, NULL,
		      "Set an absolute volume <volume>",
		      cmd_vcs_volume_set, 2, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute",
		      cmd_vcs_unmute, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute",
		      cmd_vcs_mute, 1, 0),
	SHELL_CMD_ARG(step, NULL,
		      "Set step size",
		      cmd_vcs_volume_step, 2, 0),
	SHELL_CMD_ARG(vocs_state_get, NULL,
		      "Get the offset state of a VOCS instance <inst_index>",
		      cmd_vcs_vocs_state_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_get, NULL,
		      "Get the location of a VOCS instance <inst_index>",
		      cmd_vcs_vocs_location_get, 2, 0),
	SHELL_CMD_ARG(vocs_location_set, NULL,
		      "Set the location of a VOCS instance <inst_index> "
		      "<location>",
		      cmd_vcs_vocs_location_set, 3, 0),
	SHELL_CMD_ARG(vocs_offset_set, NULL,
		      "Set the offset for a VOCS instance <inst_index> "
		      "<offset>",
		      cmd_vcs_vocs_offset_set, 3, 0),
	SHELL_CMD_ARG(vocs_output_description_get, NULL,
		      "Get the output description of a VOCS instance "
		      "<inst_index>",
		      cmd_vcs_vocs_output_description_get, 2, 0),
	SHELL_CMD_ARG(vocs_output_description_set, NULL,
		      "Set the output description of a VOCS instance "
		      "<inst_index> <description>",
		      cmd_vcs_vocs_output_description_set, 3, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_vcs_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_vcs_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_vcs_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_vcs_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_vcs_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_vcs_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_vcs_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_vcs_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain in dB of a AICS instance <inst_index> "
		      "<gain (-128 to 127)>",
		      cmd_vcs_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_vcs_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_vcs_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(vcs, &vcs_cmds, "Bluetooth VCS shell commands",
		       cmd_vcs, 1, 1);
