/** @file
 *  @brief Media Player / Media control service shell interface
 *
 *  Shell commands to trigger the media player callback functions
 *  (which are implemented by the media control service and give
 *  notifications to the media control client).
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_SHELL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_SHELL_H_

#include <shell/shell.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#ifdef __cplusplus
extern "C" {
#endif

int cmd_mpl_track_changed_cb(const struct shell *shell, size_t argc,
				    char *argv[]);

int cmd_mpl_title_changed_cb(const struct shell *shell, size_t argc,
				    char *argv[]);

int cmd_mpl_duration_changed_cb(const struct shell *shell, size_t argc,
				       char *argv[]);

int cmd_mpl_position_changed_cb(const struct shell *shell, size_t argc,
				       char *argv[]);

int cmd_mpl_playback_speed_changed_cb(const struct shell *shell, size_t argc,
					     char *argv[]);

int cmd_mpl_seeking_speed_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[]);

int cmd_mpl_current_track_id_changed_cb(const struct shell *shell, size_t argc,
					char *argv[]);

int cmd_mpl_next_track_id_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[]);

int cmd_mpl_group_id_changed_cb(const struct shell *shell, size_t argc,
				char *argv[]);

int cmd_mpl_parent_group_id_changed_cb(const struct shell *shell, size_t argc,
				       char *argv[]);

int cmd_mpl_playing_order_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[]);

int cmd_mpl_state_changed_cb(const struct shell *shell, size_t argc,
			     char *argv[]);

int cmd_mpl_media_opcodes_changed_cb(const struct shell *shell, size_t argc,
				     char *argv[]);

int cmd_mpl_search_results_changed_cb(const struct shell *shell, size_t argc,
				      char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_SHELL_H_ */
