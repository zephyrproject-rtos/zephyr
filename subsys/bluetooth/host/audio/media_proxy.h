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
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
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


/* PUBLIC API FOR PLAYERS */

/** @brief Availalable calls in a player, that the media proxy can call
 *
 * Given by a player when registering.
 */
struct media_proxy_pl_calls {

	/**
	 * @brief Read Media Player Name
	 *
	 * @return The name of the media player
	 */
	char *(*player_name_get)(void);

#ifdef CONFIG_BT_OTS
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
	uint64_t (*icon_id_get)(void);
#endif /* CONFIG_BT_OTS */

	/**
	 * @brief Read Icon URL
	 *
	 * Get a URL to the media player's icon.
	 *
	 * @return The URL of the Icon
	 */
	char *(*icon_url_get)(void);

	/**
	 * @brief Read Track Title
	 *
	 * @return The title of the current track
	 */
	char *(*track_title_get)(void);

	/**
	 * @brief Read Track Duration
	 *
	 * The duration of a track is measured in hundredths of a
	 * second.
	 *
	 * @return The duration of the current track
	 */
	int32_t (*track_duration_get)(void);

	/**
	 * @brief Read Track Position
	 *
	 * The position of the player (the playing position) is
	 * measured in hundredths of a second from the beginning of
	 * the track
	 *
	 * @return The position of the player in the current track
	 */
	int32_t (*track_position_get)(void);

	/**
	 * @brief Set Track Position
	 *
	 * Set the playing position of the media player in the current
	 * track. The position is given in in hundredths of a second,
	 * from the beginning of the track of the track for positive
	 * values, and (backwards) from the end of the track for
	 * negative values.
	 *
	 * @param position	The player position to set
	 */
	void (*track_position_set)(int32_t position);

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
	int8_t (*playback_speed_get)(void);

	/**
	 * @brief Set Playback Speed
	 *
	 * See the playback_speed_get() function for an explanation of
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
	void (*playback_speed_set)(int8_t speed);

	/**
	 * @brief Get Seeking Speed
	 *
	 * The seeking speed gives the speed with which the player is
	 * seeking. It is a factor, relative to real-time playback
	 * speed - a factor four means seeking happens at four times
	 * the real-time playback speed. Positive values are for
	 * forward seeking, negative values for backwards seeking.
	 *
	 * The seeking speed is not setable - a non-zero seeking speed
	 * is the result of "fast rewind" of "fast forward"
	 * operations.
	 *
	 * @return The seeking speed factor
	 */
	int8_t (*seeking_speed_get)(void);

#ifdef CONFIG_BT_OTS
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
	uint64_t (*track_segments_id_get)(void);

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
	uint64_t (*current_track_id_get)(void);

	/**
	 * @brief Set Current Track Object ID
	 *
	 * Change the player's current track to the track given by the ID.
	 * (Behaves similarly to the goto track operation.)
	 *
	 * @param id	The ID of a track object
	 */
	void (*current_track_id_set)(uint64_t id);

	/**
	 * @brief Read Next Track Object ID
	 *
	 * Get an ID (48 bit) that can be used to retrieve the Next
	 * Track Object from an Object Transfer Service
	 *
	 * @return The Next Track Object ID
	 */
	uint64_t (*next_track_id_get)(void);

	/**
	 * @brief Set Next Track Object ID
	 *
	 * Change the player's next track to the track given by the ID.
	 *
	 * @param id	The ID of a track object
	 */
	void (*next_track_id_set)(uint64_t id);

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
	uint64_t (*current_group_id_get)(void);

	/**
	 * @brief Set Current Group Object ID
	 *
	 * Change the player's current group to the group given by the
	 * ID, and the current track to the first track in that group.
	 *
	 * @param id	The ID of a group object
	 */
	void (*current_group_id_set)(uint64_t id);

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
	uint64_t (*parent_group_id_get)(void);
#endif /* CONFIG_BT_OTS */

	/**
	 * @brief Read Playing Order
	 *
	 * return The media player's current playing order
	 */
	uint8_t (*playing_order_get)(void);

	/**
	 * @brief Set Playing Order
	 *
	 * Set the media player's playing order
	 * See the Media Control Service spec, or the
	 * BT_MCS_PLAYING_ORDER_* defines in the mcs.h file.
	 *
	 * @param order	The playing order to set
	 */
	void (*playing_order_set)(uint8_t order);

	/**
	 * @brief Read Playing Orders Supported
	 *
	 * Read a bitmap containing the media player's supported
	 * playing orders.
	 * See the Media Control Service spec, or the
	 * BT_MCS_PLAYING_ORDERS_SUPPORTED_* defines in the mcs.h
	 * file.
	 *
	 * @return The media player's supported playing orders
	 */
	uint16_t (*playing_orders_supported_get)(void);

	/**
	 * @brief Read Media State
	 *
	 * Read the media player's state
	 * See the Media Control Service spec, or the
	 * BT_MCS_MEDIA_STATE_* defines in the mcs.h file.
	 *
	 * @return The media player's state
	 */
	uint8_t (*media_state_get)(void);

	/**
	 * @brief Set Operation
	 *
	 * Write an operation (a command) to the media player.
	 * For operations (play, pause, ... - see the Media Control
	 * Service spec, or the BT_MCS_OPC_* defines in the mcs.h
	 * file.
	 *
	 * @param operation	The operation to write
	 */
	void (*operation_set)(struct mpl_op_t operation);

