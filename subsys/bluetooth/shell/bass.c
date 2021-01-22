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

#if defined(CONFIG_BT_BASS_AUTO_SYNC)
static void pa_synced(uint8_t src_id, uint32_t bis_sync,
		      const struct bt_le_per_adv_sync_synced_info *info)
{
	shell_print(ctx_shell, "BASS src %u was PA synced (bis 0x%x)",
		    src_id, bis_sync);
}

static void pa_term(uint8_t src_id,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	shell_print(ctx_shell, "BASS src %u PA synced terminated", src_id);

}

static void pa_recv(uint8_t src_id,
		    const struct bt_le_per_adv_sync_recv_info *info,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char hex[512];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, hex, sizeof(hex));
	shell_print(ctx_shell, "src_id %u: device %s, tx_power %i, "
		    "RSSI %i, CTE %u, data length %u, data %s",
		    src_id, le_addr, info->tx_power,
		    info->rssi, info->cte_type, buf->len, hex);

}
#else
static void pa_sync_req(uint8_t src_id, uint32_t bis_sync, bt_addr_le_t *addr,
			uint8_t adv_sid, uint8_t metadata_len,
			uint8_t *metadata)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char hex[512];

	bt_addr_le_to_str(addr, le_addr, sizeof(le_addr));
	bin2hex(metadata, metadata_len, hex, sizeof(hex));
	shell_print(ctx_shell, "Request to PA sync to: src_id %u, device %s, "
		    "adv_sid %u, metadata %s", src_id, le_addr, adv_sid, hex);


	/* TODO: Establish PA sync */

}

static void pa_sync_term_req(uint8_t src_id)
{
	shell_print(ctx_shell, "Request to terminate PA sync to src_id %u",
		    src_id);

	/* TODO: Terminate PA sync */
}
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */

static struct bt_bass_cb_t cbs = {
#if defined(CONFIG_BT_BASS_AUTO_SYNC)
	.pa_synced = pa_synced,
	.pa_term = pa_term,
	.pa_recv = pa_recv
#else
	.pa_sync_req = pa_sync_req,
	.pa_sync_term_req = pa_sync_term_req
#endif /* defined(CONFIG_BT_BASS_AUTO_SYNC) */
};

static int cmd_bass_init(const struct shell *shell, size_t argc, char **argv)
{
	bt_bass_register_cb(&cbs);
	return 0;
}

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
