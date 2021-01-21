/**
 * @file
 * @brief Shell APIs for Bluetooth BASS
 *
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
#include "../host/audio/bass.h"

static int cmd_bass_synced(const struct shell *shell, size_t argc, char **argv)
{
	int result;
	long src_id;
	long pa_sync_state;
	long bis_synced;
	long encrypted;

	src_id = strtol(argv[1], NULL, 0);
	if (src_id < 0 || src_id > UINT8_MAX) {
		shell_error(shell, "adv_sid shall be 0x00-0xff");
		return -ENOEXEC;
	}

	pa_sync_state = strtol(argv[2], NULL, 0);
	if (pa_sync_state < 0 || pa_sync_state > BASS_PA_STATE_NO_PAST) {
		shell_error(shell, "Invalid pa_sync_state %u", pa_sync_state);
		return -ENOEXEC;
	}

	bis_synced = strtol(argv[3], NULL, 0);
	if (bis_synced < 0 || bis_synced > UINT32_MAX) {
		shell_error(shell, "Invalid bis_synced %u", bis_synced);
		return -ENOEXEC;
	}

	encrypted = strtol(argv[4], NULL, 0);

	result = bt_bass_set_synced(src_id, pa_sync_state, bis_synced,
				    encrypted);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static int cmd_bass(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bass_cmds,
	SHELL_CMD_ARG(synced, NULL,
		      "Set server scan state <src_id> <pa_synced> <bis_syncs> "
		      "<enc_state>", cmd_bass_synced, 5, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(bass, &bass_cmds, "Bluetooth BASS shell commands",
		       cmd_bass, 1, 1);
