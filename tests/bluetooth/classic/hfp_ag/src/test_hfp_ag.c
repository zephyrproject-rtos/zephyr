/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/bluetooth/classic/hfp_ag.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define HELP_NONE "[none]"

extern struct bt_hfp_hf_cb hf_cb;
extern struct bt_hfp_ag_cb ag_cb;
static struct bt_hfp_hf_cb hf_cb_test;
static struct bt_hfp_ag_cb ag_cb_test;

static int cmd_enable_codec_neg(const struct shell *sh, size_t argc, char **argv)
{
	bool is_neg = strtoul(argv[1], NULL, 10) == 0 ? false : true;

	if (is_neg) {
		hf_cb.codec_negotiate = hf_cb_test.codec_negotiate;
		bt_shell_print("enabled codec_neg cb");
	} else {
		hf_cb.codec_negotiate = NULL;
		bt_shell_print("disabled codec_neg cb");
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ag_cmds,
	SHELL_CMD_ARG(enable_codec_neg, NULL, "Enable codec negotiate callback [0/1]",
		      cmd_enable_codec_neg, 2, 0),
	SHELL_SUBCMD_SET_END
);

static int cmd_ag(const struct shell *sh, size_t argc, char **argv)
{
	memcpy(&hf_cb_test, &hf_cb, sizeof(hf_cb));
	memcpy(&ag_cb_test, &ag_cb, sizeof(ag_cb));

	if (argc == 1) {
		shell_help(sh);
		/* sh returns 1 when help is printed */
		return 1;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_CMD_ARG_REGISTER(hf_test, &ag_cmds, "Bluetooth test hfp ag test sh commands",
		       cmd_ag, 1, 1);
