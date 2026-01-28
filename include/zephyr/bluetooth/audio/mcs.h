/**
 * @file
 * @brief Bluetooth Media Control Service (MCS) APIs.
 */
/*
 * Copyright (c) 2019 - 2026 Nordic Semiconductor ASA
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
 * @since 3.0
 * @version 0.8.0
 *
 * @ingroup bluetooth
 * @{
 *
 * Definitions and types related to the Media Control Service and Media Control
 * Profile specifications.
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/bluetooth/audio/media_proxy.h>
#include <zephyr/bluetooth/services/ots.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A characteristic value has changed while a Read Long Value Characteristic sub-procedure is in
 * progress.
 */
#define BT_MCS_ERR_LONG_VAL_CHANGED     0x80

/**
 * @name Playback speeds
 *
 * The playback speed (s) is calculated by the value of 2 to the power of p divided by 64.
 * All values from -128 to 127 allowed, only some examples defined.
 * @{
 */
/** Minimum playback speed, resulting in 25 % speed */
#define BT_MCS_PLAYBACK_SPEED_MIN     -128
/** Quarter playback speed, resulting in 25 % speed */
#define BT_MCS_PLAYBACK_SPEED_QUARTER -128
/** Half playback speed, resulting in 50 % speed */
#define BT_MCS_PLAYBACK_SPEED_HALF     -64
/** Unity playback speed, resulting in 100 % speed */
#define BT_MCS_PLAYBACK_SPEED_UNITY      0
/** Double playback speed, resulting in 200 % speed */
#define BT_MCS_PLAYBACK_SPEED_DOUBLE    64
/** Max playback speed, resulting in 395.7 % speed (nearly 400 %) */
#define BT_MCS_PLAYBACK_SPEED_MAX      127
/** @} */

/**
 * @name Seeking speed
 *
 * The allowed values for seeking speed are the range -64 to -4
 * (endpoints included), the value 0, and the range 4 to 64
 * (endpoints included).
 * @{
 */
/** Maximum seeking speed - Can be negated */
#define BT_MCS_SEEKING_SPEED_FACTOR_MAX  64
/** Minimum seeking speed - Can be negated */
#define BT_MCS_SEEKING_SPEED_FACTOR_MIN   4
/** No seeking */
#define BT_MCS_SEEKING_SPEED_FACTOR_ZERO  0
/** @} */

/**
 * @name Playing orders
 * @{
 */
/** A single track is played once; there is no next track. */
#define BT_MCS_PLAYING_ORDER_SINGLE_ONCE    0X01
/** A single track is played repeatedly; the next track is the current track. */
#define BT_MCS_PLAYING_ORDER_SINGLE_REPEAT  0x02
/** The tracks within a group are played once in track order. */
#define BT_MCS_PLAYING_ORDER_INORDER_ONCE   0x03
/** The tracks within a group are played in track order repeatedly. */
#define BT_MCS_PLAYING_ORDER_INORDER_REPEAT 0x04
/** The tracks within a group are played once only from the oldest first. */
#define BT_MCS_PLAYING_ORDER_OLDEST_ONCE    0x05
/** The tracks within a group are played from the oldest first repeatedly. */
#define BT_MCS_PLAYING_ORDER_OLDEST_REPEAT  0x06
/** The tracks within a group are played once only from the newest first. */
#define BT_MCS_PLAYING_ORDER_NEWEST_ONCE    0x07
/** The tracks within a group are played from the newest first repeatedly. */
#define BT_MCS_PLAYING_ORDER_NEWEST_REPEAT  0x08
/** The tracks within a group are played in random order once. */
#define BT_MCS_PLAYING_ORDER_SHUFFLE_ONCE   0x09
/** The tracks within a group are played in random order repeatedly. */
#define BT_MCS_PLAYING_ORDER_SHUFFLE_REPEAT 0x0a
/** @} */

/** @name Playing orders supported
 *
 * A bitmap, in the same order as the playing orders above.
 * Note that playing order 1 corresponds to bit 0, and so on.
 * @{
 */
