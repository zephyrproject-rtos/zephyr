/** @file
 *  @brief Bluetooth MICP Microphone Controller shell.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <stdio.h>

#include "bt.h"

static struct bt_micp_mic_ctlr *mic_ctlr;
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static struct bt_micp_included micp_included;
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

static void micp_mic_ctlr_discover_cb(struct bt_micp_mic_ctlr *mic_ctlr,
				      int err, uint8_t aics_count)
{
	if (err != 0) {
		shell_error(ctx_shell, "Discovery failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Discovery done with %u AICS",
			    aics_count);

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
		if (bt_micp_mic_ctlr_included_get(mic_ctlr,
						  &micp_included) != 0) {
			shell_error(ctx_shell, "Could not get included services");
		}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
	}
}

static void micp_mic_ctlr_mute_written_cb(struct bt_micp_mic_ctlr *mic_ctlr,
					  int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute write failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute write completed");
	}
}

static void micp_mic_ctlr_unmute_written_cb(struct bt_micp_mic_ctlr *mic_ctlr,
					    int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unmute write failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Unmute write completed");
	}
}

static void micp_mic_ctlr_mute_cb(struct bt_micp_mic_ctlr *mic_ctlr, int err,
				  uint8_t mute)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute get failed (%d)", err);
	} else {
		shell_print(ctx_shell, "Mute value %u", mute);
	}
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static struct bt_micp_included micp_included;

static void micp_mic_ctlr_aics_set_gain_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Set gain failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Gain set for inst %p", inst);
	}
}

static void micp_mic_ctlr_aics_unmute_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Unmute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Unmuted inst %p", inst);
	}
}

static void micp_mic_ctlr_aics_mute_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell, "Mute failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Muted inst %p", inst);
	}
}

static void micp_mic_ctlr_aics_set_manual_mode_cb(struct bt_aics *inst, int err)
{
	if (err != 0) {
		shell_error(ctx_shell,
			    "Set manual mode failed (%d) for inst %p",
			    err, inst);
	} else {
		shell_print(ctx_shell, "Manuel mode set for inst %p", inst);
	}
}

static void micp_mic_ctlr_aics_automatic_mode_cb(struct bt_aics *inst, int err)
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

static void micp_mic_ctlr_aics_state_cb(struct bt_aics *inst, int err,
					int8_t gain, uint8_t mute, uint8_t mode)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS state get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p state gain %d, mute %u, "
			    "mode %u", inst, gain, mute, mode);
	}

}

static void micp_mic_ctlr_aics_gain_setting_cb(struct bt_aics *inst, int err,
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

static void micp_mic_ctlr_aics_input_type_cb(struct bt_aics *inst, int err,
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

static void micp_mic_ctlr_aics_status_cb(struct bt_aics *inst, int err,
					 bool active)
{
	if (err != 0) {
		shell_error(ctx_shell, "AICS status get failed (%d) for "
			    "inst %p", err, inst);
	} else {
		shell_print(ctx_shell, "AICS inst %p status %s",
			    inst, active ? "active" : "inactive");
	}

}

static void micp_mic_ctlr_aics_description_cb(struct bt_aics *inst, int err,
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
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

static struct bt_micp_mic_ctlr_cb micp_cbs = {
	.discover = micp_mic_ctlr_discover_cb,
	.mute_written = micp_mic_ctlr_mute_written_cb,
	.unmute_written = micp_mic_ctlr_unmute_written_cb,
	.mute = micp_mic_ctlr_mute_cb,

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	/* Audio Input Control Service */
	.aics_cb = {
		.state = micp_mic_ctlr_aics_state_cb,
		.gain_setting = micp_mic_ctlr_aics_gain_setting_cb,
		.type = micp_mic_ctlr_aics_input_type_cb,
		.status = micp_mic_ctlr_aics_status_cb,
		.description = micp_mic_ctlr_aics_description_cb,
		.set_gain = micp_mic_ctlr_aics_set_gain_cb,
		.unmute = micp_mic_ctlr_aics_unmute_cb,
		.mute = micp_mic_ctlr_aics_mute_cb,
		.set_manual_mode = micp_mic_ctlr_aics_set_manual_mode_cb,
		.set_auto_mode = micp_mic_ctlr_aics_automatic_mode_cb,
	}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
};

