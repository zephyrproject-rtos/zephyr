/** @file
 *  @brief Bluetooth MICP shell.
 *
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/aics.h>
#include <zephyr/bluetooth/audio/micp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include "host/shell/bt.h"

static void micp_mic_dev_mute_cb(uint8_t mute)
{
	shell_print(ctx_shell, "Mute value %u", mute);
}

static struct bt_micp_mic_dev_cb micp_mic_dev_cbs = {
	.mute = micp_mic_dev_mute_cb,
};

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
static struct bt_micp_included micp_included;

static void micp_mic_dev_aics_state_cb(struct bt_aics *inst, int err,
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
static void micp_mic_dev_aics_gain_setting_cb(struct bt_aics *inst, int err,
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
static void micp_mic_dev_aics_input_type_cb(struct bt_aics *inst, int err,
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
static void micp_mic_dev_aics_status_cb(struct bt_aics *inst, int err,
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
static void micp_mic_dev_aics_description_cb(struct bt_aics *inst, int err,
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

static struct bt_aics_cb aics_cb = {
	.state = micp_mic_dev_aics_state_cb,
	.gain_setting = micp_mic_dev_aics_gain_setting_cb,
	.type = micp_mic_dev_aics_input_type_cb,
	.status = micp_mic_dev_aics_status_cb,
	.description = micp_mic_dev_aics_description_cb,
};
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

static int cmd_micp_mic_dev_param(const struct shell *sh, size_t argc,
				  char **argv)
{
	int result;
	struct bt_micp_mic_dev_register_param micp_param;

	if (ctx_shell == NULL) {
		ctx_shell = sh;
	}

	(void)memset(&micp_param, 0, sizeof(micp_param));

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	char input_desc[CONFIG_BT_MICP_MIC_DEV_AICS_INSTANCE_COUNT][16];

	for (int i = 0; i < ARRAY_SIZE(micp_param.aics_param); i++) {
		micp_param.aics_param[i].desc_writable = true;
		snprintf(input_desc[i], sizeof(input_desc[i]),
			 "Input %d", i + 1);
		micp_param.aics_param[i].description = input_desc[i];
		micp_param.aics_param[i].type = BT_AICS_INPUT_TYPE_UNSPECIFIED;
		micp_param.aics_param[i].status = true;
		micp_param.aics_param[i].gain_mode = BT_AICS_MODE_MANUAL;
		micp_param.aics_param[i].units = 1;
		micp_param.aics_param[i].min_gain = -100;
		micp_param.aics_param[i].max_gain = 100;
		micp_param.aics_param[i].cb = &aics_cb;
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	micp_param.cb = &micp_mic_dev_cbs;

	result = bt_micp_mic_dev_register(&micp_param);
	if (result != 0) {
		shell_error(sh, "MICP register failed: %d", result);
		return result;
	}

	shell_print(sh, "MICP initialized: %d", result);

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	result = bt_micp_mic_dev_included_get(&micp_included);
	if (result != 0) {
		shell_error(sh, "MICP get failed: %d", result);
	}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

	return result;
}

static int cmd_micp_mic_dev_mute_get(const struct shell *sh, size_t argc,
				     char **argv)
{
	int result = bt_micp_mic_dev_mute_get();

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_mute(const struct shell *sh, size_t argc,
				 char **argv)
{
	int result = bt_micp_mic_dev_mute();

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_unmute(const struct shell *sh, size_t argc,
				   char **argv)
{
	int result = bt_micp_mic_dev_unmute();

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_mute_disable(const struct shell *sh, size_t argc,
					 char **argv)
{
	int result = bt_micp_mic_dev_mute_disable();

	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
static int cmd_micp_mic_dev_aics_deactivate(const struct shell *sh, size_t argc,
					    char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_deactivate(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_activate(const struct shell *sh, size_t argc,
					  char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_activate(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_state_get(const struct shell *sh,
						 size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_state_get(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_gain_setting_get(const struct shell *sh,
						  size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_gain_setting_get(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_type_get(const struct shell *sh,
						size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_type_get(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_status_get(const struct shell *sh,
						  size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_status_get(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_unmute(const struct shell *sh,
					      size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_unmute(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_mute(const struct shell *sh, size_t argc,
					    char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_mute(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_manual_input_gain_set(const struct shell *sh,
						       size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_manual_gain_set(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_automatic_input_gain_set(const struct shell *sh,
							  size_t argc,
							  char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_automatic_gain_set(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_gain_set(const struct shell *sh, size_t argc,
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

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

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

	result = bt_aics_gain_set(micp_included.aics[index], gain);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_description_get(const struct shell *sh,
						       size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_description_get(micp_included.aics[index]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_micp_mic_dev_aics_input_description_set(const struct shell *sh,
						       size_t argc, char **argv)
{
	unsigned long index;
	int result = 0;

	index = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse index: %d", result);

		return -ENOEXEC;
	}

	if (index >= micp_included.aics_cnt) {
		shell_error(sh, "Index shall be less than %u, was %lu",
			    micp_included.aics_cnt, index);

		return -ENOEXEC;
	}

	result = bt_aics_description_set(micp_included.aics[index], argv[2]);
	if (result != 0) {
		shell_error(sh, "Fail: %d", result);
	}

	return result;
}
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */

static int cmd_micp_mic_dev(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(micp_mic_dev_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_micp_mic_dev_param, 1, 0),
	SHELL_CMD_ARG(mute_get, NULL,
		      "Get the mute state",
		      cmd_micp_mic_dev_mute_get, 1, 0),
	SHELL_CMD_ARG(mute, NULL,
		      "Mute the MICP server",
		      cmd_micp_mic_dev_mute, 1, 0),
	SHELL_CMD_ARG(unmute, NULL,
		      "Unmute the MICP server",
		      cmd_micp_mic_dev_unmute, 1, 0),
	SHELL_CMD_ARG(mute_disable, NULL,
		      "Disable the MICP mute",
		      cmd_micp_mic_dev_mute_disable, 1, 0),
#if defined(CONFIG_BT_MICP_MIC_DEV_AICS)
	SHELL_CMD_ARG(aics_deactivate, NULL,
		      "Deactivates a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_deactivate, 2, 0),
	SHELL_CMD_ARG(aics_activate, NULL,
		      "Activates a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_activate, 2, 0),
	SHELL_CMD_ARG(aics_input_state_get, NULL,
		      "Get the input state of a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_input_state_get, 2, 0),
	SHELL_CMD_ARG(aics_gain_setting_get, NULL,
		      "Get the gain settings of a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_gain_setting_get, 2, 0),
	SHELL_CMD_ARG(aics_input_type_get, NULL,
		      "Get the input type of a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_input_type_get, 2, 0),
	SHELL_CMD_ARG(aics_input_status_get, NULL,
		      "Get the input status of a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_input_status_get, 2, 0),
	SHELL_CMD_ARG(aics_input_unmute, NULL,
		      "Unmute the input of a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_input_unmute, 2, 0),
	SHELL_CMD_ARG(aics_input_mute, NULL,
		      "Mute the input of a AICS instance <inst_index>",
		      cmd_micp_mic_dev_aics_input_mute, 2, 0),
	SHELL_CMD_ARG(aics_manual_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to manual "
		      "<inst_index>",
		      cmd_micp_mic_dev_aics_manual_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_automatic_input_gain_set, NULL,
		      "Set the gain mode of a AICS instance to automatic "
		      "<inst_index>",
		      cmd_micp_mic_dev_aics_automatic_input_gain_set, 2, 0),
	SHELL_CMD_ARG(aics_gain_set, NULL,
		      "Set the gain in dB of a AICS instance <inst_index> "
		      "<gain (-128 to 127)>",
		      cmd_micp_mic_dev_aics_gain_set, 3, 0),
	SHELL_CMD_ARG(aics_input_description_get, NULL,
		      "Get the input description of a AICS instance "
		      "<inst_index>",
		      cmd_micp_mic_dev_aics_input_description_get, 2, 0),
	SHELL_CMD_ARG(aics_input_description_set, NULL,
		      "Set the input description of a AICS instance "
		      "<inst_index> <description>",
		      cmd_micp_mic_dev_aics_input_description_set, 3, 0),
#endif /* CONFIG_BT_MICP_MIC_DEV_AICS */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(micp_mic_dev, &micp_mic_dev_cmds,
		       "Bluetooth MICP Microphone Device shell commands", cmd_micp_mic_dev, 1, 1);
