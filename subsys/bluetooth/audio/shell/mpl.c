/** @file
 *  @brief Media player shell
 *
 */

/*
 * Copyright (c) 2020 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_string_conv.h>

#include "shell/bt.h"

#include "../mpl_internal.h"

LOG_MODULE_REGISTER(bt_mpl_shell, CONFIG_BT_MPL_LOG_LEVEL);

#if defined(CONFIG_BT_MPL)

#if defined(CONFIG_BT_MPL_LOG_LEVEL_DBG) && defined(CONFIG_BT_TESTING)
int cmd_mpl_test_set_media_state(const struct shell *sh, size_t argc,
				 char *argv[])
{
	unsigned long state;
	int err = 0;

	state = shell_strtoul(argv[1], 0, &err);
	if (err != 0) {
		shell_error(sh, "Could not parse state: %d", err);

		return -ENOEXEC;
	}

	if (state > UINT8_MAX) {
		shell_error(sh, "Invalid state %lu", state);

		return -ENOEXEC;
	}

	mpl_test_media_state_set(state);

	return 0;
}

#ifdef CONFIG_BT_MPL_OBJECTS
int cmd_mpl_test_unset_parent_group(const struct shell *sh, size_t argc,
				    char *argv[])
{
	mpl_test_unset_parent_group();

	return 0;
}
#endif /* CONFIG_BT_MPL_OBJECTS */
#endif /* CONFIG_BT_MPL_LOG_LEVEL_DBG && CONFIG_BT_TESTING */

#if defined(CONFIG_BT_MPL_LOG_LEVEL_DBG)
int cmd_mpl_debug_dump_state(const struct shell *sh, size_t argc,
			     char *argv[])
{
	mpl_debug_dump_state();

	return 0;
}
#endif /* CONFIG_BT_MPL_LOG_LEVEL_DBG */

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

int cmd_mpl_test_player_name_cb(const struct shell *sh, size_t argc,
				char *argv[])
{
	mpl_test_player_name_changed_cb();

	return 0;
}

int cmd_mpl_test_player_icon_url_cb(const struct shell *sh, size_t argc,
				    char *argv[])
{
	mpl_test_player_icon_url_changed_cb();

	return 0;
}

int cmd_mpl_test_track_changed_cb(const struct shell *sh, size_t argc,
				  char *argv[])
{
	mpl_test_track_changed_cb();
	return 0;
}

int cmd_mpl_test_title_changed_cb(const struct shell *sh, size_t argc,
				  char *argv[])
{
	mpl_test_title_changed_cb();
	return 0;
}

int cmd_mpl_test_duration_changed_cb(const struct shell *sh, size_t argc,
				     char *argv[])
{
	mpl_test_duration_changed_cb();
	return 0;
}

int cmd_mpl_test_position_changed_cb(const struct shell *sh, size_t argc,
				     char *argv[])
{
	mpl_test_position_changed_cb();
	return 0;
}

int cmd_mpl_test_playback_speed_changed_cb(const struct shell *sh, size_t argc,
					   char *argv[])
{
	mpl_test_playback_speed_changed_cb();
	return 0;
}

int cmd_mpl_test_seeking_speed_changed_cb(const struct shell *sh, size_t argc,
					  char *argv[])
{
	mpl_test_seeking_speed_changed_cb();
	return 0;
}

#ifdef CONFIG_BT_MPL_OBJECTS
int cmd_mpl_test_current_track_id_changed_cb(const struct shell *sh, size_t argc,
					     char *argv[])
{
	mpl_test_current_track_id_changed_cb();
	return 0;
}

int cmd_mpl_test_next_track_id_changed_cb(const struct shell *sh, size_t argc,
					  char *argv[])
{
	mpl_test_next_track_id_changed_cb();
	return 0;
}

int cmd_mpl_test_current_group_id_changed_cb(const struct shell *sh, size_t argc,
					     char *argv[])
{
	mpl_test_current_group_id_changed_cb();
	return 0;
}

