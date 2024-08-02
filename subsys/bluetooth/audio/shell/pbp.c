/** @file
 *  @brief Bluetooth Public Broadcast Profile shell
 */

/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pbp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/kernel.h>
#include <zephyr/net/buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include "shell/bt.h"

#define PBS_DEMO                'P', 'B', 'P'

const uint8_t pba_metadata[] = {
	BT_AUDIO_CODEC_DATA(BT_AUDIO_METADATA_TYPE_PROGRAM_INFO, PBS_DEMO)
};

/* Buffer to hold the Public Broadcast Announcement */
NET_BUF_SIMPLE_DEFINE_STATIC(pbp_ad_buf, BT_PBP_MIN_PBA_SIZE + ARRAY_SIZE(pba_metadata));

enum bt_pbp_announcement_feature pbp_features;
extern const struct shell *ctx_shell;

static int cmd_pbp_set_features(const struct shell *sh, size_t argc, char **argv)
{
	int err = 0;
	enum bt_pbp_announcement_feature features;

	features = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse received features: %d", err);

		return -ENOEXEC;
	}

	pbp_features = features;

	return err;
}

size_t pbp_ad_data_add(struct bt_data data[], size_t data_size)
{
	int err;

	net_buf_simple_reset(&pbp_ad_buf);

	err = bt_pbp_get_announcement(pba_metadata,
				      ARRAY_SIZE(pba_metadata),
				      pbp_features,
				      &pbp_ad_buf);

	if (err != 0) {
		shell_info(ctx_shell, "Create Public Broadcast Announcement");
	} else {
		shell_error(ctx_shell, "Failed to create Public Broadcast Announcement: %d", err);
	}

	__ASSERT(data_size > 0, "No space for Public Broadcast Announcement");
	data[0].type = BT_DATA_SVC_DATA16;
	data[0].data_len = pbp_ad_buf.len;
	data[0].data = pbp_ad_buf.data;

	return 1U;
}

static int cmd_pbp(const struct shell *sh, size_t argc, char **argv)
{
	if (argc > 1) {
		shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);
	} else {
		shell_error(sh, "%s missing subcomand", argv[0]);
	}

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(pbp_cmds,
	SHELL_CMD_ARG(set_features, NULL,
		      "Set the Public Broadcast Announcement features",
		      cmd_pbp_set_features, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(pbp, &pbp_cmds, "Bluetooth pbp shell commands", cmd_pbp, 1, 1);