/** A single track is played once; there is no next track. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SINGLE_ONCE    BIT(0)
/** A single track is played repeatedly; the next track is the current track. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SINGLE_REPEAT  BIT(1)
/** The tracks within a group are played once in track order. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_ONCE   BIT(2)
/** The tracks within a group are played in track order repeatedly. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_INORDER_REPEAT BIT(3)
/** The tracks within a group are played once only from the oldest first. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_OLDEST_ONCE    BIT(4)
/** The tracks within a group are played from the oldest first repeatedly. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_OLDEST_REPEAT  BIT(5)
/** The tracks within a group are played once only from the newest first. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_NEWEST_ONCE    BIT(6)
/** The tracks within a group are played from the newest first repeatedly. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_NEWEST_REPEAT  BIT(7)
/** The tracks within a group are played in random order once. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SHUFFLE_ONCE   BIT(8)
/** The tracks within a group are played in random order repeatedly. */
#define BT_MCS_PLAYING_ORDERS_SUPPORTED_SHUFFLE_REPEAT BIT(9)
/** @} */

/**
 * @name Media states
 * @{
 */
/** The current track is invalid, and no track has been selected. */
#define BT_MCS_MEDIA_STATE_INACTIVE 0x00
/** The media player is playing the current track. */
#define BT_MCS_MEDIA_STATE_PLAYING  0x01
/** The current track is paused. The media player has a current track, but it is not being played */
#define BT_MCS_MEDIA_STATE_PAUSED   0x02
/** The current track is fast forwarding or fast rewinding. */
#define BT_MCS_MEDIA_STATE_SEEKING  0x03
/** @} */

/**
 * @name Media control point opcodes
 * @{
 */
/** Start playing the current track. */
#define BT_MCS_OPC_PLAY          0x01
/** Pause playing the current track. */
#define BT_MCS_OPC_PAUSE         0x02
/** Fast rewind the current track. */
#define BT_MCS_OPC_FAST_REWIND   0x03
/** Fast forward the current track. */
#define BT_MCS_OPC_FAST_FORWARD  0x04
/**
 * Stop current activity and return to the paused state and set the current track position to the
 * start of the current track.
 */
#define BT_MCS_OPC_STOP          0x05

/** Set a new current track position relative to the current track position. */
#define BT_MCS_OPC_MOVE_RELATIVE 0x10

/**
 * Set the current track position to the starting position of the previous segment of the current
 * track.
 */
#define BT_MCS_OPC_PREV_SEGMENT  0x20
/**
 * Set the current track position to the starting position of
 * the next segment of the current track.
 */
#define BT_MCS_OPC_NEXT_SEGMENT  0x21
/**
 * Set the current track position to the starting position of
 * the first segment of the current track.
 */
#define BT_MCS_OPC_FIRST_SEGMENT 0x22
/**
 * Set the current track position to the starting position of
 * the last segment of the current track.
 */
#define BT_MCS_OPC_LAST_SEGMENT  0x23
/**
 * Set the current track position to the starting position of
 * the nth segment of the current track.
 */
#define BT_MCS_OPC_GOTO_SEGMENT  0x24

/** Set the current track to the previous track based on the playing order. */
#define BT_MCS_OPC_PREV_TRACK    0x30
/** Set the current track to the next track based on the playing order. */
#define BT_MCS_OPC_NEXT_TRACK    0x31
/** Set the current track to the first track based on the playing order. */
#define BT_MCS_OPC_FIRST_TRACK   0x32
/** Set the current track to the last track based on the playing order. */
#define BT_MCS_OPC_LAST_TRACK    0x33
/** Set the current track to the nth track based on the playing order. */
#define BT_MCS_OPC_GOTO_TRACK    0x34

/** Set the current group to the previous group in the sequence of groups. */
#define BT_MCS_OPC_PREV_GROUP    0x40
/** Set the current group to the next group in the sequence of groups. */
#define BT_MCS_OPC_NEXT_GROUP    0x41
/** Set the current group to the first group in the sequence of groups. */
#define BT_MCS_OPC_FIRST_GROUP   0x42
/** Set the current group to the last group in the sequence of groups. */
#define BT_MCS_OPC_LAST_GROUP    0x43
/** Set the current group to the nth group in the sequence of groups. */
#define BT_MCS_OPC_GOTO_GROUP    0x44
/** @} */

/** Media control point supported opcodes length */
#define BT_MCS_OPCODES_SUPPORTED_LEN 4

/**
 * @name Media control point supported opcodes values
 * @{
 */
