/** @file
 *  @brief Bluetooth MICS client shell.
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

static void mics_discover_cb(struct bt_mics *mics, int err, uint8_t aics_count)
{
	if (err != 0) {
		shell_error(ctx_shell, "MICS discover failed (%d)", err);
	} else {
		shell_print(ctx_shell, "MICS discover done with %u AICS",
			    aics_count);

		if (bt_mics_included_get(mics, &mics_included) != 0) {
			shell_error(ctx_shell, "Could not get MICS context");
		}
	}
}

static void mics_mute_write_cb(struct bt_mics *mics, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute write failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute write completed");
	}
}


static void mics_unmute_write_cb(struct bt_mics *mics, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unmute write failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Unmute write completed");
	}
}

static void mics_aics_set_gain_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Set gain failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Gain set for inst %p", inst);
	}
}

static void mics_aics_unmute_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unmute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Unmuted inst %p", inst);
	}
}

static void mics_aics_mute_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Muted inst %p", inst);
	}
}

static void mics_aics_set_manual_mode_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "Set manual mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Manuel mode set for inst %p", inst);
	}
}

static void mics_aics_automatic_mode_cb(struct bt_aics *inst, int err)
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
	.discover = mics_discover_cb,
	.mute_write = mics_mute_write_cb,
	.unmute_write = mics_unmute_write_cb,
	.mute = mics_mute_cb,

	/* Audio Input Control Service */
	.aics_cb = {
		.state = mics_aics_state_cb,
		.gain_setting = mics_aics_gain_setting_cb,
		.type = mics_aics_input_type_cb,
		.status = mics_aics_status_cb,
		.description = mics_aics_description_cb,
		.set_gain = mics_aics_set_gain_cb,
		.unmute = mics_aics_unmute_cb,
		.mute = mics_aics_mute_cb,
		.set_manual_mode = mics_aics_set_manual_mode_cb,
		.set_auto_mode = mics_aics_automatic_mode_cb,
	}
};

static int cmd_mics_client_discover(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	result = bt_mics_client_cb_register(&mics_cbs);
	if (result != 0) {
		shell_print(sh, "Failed to register callbacks: %d", result);
	}

	if (default_conn == NULL) {
		return -ENOTCONN;
	}

	result = bt_mics_discover(default_conn, &mics);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_mute_get(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_mute_get(mics);

	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_mute(const struct shell *sh, size_t argc,
				char **argv)
{
	int result;

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_mute(mics);

	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_unmute(const struct shell *sh, size_t argc,
				  char **argv)
{
	int result;

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_unmute(mics);

	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_state_get(const struct shell *sh,
						size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_state_get(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_gain_setting_get(const struct shell *sh,
						 size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_gain_setting_get(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_type_get(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_type_get(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_status_get(const struct shell *sh,
						 size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_status_get(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_unmute(const struct shell *sh,
					     size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_unmute(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_mute(const struct shell *sh,
					   size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_mute(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_manual_input_gain_set(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_manual_gain_set(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_automatic_input_gain_set(const struct shell *sh,
							 size_t argc,
							 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_automatic_gain_set(mics,
						 mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_gain_set(const struct shell *sh, size_t argc,
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

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_gain_set(mics, mics_included.aics[index], gain);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_description_get(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= mics_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    mics_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_description_get(mics, mics_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client_aics_input_description_set(const struct shell *sh,
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

	if (mics == NULL) {
		return -ENOENT;
	}

	result = bt_mics_aics_description_set(mics, mics_included.aics[index],
					      description);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_mics_client(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mics_client_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover MICS on remote device",
		      cmd_mics_client_discover, 1, 0),
	SHELL_CMD_ARG(mute_get, NULL,
		      "Read the mute state of the MICS server.",
		      cmd_mics_client_mute_get, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute the MICS server",
		      cmd_mics_client_mute, 1, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute the MICS server",
		      cmd_mics_client_unmute, 1, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Read the input state of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Read the gain settings of a AICS instance <inst_index>",
		      cmd_mics_client_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Read the input type of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Read the input status of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_mics_client_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_mics_client_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_mics_client_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain of a AICS instance <inst_index> <gain>",
		      cmd_mics_client_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_mics_client_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_mics_client_aics_input_description_set, 3, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mics_client, &mics_client_cmds,
		       "Bluetooth MICS client shell commands",
		       cmd_mics_client, 1, 1);
