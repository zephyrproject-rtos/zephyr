/** @file
 *  @brief Bluetooth Channel Sounding (CS) shell
 *
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/cs.h>
#include <errno.h>

#include "bt.h"

static int check_cs_sync_antenna_selection_input(uint16_t input)
{
	if (input != BT_CS_ANTENNA_SELECTION_OPT_ONE && input != BT_CS_ANTENNA_SELECTION_OPT_TWO &&
	    input != BT_CS_ANTENNA_SELECTION_OPT_THREE &&
	    input != BT_CS_ANTENNA_SELECTION_OPT_FOUR &&
	    input != BT_CS_ANTENNA_SELECTION_OPT_REPETITIVE &&
	    input != BT_CS_ANTENNA_SELECTION_OPT_NO_RECOMMENDATION) {
		return -EINVAL;
	}

	return 0;
}

static int cmd_read_remote_supported_capabilities(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	err = bt_cs_read_remote_supported_capabilities(default_conn);
	if (err) {
		shell_error(sh, "bt_cs_read_remote_supported_capabilities returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_set_default_settings(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	struct bt_cs_set_default_settings_param params;
	uint16_t antenna_input;
	int16_t tx_power_input;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	params.enable_initiator_role = shell_strtobool(argv[1], 10, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 1, Enable initiator role");
		return SHELL_CMD_HELP_PRINTED;
	}

	params.enable_reflector_role = shell_strtobool(argv[2], 10, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 2, Enable reflector role");
		return SHELL_CMD_HELP_PRINTED;
	}

	antenna_input = shell_strtoul(argv[3], 16, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 3, CS_SYNC antenna selection");
		return SHELL_CMD_HELP_PRINTED;
	}

	err = check_cs_sync_antenna_selection_input(antenna_input);
	if (err) {
		shell_help(sh);
		shell_error(sh, "CS_SYNC antenna selection input invalid");
		return SHELL_CMD_HELP_PRINTED;
	}

	tx_power_input = shell_strtol(argv[4], 10, &err);
	if (err) {
		shell_help(sh);
		shell_error(sh, "Could not parse input 4, Max TX power");
		return SHELL_CMD_HELP_PRINTED;
	}

	params.cs_sync_antenna_selection = antenna_input;
	params.max_tx_power = tx_power_input;

	err = bt_cs_set_default_settings(default_conn, &params);
	if (err) {
		shell_error(sh, "bt_cs_set_default_settings returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_read_remote_fae_table(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;

	if (default_conn == NULL) {
		shell_error(sh, "Conn handle error, at least one connection is required.");
		return -ENOEXEC;
	}

	err = bt_cs_read_remote_fae_table(default_conn);
	if (err) {
		shell_error(sh, "bt_cs_read_remote_fae_table returned error %d", err);
		return -ENOEXEC;
	}

	return 0;
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	cs_cmds,
	SHELL_CMD_ARG(
		read_remote_supported_capabilities, NULL,
		"<None>",
		cmd_read_remote_supported_capabilities, 1, 0),
	SHELL_CMD_ARG(
		set_default_settings, NULL,
		"<Enable initiator role: true, false> <Enable reflector role: true, false> "
		" <CS_SYNC antenna selection: 0x01 - 0x04, 0xFE, 0xFF> <Max TX power: -127 - 20>",
		cmd_set_default_settings, 5, 0),
	SHELL_CMD_ARG(
		read_remote_fae_table, NULL,
		"<None>",
		cmd_read_remote_fae_table, 1, 0),
	SHELL_SUBCMD_SET_END);

static int cmd_cs(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);

		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -EINVAL;
}

SHELL_CMD_ARG_REGISTER(cs, &cs_cmds, "Bluetooth CS shell commands", cmd_cs, 1, 1);