/** Support the Play opcode */
#define BT_MCS_OPC_SUP_PLAY          BIT(0)
/** Support the Pause opcode */
#define BT_MCS_OPC_SUP_PAUSE         BIT(1)
/** Support the Fast Rewind opcode */
#define BT_MCS_OPC_SUP_FAST_REWIND   BIT(2)
/** Support the Fast Forward opcode */
#define BT_MCS_OPC_SUP_FAST_FORWARD  BIT(3)
/** Support the Stop opcode */
#define BT_MCS_OPC_SUP_STOP          BIT(4)

/** Support the Move Relative opcode */
#define BT_MCS_OPC_SUP_MOVE_RELATIVE BIT(5)

/** Support the Previous Segment opcode */
#define BT_MCS_OPC_SUP_PREV_SEGMENT  BIT(6)
/** Support the Next Segment opcode */
#define BT_MCS_OPC_SUP_NEXT_SEGMENT  BIT(7)
/** Support the First Segment opcode */
#define BT_MCS_OPC_SUP_FIRST_SEGMENT BIT(8)
/** Support the Last Segment opcode */
#define BT_MCS_OPC_SUP_LAST_SEGMENT  BIT(9)
/** Support the Goto Segment opcode */
#define BT_MCS_OPC_SUP_GOTO_SEGMENT  BIT(10)

/** Support the Previous Track opcode */
#define BT_MCS_OPC_SUP_PREV_TRACK    BIT(11)
/** Support the Next Track opcode */
#define BT_MCS_OPC_SUP_NEXT_TRACK    BIT(12)
/** Support the First Track opcode */
#define BT_MCS_OPC_SUP_FIRST_TRACK   BIT(13)
/** Support the Last Track opcode */
#define BT_MCS_OPC_SUP_LAST_TRACK    BIT(14)
/** Support the Goto Track opcode */
#define BT_MCS_OPC_SUP_GOTO_TRACK    BIT(15)

/** Support the Previous Group opcode */
#define BT_MCS_OPC_SUP_PREV_GROUP    BIT(16)
/** Support the Next Group opcode */
#define BT_MCS_OPC_SUP_NEXT_GROUP    BIT(17)
/** Support the First Group opcode */
#define BT_MCS_OPC_SUP_FIRST_GROUP   BIT(18)
/** Support the Last Group opcode */
#define BT_MCS_OPC_SUP_LAST_GROUP    BIT(19)
/** Support the Goto Group opcode */
#define BT_MCS_OPC_SUP_GOTO_GROUP    BIT(20)
/** @} */

/**
 * @name Media control point notification result codes
 * @{
 */
/** Action requested by the opcode write was completed successfully. */
#define BT_MCS_OPC_NTF_SUCCESS             0x01
/** An invalid or unsupported opcode was used for the Media Control Point write. */
#define BT_MCS_OPC_NTF_NOT_SUPPORTED       0x02
/**
 * The Media Player State characteristic value is Inactive when the opcode is received or the
 * result of the requested action of the opcode results in the Media Player State characteristic
 * being set to Inactive.
 */
#define BT_MCS_OPC_NTF_PLAYER_INACTIVE     0x03
/**
 * The requested action of any Media Control Point write cannot be completed successfully because of
 * a condition within the player.
 */
#define BT_MCS_OPC_NTF_CANNOT_BE_COMPLETED 0x04
/** @} */

/**
 * @name Search control point type values
 *
 * Reference: Media Control Service spec v1.0 section 3.20.2
 * @{
 */
/** Search for Track Name */
#define BT_MCS_SEARCH_TYPE_TRACK_NAME    0x01
/** Search for Artist Name */
#define BT_MCS_SEARCH_TYPE_ARTIST_NAME   0x02
/** Search for Album Name */
#define BT_MCS_SEARCH_TYPE_ALBUM_NAME    0x03
/** Search for Group Name */
#define BT_MCS_SEARCH_TYPE_GROUP_NAME    0x04
/** Search for Earliest Year */
#define BT_MCS_SEARCH_TYPE_EARLIEST_YEAR 0x05
/** Search for Latest Year */
#define BT_MCS_SEARCH_TYPE_LATEST_YEAR   0x06
/** Search for Genre */
#define BT_MCS_SEARCH_TYPE_GENRE         0x07
/** Search for Tracks only */
#define BT_MCS_SEARCH_TYPE_ONLY_TRACKS   0x08
/** Search for Groups only */
#define BT_MCS_SEARCH_TYPE_ONLY_GROUPS   0x09
/** @} */

/**
 * @name Search notification result codes
 *
 * Reference: Media Control Service spec v1.0 section 3.20.2
 * @{
 */
