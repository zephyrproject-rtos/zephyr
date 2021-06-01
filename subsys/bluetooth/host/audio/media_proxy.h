/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MEDIA_PROXY_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MEDIA_PROXY_H_

/** @brief Media proxy module
 *
 * The media proxy module is the connection point between media player
 * instances and media controllers.
 *
 * A media player has (access to) media content and knows how to
 * navigate and play this content. A media controller reads or gets
 * information from a player and controls the player by setting player
 * parameters and giving the player commands.
 *
 * The media proxy module allows media player implementations to make
 * themselves available to media controllers. And it allows
 * controllers to access, and get updates from, any player.
 *
 * The media proxy module allows both local and remote control of
 * local player instances: A media controller may be a local
 * application, or it may be a Media Control Service relaying requests
 * from a remote Media Control Client. There may be either local or
 * remote control, or both, or even multiple instances of each.
 */

#include <stdbool.h>
#include <zephyr/types.h>

#include "mcs.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Media operations */
struct mpl_op_t {
	uint8_t  opcode;
	bool  use_param;
	int32_t param;
};

/** Media operation notification */
struct mpl_op_ntf_t {
	uint8_t requested_opcode;
	uint8_t result_code;
};

#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
/** @brief Search control item */
struct mpl_sci_t {
	uint8_t len;                     /* Length of type and parameter */
	uint8_t type;                    /* MPL_SEARCH_TYPE_<...> */
	char    param[SEARCH_PARAM_MAX]; /* Search parameter */
};

/** @brief Search */
struct mpl_search_t {
	uint8_t len;
	char    search[SEARCH_LEN_MAX]; /* Concatenated search control items */
};	                                /* - (type, length, param) */
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */


/* PUBLIC API FOR PLAYERS */

int media_proxy_pl_init(void);

char *media_proxy_pl_player_name_get(void);

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_pl_icon_id_get(void);
#endif /* CONFIG_BT_OTS */

char *media_proxy_pl_icon_uri_get(void);

void media_proxy_pl_track_changed_cb(void);

char *media_proxy_pl_track_title_get(void);
void media_proxy_pl_track_title_cb(char *title);

int32_t media_proxy_pl_track_duration_get(void);
void media_proxy_pl_track_duration_cb(int32_t duration);

int32_t media_proxy_pl_track_position_get(void);
void media_proxy_pl_track_position_set(int32_t position);
void media_proxy_pl_track_position_cb(int32_t position);

int8_t media_proxy_pl_playback_speed_get(void);
void media_proxy_pl_playback_speed_set(int8_t speed);
void media_proxy_pl_playback_speed_cb(int8_t speed);

int8_t media_proxy_pl_seeking_speed_get(void);
void media_proxy_pl_seeking_speed_cb(int8_t speed);

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_pl_track_segments_id_get(void);

uint64_t media_proxy_pl_current_track_id_get(void);
void media_proxy_pl_current_track_id_set(uint64_t id);
void media_proxy_pl_current_track_id_cb(uint64_t id);

uint64_t media_proxy_pl_next_track_id_get(void);
void media_proxy_pl_next_track_id_set(uint64_t id);
void media_proxy_pl_next_track_id_cb(uint64_t id);

uint64_t media_proxy_pl_group_id_get(void);
void media_proxy_pl_group_id_set(uint64_t id);
void media_proxy_pl_group_id_cb(uint64_t id);

uint64_t media_proxy_pl_parent_group_id_get(void);
void media_proxy_pl_parent_group_id_cb(uint64_t id);
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
/* For  IOP testing - set current group to be it's own parent */
void media_proxy_pl_test_unset_parent_group(void);
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_pl_playing_order_get(void);
void media_proxy_pl_playing_order_set(uint8_t order);
void media_proxy_pl_playing_order_cb(uint8_t order);

uint16_t media_proxy_pl_playing_orders_supported_get(void);

uint8_t media_proxy_pl_media_state_get(void);
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
void media_proxy_pl_test_media_state_set(uint8_t state);
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
void media_proxy_pl_media_state_cb(uint8_t state);

void media_proxy_pl_operation_set(struct mpl_op_t operation);
void media_proxy_pl_operation_cb(struct mpl_op_ntf_t op_ntf);

uint32_t media_proxy_pl_operations_supported_get(void);
void media_proxy_pl_operations_supported_cb(uint32_t operations);

#ifdef CONFIG_BT_OTS
void media_proxy_pl_scp_set(struct mpl_search_t search);
void media_proxy_pl_search_cb(uint8_t result_code);

uint64_t media_proxy_pl_search_results_id_get(void);
void media_proxy_pl_search_results_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_pl_content_ctrl_id_get(void);

#if CONFIG_BT_DEBUG_MCS
/* Output the mediaplayer's state information */
void media_proxy_pl_debug_dump_state(void);
#endif /* CONFIG_BT_DEBUG_MCS */

struct bt_ots *bt_mcs_get_ots(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MEDIA_PROXY_H_ */
