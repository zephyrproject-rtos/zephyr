/**
 * @file
 * @brief Shell APIs for Bluetooth BASS
 *
 * Copyright (c) 2020-2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/bass.h>
#include "bt.h"

static void pa_synced(struct bt_bass_recv_state *recv_state,
		      const struct bt_le_per_adv_sync_synced_info *info)
{
	shell_print(ctx_shell, "BASS receive state %p was PA synced",
		    recv_state);
}

static void pa_term(struct bt_bass_recv_state *recv_state,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	shell_print(ctx_shell, "BASS receive state %p PA synced terminated",
		    recv_state);

}

static void pa_recv(struct bt_bass_recv_state *recv_state,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char hex[512];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, hex, sizeof(hex));
	shell_print(ctx_shell, "Receive state %p: device %s, tx_power %i, "
		    "RSSI %i, CTE %u, data length %u, data %s",
		    recv_state, le_addr, info->tx_power,
		    info->rssi, info->cte_type, buf->len, hex);

}

static struct bt_bass_cb cbs = {
	.pa_synced = pa_synced,
	.pa_term = pa_term,
	.pa_recv = pa_recv
};

static int cmd_bass_init(const struct shell *sh, size_t argc, char **argv)
{
	bt_bass_register_cb(&cbs);
	return 0;
}

static int cmd_bass_synced(const struct shell *sh, size_t argc, char **argv)
{
	int result;
	long src_id;
	long pa_sync_state;
	long bis_synced;
	long encrypted;
	uint32_t bis_syncs[CONFIG_BT_BASS_MAX_SUBGROUPS];

	src_id = strtol(argv[1], NULL, 0);
	if (src_id < 0 || src_id > UINT8_MAX) {
		shell_error(sh, "adv_sid shall be 0x00-0xff");
		return -ENOEXEC;
	}

	pa_sync_state = strtol(argv[2], NULL, 0);
	if (pa_sync_state < 0 || pa_sync_state > BT_BASS_PA_STATE_NO_PAST) {
		shell_error(sh, "Invalid pa_sync_state %ld", pa_sync_state);
		return -ENOEXEC;
	}

	bis_synced = strtol(argv[3], NULL, 0);
	if (bis_synced < 0 || bis_synced > UINT32_MAX) {
		shell_error(sh, "Invalid bis_synced %ld", bis_synced);
		return -ENOEXEC;
	}
	for (int i = 0; i < ARRAY_SIZE(bis_syncs); i++) {
		bis_syncs[i] = bis_synced;
	}

	encrypted = strtol(argv[4], NULL, 0);

	result = bt_bass_set_sync_state(src_id, pa_sync_state, bis_syncs,
					encrypted);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_bass(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bass_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_bass_init, 1, 0),
	SHELL_CMD_ARG(synced, NULL,
		      "Set server scan state <src_id> <pa_synced> <bis_syncs> "
		      "<enc_state>", cmd_bass_synced, 5, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(bass, &bass_cmds, "Bluetooth BASS shell commands",
		       cmd_bass, 1, 1);