static int cmd_micp_mic_ctlr_discover(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	result = bt_micp_mic_ctlr_cb_register(&micp_cbs);
	if (result != 0) {
		shell_print(sh, "Failed to register callbacks: %d", result);
	}

	if (default_conn == NULL) {
		return -ENOTCONN;
	}

	result = bt_micp_mic_ctlr_discover(default_conn, &mic_ctlr);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_mute_get(const struct shell *sh, size_t argc,
				    char **argv)
{
	int result;

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_micp_mic_ctlr_mute_get(mic_ctlr);

	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_mute(const struct shell *sh, size_t argc,
				char **argv)
{
	int result;

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_micp_mic_ctlr_mute(mic_ctlr);

	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_unmute(const struct shell *sh, size_t argc,
				  char **argv)
{
	int result;

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_micp_mic_ctlr_unmute(mic_ctlr);

	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
static int cmd_micp_mic_ctlr_aics_input_state_get(const struct shell *sh,
						size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_state_get(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_gain_setting_get(const struct shell *sh,
						 size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_gain_setting_get(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_input_type_get(const struct shell *sh,
					       size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_type_get(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_input_status_get(const struct shell *sh,
						 size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_status_get(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_input_unmute(const struct shell *sh,
					     size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_unmute(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_input_mute(const struct shell *sh,
					   size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_mute(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_manual_input_gain_set(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_manual_gain_set(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_automatic_input_gain_set(const struct shell *sh,
							 size_t argc,
							 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_automatic_gain_set(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_gain_set(const struct shell *sh, size_t argc,
					 char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	int gain = strtol(argv[2], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (gain > INT8_MAX || gain < INT8_MIN) {
		shell_error(sh, "Offset shall be %d-%d, was %d",
			    INT8_MIN, INT8_MAX, gain);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_gain_set(micp_included.aics[index], gain);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_input_description_get(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_description_get(micp_included.aics[index]);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_ctlr_aics_input_description_set(const struct shell *sh,
						      size_t argc, char **argv)
{
	int result;
	int index = strtol(argv[1], NULL, 0);
	char *description = argv[2];

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %u",
			    micp_included.aics_cnt, index);
		return -ENOEXEC;
	}

	if (mic_ctlr == NULL) {
		return -ENOENT;
	}

	result = bt_aics_description_set(micp_included.aics[index],
					      description);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */

static int cmd_micp_mic_ctlr(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(micp_mic_ctlr_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover MICS on remote device",
		      cmd_micp_mic_ctlr_discover, 1, 0),
	SHELL_CMD_ARG(mute_get, NULL,
		      "Read the mute state of the Microphone Device server.",
		      cmd_micp_mic_ctlr_mute_get, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute the Microphone Device server",
		      cmd_micp_mic_ctlr_mute, 1, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute the Microphone Device server",
		      cmd_micp_mic_ctlr_unmute, 1, 0),
#if defined(CONFIG_BT_MICP_MIC_CTLR_AICS)
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Read the input state of a AICS instance <inst_index>",
		      cmd_micp_mic_ctlr_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Read the gain settings of a AICS instance <inst_index>",
		      cmd_micp_mic_ctlr_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Read the input type of a AICS instance <inst_index>",
		      cmd_micp_mic_ctlr_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Read the input status of a AICS instance <inst_index>",
		      cmd_micp_mic_ctlr_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_micp_mic_ctlr_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_micp_mic_ctlr_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_micp_mic_ctlr_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_micp_mic_ctlr_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain of a AICS instance <inst_index> <gain>",
		      cmd_micp_mic_ctlr_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Read the input description of a AICS instance "
		      "<inst_index>",
		      cmd_micp_mic_ctlr_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_micp_mic_ctlr_aics_input_description_set, 3, 0),
#endif /* CONFIG_BT_MICP_MIC_CTLR_AICS */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(micp_mic_ctlr, &micp_mic_ctlr_cmds,
		       "Bluetooth Microphone Controller shell commands",
		       cmd_micp_mic_ctlr, 1, 1);