/** Search request was accepted; search has started. */
#define BT_MCS_SCP_NTF_SUCCESS  0x01
/** Search request was invalid; no search started. */
#define BT_MCS_SCP_NTF_FAILURE  0x02
/** @} */

/**
 * @name Group object object types
 *
 * Reference: Media Control Service spec v1.0 section 4.4.1
 * @{
 */
/** Group object type is track */
#define BT_MCS_GROUP_OBJECT_TRACK_TYPE 0x00
/** Group object type is group */
#define BT_MCS_GROUP_OBJECT_GROUP_TYPE 0x01
/** @} */

/**
 * @brief Get the pointer of the Object Transfer Service used by the Media Control Service
 *
 * TODO: Find best location for this call, and move this one also
 */
struct bt_ots *bt_mcs_get_ots(void);

/* Temporary forward declaration to avoid circular dependency */
struct mpl_cmd;
struct mpl_search;
struct mpl_cmd_ntf;

/** @brief Callbacks when information about the player is requested via MCS */
struct bt_mcs_cb {

	/**
	 * @brief Read Media Player Name
	 *
	 * @return The name of the media player
	 */
	const char *(*get_player_name)(void);

	/**
	 * @brief Read Icon Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Icon
	 * Object from an Object Transfer Service
	 *
	 * See the Media Control Service spec v1.0 sections 3.2 and
	 * 4.1 for a description of the Icon Object.
	 *
	 * @return The Icon Object ID
	 */
	uint64_t (*get_icon_id)(void);

	/**
	 * @brief Read Icon URL
	 *
	 * Get a URL to the media player's icon.
	 *
	 * @return The URL of the Icon
	 */
	const char *(*get_icon_url)(void);

	/**
	 * @brief Read Track Title
	 *
	 * @return The title of the current track
	 */
	const char *(*get_track_title)(void);

	/**
	 * @brief Read Track Duration
	 *
	 * The duration of a track is measured in hundredths of a
	 * second.
	 *
	 * @return The duration of the current track
	 */
	int32_t (*get_track_duration)(void);

	/**
	 * @brief Read Track Position
	 *
	 * The position of the player (the playing position) is
	 * measured in hundredths of a second from the beginning of
	 * the track
	 *
	 * @return The position of the player in the current track
	 */
	int32_t (*get_track_position)(void);

	/**
	 * @brief Set Track Position
	 *
	 * Set the playing position of the media player in the current
	 * track. The position is given in hundredths of a second,
	 * from the beginning of the track of the track for positive
	 * values, and (backwards) from the end of the track for
	 * negative values.
	 *
	 * @param position    The player position to set
	 */
	void (*set_track_position)(int32_t position);

	/**
	 * @brief Get Playback Speed
	 *
	 * The playback speed parameter is related to the actual
	 * playback speed as follows:
	 * actual playback speed = 2^(speed_parameter/64)
	 *
	 * A speed parameter of 0 corresponds to unity speed playback
	 * (i.e. playback at "normal" speed). A speed parameter of
	 * -128 corresponds to playback at one fourth of normal speed,
	 * 127 corresponds to playback at almost four times the normal
	 * speed.
	 *
	 * @return The playback speed parameter
	 */
	int8_t (*get_playback_speed)(void);

	/**
	 * @brief Set Playback Speed
	 *
	 * See the get_playback_speed() function for an explanation of
	 * the playback speed parameter.
	 *
	 * Note that the media player may not support all possible
	 * values of the playback speed parameter. If the value given
	 * is not supported, and is higher than the current value, the
	 * player should set the playback speed to the next higher
	 * supported value. (And correspondingly to the next lower
	 * supported value for given values lower than the current
	 * value.)
	 *
	 * @param speed The playback speed parameter to set
	 */
	void (*set_playback_speed)(int8_t speed);

	/**
	 * @brief Get Seeking Speed
	 *
	 * The seeking speed gives the speed with which the player is
	 * seeking. It is a factor, relative to real-time playback
	 * speed - a factor four means seeking happens at four times
	 * the real-time playback speed. Positive values are for
	 * forward seeking, negative values for backwards seeking.
	 *
	 * The seeking speed is not settable - a non-zero seeking speed
	 * is the result of "fast rewind" of "fast forward" commands.
	 *
	 * @return The seeking speed factor
	 */
	int8_t (*get_seeking_speed)(void);

