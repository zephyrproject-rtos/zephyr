/**
 * @file
 * @brief Internal header for the Media Control Profile (MCP).
 *
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MCP_INTERNAL_
#define ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MCP_INTERNAL_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/mcs.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Offset into a segment/track before the "previous" command goes to start of */
/* current segment/track instead to previous */
#define PREV_MARGIN           500  /* 500 * 0.01 = 5 seconds */

/* Increase/decrease in seeking sped factor for fast rewind/forward commands
 * The media control specification has a requirement that the speed factor
 * shall be [-64, -4], 0, [4, 64].  I.e., values between 0 and +/- 4 are not allowed.
 * Set this equal to the minimum speed factor, to ensure only valid speed factors
 * are used when changing to/from zero
 */
#define BT_MCP_SEEKING_SPEED_FACTOR_STEP 4

/* Track segments */
struct bt_mcp_tseg {
	uint8_t            name_len;
	char name[CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_SEGMENT_NAME_MAX];
	int32_t            pos;
	struct bt_mcp_tseg *prev;
	struct bt_mcp_tseg *next;
};

/* Tracks */
struct bt_mcp_track {
#if defined(CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t             id;
#endif /* CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS || CONFIG_BT_OTS_CLIENT */
	char title[CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_TRACK_TITLE_MAX];
	int32_t              duration;
	struct bt_mcp_tseg *segment;
#if defined(CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t             segments_id;
#endif /* CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS || CONFIG_BT_OTS_CLIENT */
	struct bt_mcp_track *prev;
	struct bt_mcp_track *next;
};

/* Groups */
struct bt_mcp_group {
#if defined(CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t             id;
#endif /* CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS || CONFIG_BT_OTS_CLIENT */
	char title[CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_GROUP_TITLE_MAX];
	struct bt_mcp_track *track;
	struct bt_mcp_group *parent;
	struct bt_mcp_group *prev;
	struct bt_mcp_group *next;
};

/** @brief Media Player */
struct bt_mcp_media_control_server_player {
	char name[CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_MEDIA_PLAYER_NAME_MAX];
#if defined(CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t            icon_id;
#endif /* CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS || CONFIG_BT_OTS_CLIENT */
	char icon_url[CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_ICON_URL_MAX];
	struct bt_mcp_group *group;
	int32_t             track_pos;
	uint8_t             state;
	int8_t              playback_speed_param;
	int8_t              seeking_speed_factor;
	uint8_t             playing_order;
	uint16_t            playing_orders_supported;
	uint32_t            opcodes_supported;
#if defined(CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t            search_results_id;
#endif /* CONFIG_BT_MCP_MEDIA_CONTROL_SERVER_OBJECTS || CONFIG_BT_OTS_CLIENT */
	uint8_t             content_ctrl_id;
	struct bt_mcs_cb calls;

	bool                        next_track_set; /* If next track explicitly set */
	struct {
		struct bt_mcp_track *track; /* The track explicitly set as next track */
		struct bt_mcp_group *group; /* The group of the set track */
	} next;

	struct k_work_delayable pos_work;
};

/* Special calls for testing */

/* For  IOP testing - set current group to be it's own parent */
void bt_mcp_media_control_server_test_unset_parent_group(void);

/* Force the media player into a given state */
void bt_mcp_media_control_server_test_media_state_set(uint8_t state);

/** Trigger player name changed callback */
void bt_mcp_media_control_server_test_player_name_changed_cb(void);

/** Trigger player name changed callback */
void bt_mcp_media_control_server_test_player_icon_url_changed_cb(void);

/* Trigger track changed callback */
void bt_mcp_media_control_server_test_track_changed_cb(void);

/* Trigger title changed callback */
void bt_mcp_media_control_server_test_title_changed_cb(void);

/* Trigger duration changed callback */
void bt_mcp_media_control_server_test_duration_changed_cb(void);

/* Trigger position changed callback */
void bt_mcp_media_control_server_test_position_changed_cb(void);

/* Trigger playback speed changed callback */
void bt_mcp_media_control_server_test_playback_speed_changed_cb(void);

/* Trigger seeking speed changed callback */
void bt_mcp_media_control_server_test_seeking_speed_changed_cb(void);

/* Trigger current track id changed callback */
void bt_mcp_media_control_server_test_current_track_id_changed_cb(void);

/* Trigger next track id changed callback */
void bt_mcp_media_control_server_test_next_track_id_changed_cb(void);

/* Trigger current group id changed callback */
void bt_mcp_media_control_server_test_current_group_id_changed_cb(void);

/* Trigger parent group id changed callback */
void bt_mcp_media_control_server_test_parent_group_id_changed_cb(void);

/* Trigger playing order changed callback */
void bt_mcp_media_control_server_test_playing_order_changed_cb(void);

/* Trigger media state changed callback */
void bt_mcp_media_control_server_test_media_state_changed_cb(void);

/* Trigger operations supported changed callback */
void bt_mcp_media_control_server_test_opcodes_supported_changed_cb(void);

/* Trigger search results changed callback */
void bt_mcp_media_control_server_test_search_results_changed_cb(void);

/* Output the mediaplayer's state information */
void bt_mcp_debug_dump_state(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MCP_INTERNAL_*/
