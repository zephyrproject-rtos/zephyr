/** @file
 *  @brief Media Control Client shell interface
 *
 */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCC_SHELL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCC_SHELL_H_

#include <shell/shell.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include "../host/audio/otc.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bt_otc_instance_t *bt_mcc_otc_inst(uint8_t index);

int cmd_mcc_init(const struct shell *shell, size_t argc, char **argv);
int cmd_mcc_discover_mcs(const struct shell *shell, size_t argc, char **argv);
int cmd_mcc_read_player_name(const struct shell *shell, size_t argc,
			     char *argv[]);
int cmd_mcc_read_icon_obj_id(const struct shell *shell, size_t argc,
			     char *argv[]);
int cmd_mcc_read_icon_uri(const struct shell *shell, size_t argc,
			  char *argv[]);
int cmd_mcc_read_track_title(const struct shell *shell, size_t argc,
			     char *argv[]);
int cmd_mcc_read_track_duration(const struct shell *shell, size_t argc,
				char *argv[]);
int cmd_mcc_read_track_position(const struct shell *shell, size_t argc,
				char *argv[]);
int cmd_mcc_set_track_position(const struct shell *shell, size_t argc,
			       char *argv[]);
int cmd_mcc_read_playback_speed(const struct shell *shell, size_t argc,
				char *argv[]);
int cmd_mcc_set_playback_speed(const struct shell *shell, size_t argc,
			       char *argv[]);
int cmd_mcc_read_seeking_speed(const struct shell *shell, size_t argc,
			       char *argv[]);
int cmd_mcc_read_track_segments_obj_id(const struct shell *shell,
				       size_t argc, char *argv[]);
int cmd_mcc_read_current_track_obj_id(const struct shell *shell, size_t argc,
				      char *argv[]);
int cmd_mcc_read_next_track_obj_id(const struct shell *shell, size_t argc,
				   char *argv[]);
int cmd_mcc_read_current_group_obj_id(const struct shell *shell, size_t argc,
				      char *argv[]);
int cmd_mcc_read_parent_group_obj_id(const struct shell *shell, size_t argc,
				     char *argv[]);
int cmd_mcc_read_playing_order(const struct shell *shell, size_t argc,
			       char *argv[]);
int cmd_mcc_set_playing_order(const struct shell *shell, size_t argc,
			      char *argv[]);
int cmd_mcc_read_playing_orders_supported(const struct shell *shell, size_t argc,
					  char *argv[]);
int cmd_mcc_read_media_state(const struct shell *shell, size_t argc,
			     char *argv[]);

int cmd_mcc_set_cp(const struct shell *shell, size_t argc, char *argv[]);

int cmd_mcc_read_opcodes_supported(const struct shell *shell, size_t argc,
				   char *argv[]);
/* Set search control point with raw search string as input */
int cmd_mcc_set_scp_raw(const struct shell *shell, size_t argc, char *argv[]);

/* Set search control point as required for IOP test case */
int cmd_mcc_set_scp_ioptest(const struct shell *shell, size_t argc,
			    char *argv[]);
int cmd_mcc_read_search_results_obj_id(const struct shell *shell, size_t argc,
				       char *argv[]);
int cmd_mcc_read_content_control_id(const struct shell *shell, size_t argc,
				    char *argv[]);

/* Generic OTS/OTC commands */
int cmd_mcc_otc_read_features(const struct shell *shell, size_t argc,
			      char *argv[]);
int cmd_mcc_otc_read(const struct shell *shell, size_t argc, char *argv[]);

int cmd_mcc_otc_read_metadata(const struct shell *shell, size_t argc,
			      char *argv[]);

int cmd_mcc_otc_select(const struct shell *shell, size_t argc, char *argv[]);

int cmd_mcc_otc_select_first(const struct shell *shell, size_t argc,
			     char *argv[]);
int cmd_mcc_otc_select_last(const struct shell *shell, size_t argc,
			    char *argv[]);
int cmd_mcc_otc_select_next(const struct shell *shell, size_t argc,
			    char *argv[]);
int cmd_mcc_otc_select_prev(const struct shell *shell, size_t argc,
			    char *argv[]);

/* Media control specific OTS/OTC commands */
int cmd_mcc_otc_read_icon_object(const struct shell *shell, size_t argc,
				 char *argv[]);
int cmd_mcc_otc_read_track_segments_object(const struct shell *shell,
					   size_t argc, char *argv[]);
int cmd_mcc_otc_read_current_track_object(const struct shell *shell,
					  size_t argc, char *argv[]);
int cmd_mcc_otc_read_next_track_object(const struct shell *shell, size_t argc,
				       char *argv[]);
int cmd_mcc_otc_read_current_group_object(const struct shell *shell,
					  size_t argc, char *argv[]);
int cmd_mcc_otc_read_parent_group_object(const struct shell *shell,
					 size_t argc, char *argv[]);
int cmd_mcc_ots_select_first(const struct shell *shell, size_t argc,
			     char *argv[]);
int cmd_mcc_ots_select_last(const struct shell *shell, size_t argc,
			    char *argv[]);
int cmd_mcc_ots_select_next(const struct shell *shell, size_t argc,
			    char *argv[]);
int cmd_mcc_ots_select_prev(const struct shell *shell, size_t argc,
			    char *argv[]);
#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCC_SHELL_H_ */
