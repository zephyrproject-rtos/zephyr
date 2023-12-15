/**
 * @file
 * @brief Internal header for the media player (MPL).
 *
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MPL_INTERNAL_
#define ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MPL_INTERNAL_

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
#define MPL_SEEKING_SPEED_FACTOR_STEP  4


/* Track segments */
struct mpl_tseg {
	uint8_t            name_len;
	char		   name[CONFIG_BT_MPL_SEGMENT_NAME_MAX];
	int32_t            pos;
	struct mpl_tseg   *prev;
	struct mpl_tseg   *next;
};

/* Tracks */
struct mpl_track {
#if defined(CONFIG_BT_MPL_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t             id;
#endif /* CONFIG_BT_MPL_OBJECTS || CONFIG_BT_OTS_CLIENT */
	char                 title[CONFIG_BT_MPL_TRACK_TITLE_MAX];
	int32_t              duration;
	struct mpl_tseg     *segment;
#if defined(CONFIG_BT_MPL_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t             segments_id;
#endif /* CONFIG_BT_MPL_OBJECTS || CONFIG_BT_OTS_CLIENT */
	struct mpl_track    *prev;
	struct mpl_track    *next;
};

/* Groups */
struct mpl_group {
#if defined(CONFIG_BT_MPL_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t             id;
#endif /* CONFIG_BT_MPL_OBJECTS || CONFIG_BT_OTS_CLIENT */
	char                 title[CONFIG_BT_MPL_GROUP_TITLE_MAX];
	struct mpl_track    *track;
	struct mpl_group    *parent;
	struct mpl_group    *prev;
	struct mpl_group    *next;
};

/** @brief Media Player */
struct mpl_mediaplayer   {
	char                name[CONFIG_BT_MPL_MEDIA_PLAYER_NAME_MAX];
#if defined(CONFIG_BT_MPL_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t            icon_id;
#endif /* CONFIG_BT_MPL_OBJECTS || CONFIG_BT_OTS_CLIENT */
	char                icon_url[CONFIG_BT_MPL_ICON_URL_MAX];
	struct mpl_group   *group;
	int32_t             track_pos;
	uint8_t             state;
	int8_t              playback_speed_param;
	int8_t              seeking_speed_factor;
	uint8_t             playing_order;
	uint16_t            playing_orders_supported;
	uint32_t            opcodes_supported;
#if defined(CONFIG_BT_MPL_OBJECTS) || defined(CONFIG_BT_OTS_CLIENT)
	uint64_t            search_results_id;
#endif /* CONFIG_BT_MPL_OBJECTS || CONFIG_BT_OTS_CLIENT */
	uint8_t             content_ctrl_id;
	struct media_proxy_pl_calls calls;

	bool                        next_track_set; /* If next track explicitly set */
	struct {
		struct mpl_track    *track; /* The track explicitly set as next track */
		struct mpl_group    *group; /* The group of the set track */
	} next;

	struct k_work_delayable pos_work;
};


/* Special calls for testing */

/* For  IOP testing - set current group to be it's own parent */
void mpl_test_unset_parent_group(void);

/* Force the media player into a given state */
void mpl_test_media_state_set(uint8_t state);

/** Trigger player name changed callback */
void mpl_test_player_name_changed_cb(void);

/** Trigger player name changed callback */
void mpl_test_player_icon_url_changed_cb(void);

/* Trigger track changed callback */
void mpl_test_track_changed_cb(void);

/* Trigger title changed callback */
void mpl_test_title_changed_cb(void);

/* Trigger duration changed callback */
void mpl_test_duration_changed_cb(void);

/* Trigger position changed callback */
void mpl_test_position_changed_cb(void);

/* Trigger playback speed changed callback */
void mpl_test_playback_speed_changed_cb(void);

/* Trigger seeking speed changed callback */
void mpl_test_seeking_speed_changed_cb(void);

/* Trigger current track id changed callback */
void mpl_test_current_track_id_changed_cb(void);

/* Trigger next track id changed callback */
void mpl_test_next_track_id_changed_cb(void);

/* Trigger current group id changed callback */
void mpl_test_current_group_id_changed_cb(void);

/* Trigger parent group id changed callback */
void mpl_test_parent_group_id_changed_cb(void);

/* Trigger playing order changed callback */
void mpl_test_playing_order_changed_cb(void);

/* Trigger media state changed callback */
void mpl_test_media_state_changed_cb(void);

/* Trigger operations supported changed callback */
void mpl_test_opcodes_supported_changed_cb(void);

/* Trigger search results changed callback */
void mpl_test_search_results_changed_cb(void);

/* Output the mediaplayer's state information */
void mpl_debug_dump_state(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_AUDIO_MPL_INTERNAL_*/