	/**
	 * @brief Read Current Track Segments Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Current
	 * Track Segments Object from an Object Transfer Service
	 *
	 * See the Media Control Service spec v1.0 sections 3.10 and
	 * 4.2 for a description of the Track Segments Object.
	 *
	 * @return Current The Track Segments Object ID
	 */
	uint64_t (*get_track_segments_id)(void);

	/**
	 * @brief Read Current Track Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Current
	 * Track Object from an Object Transfer Service
	 *
	 * See the Media Control Service spec v1.0 sections 3.11 and
	 * 4.3 for a description of the Current Track Object.
	 *
	 * @return The Current Track Object ID
	 */
	uint64_t (*get_current_track_id)(void);

	/**
	 * @brief Set Current Track Object ID
	 *
	 * Change the player's current track to the track given by the ID.
	 * (Behaves similarly to the goto track command.)
	 *
	 * @param id    The ID of a track object
	 */
	void (*set_current_track_id)(uint64_t id);

	/**
	 * @brief Read Next Track Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Next
	 * Track Object from an Object Transfer Service
	 *
	 * @return The Next Track Object ID
	 */
	uint64_t (*get_next_track_id)(void);

	/**
	 * @brief Set Next Track Object ID
	 *
	 * Change the player's next track to the track given by the ID.
	 *
	 * @param id    The ID of a track object
	 */
	void (*set_next_track_id)(uint64_t id);

	/**
	 * @brief Read Parent Group Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Parent
	 * Track Object from an Object Transfer Service
	 *
	 * The parent group is the parent of the current group.
	 *
	 * See the Media Control Service spec v1.0 sections 3.14 and
	 * 4.4 for a description of the Current Track Object.
	 *
	 * @return The Current Group Object ID
	 */
	uint64_t (*get_parent_group_id)(void);

	/**
	 * @brief Read Current Group Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Current
	 * Track Object from an Object Transfer Service
	 *
	 * See the Media Control Service spec v1.0 sections 3.14 and
	 * 4.4 for a description of the Current Group Object.
	 *
	 * @return The Current Group Object ID
	 */
	uint64_t (*get_current_group_id)(void);

	/**
	 * @brief Set Current Group Object ID
	 *
	 * Change the player's current group to the group given by the
	 * ID, and the current track to the first track in that group.
	 *
	 * @param id    The ID of a group object
	 */
	void (*set_current_group_id)(uint64_t id);

	/**
	 * @brief Read Playing Order
	 *
	 * return The media player's current playing order
	 */
	uint8_t (*get_playing_order)(void);

	/**
	 * @brief Set Playing Order
	 *
	 * Set the media player's playing order.
	 * See the MEDIA_PROXY_PLAYING_ORDER_* defines.
	 *
	 * @param order	The playing order to set
	 */
	void (*set_playing_order)(uint8_t order);

	/**
	 * @brief Read Playing Orders Supported
	 *
	 * Read a bitmap containing the media player's supported
	 * playing orders.
	 * See the MEDIA_PROXY_PLAYING_ORDERS_SUPPORTED_* defines.
	 *
	 * @return The media player's supported playing orders
	 */
	uint16_t (*get_playing_orders_supported)(void);

	/**
	 * @brief Read Media State
	 *
	 * Read the media player's state
	 * See the BT_MCS_MEDIA_STATE_* defines.
	 *
	 * @return The media player's state
	 */
	uint8_t (*get_media_state)(void);

	/**
	 * @brief Send Command
	 *
	 * Send a command to the media player.
	 * For command opcodes (play, pause, ...) - see the MEDIA_PROXY_OP_*
	 * defines.
	 *
	 * @param command	The command to send
	 */
	void (*send_command)(const struct mpl_cmd *command);

	/**
	 * @brief Read Commands Supported
	 *
	 * Read a bitmap containing the media player's supported
	 * command opcodes.
	 * See the MEDIA_PROXY_OP_SUP_* defines.
	 *
	 * @return The media player's supported command opcodes
	 */
	uint32_t (*get_commands_supported)(void);

	/**
	 * @brief Set Search
	 *
	 * Write a search to the media player.
	 * (For the formatting of a search, see the Media Control
	 * Service spec and the mcs.h file.)
	 *
	 * @param search	The search to write
	 */
	void (*send_search)(const struct mpl_search *search);

	/**
	 * @brief Read Search Results Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Search
	 * Results Object from an Object Transfer Service
	 *
	 * The search results object is a group object.
	 * The search results object only exists if a successful
	 * search operation has been done.
	 *
	 * @return The Search Results Object ID
	 */
	uint64_t (*get_search_results_id)(void);

