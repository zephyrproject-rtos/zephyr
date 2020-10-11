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
#include "bt.h"

static inline int cmd_bass_synced(const struct shell *shell, size_t argc,
				  char **argv)
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

static inline int cmd_bass(const struct shell *shell, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

static inline int cmd_bass_client_discover(const struct shell *shell,
					   size_t argc, char **argv)
{
	int result;

	result = bt_bass_client_discover(default_conn);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_bass_client_scan_start(const struct shell *shell,
					     size_t argc, char **argv)
{
	int result;
	int force = false;

	if (argc > 1) {
		force = strtol(argv[1], NULL, 0);

		if (force < 0 || force > 1) {
			shell_error(shell, "Value shall be boolean");
			return -ENOEXEC;
		}
	}

	result = bt_bass_client_scan_start(default_conn, (bool)force);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_bass_client_scan_stop(const struct shell *shell,
					    size_t argc, char **argv)
{
	int result;

	result = bt_bass_client_scan_stop(default_conn);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_bass_client_add_src(const struct shell *shell,
					  size_t argc, char **argv)
{
	int result;
	long adv_sid;
	long pa_sync;
	unsigned long bis_sync = 0;
	bt_addr_le_t addr;
	static uint8_t metadata[256];
	uint8_t metadata_len = 0;

	result = bt_addr_le_from_str(argv[1], argv[2], &addr);
	if (result) {
		shell_error(shell, "Invalid peer address (err %d)", result);
		return -ENOEXEC;
	}

	adv_sid = strtol(argv[3], NULL, 0);
	if (adv_sid < 0 || adv_sid > 0x0F) {
		shell_error(shell, "adv_sid shall be 0x00-0x0f");
		return -ENOEXEC;
	}

	pa_sync = strtol(argv[4], NULL, 0);
	if (pa_sync < 0 || pa_sync > 1) {
		shell_error(shell, "pa_sync shall be boolean");
		return -ENOEXEC;
	}

	if (argc > 5) {
		bis_sync = strtoul(argv[5], NULL, 0);
		if (bis_sync > UINT32_MAX) {
			shell_error(shell, "bis_sync shall be 0x00000000 "
				    "to 0xFFFFFFFF");
			return -ENOEXEC;
		}
	}

	if (argc > 6) {
		metadata_len = hex2bin(argv[6], strlen(argv[6]),
				       metadata, sizeof(metadata));

		if (!metadata_len) {
			shell_error(shell, "Could not parse metadata");
			return -ENOEXEC;
		}
	}

	result = bt_bass_client_add_src(default_conn, &addr, adv_sid,
					pa_sync, bis_sync, metadata_len,
					metadata);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_bass_client_mod_src(const struct shell *shell,
					  size_t argc, char **argv)
{
	int result;
	long src_id;
	long pa_sync;
	unsigned long bis_sync = 0;
	static uint8_t metadata[256];
	uint8_t metadata_len = 0;

	src_id = strtol(argv[1], NULL, 0);
	if (src_id < 0 || src_id > UINT8_MAX) {
		shell_error(shell, "adv_sid shall be 0x00-0xff");
		return -ENOEXEC;
	}

	pa_sync = strtol(argv[2], NULL, 0);
	if (pa_sync < 0 || pa_sync > 1) {
		shell_error(shell, "pa_sync shall be boolean");
		return -ENOEXEC;
	}

	if (argc > 3) {
		bis_sync = strtoul(argv[3], NULL, 0);
		if (bis_sync > UINT32_MAX) {
			shell_error(shell, "bis_sync shall be 0x00000000 "
				    "to 0xFFFFFFFF");
			return -ENOEXEC;
		}
	}

	if (argc > 4) {
		metadata_len = hex2bin(argv[4], strlen(argv[4]),
				       metadata, sizeof(metadata));

		if (!metadata_len) {
			shell_error(shell, "Could not parse metadata");
			return -ENOEXEC;
		}
	}

	result = bt_bass_client_mod_src(default_conn, src_id, pa_sync, bis_sync,
					metadata_len, metadata);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_bass_client_broadcast_code(const struct shell *shell,
						 size_t argc, char **argv)
{
	int result;
	int src_id;
	uint8_t broadcast_code[BASS_BROADCAST_CODE_SIZE] = { 0 };

	src_id = strtol(argv[1], NULL, 0);
	if (src_id < 0 || src_id > UINT8_MAX) {
		shell_error(shell, "adv_sid shall be 0x00-0xff");
		return -ENOEXEC;
	}

	for (int i = 0; i < argc - 2; i++) {
		broadcast_code[i] = strtol(argv[i + 2], NULL, 0);
	}

	result = bt_bass_client_broadcast_code(default_conn, src_id,
					       broadcast_code);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}

static inline int cmd_bass_client_rem_src(const struct shell *shell,
					  size_t argc, char **argv)
{
	int result;
	int src_id;

	src_id = strtol(argv[1], NULL, 0);
	if (src_id < 0 || src_id > UINT8_MAX) {
		shell_error(shell, "adv_sid shall be 0x00-0xff");
		return -ENOEXEC;
	}

	result = bt_bass_client_rem_src(default_conn, src_id);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}



static inline int cmd_bass_client_read_recv_state(const struct shell *shell,
						  size_t argc, char **argv)
{
	int result;
	int idx;

	idx = strtol(argv[1], NULL, 0);
	if (idx < 0 || idx > UINT8_MAX) {
		shell_error(shell, "adv_sid shall be 0x00-0xff");
		return -ENOEXEC;
	}

	result = bt_bass_client_rem_src(default_conn, idx);
	if (result) {
		shell_print(shell, "Fail: %d", result);
	}
	return result;
}


static inline int cmd_bass_client(const struct shell *shell, size_t argc,
				  char **argv)
{
	if (argc > 1) {
		shell_error(shell, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(shell, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bass_client_cmds,
	SHELL_CMD_ARG(discover, NULL,
		      "Discover BASS on the server",
		      cmd_bass_client_discover, 1, 0),
	SHELL_CMD_ARG(scan_start, NULL,
		      "Start scanning for BISs [force]",
		      cmd_bass_client_scan_start, 1, 1),
	SHELL_CMD_ARG(scan_stop, NULL,
		      "Stop scanning for BISs",
		      cmd_bass_client_scan_stop, 1, 0),
	SHELL_CMD_ARG(add_src, NULL,
		      "Add a source <address: XX:XX:XX:XX:XX:XX> "
		      "<type: public/random> <adv_sid> <sync_pa> [<sync_bis>] "
		      "[<metadata>]",
		      cmd_bass_client_add_src, 5, 2),
	SHELL_CMD_ARG(mod_src, NULL,
		      "Set sync <src_id> <sync_pa> [<sync_bis>] [<metadata>]",
		      cmd_bass_client_mod_src, 3, 2),
	SHELL_CMD_ARG(broadcast_code, NULL,
		      "Send a space separated broadcast code of up to 16 bytes "
		      "<src_id> [broadcast code]",
		      cmd_bass_client_broadcast_code, 2, 16),
	SHELL_CMD_ARG(rem_src, NULL,
		      "Remove a source <src_id>",
		      cmd_bass_client_rem_src, 2, 0),
	SHELL_CMD_ARG(read_state, NULL,
		      "Remove a source <index>",
		      cmd_bass_client_read_recv_state, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(bass_client, &bass_client_cmds,
		       "Bluetooth BASS client shell commands",
		       cmd_bass_client, 1, 1);