int cmd_mpl_test_parent_group_id_changed_cb(const struct shell *sh, size_t argc,
					    char *argv[])
{
	mpl_test_parent_group_id_changed_cb();
	return 0;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

int cmd_mpl_test_playing_order_changed_cb(const struct shell *sh, size_t argc,
					  char *argv[])
{
	mpl_test_playing_order_changed_cb();
	return 0;
}

int cmd_mpl_test_state_changed_cb(const struct shell *sh, size_t argc,
				  char *argv[])
{
	mpl_test_media_state_changed_cb();
	return 0;
}

int cmd_mpl_test_media_opcodes_supported_changed_cb(const struct shell *sh, size_t argc,
						    char *argv[])
{
	mpl_test_opcodes_supported_changed_cb();
	return 0;
}

#ifdef CONFIG_BT_MPL_OBJECTS
int cmd_mpl_test_search_results_changed_cb(const struct shell *sh, size_t argc,
					   char *argv[])
{
	mpl_test_search_results_changed_cb();
	return 0;
}
#endif /* CONFIG_BT_MPL_OBJECTS */

static int cmd_mpl(const struct shell *sh, size_t argc, char **argv)
{
	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(mpl_cmds,
#if defined(CONFIG_BT_MPL_LOG_LEVEL_DBG) && defined(CONFIG_BT_TESTING)
	SHELL_CMD_ARG(test_set_media_state, NULL,
		      "Set the media player state (test) <state>",
		      cmd_mpl_test_set_media_state, 2, 0),
#if CONFIG_BT_MPL_OBJECTS
	SHELL_CMD_ARG(test_unset_parent_group, NULL,
		      "Set current group to be its own parent (test)",
		      cmd_mpl_test_unset_parent_group, 1, 0),
#endif /* CONFIG_BT_MPL_OBJECTS */
#endif /* CONFIG_BT_MPL_LOG_LEVEL_DBG && CONFIG_BT_TESTING */
#if defined(CONFIG_BT_MPL_LOG_LEVEL_DBG)
	SHELL_CMD_ARG(debug_dump_state, NULL,
		      "Dump media player's state as debug output (debug)",
		      cmd_mpl_debug_dump_state, 1, 0),
#endif /* CONFIG_BT_MPL_LOG_LEVEL_DBG */
	SHELL_CMD_ARG(init, NULL,
		      "Initialize media player",
		      cmd_media_proxy_pl_init, 1, 0),
	SHELL_CMD_ARG(player_name_changed_cb, NULL,
		      "Trigger Player Name changed callback (test)",
		      cmd_mpl_test_player_name_cb, 1, 0),
	SHELL_CMD_ARG(player_icon_url_changed_cb, NULL,
		      "Trigger Player icon URL changed callback (test)",
		      cmd_mpl_test_player_icon_url_cb, 1, 0),
	SHELL_CMD_ARG(track_changed_cb, NULL,
		      "Trigger Track Changed callback (test)",
		      cmd_mpl_test_track_changed_cb, 1, 0),
	SHELL_CMD_ARG(title_changed_cb, NULL,
		      "Trigger Track Title callback (test)",
		      cmd_mpl_test_title_changed_cb, 1, 0),
	SHELL_CMD_ARG(duration_changed_cb, NULL,
		      "Trigger Track Duration callback (test)",
		      cmd_mpl_test_duration_changed_cb, 1, 0),
	SHELL_CMD_ARG(position_changed_cb, NULL,
		      "Trigger Track Position callback (test)",
		      cmd_mpl_test_position_changed_cb, 1, 0),
	SHELL_CMD_ARG(playback_speed_changed_cb, NULL,
		      "Trigger Playback Speed callback (test)",
		      cmd_mpl_test_playback_speed_changed_cb, 1, 0),
	SHELL_CMD_ARG(seeking_speed_changed_cb, NULL,
		      "Trigger Seeking Speed callback (test)",
		      cmd_mpl_test_seeking_speed_changed_cb, 1, 0),
#ifdef CONFIG_BT_MPL_OBJECTS
	SHELL_CMD_ARG(current_track_id_changed_cb, NULL,
		      "Trigger Current Track callback (test)",
		      cmd_mpl_test_current_track_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(next_track_id_changed_cb, NULL,
		      "Trigger Next Track callback (test)",
		      cmd_mpl_test_next_track_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(current_group_id_changed_cb, NULL,
		      "Trigger Current Group callback (test)",
		      cmd_mpl_test_current_group_id_changed_cb, 1, 0),
	SHELL_CMD_ARG(parent_group_id_changed_cb, NULL,
		      "Trigger Parent Group callback (test)",
		      cmd_mpl_test_parent_group_id_changed_cb, 1, 0),
#endif /* CONFIG_BT_MPL_OBJECTS */
	SHELL_CMD_ARG(playing_order_changed_cb, NULL,
		      "Trigger Playing Order callback (test)",
		      cmd_mpl_test_playing_order_changed_cb, 1, 0),
	SHELL_CMD_ARG(state_changed_cb, NULL,
		      "Trigger Media State callback (test)",
		      cmd_mpl_test_state_changed_cb, 1, 0),
	SHELL_CMD_ARG(media_opcodes_changed_cb, NULL,
		      "Trigger Opcodes Supported callback (test)",
		      cmd_mpl_test_media_opcodes_supported_changed_cb, 1, 0),
#ifdef CONFIG_BT_MPL_OBJECTS
	SHELL_CMD_ARG(search_results_changed_cb, NULL,
		      "Trigger Search Results Object ID callback (test)",
		      cmd_mpl_test_search_results_changed_cb, 1, 0),
#endif /* CONFIG_BT_MPL_OBJECTS */
	SHELL_SUBCMD_SET_END
);

/* TODO Remove this workaround macro once Scancode has been updated to
 * not report this as a false positive MPL license
 *
 * See https://github.com/nexB/scancode-toolkit/issues/2304
 * and https://github.com/nexB/scancode-toolkit/commit/6abbc4a22973f40ab74f6f8d948dd06416c97bd4
 */
#define CMD_NQM cmd_mpl
SHELL_CMD_ARG_REGISTER(mpl, &mpl_cmds, "Media player (MPL) related commands",
		       CMD_NQM, 1, 1);

#endif /* CONFIG_BT_MPL */
