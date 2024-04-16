/*
 * Copyright (c) 2019 - 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCS_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCS_H_

/**
 * @brief Media Control Service (MCS)
 *
 * @defgroup bt_mcs Media Control Service (MCS)
 *
 * @ingroup bluetooth
 * @{
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 *
 * Definitions and types related to the Media Control Service and Media Control
 * Profile specifications.
 */

#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BT_MCS_ERR_LONG_VAL_CHANGED     0x80

/** @brief Playback speeds
 *
 * All values from -128 to 127 allowed, only some defined
 */
#define BT_MCS_PLAYBACK_SPEED_MIN     -128
#define BT_MCS_PLAYBACK_SPEED_QUARTER -128
#define BT_MCS_PLAYBACK_SPEED_HALF     -64
#define BT_MCS_PLAYBACK_SPEED_UNITY      0
#define BT_MCS_PLAYBACK_SPEED_DOUBLE    64
#define BT_MCS_PLAYBACK_SPEED_MAX      127

/** @brief Seeking speed
 *
 * The allowed values for seeking speed are the range -64 to -4
 * (endpoints included), the value 0, and the range 4 to 64
 * (endpoints included).
 */
#define BT_MCS_SEEKING_SPEED_FACTOR_MAX  64
#define BT_MCS_SEEKING_SPEED_FACTOR_MIN   4
#define BT_MCS_SEEKING_SPEED_FACTOR_ZERO  0

/** Playing orders */
#define BT_MCS_PLAYING_ORDER_SINGLE_ONCE    0X01
#define BT_MCS_PLAYING_ORDER_SINGLE_REPEAT  0x02
#define BT_MCS_PLAYING_ORDER_INORDER_ONCE   0x03
#define BT_MCS_PLAYING_ORDER_INORDER_REPEAT 0x04
#define BT_MCS_PLAYING_ORDER_OLDEST_ONCE    0x05
#define BT_MCS_PLAYING_ORDER_OLDEST_REPEAT  0x06
#define BT_MCS_PLAYING_ORDER_NEWEST_ONCE    0x07
#define BT_MCS_PLAYING_ORDER_NEWEST_REPEAT  0x08
#define BT_MCS_PLAYING_ORDER_SHUFFLE_ONCE   0x09
#define BT_MCS_PLAYING_ORDER_SHUFFLE_REPEAT 0x0a

/** @brief Playing orders supported
 *
 * A bitmap, in the same order as the playing orders above.
 * Note that playing order 1 corresponds to bit 0, and so on.
 */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SINGLE_ONCE    BIT(0)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SINGLE_REPEAT  BIT(1)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_ONCE   BIT(2)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_REPEAT BIT(3)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_OLDEST_ONCE    BIT(4)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_OLDEST_REPEAT  BIT(5)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_NEWEST_ONCE    BIT(6)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_NEWEST_REPEAT  BIT(7)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SHUFFLE_ONCE   BIT(8)
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SHUFFLE_REPEAT BIT(9)

/** Media states */
#define BT_MCS_MEDIA_STATE_INACTIVE 0x00
#define BT_MCS_MEDIA_STATE_PLAYING  0x01
#define BT_MCS_MEDIA_STATE_PAUSED   0x02
#define BT_MCS_MEDIA_STATE_SEEKING  0x03
#define BT_MCS_MEDIA_STATE_LAST     0x04

/** Media control point opcodes */
#define BT_MCS_OPC_PLAY          0x01
#define BT_MCS_OPC_PAUSE         0x02
#define BT_MCS_OPC_FAST_REWIND   0x03
#define BT_MCS_OPC_FAST_FORWARD  0x04
#define BT_MCS_OPC_STOP          0x05

#define BT_MCS_OPC_MOVE_RELATIVE 0x10

#define BT_MCS_OPC_PREV_SEGMENT  0x20
#define BT_MCS_OPC_NEXT_SEGMENT  0x21
#define BT_MCS_OPC_FIRST_SEGMENT 0x22
#define BT_MCS_OPC_LAST_SEGMENT  0x23
#define BT_MCS_OPC_GOTO_SEGMENT  0x24

#define BT_MCS_OPC_PREV_TRACK    0x30
#define BT_MCS_OPC_NEXT_TRACK    0x31
#define BT_MCS_OPC_FIRST_TRACK   0x32
#define BT_MCS_OPC_LAST_TRACK    0x33
#define BT_MCS_OPC_GOTO_TRACK    0x34

