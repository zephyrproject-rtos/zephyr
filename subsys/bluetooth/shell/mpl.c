/** @file
 *  @brief Media player shell
 *
 */

/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <shell/shell.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "bt.h"

#include "../host/audio/media_proxy.h"
#include "../host/audio/mpl_internal.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_MCS)
#define LOG_MODULE_NAME bt_mpl_shell
#include "common/log.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BT_MCS)

#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
int cmd_mpl_test_set_media_state(const struct shell *sh, size_t argc,
				 char *argv[])
{
	uint8_t state = strtol(argv[1], NULL, 0);

	mpl_test_media_state_set(state);

	return 0;
}

#ifdef CONFIG_BT_OTS
int cmd_mpl_test_unset_parent_group(const struct shell *sh, size_t argc,
				    char *argv[])
{
	mpl_test_unset_parent_group();

	return 0;
}
#endif /* CONFIG_BT_OTS */

/* Interface to _local_ control point, for testing and debugging */
/* TODO: Enable again after MCS/MPL/Media Proxy interface rework
 *int cmd_media_proxy_pl_test_set_operation(const struct shell *sh, size_t argc,
 *				char *argv[])
 *{
 *	struct mpl_op_t op;
 *
 *
 *	if (argc > 1) {
 *		op.opcode = strtol(argv[1], NULL, 0);
 *	} else {
 *		shell_error(sh, "Invalid parameter");
 *		return -ENOEXEC;
 *	}
 *
 *	if (argc > 2) {
 *		op.use_param = true;
 *		op.param = strtol(argv[2], NULL, 0);
 *	} else {
 *		op.use_param = false;
 *		op.param = 0;
 *	}
 *
 *	media_proxy_pl_operation_set(op);
 *
 *	return 0;
 *}
 */
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */

#if defined(CONFIG_BT_DEBUG_MCS)
int cmd_mpl_debug_dump_state(const struct shell *sh, size_t argc,
			     char *argv[])
{
	mpl_debug_dump_state();

	return 0;
}
#endif /* CONFIG_BT_DEBUG_MCS */

int cmd_media_proxy_pl_init(const struct shell *sh, size_t argc, char *argv[])
{
	if (!ctx_shell) {
		ctx_shell = sh;
	}

	int err = media_proxy_pl_init();

	if (err) {
		shell_error(sh, "Could not init mpl");
	}

	return err;
}


int cmd_media_proxy_pl_track_changed_cb(const struct shell *sh, size_t argc,
			     char *argv[])
{
	media_proxy_pl_track_changed_cb();

	return 0;
}

int cmd_media_proxy_pl_title_changed_cb(const struct shell *sh, size_t argc,
			     char *argv[])
{
	media_proxy_pl_track_title_cb("Interlude #3");

	return 0;
}

int cmd_media_proxy_pl_duration_changed_cb(const struct shell *sh, size_t argc,
				char *argv[])
{
	media_proxy_pl_track_duration_cb(12000);

	return 0;
}

int cmd_media_proxy_pl_position_changed_cb(const struct shell *sh, size_t argc,
				char *argv[])
{
	media_proxy_pl_track_position_cb(2048);

	return 0;
}

int cmd_media_proxy_pl_playback_speed_changed_cb(const struct shell *sh, size_t argc,
				      char *argv[])
{
	media_proxy_pl_playback_speed_cb(96);

	return 0;
}

int cmd_media_proxy_pl_seeking_speed_changed_cb(const struct shell *sh, size_t argc,
				     char *argv[])
{
	media_proxy_pl_seeking_speed_cb(4);

	return 0;
}

#ifdef CONFIG_BT_OTS
int cmd_media_proxy_pl_current_track_id_changed_cb(const struct shell *sh, size_t argc,
					char *argv[])
{
	media_proxy_pl_current_track_id_cb(16);

	return 0;
}

int cmd_media_proxy_pl_next_track_id_changed_cb(const struct shell *sh, size_t argc,
				     char *argv[])
{
	media_proxy_pl_next_track_id_cb(17);

	return 0;
}

int cmd_media_proxy_pl_current_group_id_changed_cb(const struct shell *sh, size_t argc,
				char *argv[])
{
	media_proxy_pl_current_group_id_cb(19);

	return 0;
}

int cmd_media_proxy_pl_parent_group_id_changed_cb(const struct shell *sh, size_t argc,
				       char *argv[])
{
	media_proxy_pl_parent_group_id_cb(23);

	return 0;
}
#endif /* CONFIG_BT_OTS */

int cmd_media_proxy_pl_playing_order_changed_cb(const struct shell *sh, size_t argc,
				     char *argv[])
{
	media_proxy_pl_playing_order_cb(1);

	return 0;
}

