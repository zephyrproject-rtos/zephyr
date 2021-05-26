/* @file
 * @brief Media player skeleton
 *
 * For use with the Media Control Service (MCS)
 *
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MPL_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MPL_H_

#include <stdbool.h>
#include <zephyr/types.h>

#include "mcs.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Offset into a segment/track before the "previous" command goes to start of */
/* current segment/track instead to previous */
#define PREV_MARGIN           500  /* 500 * 0.01 = 5 seconds */

/* Increase/decrease in seeking sped for fast rewind/forward commands */
#define MPL_SEEKING_SPEED_FACTOR_STEP  4


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
	char                icon_uri[CONFIG_BT_MCS_ICON_URI_MAX];
	struct mpl_group_t *group;
	int32_t             track_pos;
	uint8_t             state;
	int8_t              playback_speed_param;
	int8_t              seeking_speed_factor;
	uint8_t             playing_order;
	uint16_t            playing_orders_supported;
	uint32_t            operations_supported;
#if defined(CONFIG_BT_OTS) || defined(CONFIG_BT_OTC)
	uint64_t            search_results_id;
#endif /* CONFIG_BT_OTS || CONFIG_BT_OTC */
	uint8_t             content_ctrl_id;
};


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

int mpl_init(void);

char *mpl_player_name_get(void);

#ifdef CONFIG_BT_OTS
uint64_t mpl_icon_id_get(void);
#endif /* CONFIG_BT_OTS */

char *mpl_icon_uri_get(void);

void mpl_track_changed_cb(void);

char *mpl_track_title_get(void);
void mpl_track_title_cb(char *title);

int32_t mpl_track_duration_get(void);
void mpl_track_duration_cb(int32_t duration);

int32_t mpl_track_position_get(void);
void mpl_track_position_set(int32_t position);
void mpl_track_position_cb(int32_t position);

int8_t mpl_playback_speed_get(void);
void mpl_playback_speed_set(int8_t speed);
void mpl_playback_speed_cb(int8_t speed);

int8_t mpl_seeking_speed_get(void);
void mpl_seeking_speed_cb(int8_t speed);

#ifdef CONFIG_BT_OTS
uint64_t mpl_track_segments_id_get(void);

uint64_t mpl_current_track_id_get(void);
void mpl_current_track_id_set(uint64_t id);
void mpl_current_track_id_cb(uint64_t id);

uint64_t mpl_next_track_id_get(void);
void mpl_next_track_id_set(uint64_t id);
void mpl_next_track_id_cb(uint64_t id);

uint64_t mpl_group_id_get(void);
void mpl_group_id_set(uint64_t id);
void mpl_group_id_cb(uint64_t id);

uint64_t mpl_parent_group_id_get(void);
void mpl_parent_group_id_cb(uint64_t id);
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
/* For  IOP testing - set current group to be it's own parent */
void mpl_test_unset_parent_group(void);
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
#endif /* CONFIG_BT_OTS */

uint8_t mpl_playing_order_get(void);
void mpl_playing_order_set(uint8_t order);
void mpl_playing_order_cb(uint8_t order);

uint16_t mpl_playing_orders_supported_get(void);

uint8_t mpl_media_state_get(void);
#if defined(CONFIG_BT_DEBUG_MCS) && defined(CONFIG_BT_TESTING)
void mpl_test_media_state_set(uint8_t state);
#endif /* CONFIG_BT_DEBUG_MCS && CONFIG_BT_TESTING */
void mpl_media_state_cb(uint8_t state);

void mpl_operation_set(struct mpl_op_t operation);
void mpl_operation_cb(struct mpl_op_ntf_t op_ntf);

uint32_t mpl_operations_supported_get(void);
void mpl_operations_supported_cb(uint32_t operations);

#ifdef CONFIG_BT_OTS
void mpl_scp_set(struct mpl_search_t search);
void mpl_search_cb(uint8_t result_code);

uint64_t mpl_search_results_id_get(void);
void mpl_search_results_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS */

uint8_t mpl_content_ctrl_id_get(void);

#if CONFIG_BT_DEBUG_MCS
/* Output the mediaplayer's state information */
void mpl_debug_dump_state(void);
#endif /* CONFIG_BT_DEBUG_MCS */

struct bt_ots *bt_mcs_get_ots(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MPL_H_ */