#define BT_MCS_OPC_PREV_GROUP    0x40
#define BT_MCS_OPC_NEXT_GROUP    0x41
#define BT_MCS_OPC_FIRST_GROUP   0x42
#define BT_MCS_OPC_LAST_GROUP    0x43
#define BT_MCS_OPC_GOTO_GROUP    0x44

/** Media control point supported opcodes length */
#define BT_MCS_OPCODES_SUPPORTED_LEN 4

/** Media control point supported opcodes values */
#define BT_MCS_OPC_SUP_PLAY          BIT(0)
#define BT_MCS_OPC_SUP_PAUSE         BIT(1)
#define BT_MCS_OPC_SUP_FAST_REWIND   BIT(2)
#define BT_MCS_OPC_SUP_FAST_FORWARD  BIT(3)
#define BT_MCS_OPC_SUP_STOP          BIT(4)

#define BT_MCS_OPC_SUP_MOVE_RELATIVE BIT(5)

#define BT_MCS_OPC_SUP_PREV_SEGMENT  BIT(6)
#define BT_MCS_OPC_SUP_NEXT_SEGMENT  BIT(7)
#define BT_MCS_OPC_SUP_FIRST_SEGMENT BIT(8)
#define BT_MCS_OPC_SUP_LAST_SEGMENT  BIT(9)
#define BT_MCS_OPC_SUP_GOTO_SEGMENT  BIT(10)

#define BT_MCS_OPC_SUP_PREV_TRACK    BIT(11)
#define BT_MCS_OPC_SUP_NEXT_TRACK    BIT(12)
#define BT_MCS_OPC_SUP_FIRST_TRACK   BIT(13)
#define BT_MCS_OPC_SUP_LAST_TRACK    BIT(14)
#define BT_MCS_OPC_SUP_GOTO_TRACK    BIT(15)

#define BT_MCS_OPC_SUP_PREV_GROUP    BIT(16)
#define BT_MCS_OPC_SUP_NEXT_GROUP    BIT(17)
#define BT_MCS_OPC_SUP_FIRST_GROUP   BIT(18)
#define BT_MCS_OPC_SUP_LAST_GROUP    BIT(19)
#define BT_MCS_OPC_SUP_GOTO_GROUP    BIT(20)

/** Media control point notification result codes */
#define BT_MCS_OPC_NTF_SUCCESS             0x01
#define BT_MCS_OPC_NTF_NOT_SUPPORTED       0x02
#define BT_MCS_OPC_NTF_PLAYER_INACTIVE     0x03
#define BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED 0x04

/** Search control point type values */
/* Reference: Media Control Service spec v1.0 section 3.20.2 */
#define BT_MCS_SEARCH_TYPE_TRACK_NAME    0x01
#define BT_MCS_SEARCH_TYPE_ARTIST_NAME   0x02
#define BT_MCS_SEARCH_TYPE_ALBUM_NAME    0x03
#define BT_MCS_SEARCH_TYPE_GROUP_NAME    0x04
#define BT_MCS_SEARCH_TYPE_EARLIEST_YEAR 0x05
#define BT_MCS_SEARCH_TYPE_LATEST_YEAR   0x06
#define BT_MCS_SEARCH_TYPE_GENRE         0x07
#define BT_MCS_SEARCH_TYPE_ONLY_TRACKS   0x08
#define BT_MCS_SEARCH_TYPE_ONLY_GROUPS   0x09

/** Search control point values */
#define SEARCH_LEN_MIN 2       /* At least one search control item (SCI),
				* consisting of the length octet and the type
				* octet. (The parameter field may be empty.)
				*/

#define SEARCH_SCI_LEN_MIN 1   /* An SCI length can be as little as one byte,
				* for an SCI that has only the type field.
				* (The SCI len is the length of type + param.)
				*/

#define SEARCH_LEN_MAX 64      /* Max total length of search, defined by spec */

#define SEARCH_PARAM_MAX 62    /* A search may have a single search control item
				* consisting of length, type and parameter
				*/

/** Search notification result codes */
/* Reference: Media Control Service spec v1.0 section 3.20.2 */
#define BT_MCS_SCP_NTF_SUCCESS  0x01
#define BT_MCS_SCP_NTF_FAILURE  0x02

/* Group object object types */
/* Reference: Media Control Service spec v1.0 section 4.4.1 */
#define BT_MCS_GROUP_OBJECT_TRACK_TYPE 0x00
#define BT_MCS_GROUP_OBJECT_GROUP_TYPE 0x01


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCS_H_ */