int cmd_media_proxy_pl_state_changed_cb(const struct shell *sh, size_t argc,
			     char *argv[])
{
	media_proxy_pl_media_state_cb(BT_MCS_MEDIA_STATE_SEEKING);

	return 0;
}

int cmd_media_proxy_pl_media_opcodes_changed_cb(const struct shell *sh, size_t argc,
				     char *argv[])
{
	media_proxy_pl_operations_supported_cb(0x00aa55aa);

	return 0;
}

#ifdef CONFIG_BT_OTS
int cmd_media_proxy_pl_search_results_changed_cb(const struct shell *sh, size_t argc,
				      char *argv[])
{
	media_proxy_pl_search_results_id_cb(19);

	return 0;
}
#endif /* CONFIG_BT_OTS */

static int cmd_mpl(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mpl_cmds,
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
	SHELL_CMD_ARG(test_set_media_state, NULL,
		      "Set the media player state (test) <state>",
		      cmd_mpl_test_set_media_state, 2, 0),
#if CONFIG_BT_OTS
	SHELL_CMD_ARG(test_unset_parent_group, NULL,
		      "Set current group to be its own parent (test)",
		      cmd_mpl_test_unset_parent_group, 1, 0),
#endif /* CONFIG_BT_OTS */
/* TODO: Reenable after MCS/MPL/MPLC rework
	SHELL_CMD_ARG(test_set_operation, NULL,
		      "Write opcode to local control point (test) <opcode> [argument]",
		      cmd_media_proxy_pl_test_set_operation, 2, 1),
*/
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
#if defined(CONFIG_BT_DEBUG_MCS)
	SHELL_CMD_ARG(debug_dump_state, NULL,
		      "Dump media player's state as debug output (debug)",
		      cmd_mpl_debug_dump_state, 1, 0),
#endif /* CONFIG_BT_DEBUG_MCC */
	SHELL_CMD_ARG(init, NULL,
		      "Initialize media player",
		      cmd_media_proxy_pl_init, 1, 0),
	SHELL_CMD_ARG(track_changed_cb, NULL,
		      "Send Track Changed notification",
		      cmd_media_proxy_pl_track_changed_cb, 1, 0),
	SHELL_CMD_ARG(title_changed_cb, NULL,
		      "Send (fake) Track Title notification",
		      cmd_media_proxy_pl_title_changed_cb, 1, 0),
	SHELL_CMD_ARG(duration_changed_cb, NULL,
		      "Send Track Duration notification",
		      cmd_media_proxy_pl_duration_changed_cb, 1, 0),
	SHELL_CMD_ARG(position_changed_cb, NULL,
		      "Send Track Position notification",
		      cmd_media_proxy_pl_position_changed_cb, 1, 0),
	SHELL_CMD_ARG(playback_speed_changed_cb, NULL,
		      "Send Playback Speed notification",
		      cmd_media_proxy_pl_playback_speed_changed_cb, 1, 0),
	SHELL_CMD_ARG(seeking_speed_changed_cb, NULL,
		      "Send Seeking Speed notification",
		      cmd_media_proxy_pl_seeking_speed_changed_cb, 1, 0),
#ifdef CONFIG_BT_OTS
	SHELL_CMD_ARG(current_track_id_changed_cb, NULL,
		      "Send Current Track notification",
		      cmd_media_proxy_pl_current_track_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(next_track_id_changed_cb, NULL,
		      "Send Next Track notification",
		      cmd_media_proxy_pl_next_track_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(current_group_id_changed_cb, NULL,
		      "Send Current Group notification",
		      cmd_media_proxy_pl_current_group_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(parent_group_id_changed_cb, NULL,
		      "Send Parent Group notification",
		      cmd_media_proxy_pl_parent_group_id_changed_cb, 1, 0),
#endif /* CONFIG_BT_OTS */
	SHELL_CMD_ARG(playing_order_changed_cb, NULL,
		      "Send Playing Order notification",
		      cmd_media_proxy_pl_playing_order_changed_cb, 1, 0),
	SHELL_CMD_ARG(state_changed_cb, NULL,
		      "Send Media State notification",
		      cmd_media_proxy_pl_state_changed_cb, 1, 0),
	SHELL_CMD_ARG(media_opcodes_changed_cb, NULL,
		      "Send Supported Opcodes notfication",
		      cmd_media_proxy_pl_media_opcodes_changed_cb, 1, 0),
#ifdef CONFIG_BT_OTS
	SHELL_CMD_ARG(search_results_changed_cb, NULL,
		      "Send Search Results Object ID notification",
		      cmd_media_proxy_pl_search_results_changed_cb, 1, 0),
#endif /* CONFIG_BT_OTS */
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_ARG_REGISTER(mpl, &mpl_cmds, "Media player (MCS) related commands",
		       cmd_mpl, 1, 1);

#endif /* CONFIG_BT_MCS */

#ifdef __cplusplus
}
#endif
