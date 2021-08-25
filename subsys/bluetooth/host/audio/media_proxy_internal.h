/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_

/** @brief Internal APIs for Bluetooth Media Control */

#include "media_proxy.h"

#define MPL_NO_TRACK_ID 0

/* Debug output of 48 bit Object ID value */
/* (Zephyr does not yet support debug output of more than 32 bit values.) */
/* Takes a text and a 64-bit integer as input */
#define BT_DBG_OBJ_ID(text, id64) \
	do { \
		if (IS_ENABLED(CONFIG_BT_DEBUG_MCS)) { \
			char t[BT_OTS_OBJ_ID_STR_LEN]; \
			(void)bt_ots_obj_id_to_str(id64, t, sizeof(t)); \
			BT_DBG(text "0x%s", log_strdup(t)); \
		} \
	} while (0)


/* SYNCHRONOUS (CALL/RETURN) API FOR CONTROLLERS */

/** @brief Callbacks to a controller, from the media proxy */
struct media_proxy_sctrl_cbs {

	void (*track_changed)(void);

	void (*track_title)(const char *title);

	void (*track_duration)(int32_t duration);

	void (*track_position)(int32_t position);

	void (*playback_speed)(int8_t speed);

	void (*seeking_speed)(int8_t speed);
#ifdef CONFIG_BT_OTS
	void (*current_track_id)(uint64_t id);

	void (*next_track_id)(uint64_t id);

	void (*current_group_id)(uint64_t id);

	void (*parent_group_id)(uint64_t id);
#endif /* CONFIG_BT_OTS */

	void (*playing_order)(uint8_t order);

	void (*media_state)(uint8_t state);

	void (*command)(struct mpl_cmd_ntf_t cmd_ntf);

	void (*commands_supported)(uint32_t opcodes);

#ifdef CONFIG_BT_OTS
	void (*search)(uint8_t result_code);

	void (*search_results_id)(uint64_t id);
#endif /* CONFIG_BT_OTS */
};

/** @brief Register a controller with the media proxy */
int media_proxy_sctrl_register(struct media_proxy_sctrl_cbs *sctrl_cbs);


const char *media_proxy_sctrl_player_name_get(void);

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_sctrl_icon_id_get(void);
#endif /* CONFIG_BT_OTS */

const char *media_proxy_sctrl_icon_url_get(void);


const char *media_proxy_sctrl_track_title_get(void);

int32_t media_proxy_sctrl_track_duration_get(void);

int32_t media_proxy_sctrl_track_position_get(void);
void media_proxy_sctrl_track_position_set(int32_t position);

int8_t media_proxy_sctrl_playback_speed_get(void);
void media_proxy_sctrl_playback_speed_set(int8_t speed);

int8_t media_proxy_sctrl_seeking_speed_get(void);

#ifdef CONFIG_BT_OTS
uint64_t media_proxy_sctrl_track_segments_id_get(void);

uint64_t media_proxy_sctrl_current_track_id_get(void);
void media_proxy_sctrl_current_track_id_set(uint64_t id);

uint64_t media_proxy_sctrl_next_track_id_get(void);
void media_proxy_sctrl_next_track_id_set(uint64_t id);

uint64_t media_proxy_sctrl_current_group_id_get(void);
void media_proxy_sctrl_current_group_id_set(uint64_t id);

uint64_t media_proxy_sctrl_parent_group_id_get(void);
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_sctrl_playing_order_get(void);
void media_proxy_sctrl_playing_order_set(uint8_t order);

uint16_t media_proxy_sctrl_playing_orders_supported_get(void);

uint8_t media_proxy_sctrl_media_state_get(void);

void media_proxy_sctrl_command_send(struct mpl_cmd_t command);

uint32_t media_proxy_sctrl_commands_supported_get(void);

#ifdef CONFIG_BT_OTS
void media_proxy_sctrl_search_set(struct mpl_search_t search);

uint64_t media_proxy_sctrl_search_results_id_get(void);
#endif /* CONFIG_BT_OTS */

uint8_t media_proxy_sctrl_content_ctrl_id_get(void);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MEDIA_PROXY_INTERNAL_H_ */
