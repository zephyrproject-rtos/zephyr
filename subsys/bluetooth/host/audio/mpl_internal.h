/**
 * @file
 * @brief Internal header for the media player (MPL).
 *
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_INTERNAL_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_INTERNAL_

#include "media_proxy.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Offset into a segment/track before the "previous" command goes to start of */
/* current segment/track instead to previous */
#define PREV_MARGIN           500  /* 500 * 0.01 = 5 seconds */

/* Increase/decrease in seeking sped factor for fast rewind/forward commands
 * Set this equal to the minimum speed factor, to ensure only valid speed factors
 * are used when changing to/from zero
 */
#define MPL_SEEKING_SPEED_FACTOR_STEP  BT_MCS_SEEKING_SPEED_FACTOR_MIN


/* Track segments */
struct mpl_tseg_t {
	uint8_t            name_len;
	char		   name[CONFIG_BT_MCS_SEGMENT_NAME_MAX];
	int32_t            pos;
	struct mpl_tseg_t *prev;
	struct mpl_tseg_t *next;
};

/* Tracks */
struct mpl_track_t {
#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
	uint64_t             id;
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */
	char                 title[CONFIG_BT_MCS_TRACK_TITLE_MAX];
	int32_t              duration;
	struct mpl_tseg_t   *segment;
#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
	uint64_t             segments_id;
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */
	struct mpl_track_t  *prev;
	struct mpl_track_t  *next;
};

/* Groups */
struct mpl_group_t {
#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
	uint64_t             id;
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */
	char                 title[CONFIG_BT_MCS_GROUP_TITLE_MAX];
	struct mpl_track_t  *track;
	struct mpl_group_t  *parent;
	struct mpl_group_t  *prev;
	struct mpl_group_t  *next;
};

/** @brief Media Player */
struct mpl_mediaplayer_t {
	char                name[CONFIG_BT_MCS_MEDIA_PLAYER_NAME_MAX];
#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
	uint64_t            icon_id;
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */
	char                icon_url[CONFIG_BT_MCS_ICON_URL_MAX];
	struct mpl_group_t  *group;
	int32_t              track_pos;
	uint8_t              state;
	int8_t               playback_speed_param;
	int8_t               seeking_speed_factor;
	uint8_t              playing_order;
	uint16_t             playing_orders_supported;
	uint32_t             operations_supported;
#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
	uint64_t             search_results_id;
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */
	uint8_t              content_ctrl_id;
	struct media_proxy_pl_calls calls;
};


/* Special calls for testing */

/* For  IOP testing - set current group to be it's own parent */
void mpl_test_unset_parent_group(void);

/* Force the media player into a given state */
void mpl_test_media_state_set(uint8_t state);

/* Output the mediaplayer's state information */
void mpl_debug_dump_state(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MPL_INTERNAL_*/
