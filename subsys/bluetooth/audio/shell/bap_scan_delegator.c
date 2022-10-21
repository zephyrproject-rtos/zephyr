/**
 * @file
 * @brief Shell APIs for Bluetooth BAP scan delegator
 *
 * Copyright (c) 2020-2022 Nordic Semiconductor ASA
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
#include <zephyr/bluetooth/audio/bap.h>
#include "shell/bt.h"

static void pa_synced(const struct bt_bap_scan_delegator_recv_state *recv_state,
		      const struct bt_le_per_adv_sync_synced_info *info)
{
	shell_print(ctx_shell,
		    "BAP scan delegator receive state %p was PA synced",
		    recv_state);
}

static void pa_term(const struct bt_bap_scan_delegator_recv_state *recv_state,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	shell_print(ctx_shell,
		    "BAP scan delegator receive state %p PA synced terminated",
		    recv_state);

}

static void pa_recv(const struct bt_bap_scan_delegator_recv_state *recv_state,
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

static struct bt_bap_scan_delegator_cb cbs = {
	.pa_synced = pa_synced,
	.pa_term = pa_term,
	.pa_recv = pa_recv
};

static int cmd_bap_scan_delegator_init(const struct shell *sh, size_t argc,
				       char **argv)
{
	bt_bap_scan_delegator_register_cb(&cbs);
	return 0;
}

static int cmd_bap_scan_delegator_synced(const struct shell *sh, size_t argc,
					 char **argv)
{
	uint32_t bis_syncs[CONFIG_BT_BAP_SCAN_DELEGATOR_MAX_SUBGROUPS];
	unsigned long pa_sync_state;
	unsigned long bis_synced;
	unsigned long src_id;
	bool encrypted;
	int result = 0;

	src_id = shell_strtoul(argv[1], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse src_id: %d", result);

		return -ENOEXEC;
	}

	if (src_id > UINT8_MAX) {
		shell_error(sh, "Invalid src_id: %lu", src_id);

		return -ENOEXEC;
	}

	pa_sync_state = shell_strtoul(argv[2], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse pa_sync_state: %d", result);

		return -ENOEXEC;
	}

	if (pa_sync_state > BT_BAP_PA_STATE_NO_PAST) {
		shell_error(sh, "Invalid pa_sync_state %ld", pa_sync_state);

		return -ENOEXEC;
	}

	bis_synced = shell_strtoul(argv[3], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse bis_synced: %d", result);

		return -ENOEXEC;
	}

	if (bis_synced > UINT32_MAX) {
		shell_error(sh, "Invalid bis_synced %ld", bis_synced);

		return -ENOEXEC;
	}

	for (size_t i = 0U; i < ARRAY_SIZE(bis_syncs); i++) {
		bis_syncs[i] = bis_synced;
	}

	encrypted = shell_strtobool(argv[4], 0, &result);
	if (result != 0) {
		shell_error(sh, "Could not parse encrypted: %d", result);

		return -ENOEXEC;
	}

	result = bt_bap_scan_delegator_set_sync_state(src_id, pa_sync_state,
						      bis_syncs, encrypted);
	if (result != 0) {
		shell_print(sh, "Fail: %d", result);
	}

	return result;
}

static int cmd_bap_scan_delegator(const struct shell *sh, size_t argc,
				  char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s",
			    argv[0], argv[1]);
	} else {
		shell_error(sh, "%s Missing subcommand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(bap_scan_delegator_cmds,
	SHELL_CMD_ARG(init, NULL,
		      "Initialize the service and register callbacks",
		      cmd_bap_scan_delegator_init, 1, 0),
	SHELL_CMD_ARG(synced, NULL,
		      "Set server scan state <src_id> <pa_synced> <bis_syncs> "
		      "<enc_state>", cmd_bap_scan_delegator_synced, 5, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(bap_scan_delegator, &bap_scan_delegator_cmds,
		       "Bluetooth BAP scan delegator shell commands",
		       cmd_bap_scan_delegator, 1, 1);
