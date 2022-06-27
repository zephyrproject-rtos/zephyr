/** @file
 *  @brief Bluetooth MICS shell.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/mics.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "bt.h"

static struct bt_mics *mics;
static struct bt_mics_included mics_included;

static void mics_mute_cb(struct bt_mics *mics, int err, uint8_t mute)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute value %u", mute);
	}
}

static void mics_aics_state_cb(struct bt_aics *inst, int err, int8_t gain,
			       uint8_t mute, uint8_t mode)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS state get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p state gain %d, mute %u, "
			    "mode %u", inst, gain, mute, mode);
	}

}
static void mics_aics_gain_setting_cb(struct bt_aics *inst, int err,
				      uint8_t units, int8_t minimum,
				      int8_t maximum)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS gain settings get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p gain settings units %u, "
			    "min %d, max %d", inst, units, minimum,
			    maximum);
	}

}
static void mics_aics_input_type_cb(struct bt_aics *inst, int err,
				    uint8_t input_type)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS input type get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p input type %u",
			    inst, input_type);
	}

}
static void mics_aics_status_cb(struct bt_aics *inst, int err, bool active)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS status get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p status %s",
			    inst, active ? "active" : "inactive");
	}

}
static void mics_aics_description_cb(struct bt_aics *inst, int err,
				     char *description)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS description get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p description %s",
			    inst, description);
	}
}

static struct bt_mics_cb mics_cbs = {
	.mute = mics_mute_cb,
};

static struct bt_aics_cb aics_cb = {
	.state = mics_aics_state_cb,
	.gain_setting = mics_aics_gain_setting_cb,
	.type = mics_aics_input_type_cb,
	.status = mics_aics_status_cb,
	.description = mics_aics_description_cb,
};

static int cmd_mics_param(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	struct bt_mics_register_param mics_param;
	char input_desc[CONFIG_BT_MICS_AICS_INSTANCE_COUNT][16];

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	(void)memset(&mics_param, 0, sizeof(mics_param));

	for (int i = 0; i < ARRAY_SIZE(mics_param.aics_param); i++) {
		mics_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		mics_param.aics_param[i].description = input_desc[i];
		mics_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		mics_param.aics_param[i].status = true;
		mics_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		mics_param.aics_param[i].units = 1;
		mics_param.aics_param[i].min_gain = -100;
		mics_param.aics_param[i].max_gain = 100;
		mics_param.aics_param[i].cb = &aics_cb;
	}

	mics_param.cb = &mics_cbs;

	result = bt_mics_register(&mics_param, &mics);
	if (result != 0) {
		shell_error(sh, "MICS register failed: %d", result);
		return result;
	}

	shell_print(sh, "MICS initialized: %d", result);

	result = bt_mics_included_get(NULL, &mics_included);
	if (result != 0) {
		shell_error(sh, "MICS get failed: %d", result);
	}

	return result;
}

static int cmd_mics_mute_get(const struct shell *sh, size_t argc, char **argv)
{
	int result = bt_mics_mute_get(NULL);

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_mute(const struct shell *sh, size_t argc, char **argv)
{
	int result = bt_mics_mute(NULL);

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_unmute(const struct shell *sh, size_t argc, char **argv)
{
	int result = bt_mics_unmute(NULL);

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_mute_disable(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result = bt_mics_mute_disable(mics);

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_deactivate(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_deactivate(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_activate(const struct shell *sh, size_t argc,
				  char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_activate(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_state_get(const struct shell *sh, size_t argc,
					 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_state_get(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_gain_setting_get(const struct shell *sh, size_t argc,
					  char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_gain_setting_get(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_type_get(const struct shell *sh, size_t argc,
					char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_type_get(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_status_get(const struct shell *sh, size_t argc,
					  char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_status_get(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_unmute(const struct shell *sh, size_t argc,
				      char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_unmute(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_mute(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_mute(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_manual_input_gain_set(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_manual_gain_set(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_automatic_input_gain_set(const struct shell *sh,
						  size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_automatic_gain_set(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_gain_set(const struct shell *sh, size_t argc,
				  char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int gain = strtol(argv[2], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(sh, "Offset shall be %d-%d, was %d",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	result = bt_mics_aics_gain_set(NULL, mics_included.aics[index], gain);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_description_get(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_description_get(NULL, mics_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_aics_input_description_set(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	result = bt_mics_aics_description_set(NULL, mics_included.aics[index],
					      description);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mics_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_mics_param, 1, 0),
	SHELL_CMD_ARG(mute_get, NULL,
		      "Get the mute state",
		      cmd_mics_mute_get, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute the MICS server",
		      cmd_mics_mute, 1, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute the MICS server",
		      cmd_mics_unmute, 1, 0),
	SHELL_CMD_ARG(mute_disable, NULL,
		      "Disable the MICS mute",
		      cmd_mics_mute_disable, 1, 0),
	SHELL_CMD_ARG(aics_deactivate, NULL,
		      "Deactivates a AICS instance <inst_index>",
		      cmd_mics_aics_deactivate, 2, 0),
	SHELL_CMD_ARG(aics_activate, NULL,
		      "Activates a AICS instance <inst_index>",
		      cmd_mics_aics_activate, 2, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_mics_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_mics_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_mics_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_mics_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_mics_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_mics_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_mics_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_mics_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain in dB of a AICS instance <inst_index> "
		      "<gain (-128 to 127)>",
		      cmd_mics_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Get the input description of a AICS instance "
		      "<inst_index>",
		      cmd_mics_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_mics_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mics, &mics_cmds, "Bluetooth MICS shell commands",
		       cmd_mics, 1, 1);
