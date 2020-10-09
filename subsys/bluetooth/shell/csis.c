/**
 * @file
 * @brief Shell APIs for Bluetooth CSIS
 *
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr.h>
#include <zephyr/types.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <bluetooth/gatt.h>
#include <bluetooth/bluetooth.h>
#include "../host/audio/csis.h"

static int cmd_csis_advertise(const struct shell *shell, size_t argc,
				     char *argv[])
{
	if (!strcmp(argv[1], "off")) {
		if (bt_csis_advertise(false) != 0) {
			shell_error(shell, "Failed to stop advertising");
			return -ENOEXEC;
		}
		shell_print(shell, "Advertising stopped");
	} else if (!strcmp(argv[1], "on")) {
		if (bt_csis_advertise(true) != 0) {
			shell_error(shell, "Failed to start advertising");
			return -ENOEXEC;
		}
		shell_print(shell, "Advertising started");
	} else {
		shell_error(shell, "Invalid argument: %s", argv[1]);
		return -ENOEXEC;
	}

	return 0;
}

static int cmd_csis_update_psri(const struct shell *shell, size_t argc,
				       char *argv[])
{
	int err;

	if (bt_csis_advertise(false) != 0) {
		shell_error(shell,
			    "Failed to stop advertising - psri not updated");
		return -ENOEXEC;
	}
	err = bt_csis_advertise(true);
	if (err != 0) {
		shell_error(shell,
			    "Failed to start advertising  - psri not updated");
		return -ENOEXEC;
	}

	shell_print(shell, "PSRI and optionally RPA updated");

	return 0;
}

static int cmd_csis_print_sirk(const struct shell *shell, size_t argc,
				      char *argv[])
{
	bt_csis_print_sirk();
	return 0;
}

static int cmd_csis_lock(const struct shell *shell, size_t argc,
				char *argv[])
{
	if (bt_csis_lock(true, false) != 0) {
		shell_error(shell, "Failed to set lock");
		return -ENOEXEC;
	}

	shell_print(shell, "Set locked");

	return 0;
}

static int cmd_csis_release(const struct shell *shell, size_t argc,
				   char *argv[])
{
	bool force = false;

	if (argc > 1) {
		if (strcmp(argv[1], "force") == 0) {
			force = true;
		} else {
			shell_error(shell, "Unknown parameter: %s", argv[1]);
			return -ENOEXEC;
		}
	}

	if (bt_csis_lock(false, force) != 0) {
		shell_error(shell, "Failed to release lock");
		return -ENOEXEC;
	}

	shell_print(shell, "Set release");

	return 0;
}
static int cmd_csis(const struct shell *shell, size_t argc, char **argv)
{
	shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}


SHELL_STATIC_SUBCMD_SET_CREATE(csis_cmds,
	SHELL_CMD_ARG(advertise, NULL,
		      "Start/stop advertising CSIS PSRIs <on/off>",
		      cmd_csis_advertise, 2, 0),
	SHELL_CMD_ARG(update_psri, NULL,
		      "Update the advertised PSRI",
		      cmd_csis_update_psri, 1, 0),
	SHELL_CMD_ARG(lock, NULL,
		      "Lock the set",
		      cmd_csis_lock, 1, 0),
	SHELL_CMD_ARG(release, NULL,
		      "Release the set [force]",
		      cmd_csis_release, 1, 1),
	SHELL_CMD_ARG(print_sirk, NULL,
		      "Print the currently used SIRK",
			  cmd_csis_print_sirk, 1, 0),
			  SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(csis, &csis_cmds, "Bluetooth CSIS shell commands",
		       cmd_csis, 1, 1);