	/**
	 * @brief Read Operations Supported
	 *
	 * Read a bitmap containing the media player's supported
	 * operations.
	 * See the Media Control Service spec, or the
	 * BT_MCS_OPC_SUP_* defines in the mcs.h file.
	 *
	 * @return The media player's supported operations
	 */
	uint32_t (*operations_supported_get)(void);

#ifdef CONFIG_BT_OTS
	/**
	 * @brief Set Search
	 *
	 * Write a search to the media player.
	 * (For the formatting of a search, see the Media Control
	 * Service spec and the mcs.h file.)
	 *
	 * @param search	The search to write
	 */
	void (*search_set)(struct mpl_search_t search);

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
	uint64_t (*search_results_id_get)(void);
#endif /* CONFIG_BT_OTS */

	/**
	 * @brief Read Content Control ID
	 *
	 * The content control ID identifies a content control service
	 * on a device, and links it to the corresponding audio
	 * stream.
	 *
	 * @return The content control ID for the media player
	 */
	uint8_t (*content_ctrl_id_get)(void);
};

/**
 * @brief Register a player with the media proxy
 *
 * Register a player with the media proxy module, for use by media
 * controllers.
 *
 * The media proxy may call any non-NULL function pointers in the
 * supplied media_proxy_pl_calls structure.
 *
 * @param pl_calls	Function pointers to the media player's calls
 */
int media_proxy_pl_register(struct media_proxy_pl_calls *pl_calls);

/* Initialize player - TODO: Move to player header file */
int media_proxy_pl_init(void);

/* TODO: Find best location for this call, and move this one also */
struct bt_ots *bt_mcs_get_ots(void);

/* Callbacks from the player to the media proxy */

/**
 * @brief Track changed callback
 *
 * To be called when the player's current track is changed
 */
void media_proxy_pl_track_changed_cb(void);

/**
 * @brief Track title callback
 *
 * To be called when the player's current track is changed
 *
 * @param title	The title of the track
 */
void media_proxy_pl_track_title_cb(char *title);

/**
 * @brief Track duration callback
 *
 * To be called when the current track's duration is changed (e.g. due
 * to a track change)
 *
 * The track duration is given in hundredths of a second.
 *
 * @param duration	The track duration
 */
void media_proxy_pl_track_duration_cb(int32_t duration);

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
 *
 *  @param position	The media player's position in the track
 */
void media_proxy_pl_track_position_cb(int32_t position);

/**
 * @brief Playback speed callback
 *
 * To be called when the playback speed is changed.
 *
 * @param speed	The playback speed parameter
 */
void media_proxy_pl_playback_speed_cb(int8_t speed);

/**
 * @brief Seeking speed callback
 *
 * To be called when the seeking speed is changed.
 *
 * @param speed	The seeking speed factor
 */
void media_proxy_pl_seeking_speed_cb(int8_t speed);

#ifdef CONFIG_BT_OTS
/**
 * @brief Current track object ID callback
 *
 * To be called when the ID of the current track is changed (e.g. due
 * to a track change).
 *
 * @param speed	The ID of the current track object in the OTS
 */
void media_proxy_pl_current_track_id_cb(uint64_t id);

/**
 * @brief Next track object ID callback
 *
 * To be called when the ID of the current track is changes
 *
 * @param speed	The ID of the next track object in the OTS
 */
void media_proxy_pl_next_track_id_cb(uint64_t id);

/**
 * @brief Current group object ID callback
 *
 * To be called when the ID of the current group is changed
 *
 * @param speed	The ID of the current group object in the OTS
 */
void media_proxy_pl_current_group_id_cb(uint64_t id);

/**
 * @brief Parent group object ID callback
 *
 * To be called when the ID of the parent group is changed
 *
 * @param speed	The ID of the parent group object in the OTS
 */
void media_proxy_pl_parent_group_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS */

/**
 * @brief Playing order callback
 *
 * To be called when the playing order is changed
 *
 * @param order	The playing order
 */
void media_proxy_pl_playing_order_cb(uint8_t order);

/**
 * @brief Media state callback
 *
 * To be called when the media state is changed
 *
 * @param state	The media player's state
 */
void media_proxy_pl_media_state_cb(uint8_t state);

/**
 * @brief Operation callback
 *
 * To be called when an operation has been set, to notify whether the
 * operation was successfully performed or not.
 * See the Media Control Service spec, or the BT_MCS_OPC_NTF_*
 * defines in the mcs.h file.
 *
 * @param op_ntf	The result of the operation
 */
void media_proxy_pl_operation_cb(struct mpl_op_ntf_t op_ntf);

/**
 * @brief Operations supported callback
 *
 * To be called when the set of supported operations is changed
 *
 * @param operations	The supported operation
 */
void media_proxy_pl_operations_supported_cb(uint32_t operations);

#ifdef CONFIG_BT_OTS
/**
 * @brief Search callback
 *
 * To be called when a search has been set to notify whether the
 * search was successfully performed or not.
 * See the Media Control Service spec, or the BT_MCS_SCP_NTF_*
 * defines in the mcs.h file.
 *
 * The actual results of the search, if successful, can be found in
 * the search results object.
 *
 * @param result_code	The result (success or failure) of the search
 */
void media_proxy_pl_search_cb(uint8_t result_code);

/**
 * @brief Search Results object ID callback
 *
 * To be called when the ID of the search results is changed
 * (typically as the result of a new successful search).
 *
 * @param id	The ID of the search results object in the OTS
 */
void media_proxy_pl_search_results_id_cb(uint64_t id);
#endif /* CONFIG_BT_OTS */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_HOST_AUDIO_MEDIA_PROXY_H_ */