	/**
	 * @brief Read Content Control ID
	 *
	 * The content control ID identifies a content control service
	 * on a device, and links it to the corresponding audio
	 * stream.
	 *
	 * @return The content control ID for the media player
	 */
	uint8_t (*get_content_ctrl_id)(void);
};

/**
 * @brief Initialize MCS
 *
 * @param ots_cbs Pointer to callback structure for OTS. May be NULL.

 * @return 0 if success, errno on failure
 */
int bt_mcs_init(struct bt_ots_cb *ots_cbs);

/**
 * @brief Register Media Control Service callbacks
 *
 * @param cbs Pointer to the callbacks
 *
 * @return 0 if success, errno on failure
 */
int bt_mcs_register_cb(struct bt_mcs_cb *cbs);

/**
 * @brief Player name changed callback
 *
 * To be called when the player's name is changed.
 */
void bt_mcs_player_name_changed(void);

/**
 * @brief Player icon URL changed callback
 *
 * To be called when the player's icon URL is changed.
 */
void bt_mcs_icon_url_changed(void);

/**
 * @brief Track changed callback
 *
 * To be called when the player's current track is changed
 */
void bt_mcs_track_changed(void);

/**
 * @brief Track title callback
 *
 * To be called when the player's current track is changed
 */
void bt_mcs_track_title_changed(void);

/**
 * @brief Track position callback
 *
 * To be called when the media player's position in the track is
 * changed, or when the player is paused or similar.
 *
 * Exception: This callback should not be called when the position
 * changes during regular playback, i.e. while the player is playing
 * and playback happens at a constant speed.
 *
 * The track position is given in hundredths of a second from the
 * start of the track.
 */
void bt_mcs_track_position_changed(void);

/**
 * @brief Track duration callback
 *
 * To be called when the current track's duration is changed (e.g. due
 * to a track change)
 *
 * The track duration is given in hundredths of a second.
 */
void bt_mcs_track_duration_changed(void);

/**
 * @brief Playback speed callback
 *
 * To be called when the playback speed is changed.
 */
void bt_mcs_playback_speed_changed(void);

/**
 * @brief Seeking speed callback
 *
 * To be called when the seeking speed is changed.
 */
void bt_mcs_seeking_speed_changed(void);

/**
 * @brief Current track object ID callback
 *
 * To be called when the ID of the current track is changed (e.g. due
 * to a track change).
 */
void bt_mcs_current_track_id_changed(void);

/**
 * @brief Next track object ID callback
 *
 * To be called when the ID of the current track is changes
 */
void bt_mcs_next_track_id_changed(void);

/**
 * @brief Parent group object ID callback
 *
 * To be called when the ID of the parent group is changed
 */
void bt_mcs_parent_group_id_changed(void);

/**
 * @brief Current group object ID callback
 *
 * To be called when the ID of the current group is changed
 */
void bt_mcs_current_group_id_changed(void);

/**
 * @brief Playing order callback
 *
 * To be called when the playing order is changed
 */
void bt_mcs_playing_order_changed(void);

/**
 * @brief Media state callback
 *
 * To be called when the media state is changed
 */
void bt_mcs_media_state_changed(void);

/**
 * @brief Commands supported callback
 *
 * To be called when the set of commands supported is changed
 */
void bt_mcs_commands_supported_changed(void);

/**
 * @brief Search Results object ID callback
 *
 * To be called when the ID of the search results is changed
 * (typically as the result of a new successful search).
 */
void bt_mcs_search_results_id_changed(void);

/**
 * @brief Command callback
 *
 * To be called when a command has been sent, to notify whether the
 * command was successfully performed or not.
 * See the MEDIA_PROXY_CMD_* result code defines.
 *
 * @param cmd_ntf	The result of the command
 */
void bt_mcs_command_complete(const struct mpl_cmd_ntf *cmd_ntf);

/**
 * @brief Search callback
 *
 * To be called when a search has been set to notify whether the
 * search was successfully performed or not.
 * See the MEDIA_PROXY_SEARCH_* result code defines.
 *
 * The actual results of the search, if successful, can be found in
 * the search results object.
 *
 * @param result_code	The result (success or failure) of the search
 */
void bt_mcs_search_complete(uint8_t result_code);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_AUDIO_MCS_H_ */
