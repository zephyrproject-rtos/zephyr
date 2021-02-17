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

#ifdef __cplusplus
extern "C" {
#endif


/* Offset into a segment/track before the "previous" command goes to start of */
/* current segment/track instead to previous */
#define PREV_MARGIN           500  /* 500 * 0.01 = 5 seconds */

#define SEARCH_LEN_MIN 2   /* At least one search control item, consisting of */
			   /* the length octet and the type octet. */
			   /* (The parameter field may be empty.) */
#define SEARCH_SCI_LEN_MIN 1 /* An SCI length can be as little as one byte, */
			     /* for an SCI that has only the type field. */
			     /* (The SCI length is the length of type + param.) */

#define SEARCH_LEN_MAX 64  /* Max total length of the search, defined by spec */

#define SEARCH_PARAM_MAX 62 /* A search may have only one search control item */
			    /* consisting of length, type and parameter */

/* Playback speeds */
/* (All values from -128 to 127 allowed, only some defined) */
#define MPL_PLAYBACK_SPEED_MIN     -128
#define MPL_PLAYBACK_SPEED_QUARTER -128
#define MPL_PLAYBACK_SPEED_HALF     -64
#define MPL_PLAYBACK_SPEED_UNITY      0
#define MPL_PLAYBACK_SPEED_DOUBLE    64
#define MPL_PLAYBACK_SPEED_MAX      127

/* Seeking speed */
/* The allowed values for seeking speed are
 * the range -64 to -4 (endpoints included),
 * the value 0,
 * and the range 4 to 64 (endpoints included)
 */
#define MPL_SEEKING_SPEED_FACTOR_MAX  64
#define MPL_SEEKING_SPEED_FACTOR_MIN   4
#define MPL_SEEKING_SPEED_FACTOR_ZERO  0
#define MPL_SEEKING_SPEED_FACTOR_STEP  4

/* Playing orders */
#define MPL_PLAYING_ORDER_SINGLE_ONCE    0X01
#define MPL_PLAYING_ORDER_SINGLE_REPEAT  0x02
#define MPL_PLAYING_ORDER_INORDER_ONCE   0x03
#define MPL_PLAYING_ORDER_INORDER_REPEAT 0x04
#define MPL_PLAYING_ORDER_OLDEST_ONCE    0x05
#define MPL_PLAYING_ORDER_OLDEST_REPEAT  0x06
#define MPL_PLAYING_ORDER_NEWEST_ONCE    0x07
#define MPL_PLAYING_ORDER_NEWEST_REPEAT  0x08
#define MPL_PLAYING_ORDER_SHUFFLE_ONCE   0x09
#define MPL_PLAYING_ORDER_SHUFFLE_REPEAT 0x0a

/* Playing orders supported */
/* A bitmap, in the same order as the playing orders above */
/* Note that playing order 1 corresponds to bit 0, and so on. */
#define MPL_PLAYING_ORDERS_SUPPORTED_SINGLE_ONCE    BIT(0)
#define MPL_PLAYING_ORDERS_SUPPORTED_SINGLE_REPEAT  BIT(1)
#define MPL_PLAYING_ORDERS_SUPPORTED_INORDER_ONCE   BIT(2)
#define MPL_PLAYING_ORDERS_SUPPORTED_INORDER_REPEAT BIT(3)
#define MPL_PLAYING_ORDERS_SUPPORTED_OLDEST_ONCE    BIT(4)
#define MPL_PLAYING_ORDERS_SUPPORTED_OLDEST_REPEAT  BIT(5)
#define MPL_PLAYING_ORDERS_SUPPORTED_NEWEST_ONCE    BIT(6)
#define MPL_PLAYING_ORDERS_SUPPORTED_NEWEST_REPEAT  BIT(7)
#define MPL_PLAYING_ORDERS_SUPPORTED_SHUFFLE_ONCE   BIT(8)
#define MPL_PLAYING_ORDERS_SUPPORTED_SHUFFLE_REPEAT BIT(9)

/* Media states */
#define MPL_MEDIA_STATE_INACTIVE 0x00
#define MPL_MEDIA_STATE_PLAYING  0x01
#define MPL_MEDIA_STATE_PAUSED   0x02
#define MPL_MEDIA_STATE_SEEKING  0x03
#define MPL_MEDIA_STATE_LAST     0x04

/* Media control point opcodes */
/* These will need to be verified as the spec is completed */
#define MPL_OPC_PLAY          0x01
#define MPL_OPC_PAUSE         0x02
#define MPL_OPC_FAST_REWIND   0x03
#define MPL_OPC_FAST_FORWARD  0x04
#define MPL_OPC_STOP          0x05

#define MPL_OPC_MOVE_RELATIVE 0x10

#define MPL_OPC_PREV_SEGMENT  0x20
#define MPL_OPC_NEXT_SEGMENT  0x21
#define MPL_OPC_FIRST_SEGMENT 0x22
#define MPL_OPC_LAST_SEGMENT  0x23
#define MPL_OPC_GOTO_SEGMENT  0x24

#define MPL_OPC_PREV_TRACK    0x30
#define MPL_OPC_NEXT_TRACK    0x31
#define MPL_OPC_FIRST_TRACK   0x32
#define MPL_OPC_LAST_TRACK    0x33
#define MPL_OPC_GOTO_TRACK    0x34

#define MPL_OPC_PREV_GROUP    0x40
#define MPL_OPC_NEXT_GROUP    0x41
#define MPL_OPC_FIRST_GROUP   0x42
#define MPL_OPC_LAST_GROUP    0x43
#define MPL_OPC_GOTO_GROUP    0x44

/* Media control point supported opcodes values*/
/* These will need to be updated as the spec is completed */
#define OPCODES_SUPPORTED_LEN 4

#define MPL_OPC_SUP_PLAY          0x00000001
#define MPL_OPC_SUP_PAUSE         0x00000002
#define MPL_OPC_SUP_FAST_REWIND   0X00000004
#define MPL_OPC_SUP_FAST_FORWARD  0x00000008
#define MPL_OPC_SUP_STOP          0x00000010
#define MPL_OPC_SUP_MOVE_RELATIVE 0x00000020

#define MPL_OPC_SUP_PREV_SEGMENT  0x00000040
#define MPL_OPC_SUP_NEXT_SEGMENT  0x00000080
#define MPL_OPC_SUP_FIRST_SEGMENT 0x00000100
#define MPL_OPC_SUP_LAST_SEGMENT  0x00000200
#define MPL_OPC_SUP_GOTO_SEGMENT  0x00000400

#define MPL_OPC_SUP_PREV_TRACK    0x00000800
#define MPL_OPC_SUP_NEXT_TRACK    0x00001000
#define MPL_OPC_SUP_FIRST_TRACK   0x00002000
#define MPL_OPC_SUP_LAST_TRACK    0x00004000
#define MPL_OPC_SUP_GOTO_TRACK    0x00008000

#define MPL_OPC_SUP_PREV_GROUP    0x00010000
#define MPL_OPC_SUP_NEXT_GROUP    0x00020000
#define MPL_OPC_SUP_FIRST_GROUP   0x00040000
#define MPL_OPC_SUP_LAST_GROUP    0x00080000
#define MPL_OPC_SUP_GOTO_GROUP    0x00100000

/* Media control point notification result codes */
#define MPL_OPC_NTF_SUCCESS             0x01
#define MPL_OPC_NTF_NOT_SUPPORTED       0x02
#define MPL_OPC_NTF_PLAYER_INACTIVE     0x03
#define MPL_OPC_NTF_CANNOT_BE_COMPLETED 0x04

/* Search control point type values */
/* These will need to be updated as the spec is completed */
#define MPL_SEARCH_TYPE_TRACK_NAME    0x01
#define MPL_SEARCH_TYPE_ARTIST_NAME   0x02
#define MPL_SEARCH_TYPE_ALBUM_NAME    0x03
#define MPL_SEARCH_TYPE_GROUP_NAME    0x04
#define MPL_SEARCH_TYPE_EARLIEST_YEAR 0x05
#define MPL_SEARCH_TYPE_LATEST_YEAR   0x06
#define MPL_SEARCH_TYPE_GENRE         0x07
#define MPL_SEARCH_TYPE_ONLY_TRACKS   0x08
#define MPL_SEARCH_TYPE_ONLY_GROUPS   0x09

/* Search notification result codes */
#define MPL_SCP_NTF_SUCCESS  0x01
#define MPL_SCP_NTF_FAILURE  0x02

/* Group object object types */
/* Reference: Media control service spec d09r10 section 4.4.1 */
#define MPL_GROUP_OBJECT_TRACK_TYPE 0
#define MPL_GROUP_OBJECT_GROUP_TYPE 1

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
#if defined(CONFIG_BT_OTS_TEMP) || defined(CONFIG_BT_OTC)
	uint64_t             id;
#endif /* CONFIG_BT_OTS_TEMP || CONFIG_BT_OTC */
	char                 title[CONFIG_BT_MCS_TRACK_TITLE_MAX];
	int32_t              duration;
	struct mpl_tseg_t   *segment;
#if defined(CONFIG_BT_OTS_TEMP) || defined(CONFIG_BT_OTC)
	uint64_t             segments_id;
#endif /* CONFIG_BT_OTS_TEMP || CONFIG_BT_OTC */
	struct mpl_track_t  *prev;
	struct mpl_track_t  *next;
};

/* Groups */
struct mpl_group_t {
#if defined(CONFIG_BT_OTS_TEMP) || defined(CONFIG_BT_OTC)
	uint64_t             id;
#endif /* CONFIG_BT_OTS_TEMP || CONFIG_BT_OTC */
	char                 title[CONFIG_BT_MCS_GROUP_TITLE_MAX];
	struct mpl_track_t  *track;
	struct mpl_group_t  *parent;
	struct mpl_group_t  *prev;
	struct mpl_group_t  *next;
};

/** @brief Media Player */
struct mpl_mediaplayer_t {
	char                name[CONFIG_BT_MCS_MEDIA_PLAYER_NAME_MAX];
#if defined(CONFIG_BT_OTS_TEMP) || defined(CONFIG_BT_OTC)
	uint64_t            icon_id;
#endif /* CONFIG_BT_OTS_TEMP || CONFIG_BT_OTC */
	char                icon_uri[CONFIG_BT_MCS_ICON_URI_MAX];
	struct mpl_group_t *group;
	int32_t             track_pos;
	uint8_t             state;
	int8_t              playback_speed_param;
	int8_t              seeking_speed_factor;
	uint8_t             playing_order;
	uint16_t            playing_orders_supported;
	uint32_t            operations_supported;
#if defined(CONFIG_BT_OTS_TEMP) || defined(CONFIG_BT_OTC)
	uint64_t            search_results_id;
#endif /* CONFIG_BT_OTS_TEMP || CONFIG_BT_OTC */
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

#if defined(CONFIG_BT_OTS_TEMP) || defined(CONFIG_BT_OTC)
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
#endif /* CONFIG_BT_OTS_TEMP || CONFIG_BT_OTC */

int mpl_init(void);

char *mpl_player_name_get(void);

#ifdef CONFIG_BT_OTS_TEMP
uint64_t mpl_icon_id_get(void);
#endif /* CONFIG_BT_OTS_TEMP */

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

#ifdef CONFIG_BT_OTS_TEMP
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
#endif /* CONFIG_BT_OTS_TEMP */

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

#ifdef CONFIG_BT_OTS_TEMP
void mpl_scp_set(struct mpl_search_t search);
void mpl_search_cb(uint8_t result_code);

uint64_t mpl_search_results_id_get(void);
void mpl_search_results_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS_TEMP */

uint8_t mpl_content_ctrl_id_get(void);

#if CONFIG_BT_DEBUG_MCS
/* Output the mediaplayer's state information */
void mpl_debug_dump_state(void);
#endif /* CONFIG_BT_DEBUG_MCS */

struct ots_svc_inst_t *bt_mcs_get_ots(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MPL_H_ */
